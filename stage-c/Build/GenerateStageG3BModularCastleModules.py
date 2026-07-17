from __future__ import annotations

import hashlib
import json
import math
from datetime import datetime
from pathlib import Path

import bpy


REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
OUTPUT_ROOT = (
    REPOSITORY_ROOT / "stage-c" / "SourceArt" / "StageG3B" /
    "ModularCastle" / "v0.1"
)
MODEL_DIR = OUTPUT_ROOT / "Models"
REPORT_DIR = OUTPUT_ROOT / "Reports"


def clear_scene() -> None:
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials):
        for datablock in list(datablocks):
            if datablock.users == 0:
                datablocks.remove(datablock)


def apply_bevel(obj, width: float, segments: int = 3):
    if width <= 0:
        return obj
    bpy.context.view_layer.objects.active = obj
    modifier = obj.modifiers.new(name="HandCutStoneEdge", type="BEVEL")
    modifier.width = width
    modifier.segments = segments
    modifier.limit_method = "ANGLE"
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    return obj


def box(name: str, width: float, depth: float, height: float,
        location=(0.0, 0.0, 0.0), bevel: float = 0.0):
    bpy.ops.mesh.primitive_cube_add(
        location=(location[0], location[1], location[2] + height * 0.5)
    )
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = (width, depth, height)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return apply_bevel(obj, bevel)


def mesh_object(name: str, vertices, faces, bevel: float = 0.0):
    mesh = bpy.data.meshes.new(f"{name}_Mesh")
    mesh.from_pydata(vertices, [], faces)
    mesh.update(calc_edges=True)
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    return apply_bevel(obj, bevel)


def join_objects(name: str, objects):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = objects[0]
    bpy.ops.object.join()
    objects[0].name = name
    return objects[0]


def octagonal_profile(name: str, rings, bevel: float = 0.0):
    sides = 8
    vertices = []
    for z, radius in rings:
        for index in range(sides):
            angle = math.tau * index / sides + math.pi / 8.0
            vertices.append((math.cos(angle) * radius, math.sin(angle) * radius, z))
    faces = []
    for ring_index in range(len(rings) - 1):
        start = ring_index * sides
        next_start = (ring_index + 1) * sides
        for index in range(sides):
            nxt = (index + 1) % sides
            faces.append((start + index, start + nxt, next_start + nxt, next_start + index))
    faces.append(tuple(reversed(range(sides))))
    top_start = (len(rings) - 1) * sides
    faces.append(tuple(top_start + index for index in range(sides)))
    return mesh_object(name, vertices, faces, bevel)


def wall_bay(name: str, width=300.0, depth=240.0, height=620.0):
    pieces = [
        box(f"{name}_Plinth", width + 30, depth + 30, 58, bevel=8),
        box(f"{name}_Lower", width, depth, height - 58, location=(0, 0, 58), bevel=10),
        box(f"{name}_StringCourse", width + 24, depth + 16, 24,
            location=(0, 0, 335), bevel=5),
        box(f"{name}_Cornice", width + 34, depth + 28, 34,
            location=(0, 0, height - 34), bevel=6),
    ]
    return join_objects(name, pieces)


def buttress(name: str):
    pieces = [
        box(f"{name}_Foot", 125, 205, 70, bevel=8),
        box(f"{name}_Lower", 112, 188, 240, location=(0, 10, 55), bevel=7),
        box(f"{name}_Middle", 86, 150, 220, location=(0, 23, 275), bevel=6),
        box(f"{name}_Upper", 60, 116, 175, location=(0, 35, 470), bevel=5),
    ]
    cap = octagonal_profile(
        f"{name}_Cap",
        [(0, 73), (32, 73), (105, 0)],
        bevel=3,
    )
    cap.location.z = 635
    pieces.append(cap)
    return join_objects(name, pieces)


