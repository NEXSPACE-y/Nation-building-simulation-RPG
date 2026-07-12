import unreal


MAP_PATH = "/Game/Maps/StageG0_VisualPoC"

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_editor.new_level(MAP_PATH):
    if not level_editor.load_level(MAP_PATH):
        raise RuntimeError(f"Unable to create or load {MAP_PATH}")

world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world is available")

settings = world.get_world_settings()
settings.set_editor_property("default_game_mode", unreal.StageDGameMode)
settings.set_actor_label("Stage G-0 Visual PoC World Settings")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
player_starts = [actor for actor in actor_subsystem.get_all_level_actors() if isinstance(actor, unreal.PlayerStart)]
if not player_starts:
    player_start = actor_subsystem.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 140.0))
    if player_start is None:
        raise RuntimeError("Unable to create Stage G-0 PlayerStart")
    player_start.set_actor_label("StageG0_PlayerStart")

nav_bounds = [actor for actor in actor_subsystem.get_all_level_actors()
              if isinstance(actor, unreal.NavMeshBoundsVolume)]
if not nav_bounds:
    nav_volume = actor_subsystem.spawn_actor_from_class(
        unreal.NavMeshBoundsVolume, unreal.Vector(0.0, 0.0, 0.0))
    if nav_volume is None:
        raise RuntimeError("Unable to create Stage G-0 NavMesh bounds")
    nav_volume.set_actor_label("StageG0_NavMeshBounds")
    nav_volume.set_actor_scale3d(unreal.Vector(48.0, 48.0, 12.0))

if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save {MAP_PATH}")

unreal.log(f"STAGE_G0_MAP_READY {MAP_PATH}")
