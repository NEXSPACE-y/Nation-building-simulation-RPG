import unreal


MAP_PATH = "/Game/Maps/StageG1_3DCharacterPoC"

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_editor.new_level(MAP_PATH):
    if not level_editor.load_level(MAP_PATH):
        raise RuntimeError(f"Unable to create or load {MAP_PATH}")

world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world is available")

settings = world.get_world_settings()
settings.set_editor_property("default_game_mode", unreal.StageG1AGameMode)
settings.set_actor_label("Stage G-1A Standard 3D Character PoC World Settings")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actor_subsystem.get_all_level_actors():
    if isinstance(actor, (unreal.PlayerStart, unreal.NavMeshBoundsVolume)):
        actor_subsystem.destroy_actor(actor)

player_start = actor_subsystem.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(500.0, -1750.0, 96.0))
if player_start is None:
    raise RuntimeError("Unable to create Stage G-1A PlayerStart")
player_start.set_actor_label("StageG1A_PlayerStart")
player_start.set_actor_rotation(unreal.Rotator(0.0, 35.0, 0.0), False)

nav_volume = actor_subsystem.spawn_actor_from_class(
    unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 100.0))
if nav_volume is None:
    raise RuntimeError("Unable to create Stage G-1A NavMesh bounds")
nav_volume.set_actor_label("StageG1A_NavMeshBounds")
nav_volume.set_actor_scale3d(unreal.Vector(36.0, 36.0, 8.0))

if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save {MAP_PATH}")

unreal.log(f"STAGE_G1A_MAP_READY {MAP_PATH} player_start=(500,-1750,96) nav_bounds=(36,36,8)")