def pointed_path(width: float, spring_z: float, apex_z: float, samples=7):
    points = [(-width * 0.5, spring_z)]
    # Two quadratic shoulders make a recognisable Gothic point, not a round portal.
    for index in range(1, samples + 1):
        t = index / samples
        x = -width * 0.5 * (1.0 - t)
        z = spring_z + (apex_z - spring_z) * (0.80 * t + 0.20 * t * t)
        points.append((x, z))
    for index in range(1, samples + 1):
        t = index / samples
        x = width * 0.5 * t
        z = apex_z - (apex_z - spring_z) * (0.80 * t + 0.20 * t * t)
        points.append((x, z))
    return points


def beam_between(name: str, p0, p1, depth: float, thickness: float, y=0.0):
    x0, z0 = p0
    x1, z1 = p1
    length = math.hypot(x1 - x0, z1 - z0)
    obj = box(name, length + thickness * 0.2, depth, thickness,
              location=((x0 + x1) * 0.5, y, (z0 + z1) * 0.5 - thickness * 0.5), bevel=3)
    obj.rotation_euler[1] = -math.atan2(z1 - z0, x1 - x0)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return obj


def gate_arch(name: str, opening_width=600.0, spring_z=395.0, apex_z=565.0, depth=250.0):
    pieces = [
        box(f"{name}_PierL", 155, depth, spring_z + 75,
            location=(-opening_width * 0.5 - 77.5, 0, 0), bevel=10),
        box(f"{name}_PierR", 155, depth, spring_z + 75,
            location=(opening_width * 0.5 + 77.5, 0, 0), bevel=10),
        box(f"{name}_PierCapL", 190, depth + 26, 38,
            location=(-opening_width * 0.5 - 77.5, 0, spring_z - 12), bevel=5),
        box(f"{name}_PierCapR", 190, depth + 26, 38,
            location=(opening_width * 0.5 + 77.5, 0, spring_z - 12), bevel=5),
    ]
    path = pointed_path(opening_width, spring_z, apex_z)
    for index, (p0, p1) in enumerate(zip(path, path[1:])):
        pieces.append(beam_between(f"{name}_Voussoir_{index:02d}", p0, p1, depth + 18, 72))
    # Stepped crown breaks the facade into readable horizontal layers.
    pieces.append(box(f"{name}_Crown", opening_width + 340, depth + 30, 95,
                      location=(0, 0, apex_z + 15), bevel=10))
    pieces.append(box(f"{name}_CrownBand", opening_width + 370, depth + 42, 28,
                      location=(0, 0, apex_z + 105), bevel=5))
    return join_objects(name, pieces)


def pointed_window_frame(name: str, width=72.0, spring_z=105.0, apex_z=155.0, depth=22.0):
    pieces = [
        box(f"{name}_Left", 18, depth, spring_z, location=(-width * 0.5 - 9, 0, 0), bevel=3),
        box(f"{name}_Right", 18, depth, spring_z, location=(width * 0.5 + 9, 0, 0), bevel=3),
        box(f"{name}_Sill", width + 38, depth + 8, 18, bevel=3),
    ]
    path = pointed_path(width, spring_z, apex_z, samples=4)
    for index, (p0, p1) in enumerate(zip(path, path[1:])):
        pieces.append(beam_between(f"{name}_Arch_{index:02d}", p0, p1, depth, 18))
    hood_path = pointed_path(width + 38, spring_z + 8, apex_z + 30, samples=4)
    for index, (p0, p1) in enumerate(zip(hood_path, hood_path[1:])):
        pieces.append(beam_between(f"{name}_Hood_{index:02d}", p0, p1, depth + 9, 10))
    return join_objects(name, pieces)


def pointed_window_inset(name: str, width=66.0, spring_z=103.0, apex_z=149.0, depth=12.0):
    points = [(-width * 0.5, 0.0), (width * 0.5, 0.0),
              (width * 0.5, spring_z), (0.0, apex_z), (-width * 0.5, spring_z)]
    vertices = [(x, -depth * 0.5, z) for x, z in points] + [(x, depth * 0.5, z) for x, z in points]
    faces = [(0, 1, 2, 3, 4), (9, 8, 7, 6, 5)]
    for index in range(5):
        nxt = (index + 1) % 5
        faces.append((index, nxt, nxt + 5, index + 5))
    return mesh_object(name, vertices, faces, bevel=2)


