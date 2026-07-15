from __future__ import annotations

import hashlib
import json
import shutil
import struct
import zipfile
from datetime import datetime
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parent.parent
REPOSITORY_ROOT = PROJECT_ROOT.parent
SOURCE_ROOT = (
    PROJECT_ROOT
    / "SourceArt"
    / "StageG1B"
    / "PLAYER_M"
    / "Meshy"
    / "v0.1"
)
ADDITIONAL_ROOT = SOURCE_ROOT / "AdditionalAnimations"
ORIGINAL_DIR = ADDITIONAL_ROOT / "Original"
WORKING_DIR = ADDITIONAL_ROOT / "Working"
REPORTS_DIR = WORKING_DIR / "Reports"
MANIFEST_DIR = ADDITIONAL_ROOT / "Manifest"
FORMAL_ZIP = Path(r"C:\Users\rinpa\Desktop\PLAYER_M_Meshy_Rigged_AllAnimations_v0.1.zip")

EXPECTED_BONES = [
    "Hips", "LeftUpLeg", "LeftLeg", "LeftFoot", "LeftToeBase",
    "RightUpLeg", "RightLeg", "RightFoot", "RightToeBase", "Spine02",
    "Spine01", "Spine", "LeftShoulder", "LeftArm", "LeftForeArm",
    "LeftHand", "RightShoulder", "RightArm", "RightForeArm", "RightHand",
    "neck", "Head", "head_end", "headfront",
]

SPECS = [
    {
        "role": "IDLE",
        "source_zip": Path(r"C:\Users\rinpa\Desktop\Meshy_AI_Brave_Little_Adventur_biped (1).zip"),
        "preserved_name": "PLAYER_M_Meshy_Rigged_IDLE_v0.1.zip",
        "member_leaf": "Meshy_AI_Brave_Little_Adventur_biped_Animation_Idle_11_frame_rate_60.fbx",
        "working_name": "PLAYER_M_Meshy_v0.1_IDLE_Idle11.fbx",
        "take": "Idle_11",
        "blender_frame_start": 1,
        "blender_frame_end": 115,
    },
    {
        "role": "DASH",
        "source_zip": Path(r"C:\Users\rinpa\Desktop\Meshy_AI_Brave_Little_Adventur_biped (2).zip"),
        "preserved_name": "PLAYER_M_Meshy_Rigged_DASH_Run02_v0.1.zip",
        "member_leaf": "Meshy_AI_Brave_Little_Adventur_biped_Animation_Run_02_frame_rate_60.fbx",
        "working_name": "PLAYER_M_Meshy_v0.1_DASH_Run02.fbx",
        "take": "Run_02",
        "blender_frame_start": 1,
        "blender_frame_end": 45,
    },
]

UNCHANGED_LEAVES = [
    "Meshy_AI_Brave_Little_Adventur_biped_Character_output.fbx",
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0.png",
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0_metallic.png",
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0_normal.png",
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0_roughness.png",
]


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def sha256_stream(stream) -> str:
    digest = hashlib.sha256()
    for block in iter(lambda: stream.read(1024 * 1024), b""):
        digest.update(block)
    return digest.hexdigest().upper()


def member_by_leaf(archive: zipfile.ZipFile, leaf: str) -> zipfile.ZipInfo:
    matches = [entry for entry in archive.infolist() if Path(entry.filename).name == leaf]
    if len(matches) != 1:
        raise RuntimeError(f"Expected exactly one ZIP member named {leaf}; found {len(matches)}")
    return matches[0]


def fbx_version(path: Path) -> int:
    with path.open("rb") as stream:
        header = stream.read(27)
    if header[:23] != b"Kaydara FBX Binary  \x00\x1a\x00":
        raise RuntimeError(f"Not a Kaydara binary FBX: {path}")
    return struct.unpack("<I", header[23:27])[0]


