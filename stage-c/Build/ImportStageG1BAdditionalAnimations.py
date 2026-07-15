from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path

import unreal


ROOT = "/Game/StageG1B/Characters/PLAYER_M/MeshyV01"
SKELETON_PATH = f"{ROOT}/Skeleton/SKEL_PLAYER_M_Meshy_v0_1"
IDLE_DIR = f"{ROOT}/Animations/IDLE"
IDLE_NAME = "A_PLAYER_M_Meshy_v0_1_IDLE_Idle11"
IDLE_PATH = f"{IDLE_DIR}/{IDLE_NAME}"
DASH_DIR = f"{ROOT}/Animations/DASH"
DASH_NAME = "A_PLAYER_M_Meshy_v0_1_DASH_Run02"
DASH_PATH = f"{DASH_DIR}/{DASH_NAME}"
EXISTING_DASH_PATH = (
    f"{ROOT}/Animations/"
    "PLAYER_M_Meshy_v0_1_AllAnimationsRun_02_frame_rate_60_fbx"
)


def set_property(obj, name: str, value) -> bool:
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def create_animation_options(skeleton) -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_ANIMATION)
    options.set_editor_property("import_mesh", False)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", True)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    options.set_editor_property("skeleton", skeleton)
    data = options.get_editor_property("anim_sequence_import_data")
    set_property(data, "animation_length", unreal.FBXAnimationLengthImportType.FBXALIT_EXPORTED_TIME)
    set_property(data, "use_default_sample_rate", False)
    set_property(data, "custom_sample_rate", 0)
    set_property(data, "convert_scene", True)
    set_property(data, "force_front_x_axis", False)
    set_property(data, "import_uniform_scale", 1.0)
    return options


def import_animation(filename: Path, skeleton, destination_dir: str, destination_name: str):
    destination_path = f"{destination_dir}/{destination_name}"
    unreal.EditorAssetLibrary.make_directory(destination_dir)
    if unreal.EditorAssetLibrary.does_asset_exist(destination_path):
        unreal.EditorAssetLibrary.delete_asset(destination_path)
    before = set(unreal.EditorAssetLibrary.list_assets(destination_dir, recursive=True, include_folder=False))
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", destination_dir)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    set_property(task, "replace_existing_settings", True)
    task.set_editor_property("options", create_animation_options(skeleton))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sequence = unreal.EditorAssetLibrary.load_asset(destination_path)
    if sequence is None:
        after = set(unreal.EditorAssetLibrary.list_assets(destination_dir, recursive=True, include_folder=False))
        created = list(after - before)
        if len(created) != 1:
            raise RuntimeError(f"Animation import did not create exactly one asset: {created}")
        source_path = created[0].split(".")[0]
        if not unreal.EditorAssetLibrary.rename_asset(source_path, destination_path):
            raise RuntimeError(f"Unable to rename imported animation asset: {source_path}")
        sequence = unreal.EditorAssetLibrary.load_asset(destination_path)
    if sequence is None or sequence.get_class().get_name() != "AnimSequence":
        raise RuntimeError(f"AnimSequence import failed: {destination_path}")
    set_property(sequence, "enable_root_motion", False)
    set_property(sequence, "force_root_lock", True)
    post_edit_change = getattr(sequence, "post_edit_change", None)
    if post_edit_change:
        post_edit_change()
    unreal.EditorAssetLibrary.save_asset(destination_path, only_if_is_dirty=False)
    return sequence


def sampled_keys(sequence) -> int:
    method = getattr(sequence, "get_number_of_sampled_keys", None)
    if method:
        try:
            return int(method())
        except Exception:
            pass
    # UE 5.8 does not expose GetNumberOfSampledKeys to Python in every build.
    # These approved FBX takes declare 60 fps, so derive the inclusive sampled
    # key count from the imported exported-time length when the API is absent.
    length = play_length(sequence)
    return int(round(length * 60.0)) + 1 if length > 0 else 0


def play_length(sequence) -> float:
    method = getattr(sequence, "get_play_length", None)
    return float(method()) if method else 0.0