def gable_roof(name: str, width: float, depth: float, rise: float):
    x = width * 0.5
    y = depth * 0.5
    vertices = [
        (-x, -y, 0), (x, -y, 0), (0, -y, rise),
        (-x, y, 0), (x, y, 0), (0, y, rise),
    ]
    faces = [(0, 1, 2), (5, 4, 3), (0, 3, 4, 1), (1, 4, 5, 2), (2, 5, 3, 0)]
    roof = mesh_object(name, vertices, faces, bevel=8)
    # Deep eaves and ridge remove the thin triangular-prism look.
    eaves = [
        box(f"{name}_EaveL", 34, depth + 46, 32, location=(-x + 10, 0, -18), bevel=5),
        box(f"{name}_EaveR", 34, depth + 46, 32, location=(x - 10, 0, -18), bevel=5),
        box(f"{name}_Ridge", 32, depth + 54, 34, location=(0, 0, rise - 15), bevel=5),
    ]
    return join_objects(name, [roof] + eaves)


def tower(name: str, diameter: float, height: float):
    radius = diameter * 0.5
    body = octagonal_profile(name, [
        (0, radius * 1.12), (65, radius * 1.12), (105, radius),
        (height * 0.43, radius), (height * 0.46, radius * 1.055),
        (height * 0.50, radius * 1.055), (height * 0.53, radius),
        (height * 0.78, radius), (height * 0.81, radius * 1.10),
        (height * 0.86, radius * 1.10), (height, radius * 0.92),
    ], bevel=5)
    # Four narrow engaged piers make the silhouette vertical and non-cubic.
    pieces = [body]
    for index, angle in enumerate((45, 135, 225, 315)):
        rad = math.radians(angle)
        pieces.append(box(f"{name}_Pilaster_{index}", 50, 72, height * 0.72,
                          location=(math.cos(rad) * radius * 0.84,
                                    math.sin(rad) * radius * 0.84, 70), bevel=5))
    return join_objects(name, pieces)


def spire(name: str, diameter: float, height: float):
    radius = diameter * 0.5
    roof = octagonal_profile(name, [
        (0, radius * 1.18), (34, radius * 1.18), (70, radius),
        (height * 0.82, radius * 0.13), (height, 0.0),
    ], bevel=4)
    finial = octagonal_profile(f"{name}_Finial", [(0, 18), (45, 13), (92, 0)], bevel=2)
    finial.location.z = height
    return join_objects(name, [roof, finial])


def crenellation(name: str, length=300.0):
    pieces = [box(f"{name}_Parapet", length, 92, 72, bevel=6)]
    spacing = length / 4.0
    for index in range(4):
        x = -length * 0.5 + spacing * (index + 0.5)
        pieces.append(box(f"{name}_Merlon_{index}", 45, 105, 92,
                          location=(x, 0, 62), bevel=6))
    return join_objects(name, pieces)


def stair(name: str):
    pieces = []
    for index in range(7):
        pieces.append(box(f"{name}_{index}", 760 - index * 34, 70, 18,
                          location=(0, index * 70, index * 18), bevel=3))
    return join_objects(name, pieces)


def banner_cloth(name: str):
    vertices = [(-60, 0, 0), (60, 0, 0), (60, 0, 220), (0, 0, 185), (-60, 0, 220)]
    faces = [(0, 1, 2, 3, 4)]
    return mesh_object(name, vertices, faces, bevel=2)


def export_object(obj, asset_name: str) -> Path:
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = MODEL_DIR / f"{asset_name}.fbx"
    bpy.ops.export_scene.fbx(
        filepath=str(path), use_selection=True, object_types={"MESH"},
        axis_forward="-Y", axis_up="Z", apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS", bake_space_transform=False,
        use_mesh_modifiers=True, add_leaf_bones=False, path_mode="STRIP",
    )
    bpy.data.objects.remove(obj, do_unlink=True)
    return path


