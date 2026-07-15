from __future__ import annotations

import json
import math
from datetime import datetime
from pathlib import Path

import unreal


SOURCE_MAP = "/Game/Maps/StageG2A_CameraRedesignPoC"
TARGET_MAP = "/Game/Maps/StageG2B_GuardM_PoC"
BP_PACKAGE = "/Game/StageG2B/Characters/GUARD_M/MeshyV01/Blueprints"
BP_NAME = "BP_StageG2B_GUARD_M"
G1B_ADAPTER_CLASS = "BP_StageG1B_PLAYER_M_C"
G2A_ADAPTER_CLASS = "BP_StageG2A_CameraModeAdapter_C"


def replace_blueprint(package_path: str, name: str, parent_class):
    object_path = f"{package_path}/{name}.{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        asset = unreal.load_asset(object_path)
        if asset is None:
            raise RuntimeError(f"Unable to reload Stage G-2B Blueprint: {object_path}")
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        return asset
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, package_path, None, factory)
    if asset is None:
        raise RuntimeError(f"Unable to create Stage G-2B Blueprint: {object_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    return asset


if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_MAP):
    raise RuntimeError(f"Accepted Stage G-2A map is missing: {SOURCE_MAP}")

replace_blueprint(BP_PACKAGE, BP_NAME, unreal.StageG2BGuardMActor)

if unreal.EditorAssetLibrary.does_asset_exist(TARGET_MAP):
    if not unreal.EditorAssetLibrary.delete_asset(TARGET_MAP):
        raise RuntimeError(f"Unable to replace Stage G-2B map: {TARGET_MAP}")
duplicated_map = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, TARGET_MAP)
if duplicated_map is None:
    raise RuntimeError(f"Unable to duplicate {SOURCE_MAP} to {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_asset(TARGET_MAP, only_if_is_dirty=False):
    raise RuntimeError(f"Unable to save duplicated Stage G-2B map: {TARGET_MAP}")
duplicated_map = None
unreal.SystemLibrary.collect_garbage()

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_editor.load_level(TARGET_MAP)
world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world after loading Stage G-2B map")
world.get_world_settings().set_actor_label("Stage G-2B GUARD_M single-character PoC World Settings")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_subsystem.get_all_level_actors()
g1b_count = sum(1 for actor in actors if actor.get_class().get_name() == G1B_ADAPTER_CLASS)
g2a_count = sum(1 for actor in actors if actor.get_class().get_name() == G2A_ADAPTER_CLASS)
if g1b_count != 1 or g2a_count != 1:
    raise RuntimeError(f"Accepted adapters missing in duplicated G-2B map: G1B={g1b_count} G2A={g2a_count}")

guard_class = unreal.EditorAssetLibrary.load_blueprint_class(f"{BP_PACKAGE}/{BP_NAME}")
if guard_class is None:
    raise RuntimeError("Unable to load Stage G-2B GUARD_M generated class")
for actor in actors:
    if actor.get_class().get_name() == f"{BP_NAME}_C":
        actor_subsystem.destroy_actor(actor)

player_starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
if not player_starts:
    raise RuntimeError("Stage G-2B duplicated map has no PlayerStart")
player_start = player_starts[0]
start_location = player_start.get_actor_location()
start_yaw = player_start.get_actor_rotation().yaw
forward_x = math.cos(math.radians(start_yaw))
forward_y = math.sin(math.radians(start_yaw))
guard_location = unreal.Vector(
    start_location.x + forward_x * 425.0,
    start_location.y + forward_y * 425.0,
    88.0,
)
guard_rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=start_yaw + 180.0)
guard = actor_subsystem.spawn_actor_from_class(guard_class, guard_location, guard_rotation)
if guard is None:
    raise RuntimeError("Unable to place Stage G-2B GUARD_M")
guard.set_actor_label("BP_StageG2B_GUARD_M_Single_PoC")

guard_count = sum(
    1 for actor in actor_subsystem.get_all_level_actors()
    if actor.get_class().get_name() == f"{BP_NAME}_C"
)
if guard_count != 1:
    raise RuntimeError(f"Stage G-2B map must contain exactly one GUARD_M: {guard_count}")

if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save Stage G-2B map: {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_directory("/Game/StageG2B", only_if_is_dirty=False, recursive=True):
    raise RuntimeError("Unable to save Stage G-2B assets")

project_root = Path(unreal.Paths.project_dir()).resolve()
reports = project_root / "SourceArt" / "StageG2B" / "GUARD_M" / "Meshy" / "v0.1" / "Working" / "Reports"
reports.mkdir(parents=True, exist_ok=True)
(reports / "map_generation_evidence.json").write_text(json.dumps({
    "created_at": datetime.now().astimezone().isoformat(),
    "source_map": SOURCE_MAP,
    "target_map": TARGET_MAP,
    "g1b_player_adapter_count": g1b_count,
    "g2a_camera_adapter_count": g2a_count,
    "guard_actor_count": guard_count,
    "guard_actor_blueprint": f"{BP_PACKAGE}/{BP_NAME}",
    "guard_location": {"x": guard_location.x, "y": guard_location.y, "z": guard_location.z},
    "distance_from_player_start_uu": 425.0,
    "ai_connected": False,
    "causal_core_connected": False,
    "save_schema_connected": False,
    "status": "PASS",
}, ensure_ascii=False, indent=2), encoding="utf-8")

unreal.log(
    "STAGE_G2B_GUARD_M_ASSETS PASS "
    f"source={SOURCE_MAP} target={TARGET_MAP} guard={BP_PACKAGE}/{BP_NAME} "
    "guard_count=1 g1b_adapter=1 g2a_adapter=1 ai=0 causal=0 save=0"
)
