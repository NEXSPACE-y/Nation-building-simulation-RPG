import unreal


MAP_PATH = "/Game/Maps/StageD_Capital"

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not level_editor.new_level(MAP_PATH):
    if not level_editor.load_level(MAP_PATH):
        raise RuntimeError(f"Unable to create or load {MAP_PATH}")

world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world is available")

settings = world.get_world_settings()
settings.set_editor_property("default_game_mode", unreal.StageDGameMode)
settings.set_actor_label("Stage D Capital World Settings")

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
player_starts = [actor for actor in actor_subsystem.get_all_level_actors() if isinstance(actor, unreal.PlayerStart)]
if not player_starts:
    player_start = actor_subsystem.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0.0, 0.0, 140.0))
    if player_start is None:
        raise RuntimeError("Unable to create Stage D PlayerStart")
    player_start.set_actor_label("StageD_PlayerStart")

if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save {MAP_PATH}")

unreal.log(f"STAGE_D_MAP_READY {MAP_PATH}")
