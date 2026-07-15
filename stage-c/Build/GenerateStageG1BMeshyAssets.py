import unreal


ROOT = "/Game/StageG1B/Characters/PLAYER_M/MeshyV01"
SKELETON_PATH = f"{ROOT}/Skeleton/SKEL_PLAYER_M_Meshy_v0_1.SKEL_PLAYER_M_Meshy_v0_1"
MESH_PATH = f"{ROOT}/Mesh/SK_PLAYER_M_Meshy_v0_1.SK_PLAYER_M_Meshy_v0_1"
ABP_PACKAGE = f"{ROOT}/AnimationBlueprint"
ABP_NAME = "ABP_PLAYER_M_Meshy_v0_1"
BP_PACKAGE = f"{ROOT}/Blueprints"
BP_NAME = "BP_StageG1B_PLAYER_M"
SOURCE_MAP = "/Game/Maps/StageG1_3DCharacterPoC"
TARGET_MAP = "/Game/Maps/StageG1B_OriginalPlayerPoC"


def require_asset(path, label):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Missing {label}: {path}")
    return asset


def replace_asset(package_path, name, factory):
    object_path = f"{package_path}/{name}.{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        if not unreal.EditorAssetLibrary.delete_asset(object_path):
            raise RuntimeError(f"Unable to replace asset: {object_path}")
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(name, package_path, None, factory)
    if asset is None:
        raise RuntimeError(f"Unable to create asset: {object_path}")
    return asset


skeleton = require_asset(SKELETON_PATH, "Meshy Skeleton")
mesh = require_asset(MESH_PATH, "Meshy SkeletalMesh")

anim_factory = unreal.AnimBlueprintFactory()
anim_factory.set_editor_property("target_skeleton", skeleton)
anim_factory.set_editor_property("preview_skeletal_mesh", mesh)
anim_factory.set_editor_property("parent_class", unreal.StageG1BMeshyAnimInstance)
anim_blueprint = replace_asset(ABP_PACKAGE, ABP_NAME, anim_factory)
unreal.BlueprintEditorLibrary.compile_blueprint(anim_blueprint)

adapter_factory = unreal.BlueprintFactory()
adapter_factory.set_editor_property("parent_class", unreal.StageG1BPlayerVisualAdapter)
adapter_blueprint = replace_asset(BP_PACKAGE, BP_NAME, adapter_factory)
unreal.BlueprintEditorLibrary.compile_blueprint(adapter_blueprint)

if unreal.EditorAssetLibrary.does_asset_exist(TARGET_MAP):
    if not unreal.EditorAssetLibrary.delete_asset(TARGET_MAP):
        raise RuntimeError(f"Unable to replace target map: {TARGET_MAP}")
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_editor.new_level(TARGET_MAP):
    raise RuntimeError(f"Unable to create target map: {TARGET_MAP}")

world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world after loading Stage G-1B map")

settings = world.get_world_settings()
settings.set_editor_property("default_game_mode", unreal.StageG1AGameMode)
settings.set_actor_label("Stage G-1B Meshy PLAYER_M PoC World Settings")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
player_start = actor_subsystem.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(500.0, -1750.0, 96.0))
if player_start is None:
    raise RuntimeError("Unable to create Stage G-1B PlayerStart")
player_start.set_actor_label("StageG1B_PlayerStart")
player_start.set_actor_rotation(unreal.Rotator(0.0, 35.0, 0.0), False)

nav_volume = actor_subsystem.spawn_actor_from_class(
    unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 100.0))
if nav_volume is None:
    raise RuntimeError("Unable to create Stage G-1B NavMesh bounds")
nav_volume.set_actor_label("StageG1B_NavMeshBounds")
nav_volume.set_actor_scale3d(unreal.Vector(36.0, 36.0, 8.0))

adapter_class = unreal.EditorAssetLibrary.load_blueprint_class(f"{BP_PACKAGE}/{BP_NAME}")
if adapter_class is None:
    raise RuntimeError("Unable to load Stage G-1B adapter generated class")

for actor in actor_subsystem.get_all_level_actors():
    if actor.get_class().get_name() == f"{BP_NAME}_C":
        actor_subsystem.destroy_actor(actor)

adapter = actor_subsystem.spawn_actor_from_class(
    adapter_class, unreal.Vector(0.0, 0.0, 0.0), unreal.Rotator(0.0, 0.0, 0.0))
if adapter is None:
    raise RuntimeError("Unable to place Stage G-1B visual adapter")
adapter.set_actor_label("BP_StageG1B_PLAYER_M_VisualAdapter")

if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save target map: {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True):
    raise RuntimeError(f"Unable to save generated assets: {ROOT}")

source_map_file = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_content_dir() + "Maps/StageG1_3DCharacterPoC.umap")
target_map_file = unreal.Paths.convert_relative_path_to_full(
    unreal.Paths.project_content_dir() + "Maps/StageG1B_OriginalPlayerPoC.umap")
unreal.log(
    "STAGE_G1B_ASSETS PASS "
    f"abp={ABP_PACKAGE}/{ABP_NAME} bp={BP_PACKAGE}/{BP_NAME} map={TARGET_MAP} "
    f"source_map={source_map_file} target_map={target_map_file}"
)