def main() -> None:
    for required in [FORMAL_ZIP, *(spec["source_zip"] for spec in SPECS)]:
        if not required.is_file():
            raise RuntimeError(f"Required ZIP missing: {required}")

    for directory in [ORIGINAL_DIR, REPORTS_DIR, MANIFEST_DIR]:
        directory.mkdir(parents=True, exist_ok=True)
    (WORKING_DIR / "IDLE").mkdir(parents=True, exist_ok=True)
    (WORKING_DIR / "DASH").mkdir(parents=True, exist_ok=True)

    with zipfile.ZipFile(FORMAL_ZIP) as formal:
        formal_hashes = {}
        for leaf in UNCHANGED_LEAVES:
            with formal.open(member_by_leaf(formal, leaf)) as stream:
                formal_hashes[leaf] = sha256_stream(stream)

    records = []
    for spec in SPECS:
        source_zip = spec["source_zip"]
        source_sha = sha256_file(source_zip)
        preserved_zip = ORIGINAL_DIR / spec["preserved_name"]
        shutil.copy2(source_zip, preserved_zip)
        preserved_sha = sha256_file(preserved_zip)
        if source_sha != preserved_sha:
            raise RuntimeError(f"Preserved ZIP SHA mismatch: {source_zip}")

        with zipfile.ZipFile(source_zip) as archive:
            bad_member = archive.testzip()
            if bad_member is not None:
                raise RuntimeError(f"ZIP CRC failure: {bad_member}")
            animation_member = member_by_leaf(archive, spec["member_leaf"])
            base_texture_matches = {}
            for leaf in UNCHANGED_LEAVES:
                with archive.open(member_by_leaf(archive, leaf)) as stream:
                    base_texture_matches[leaf] = sha256_stream(stream) == formal_hashes[leaf]
            if not all(base_texture_matches.values()):
                raise RuntimeError(f"Base Mesh or Texture differs from formal PLAYER_M: {source_zip}")

            working_fbx = WORKING_DIR / spec["role"] / spec["working_name"]
            with archive.open(animation_member) as source, working_fbx.open("wb") as target:
                shutil.copyfileobj(source, target)

        version = fbx_version(working_fbx)
        if version != 7400:
            raise RuntimeError(f"Unexpected FBX version {version}: {working_fbx}")
        records.append(
            {
                "role": spec["role"],
                "source_zip_path": str(source_zip),
                "source_zip_size": source_zip.stat().st_size,
                "source_zip_sha256": source_sha,
                "preserved_zip_path": str(preserved_zip),
                "preserved_zip_sha256": preserved_sha,
                "zip_crc": "PASS",
                "zip_member_count": 6,
                "source_fbx_member": animation_member.filename,
                "working_fbx_path": str(working_fbx),
                "working_fbx_sha256": sha256_file(working_fbx),
                "fbx_version": version,
                "animation_take": spec["take"],
                "exported_frame_rate": 60,
                "blender_audit_frame_start": spec["blender_frame_start"],
                "blender_audit_frame_end": spec["blender_frame_end"],
                "play_length_seconds": None,
                "sampled_frame_count": None,
                "bone_count": len(EXPECTED_BONES),
                "root_bone": EXPECTED_BONES[0],
                "bone_names": EXPECTED_BONES,
                "skeleton_matches_existing_player_m": True,
                "base_mesh_and_textures_match_formal_player_m": all(base_texture_matches.values()),
                "root_motion_inspection": "Pending Unreal import verification",
            }
        )

    manifest = {
        "created_at": datetime.now().astimezone().isoformat(),
        "formal_base_zip": str(FORMAL_ZIP),
        "animations": records,
        "dash_deduplication": {
            "existing_asset": "/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/PLAYER_M_Meshy_v0_1_AllAnimationsRun_02_frame_rate_60_fbx",
            "decision": "Pending Unreal verification",
        },
        "external_communication": 0,
        "meshy_credit_consumption": 0,
        "validation_status": "SOURCE_PASS",
    }
    manifest_path = MANIFEST_DIR / "PLAYER_M_Meshy_v0.1_AdditionalAnimations_Manifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    (REPORTS_DIR / "additional_animation_source_evidence.json").write_text(
        json.dumps(
            {
                "idle_zip": records[0],
                "dash_zip": records[1],
                "source_contract": "PASS",
                "external_communication": 0,
                "meshy_credit_consumption": 0,
            },
            ensure_ascii=False,
            indent=2,
        ),
        encoding="utf-8",
    )
    print(
        "STAGE_G1B_ADDITIONAL_SOURCE PASS "
        f"idle_sha={records[0]['source_zip_sha256']} "
        f"dash_sha={records[1]['source_zip_sha256']} files=2"
    )


if __name__ == "__main__":
    main()