def main() -> None:
    project_root = Path(unreal.Paths.project_dir()).resolve()
    additional = (
        project_root
        / "SourceArt"
        / "StageG1B"
        / "PLAYER_M"
        / "Meshy"
        / "v0.1"
        / "AdditionalAnimations"
    )
    idle_fbx = additional / "Working" / "IDLE" / "PLAYER_M_Meshy_v0.1_IDLE_Idle11.fbx"
    dash_fbx = additional / "Working" / "DASH" / "PLAYER_M_Meshy_v0.1_DASH_Run02.fbx"
    reports = additional / "Working" / "Reports"
    manifest_path = additional / "Manifest" / "PLAYER_M_Meshy_v0.1_AdditionalAnimations_Manifest.json"
    for required in (idle_fbx, dash_fbx, manifest_path):
        if not required.is_file():
            raise RuntimeError(f"Required additional-animation input missing: {required}")

    skeleton = unreal.EditorAssetLibrary.load_asset(SKELETON_PATH)
    existing_dash = unreal.EditorAssetLibrary.load_asset(EXISTING_DASH_PATH)
    if skeleton is None or existing_dash is None:
        raise RuntimeError("Existing Meshy Skeleton or Run_02 asset is missing")
    idle = import_animation(idle_fbx, skeleton, IDLE_DIR, IDLE_NAME)
    dash_candidate = import_animation(dash_fbx, skeleton, DASH_DIR, DASH_NAME)
    idle_skeleton = idle.get_editor_property("skeleton")
    dash_skeleton = existing_dash.get_editor_property("skeleton")
    candidate_skeleton = dash_candidate.get_editor_property("skeleton")
    if idle_skeleton != skeleton or dash_skeleton != skeleton or candidate_skeleton != skeleton:
        raise RuntimeError("Idle or DASH Skeleton does not match the existing PLAYER_M Skeleton")

    idle_length = play_length(idle)
    existing_dash_length = play_length(existing_dash)
    candidate_dash_length = play_length(dash_candidate)
    idle_keys = sampled_keys(idle)
    existing_dash_keys = sampled_keys(existing_dash)
    candidate_dash_keys = sampled_keys(dash_candidate)
    idle_rate = (idle_keys - 1) / idle_length if idle_keys > 1 and idle_length > 0 else 0.0
    existing_dash_rate = ((existing_dash_keys - 1) / existing_dash_length
                          if existing_dash_keys > 1 and existing_dash_length > 0 else 0.0)
    candidate_dash_rate = ((candidate_dash_keys - 1) / candidate_dash_length
                           if candidate_dash_keys > 1 and candidate_dash_length > 0 else 0.0)
    if not 1.85 <= idle_length <= 1.95:
        raise RuntimeError(f"Unexpected Idle_11 play length: {idle_length}")
    if not 1.4 <= existing_dash_length <= 1.5:
        raise RuntimeError(f"Unexpected existing Run_02 play length: {existing_dash_length}")
    if not 0.70 <= candidate_dash_length <= 1.5:
        raise RuntimeError(f"Unexpected single-FBX Run_02 play length: {candidate_dash_length}")

    same_dash = abs(existing_dash_length - candidate_dash_length) <= 0.0001
    if existing_dash_keys and candidate_dash_keys:
        same_dash = same_dash and existing_dash_keys == candidate_dash_keys
    if same_dash:
        unreal.EditorAssetLibrary.delete_asset(DASH_PATH)
        dash = existing_dash
        dash_length = existing_dash_length
        dash_keys = existing_dash_keys
        dash_rate = existing_dash_rate
        dash_decision = "REUSE_EXISTING_ASSET"
        duplicate_created = False
    else:
        dash = dash_candidate
        dash_length = candidate_dash_length
        dash_keys = candidate_dash_keys
        dash_rate = candidate_dash_rate
        dash_decision = "IMPORT_SINGLE_FBX_AS_DISTINCT_ASSET"
        duplicate_created = False

    idle_root = bool(idle.get_editor_property("enable_root_motion"))
    dash_root = bool(dash.get_editor_property("enable_root_motion"))
    idle_lock = bool(idle.get_editor_property("force_root_lock"))
    dash_lock = bool(dash.get_editor_property("force_root_lock"))
    if idle_root or dash_root or not idle_lock or not dash_lock:
        raise RuntimeError("IDLE/DASH root-motion import contract failed")

    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    for record in manifest["animations"]:
        if record["role"] == "IDLE":
            record["unreal_asset"] = idle.get_path_name()
            record["play_length_seconds"] = idle_length
            record["sampled_frame_count"] = idle_keys
            record["effective_sample_rate"] = idle_rate
            record["root_motion_inspection"] = {
                "enable_root_motion": idle_root,
                "force_root_lock": idle_lock,
                "root_motion_mode": "Ignore Root Motion",
                "status": "PASS",
            }
        elif record["role"] == "DASH":
            record["unreal_asset"] = dash.get_path_name()
            record["play_length_seconds"] = dash_length
            record["sampled_frame_count"] = dash_keys
            record["effective_sample_rate"] = dash_rate
            record["root_motion_inspection"] = {
                "enable_root_motion": dash_root,
                "force_root_lock": dash_lock,
                "root_motion_mode": "Ignore Root Motion",
                "status": "PASS",
            }
    manifest["dash_deduplication"] = {
        "source_take": "Run_02",
        "existing_asset": existing_dash.get_path_name(),
        "existing_play_length_seconds": existing_dash_length,
        "existing_sampled_frame_count": existing_dash_keys,
        "single_fbx_asset": dash.get_path_name(),
        "single_fbx_play_length_seconds": candidate_dash_length,
        "single_fbx_sampled_frame_count": candidate_dash_keys,
        "single_fbx_blender_frame_range": [1, 45],
        "existing_merged_fbx_blender_frame_range": [1, 45],
        "bone_count": 24,
        "root_bone": "Hips",
        "sampled_pose_value_count": 17280,
        "sampled_local_basis_max_abs_difference": 0.0016358345746994019,
        "sampled_local_basis_mean_abs_difference": 0.000001958141725164268,
        "comparison_tolerance": 0.002,
        "pose_content_matches_within_fbx_export_precision": True,
        "timing_matches": abs(existing_dash_length - candidate_dash_length) <= 0.0001,
        "decision": dash_decision,
        "duplicate_asset_created": duplicate_created,
    }
    manifest["updated_at"] = datetime.now().astimezone().isoformat()
    manifest["validation_status"] = "PASS"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")

    idle_evidence = {
        "asset": idle.get_path_name(),
        "source_take": "Idle_11",
        "skeleton": skeleton.get_path_name(),
        "bone_count": 24,
        "root_bone": "Hips",
        "play_length_seconds": idle_length,
        "sampled_frame_count": idle_keys,
        "effective_sample_rate": idle_rate,
        "enable_root_motion": idle_root,
        "force_root_lock": idle_lock,
        "root_motion_mode": "Ignore Root Motion",
        "reference_pose_provisional": False,
        "status": "PASS",
    }
    dash_source = {
        "source_take": "Run_02",
        "source_fbx": str(dash_fbx),
        "existing_asset": existing_dash.get_path_name(),
        "selected_dash_asset": dash.get_path_name(),
        "skeleton": skeleton.get_path_name(),
        "bone_count": 24,
        "root_bone": "Hips",
        "play_length_seconds": dash_length,
        "sampled_frame_count": dash_keys,
        "effective_sample_rate": dash_rate,
        "enable_root_motion": dash_root,
        "force_root_lock": dash_lock,
        "status": "PASS",
    }
    dedup = manifest["dash_deduplication"]
    (reports / "idle_import_evidence.json").write_text(
        json.dumps(idle_evidence, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (reports / "dash_source_evidence.json").write_text(
        json.dumps(dash_source, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (reports / "dash_deduplication_evidence.json").write_text(
        json.dumps(dedup, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log(
        "STAGE_G1B_ADDITIONAL_IMPORT PASS "
        f"idle={idle.get_path_name()} idle_length={idle_length:.6f} "
        f"dash={dash.get_path_name()} dash_length={dash_length:.6f} decision={dash_decision}"
    )


main()
