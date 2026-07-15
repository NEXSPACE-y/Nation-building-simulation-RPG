import unreal


SOURCE_MAP = "/Game/Maps/StageG1B_OriginalPlayerPoC"
TARGET_MAP = "/Game/Maps/StageG2A_CameraRedesignPoC"
BP_PACKAGE = "/Game/StageG2A/CameraRedesign/Blueprints"
BP_NAME = "BP_StageG2A_CameraModeAdapter"
G1B_ADAPTER_CLASS = "BP_StageG1B_PLAYER_M_C"


def replace_blueprint(package_path, name, parent_class):
    object_path = f"{package_path}/{name}.{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        asset = unreal.load_asset(object_path)
        if asset is None:
            raise RuntimeError(f"Unable to reload Stage G-2A redesign asset: {object_path}")
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
        return asset
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, package_path, None, factory)
    if asset is None:
        raise RuntimeError(f"Unable to create Stage G-2A redesign asset: {object_path}")
    unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    return asset


if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_MAP):
    raise RuntimeError(f"Accepted Stage G-1B map is missing: {SOURCE_MAP}")

replace_blueprint(BP_PACKAGE, BP_NAME, unreal.StageG2ACameraModeAdapter)

if unreal.EditorAssetLibrary.does_asset_exist(TARGET_MAP):
    if not unreal.EditorAssetLibrary.delete_asset(TARGET_MAP):
        raise RuntimeError(f"Unable to replace target map: {TARGET_MAP}")
duplicated_map = unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, TARGET_MAP)
if duplicated_map is None:
    raise RuntimeError(f"Unable to duplicate {SOURCE_MAP} to {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_asset(TARGET_MAP, only_if_is_dirty=False):
    raise RuntimeError(f"Unable to save duplicated map before loading: {TARGET_MAP}")
duplicated_map = None
unreal.SystemLibrary.collect_garbage()

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_editor.load_level(TARGET_MAP)
world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world after loading Stage G-2A redesign map")
world.get_world_settings().set_actor_label(
    "Stage G-2A Standard Character + F6 Tactical Overlook World Settings")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actor_subsystem.get_all_level_actors()
if not any(actor.get_class().get_name() == G1B_ADAPTER_CLASS for actor in actors):
    raise RuntimeError("Duplicated map lost the accepted Stage G-1B PLAYER_M adapter")

camera_class = unreal.EditorAssetLibrary.load_blueprint_class(f"{BP_PACKAGE}/{BP_NAME}")
if camera_class is None:
    raise RuntimeError("Unable to load Stage G-2A redesigned Camera Adapter class")
for actor in actors:
    if actor.get_class().get_name() == f"{BP_NAME}_C":
        actor_subsystem.destroy_actor(actor)

camera_adapter = actor_subsystem.spawn_actor_from_class(
    camera_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
if camera_adapter is None:
    raise RuntimeError("Unable to place Stage G-2A redesigned Camera Adapter")
camera_adapter.set_actor_label("BP_StageG2A_StandardAndTacticalCameraAdapter")

cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
if cube is None:
    raise RuntimeError("Engine Cube is unavailable")


def spawn_collision_fixture(label, location, scale):
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor, location, unreal.Rotator(0.0, 0.0, 0.0))
    if actor is None:
        raise RuntimeError(f"Unable to spawn {label}")
    actor.set_actor_label(label)
    actor.set_actor_scale3d(scale)
    component = actor.static_mesh_component
    component.set_static_mesh(cube)
    component.set_collision_profile_name("BlockAll")
    component.set_editor_property("cast_shadow", True)
    return actor


# Engine-only fixtures for camera collision and framing; no production city assets.
spawn_collision_fixture(
    "StageG2A_Redesign_CameraWall",
    unreal.Vector(2050.0, -1000.0, 300.0), unreal.Vector(0.8, 6.0, 6.0))
spawn_collision_fixture(
    "StageG2A_Redesign_CameraColumn",
    unreal.Vector(1250.0, 1180.0, 250.0), unreal.Vector(1.8, 1.8, 5.0))
spawn_collision_fixture(
    "StageG2A_Redesign_LowRoof",
    unreal.Vector(-950.0, 1250.0, 460.0), unreal.Vector(8.0, 8.0, 0.8))

if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save target map: {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_directory(
        "/Game/StageG2A", only_if_is_dirty=False, recursive=True):
    raise RuntimeError("Unable to save Stage G-2A redesign assets")

unreal.log(
    "STAGE_G2A_REDESIGN_MAP PASS "
    f"source={SOURCE_MAP} target={TARGET_MAP} adapter={BP_PACKAGE}/{BP_NAME} "
    "initial_mode=StandardCharacterCamera tactical_toggle=F6 "
    "g1b_adapter_preserved=1 collision_fixtures=3 external_assets=0"
)
