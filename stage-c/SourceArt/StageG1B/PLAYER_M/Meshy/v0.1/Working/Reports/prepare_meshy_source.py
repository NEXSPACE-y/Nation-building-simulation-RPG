from __future__ import annotations

import argparse
import hashlib
import json
import shutil
import struct
import zipfile
from datetime import datetime
from pathlib import Path


EXPECTED_ZIP_SIZE = 33_506_378
EXPECTED_ZIP_SHA256 = "EBD21F89AC9441E4E9D94EAD79A0A7B5F58B8F0BCE3BCEDAD3A07553050B3367"
EXPECTED_FILES = {
    "Meshy_AI_Brave_Little_Adventur_biped_Character_output.fbx": ("FBX", "PLAYER_M_Meshy_v0.1_Base_Rigged.fbx"),
    "Meshy_AI_Brave_Little_Adventur_biped_Meshy_AI_Meshy_Merged_Animations.fbx": ("FBX", "PLAYER_M_Meshy_v0.1_AllAnimations.fbx"),
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0.png": ("Textures", "T_PLAYER_M_BaseColor_2K.png"),
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0_normal.png": ("Textures", "T_PLAYER_M_Normal_2K.png"),
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0_metallic.png": ("Textures", "T_PLAYER_M_Metallic_2K.png"),
    "Meshy_AI_Brave_Little_Adventur_biped_texture_0_roughness.png": ("Textures", "T_PLAYER_M_Roughness_2K.png"),
}


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(4 * 1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest().upper()


def png_contract(data: bytes) -> dict:
    if data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        raise RuntimeError("Invalid PNG header")
    width, height = struct.unpack(">II", data[16:24])
    bit_depth, color_type = data[24], data[25]
    mode = {0: "Grayscale", 2: "RGB"}.get(color_type, f"ColorType{color_type}")
    return {"width": width, "height": height, "bit_depth": bit_depth, "mode": mode}


def fbx_contract(data: bytes) -> dict:
    if data[:23] != b"Kaydara FBX Binary  \x00\x1a\x00":
        raise RuntimeError("FBX is not Kaydara binary")
    version = struct.unpack("<I", data[23:27])[0]
    return {"format": "Kaydara FBX Binary", "version": version, "version_family": "7.4" if version == 7400 else "unexpected"}


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-zip", type=Path, required=True)
    parser.add_argument("--base", type=Path, required=True)
    args = parser.parse_args()
    source_zip = args.source_zip.resolve()
    base = args.base.resolve()
    source_size = source_zip.stat().st_size
    source_sha = sha256_file(source_zip)
    if source_size != EXPECTED_ZIP_SIZE or source_sha != EXPECTED_ZIP_SHA256:
        raise RuntimeError("Source ZIP size/SHA contract failed")

    original_path = base / "Original" / "Meshy_AI_Brave_Little_Adventur_biped.zip"
    original_path.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source_zip, original_path)
    if sha256_file(original_path) != source_sha:
        raise RuntimeError("Original ZIP copy SHA mismatch")

    records = []
    with zipfile.ZipFile(source_zip) as archive:
        entries = [info for info in archive.infolist() if not info.is_dir()]
        leaf_names = [Path(info.filename).name for info in entries]
        if len(entries) != 6 or set(leaf_names) != set(EXPECTED_FILES):
            raise RuntimeError(f"ZIP member contract failed: {len(entries)}")
        if archive.testzip() is not None:
            raise RuntimeError("ZIP CRC failure")
        for info in sorted(entries, key=lambda value: Path(value.filename).name):
            name = Path(info.filename).name
            data = archive.read(info)
            category, work_name = EXPECTED_FILES[name]
            destination = base / "Working" / category / work_name
            destination.parent.mkdir(parents=True, exist_ok=True)
            destination.write_bytes(data)
            internal_sha = sha256_bytes(data)
            working_sha = sha256_file(destination)
            contract = png_contract(data) if name.lower().endswith(".png") else fbx_contract(data)
            if contract.get("width") and (contract["width"], contract["height"]) != (2048, 2048):
                raise RuntimeError(f"Texture dimension mismatch: {name}")
            if name.endswith("texture_0.png") or name.endswith("texture_0_normal.png"):
                if contract.get("mode") != "RGB":
                    raise RuntimeError(f"RGB texture contract failed: {name}")
            if name.endswith("_metallic.png") or name.endswith("_roughness.png"):
                if contract.get("mode") != "Grayscale":
                    raise RuntimeError(f"Grayscale texture contract failed: {name}")
            if name.lower().endswith(".fbx") and contract.get("version") != 7400:
                raise RuntimeError(f"FBX version contract failed: {name}")
            if internal_sha != working_sha:
                raise RuntimeError(f"Working copy SHA mismatch: {name}")
            records.append(
                {
                    "internal_file_name": name,
                    "internal_archive_path": info.filename,
                    "internal_file_size": len(data),
                    "internal_file_sha256": internal_sha,
                    "working_file_name": work_name,
                    "working_path": str(destination),
                    "working_file_sha256": working_sha,
                    "sha_match": True,
                    "contract": contract,
                }
            )

    manifest = {
        "schema_version": "1.0",
        "created_at": datetime.now().astimezone().isoformat(),
        "zip_source_path": str(source_zip),
        "zip_original_preservation_path": str(original_path),
        "zip_size": source_size,
        "zip_sha256": source_sha,
        "zip_original_copy_sha256": sha256_file(original_path),
        "internal_file_count": len(records),
        "internal_files": records,
        "external_communication": 0,
        "meshy_credit_consumption": 0,
        "validation_status": "PASS",
    }
    manifest_dir = base / "Working" / "Manifest"
    reports_dir = base / "Working" / "Reports"
    manifest_path = manifest_dir / "PLAYER_M_Meshy_v0.1_SourceManifest.json"
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    (reports_dir / "source_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    (reports_dir / "source_zip_evidence.json").write_text(
        json.dumps(
            {
                "source_zip": str(source_zip),
                "expected_size": EXPECTED_ZIP_SIZE,
                "actual_size": source_size,
                "expected_sha256": EXPECTED_ZIP_SHA256,
                "actual_sha256": source_sha,
                "internal_file_count": len(records),
                "zip_crc": "PASS",
                "contract_status": "PASS",
                "external_communication": 0,
                "meshy_credit_consumption": 0,
            },
            ensure_ascii=False,
            indent=2,
        ),
        encoding="utf-8",
    )
    print(f"SOURCE_PREP_PASS files={len(records)} zip_sha_match=true working_sha_mismatches=0")


if __name__ == "__main__":
    main()