def main() -> None:
    MODEL_DIR.mkdir(parents=True, exist_ok=True)
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    clear_scene()
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.length_unit = "CENTIMETERS"
    scene.unit_settings.scale_length = 0.01

    modules = [
        (wall_bay("SM_G3BR_WallBay_300"), "SM_G3BR_WallBay_300", "stone"),
        (wall_bay("SM_G3BR_WallBayTall_300", height=780), "SM_G3BR_WallBayTall_300", "stone"),
        (gate_arch("SM_G3BR_GateArch_600"), "SM_G3BR_GateArch_600", "stone"),
        (box("SM_G3BR_TunnelWall", 150, 380, 500, bevel=12), "SM_G3BR_TunnelWall", "stone"),
        (gable_roof("SM_G3BR_TunnelRoof", 900, 420, 285), "SM_G3BR_TunnelRoof", "roof"),
        (gable_roof("SM_G3BR_RoofBaySteep_320", 340, 520, 310), "SM_G3BR_RoofBaySteep_320", "roof"),
        (gable_roof("SM_G3BR_RoofCentral_680", 700, 560, 470), "SM_G3BR_RoofCentral_680", "roof"),
        (tower("SM_G3BR_SideTower_400", 400, 1080), "SM_G3BR_SideTower_400", "stone"),
        (tower("SM_G3BR_CentralTower_520", 520, 1480), "SM_G3BR_CentralTower_520", "stone"),
        (tower("SM_G3BR_CornerTurret_180", 180, 560), "SM_G3BR_CornerTurret_180", "stone_light"),
        (spire("SM_G3BR_SideSpire_500", 500, 420), "SM_G3BR_SideSpire_500", "roof"),
        (spire("SM_G3BR_CentralSpire_640", 640, 520), "SM_G3BR_CentralSpire_640", "roof"),
        (spire("SM_G3BR_CornerSpire_220", 220, 410), "SM_G3BR_CornerSpire_220", "roof"),
        (buttress("SM_G3BR_ButtressStepped"), "SM_G3BR_ButtressStepped", "stone_light"),
        (pointed_window_frame("SM_G3BR_WindowFramePointed"), "SM_G3BR_WindowFramePointed", "stone_light"),
        (pointed_window_inset("SM_G3BR_WindowInsetPointed"), "SM_G3BR_WindowInsetPointed", "window"),
        (crenellation("SM_G3BR_Crenellation_300"), "SM_G3BR_Crenellation_300", "stone_light"),
        (stair("SM_G3BR_EntranceStair"), "SM_G3BR_EntranceStair", "plaza"),
        (box("SM_G3BR_BannerPole", 18, 18, 330, bevel=3), "SM_G3BR_BannerPole", "metal"),
        (banner_cloth("SM_G3BR_BannerCloth"), "SM_G3BR_BannerCloth", "banner"),
        (octagonal_profile("SM_G3BR_MagicPlinth", [(0, 54), (35, 54), (55, 38), (185, 32), (210, 48)], bevel=4), "SM_G3BR_MagicPlinth", "stone_dark"),
        (octagonal_profile("SM_G3BR_MagicCrystal", [(0, 34), (48, 25), (105, 0)], bevel=2), "SM_G3BR_MagicCrystal", "magic"),
    ]

    records = []
    for obj, asset_name, material_role in modules:
        path = export_object(obj, asset_name)
        payload = path.read_bytes()
        records.append({
            "asset_name": asset_name,
            "source_file": str(path),
            "material_role": material_role,
            "bytes": len(payload),
            "sha256": hashlib.sha256(payload).hexdigest().upper(),
        })

    manifest = {
        "created_at": datetime.now().astimezone().isoformat(),
        "stage": "G-3B-R",
        "module_count": len(records),
        "coordinate_unit": "centimeter",
        "external_assets": 0,
        "marketplace_assets": 0,
        "files": records,
        "status": "PASS",
    }
    (REPORT_DIR / "module_source_manifest.json").write_text(
        json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8"
    )
    print(f"STAGE_G3BR_MODULAR_SOURCE_PASS count={len(records)} external_assets=0")


if __name__ == "__main__":
    main()
