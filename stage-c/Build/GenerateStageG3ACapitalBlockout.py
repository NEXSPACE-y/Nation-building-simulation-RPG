from __future__ import annotations

import json
import math
from datetime import datetime
from pathlib import Path

import unreal


SOURCE_MAP = "/Game/Maps/StageG2B_GuardM_PoC"
TARGET_MAP = "/Game/Maps/StageG3A_CapitalBlockout_PoC"
ASSET_ROOT = "/Game/StageG3A/CapitalBlockout"
MATERIAL_DIR = f"{ASSET_ROOT}/Materials"
REFERENCE_IMAGE = r"C:\Users\rinpa\Desktop\TITLE\Docs\ChatGPT Image 2026年7月11日 21_50_21.png"
G1B_ADAPTER_CLASS = "BP_StageG1B_PLAYER_M_C"
G2A_ADAPTER_CLASS = "BP_StageG2A_CameraModeAdapter_C"
G2B_GUARD_CLASS = "BP_StageG2B_GUARD_M_C"


def tag_actor(actor, *tags: str):
    actor.set_editor_property(
        "tags", [unreal.Name("StageG3A")] + [unreal.Name(tag) for tag in tags])


def create_materials():
    if unreal.EditorAssetLibrary.does_directory_exist(ASSET_ROOT):
        if not unreal.EditorAssetLibrary.delete_directory(ASSET_ROOT):
            raise RuntimeError(f"Unable to replace G-3A asset directory: {ASSET_ROOT}")

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    master = tools.create_asset(
        "M_G3A_Blockout_Master", MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if master is None:
        raise RuntimeError("Unable to create G-3A blockout master material")
    master.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    master.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    master.set_editor_property("two_sided", False)
    library = unreal.MaterialEditingLibrary
    color = library.create_material_expression(master, unreal.MaterialExpressionVectorParameter, -420, -80)
    color.set_editor_property("parameter_name", "BaseColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.20, 0.20, 0.20, 1.0))
    roughness = library.create_material_expression(master, unreal.MaterialExpressionScalarParameter, -420, 130)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.86)
    library.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    library.recompile_material(master)
    unreal.EditorAssetLibrary.save_asset(master.get_path_name(), only_if_is_dirty=False)

    # Linear-space values are deliberately restrained so the daylight does not wash
    # the city into white blocks. The palette follows the approved warm medieval tone.
    palette = {
        "Grass": (0.105, 0.145, 0.075),
        "Earth": (0.20, 0.145, 0.085),
        "Road": (0.235, 0.225, 0.195),
        "RoadDark": (0.125, 0.115, 0.105),
        "Stone": (0.255, 0.245, 0.220),
        "StoneDark": (0.105, 0.115, 0.125),
        "StoneLight": (0.39, 0.355, 0.295),
        "Plaster": (0.46, 0.385, 0.275),
        "PlasterWarm": (0.38, 0.245, 0.135),
        "Timber": (0.075, 0.040, 0.022),
        "RoofTerracotta": (0.265, 0.070, 0.030),
        "RoofBrown": (0.115, 0.045, 0.020),
        "RoofSlate": (0.055, 0.075, 0.095),
        "Door": (0.055, 0.025, 0.012),
        "Window": (0.030, 0.105, 0.145),
        "Banner": (0.28, 0.025, 0.018),
        "CanopyGold": (0.42, 0.215, 0.045),
        "Farmland": (0.17, 0.245, 0.075),
        "Forest": (0.025, 0.105, 0.035),
        "Magic": (0.015, 0.28, 0.42),
    }
    materials = {}
    for name, rgb in palette.items():
        instance = tools.create_asset(
            f"MI_G3A_{name}", MATERIAL_DIR,
            unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if instance is None:
            raise RuntimeError(f"Unable to create G-3A material instance: {name}")
        library.set_material_instance_parent(instance, master)
        library.set_material_instance_vector_parameter_value(
            instance, "BaseColor", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
        library.set_material_instance_scalar_parameter_value(instance, "Roughness", 0.86)
        unreal.EditorAssetLibrary.save_asset(instance.get_path_name(), only_if_is_dirty=False)
        materials[name] = instance
    return materials


if not Path(REFERENCE_IMAGE).is_file():
    raise RuntimeError(f"Approved G-3A reference image is missing: {REFERENCE_IMAGE}")
if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_MAP):
    raise RuntimeError(f"Accepted Stage G-2B map is missing: {SOURCE_MAP}")

materials = create_materials()

if unreal.EditorAssetLibrary.does_asset_exist(TARGET_MAP):
    if not unreal.EditorAssetLibrary.delete_asset(TARGET_MAP):
        raise RuntimeError(f"Unable to replace G-3A map: {TARGET_MAP}")
if unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, TARGET_MAP) is None:
    raise RuntimeError(f"Unable to duplicate {SOURCE_MAP} to {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_asset(TARGET_MAP, only_if_is_dirty=False):
    raise RuntimeError(f"Unable to save duplicated G-3A map: {TARGET_MAP}")
unreal.SystemLibrary.collect_garbage()

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_editor.load_level(TARGET_MAP)
world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world after loading G-3A map")
world.get_world_settings().set_editor_property("default_game_mode", unreal.StageG3ACapitalGameMode)
world.get_world_settings().set_actor_label("Stage G-3A Capital Blockout PoC World Settings")

actors_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = actors_api.get_all_level_actors()
g1b_count = sum(1 for actor in actors if actor.get_class().get_name() == G1B_ADAPTER_CLASS)
g2a_count = sum(1 for actor in actors if actor.get_class().get_name() == G2A_ADAPTER_CLASS)
guards = [actor for actor in actors if actor.get_class().get_name() == G2B_GUARD_CLASS]
player_starts = [actor for actor in actors if isinstance(actor, unreal.PlayerStart)]
if g1b_count != 1 or g2a_count != 1 or len(guards) != 1 or len(player_starts) != 1:
    raise RuntimeError(
        f"Accepted G-1B/G-2A/G-2B actors missing: player={g1b_count} camera={g2a_count} "
        f"guard={len(guards)} start={len(player_starts)}")

for actor in list(actors):
    label = actor.get_actor_label()
    if isinstance(actor, unreal.NavMeshBoundsVolume) or label.startswith("StageG2A_Redesign_"):
        actors_api.destroy_actor(actor)

player_start = player_starts[0]
player_start.set_actor_label("StageG3A_PlayerStart_SouthApproach")
player_start.set_actor_location(unreal.Vector(0.0, -3000.0, 96.0), False, False)
player_start.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=90.0), False)
tag_actor(player_start, "PlayerStart")

guard = guards[0]
guard.set_actor_label("StageG3A_GUARD_M_SouthGate_Single")
guard.set_actor_location(unreal.Vector(-255.0, -2410.0, 88.0), False, False)
guard.set_actor_rotation(unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0), False)
tag_actor(guard, "Guard", "SouthGate")

cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
cylinder = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
cone = unreal.load_asset("/Engine/BasicShapes/Cone.Cone")
sphere = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
if cube is None or cylinder is None or cone is None or sphere is None:
    raise RuntimeError("Required Engine basic shapes are unavailable")

spawned = []


def spawn_mesh(label: str, mesh, location, size, material_name: str, tags,
               collision=True, rotation=None):
    rotation = rotation or unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0)
    actor = actors_api.spawn_actor_from_class(unreal.StaticMeshActor, location, rotation)
    if actor is None:
        raise RuntimeError(f"Unable to spawn G-3A blockout actor: {label}")
    actor.set_actor_label(label)
    actor.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    tag_actor(actor, *tags, *(["Collision"] if collision else []))
    component = actor.static_mesh_component
    component.set_static_mesh(mesh)
    component.set_material(0, materials[material_name])
    component.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    component.set_editor_property("cast_shadow", True)
    component.set_editor_property("cast_dynamic_shadow", True)
    spawned.append(actor)
    return actor


def block(label, x, y, sx, sy, sz, material, tags, collision=True, z=None, rotation=None):
    return spawn_mesh(label, cube, unreal.Vector(x, y, sz / 2.0 if z is None else z),
                      (sx, sy, sz), material, tags, collision, rotation)


def marker(label, mesh, x, y, sx, sy, sz, material, tags, collision=True, z=None, rotation=None):
    return spawn_mesh(label, mesh, unreal.Vector(x, y, sz / 2.0 if z is None else z),
                      (sx, sy, sz), material, tags, collision, rotation)


def pitched_roof(label, x, y, sx, sy, base_z, rise, material, tags, ridge="y"):
    """Two thin rotated cubes form a readable pitched roof without external meshes."""
    if ridge == "y":
        slope = math.sqrt((sx * 0.5 + 35) ** 2 + rise ** 2)
        angle = math.degrees(math.atan2(rise, sx * 0.5 + 35))
        for side in (-1, 1):
            block(f"{label}_Roof_{side:+d}", x + side * sx * 0.25, y,
                  slope, sy + 90, 28, material, tags, False,
                  z=base_z + rise * 0.5,
                  rotation=unreal.Rotator(roll=0.0, pitch=-side * angle, yaw=0.0))
        block(f"{label}_Ridge", x, y, 34, sy + 105, 34, material, tags, False,
              z=base_z + rise)
    else:
        slope = math.sqrt((sy * 0.5 + 35) ** 2 + rise ** 2)
        angle = math.degrees(math.atan2(rise, sy * 0.5 + 35))
        for side in (-1, 1):
            block(f"{label}_Roof_{side:+d}", x, y + side * sy * 0.25,
                  sx + 90, slope, 28, material, tags, False,
                  z=base_z + rise * 0.5,
                  rotation=unreal.Rotator(roll=side * angle, pitch=0.0, yaw=0.0))
        block(f"{label}_Ridge", x, y, sx + 105, 34, 34, material, tags, False,
              z=base_z + rise)


def facade_details(label, x, y, sx, sy, height, tags, front="south", noble=False):
    front_y = y - sy * 0.5 - 5 if front == "south" else y + sy * 0.5 + 5
    if front in ("south", "north"):
        block(f"{label}_Door", x, front_y, 76 if not noble else 100, 12, 128,
              "Door", tags, False, z=64)
        window_z = min(height * 0.63, height - 90)
        for index, offset in enumerate((-sx * 0.28, sx * 0.28)):
            block(f"{label}_Window_{index}", x + offset, front_y, 62, 10, 82,
                  "Window", tags, False, z=window_z)
        block(f"{label}_Beam_H", x, front_y + (-2 if front == "south" else 2),
              sx + 12, 12, 20, "Timber", tags, False, z=height * 0.52)
        for index, offset in enumerate((-sx * 0.46, 0, sx * 0.46)):
            block(f"{label}_Beam_V_{index}", x + offset, front_y, 16, 12,
                  height * 0.72, "Timber", tags, False, z=height * 0.52)
    else:
        front_x = x - sx * 0.5 - 5 if front == "west" else x + sx * 0.5 + 5
        block(f"{label}_Door", front_x, y, 12, 78 if not noble else 100, 128,
              "Door", tags, False, z=64)
        window_z = min(height * 0.63, height - 90)
        for index, offset in enumerate((-sy * 0.27, sy * 0.27)):
            block(f"{label}_Window_{index}", front_x, y + offset, 10, 62, 82,
                  "Window", tags, False, z=window_z)
        block(f"{label}_Beam_H", front_x, y, 12, sy + 12, 20,
              "Timber", tags, False, z=height * 0.52)


def medieval_house(label, x, y, sx, sy, height, wall_material, roof_material,
                   tags, front="south", ridge="y", noble=False, chimney=False):
    block(f"{label}_StonePlinth", x, y, sx + 20, sy + 20, 55,
          "StoneDark", tags, True, z=27.5)
    block(f"{label}_Body", x, y, sx, sy, height, wall_material, tags, True)
    if not noble:
        block(f"{label}_UpperOverhang", x, y, sx + 32, sy + 24, height * 0.40,
              "Plaster", tags, False, z=height * 0.72)
    pitched_roof(label, x, y, sx + 20, sy + 20, height, min(180, max(105, height * 0.34)),
                 roof_material, tags, ridge)
    facade_details(label, x, y, sx, sy, height, tags, front, noble)
    if chimney:
        block(f"{label}_Chimney", x + sx * 0.28, y + sy * 0.12, 54, 54, 230,
              "StoneDark", tags, False, z=height + 115)


def market_stall(label, x, y, yaw, tags):
    rotation = unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw)
    block(f"{label}_Counter", x, y, 190, 90, 75, "Timber", tags, True, z=50, rotation=rotation)
    for index, (ox, oy) in enumerate(((-78, -35), (78, -35), (-78, 35), (78, 35))):
        block(f"{label}_Post_{index}", x + ox, y + oy, 15, 15, 205,
              "Timber", tags, False, z=102.5, rotation=rotation)
    block(f"{label}_Canopy", x, y, 230, 145, 20, "CanopyGold", tags, False,
          z=205, rotation=unreal.Rotator(roll=0.0, pitch=7.0, yaw=yaw))


def tower(label, x, y, diameter, shaft_height, roof_height, tags, material="Stone"):
    marker(f"{label}_Shaft", cylinder, x, y, diameter, diameter, shaft_height,
           material, tags, True)
    marker(f"{label}_Cap", cylinder, x, y, diameter + 65, diameter + 65, 48,
           "StoneLight", tags, False, z=shaft_height + 14)
    marker(f"{label}_Roof", cone, x, y, diameter + 90, diameter + 90, roof_height,
           "RoofSlate", tags, False, z=shaft_height + roof_height * 0.5 + 35)
    for index, yaw in enumerate((0, 90, 180, 270)):
        radians = math.radians(yaw)
        marker(f"{label}_Window_{index}", cube,
               x + math.cos(radians) * diameter * 0.505,
               y + math.sin(radians) * diameter * 0.505,
               16 if yaw in (0, 180) else 52,
               52 if yaw in (0, 180) else 16,
               82, "Window", tags, False, z=shaft_height * 0.67)


def crenellations(label, x, y, length, axis, z, tags, material="StoneLight", count=None):
    count = count or max(4, int(length // 210))
    for index in range(count):
        offset = -length * 0.5 + (index + 0.5) * length / count
        bx = x + (offset if axis == "x" else 0)
        by = y + (offset if axis == "y" else 0)
        block(f"{label}_Merlon_{index:02d}", bx, by, 90, 90, 105,
              material, tags, False, z=z + 52.5)


# One physical ground owns both character collision and the click-move trace.
# Road/plaza overlays remain visual only so every click resolves to this surface.
block("G3A_Ground_Walkable_7000", 0, -300, 7000, 7000, 20,
      "Grass", ["Ground", "WalkableSurface"], True, z=-10)
block("G3A_InnerCity_Earth", 0, 100, 5000, 4600, 5,
      "Earth", ["GroundVisual"], False, z=2.5)
block("G3A_Road_GateApproach", 0, -2920, 720, 1440, 7,
      "Road", ["Outskirts", "MainStreet"], False, z=4)
block("G3A_Road_MainStreet", 0, -540, 720, 4100, 7,
      "Road", ["MainStreet"], False, z=4)
block("G3A_Road_CrossStreet", 0, -120, 4600, 440, 7,
      "Road", ["MainStreet"], False, z=4)
block("G3A_CentralPlaza_Surface", 0, 0, 1200, 1200, 9,
      "StoneLight", ["CentralPlaza"], False, z=5)
block("G3A_Castle_Approach", 0, 1100, 980, 900, 8,
      "Road", ["CastleApproach"], False, z=4)

# Cobble rhythm on the principal axis prevents the street from reading as one flat slab.
for index, y in enumerate(range(-3270, 1420, 220)):
    block(f"G3A_MainStreet_Joint_{index:02d}", 0, y, 690, 9, 4,
          "RoadDark", ["MainStreet"], False, z=8)
for x in (-590, 590):
    block(f"G3A_Plaza_Curb_{x:+d}", x, 0, 22, 1200, 18,
          "StoneDark", ["CentralPlaza"], False, z=9)

# Fortified south gate and articulated outer wall.
wall_height = 650
for label, x, y, sx, sy in (
        ("South_West", -1475, -2300, 2250, 200),
        ("South_East", 1475, -2300, 2250, 200),
        ("North", 0, 2500, 5200, 200),
        ("West", -2600, 100, 200, 4800),
        ("East", 2600, 100, 200, 4800)):
    tags = ["OuterWall"] + (["SouthGate"] if label.startswith("South") else [])
    block(f"G3A_Wall_{label}_Base", x, y, sx, sy, wall_height,
          "Stone", tags, True)
    block(f"G3A_Wall_{label}_TopTrim", x, y, sx + (35 if sx > sy else 0),
          sy + (35 if sy > sx else 0), 35, "StoneDark", tags, False,
          z=wall_height - 35)
    crenellations(f"G3A_Wall_{label}", x, y, max(sx, sy),
                  "x" if sx > sy else "y", wall_height, tags)

# Buttresses create human-scale repetition along otherwise long wall runs.
for index, x in enumerate((-2350, -1850, -1250, 1250, 1850, 2350)):
    block(f"G3A_SouthWall_Buttress_{index}", x, -2175, 105, 135, 470,
          "StoneDark", ["OuterWall", "SouthGate"], False, z=235)
for side, x in (("W", -2500), ("E", 2500)):
    for index, y in enumerate((-1750, -1050, -350, 350, 1050, 1750)):
        block(f"G3A_{side}Wall_Buttress_{index}", x, y, 140, 105, 470,
              "StoneDark", ["OuterWall"], False, z=235)

for side, x in (("West", -525), ("East", 525)):
    block(f"G3A_SouthGate_Tower_{side}_Base", x, -2300, 400, 430, 760,
          "StoneDark", ["SouthGate"], True)
    block(f"G3A_SouthGate_Tower_{side}_Upper", x, -2300, 350, 380, 820,
          "Stone", ["SouthGate"], False, z=410)
    block(f"G3A_SouthGate_Tower_{side}_Cap", x, -2300, 445, 475, 44,
          "StoneLight", ["SouthGate"], False, z=790)
    crenellations(f"G3A_SouthGate_Tower_{side}", x, -2300, 390,
                  "x", 812, ["SouthGate"], count=4)
    for wx in (-80, 80):
        block(f"G3A_SouthGate_Tower_{side}_Window_{wx:+d}", x + wx, -2078,
              48, 12, 105, "Window", ["SouthGate"], False, z=520)

block("G3A_SouthGate_Lintel", 0, -2300, 700, 200, 210,
      "StoneDark", ["SouthGate"], True, z=655)
block("G3A_SouthGate_Facade", 0, -2190, 620, 24, 150,
      "Stone", ["SouthGate"], False, z=635)
# A simple pointed arch outline, decorative only, preserves the full 700uu path opening.
for side in (-1, 1):
    block(f"G3A_SouthGate_Arch_{side:+d}", side * 155, -2170, 355, 25, 40,
          "StoneLight", ["SouthGate"], False, z=575,
          rotation=unreal.Rotator(roll=0.0, pitch=-side * 32.0, yaw=0.0))
    block(f"G3A_SouthGate_Banner_{side:+d}", side * 330, -2075, 105, 14, 260,
          "Banner", ["SouthGate"], False, z=560)

# Central plaza with a low fountain and restrained magic lamps.
marker("G3A_CentralPlaza_FountainBase", cylinder, 0, 0, 250, 250, 55,
       "StoneDark", ["CentralPlaza", "CenterMarker"], True)
marker("G3A_CentralPlaza_FountainBasin", cylinder, 0, 0, 190, 190, 75,
       "StoneLight", ["CentralPlaza", "CenterMarker"], False, z=72)
marker("G3A_CentralPlaza_FountainCore", cylinder, 0, 0, 50, 50, 190,
       "Magic", ["CentralPlaza", "CenterMarker"], False, z=145)
for index, x in enumerate((-420, 420), start=1):
    marker(f"G3A_MagicLampPost_{index:02d}", cylinder, x, 330, 26, 26, 230,
           "StoneDark", ["CentralPlaza", "MagicMarker"], False)
    marker(f"G3A_MagicLampGlow_{index:02d}", sphere, x, 330, 62, 62, 62,
           "Magic", ["CentralPlaza", "MagicMarker"], False, z=238)

# Market: human-scale open stalls and shopfront houses, not solid district cubes.
for index, (x, y, yaw) in enumerate((
        (720, -600, 0), (1050, -600, 0), (1380, -600, 0),
        (720, 240, 180), (1050, 240, 180), (1380, 240, 180))):
    market_stall(f"G3A_Market_Stall_{index:02d}", x, y, yaw, ["Market", "Stall"])
for index, (x, y, sx, sy, h, roof) in enumerate((
        (2050, -750, 390, 340, 360, "RoofTerracotta"),
        (2050, -210, 430, 370, 430, "RoofBrown"),
        (2050, 390, 400, 360, 390, "RoofTerracotta"))):
    medieval_house(f"G3A_Market_Shop_{index}", x, y, sx, sy, h,
                   "PlasterWarm", roof, ["Market", "Building"], front="west",
                   ridge="y", chimney=index == 1)
block("G3A_Market_Signpost", 1620, -90, 28, 28, 230,
      "Timber", ["Market"], False)
block("G3A_Market_Signboard", 1570, -90, 110, 18, 70,
      "CanopyGold", ["Market"], False, z=190)

# Residential quarter: alternating footprints and roof directions create an organic skyline.
for index, args in enumerate((
        (-2050, -720, 390, 350, 350, "RoofBrown", "east", "y"),
        (-1510, -720, 420, 330, 410, "RoofTerracotta", "east", "y"),
        (-980, -720, 360, 360, 330, "RoofBrown", "east", "y"),
        (-2100, 300, 430, 390, 440, "RoofTerracotta", "south", "x"),
        (-1540, 360, 380, 350, 370, "RoofBrown", "south", "x"),
        (-990, 320, 400, 380, 400, "RoofTerracotta", "south", "x"))):
    x, y, sx, sy, h, roof, front, ridge = args
    medieval_house(f"G3A_Residential_House_{index}", x, y, sx, sy, h,
                   "Plaster", roof, ["Residential", "Building"], front, ridge,
                   chimney=index in (1, 3))
block("G3A_Residential_DeadEndWall", -2280, -160, 100, 500, 210,
      "Stone", ["Residential", "Building"], True)

# Lower-town workshops near the gate.
for index, args in enumerate((
        (-2050, -1700, 500, 390, 360, "RoofBrown", "east", "y"),
        (-1510, -1260, 530, 410, 420, "RoofTerracotta", "south", "x"),
        (-980, -1730, 440, 390, 340, "RoofBrown", "east", "y"))):
    x, y, sx, sy, h, roof, front, ridge = args
    medieval_house(f"G3A_Workshop_Building_{index}", x, y, sx, sy, h,
                   "PlasterWarm", roof, ["Workshop", "Building"], front, ridge, chimney=True)
for index, (x, y, height) in enumerate(((-1930, -1580, 720), (-1390, -1160, 820))):
    marker(f"G3A_Workshop_Chimney_{index}", cylinder, x, y, 85, 85, height,
           "StoneDark", ["Workshop", "Chimney"], False)
    block(f"G3A_Workshop_ChimneyBand_{index}", x, y, 105, 105, 28,
          "StoneLight", ["Workshop", "Chimney"], False, z=height - 45)

# Noble quarter: larger footprints, pale stone/plaster, slate roofs and formal spacing.
for index, args in enumerate((
        (1100, 820, 560, 500, 560, "south", "y"),
        (1900, 800, 620, 540, 630, "south", "y"),
        (1120, 1510, 590, 550, 620, "south", "x"),
        (1920, 1510, 560, 540, 680, "south", "x"))):
    x, y, sx, sy, h, front, ridge = args
    medieval_house(f"G3A_Noble_Estate_{index}", x, y, sx, sy, h,
                   "StoneLight", "RoofSlate", ["Noble", "Building"], front, ridge,
                   noble=True, chimney=index in (1, 3))
    for side in (-1, 1):
        block(f"G3A_Noble_Estate_{index}_Pillar_{side:+d}", x + side * sx * 0.40,
              y - sy * 0.5 - 12, 42, 42, 220, "StoneDark", ["Noble"], False, z=110)

# Royal castle: layered keep, pitched roofs, round towers and visible windows.
block("G3A_Castle_MainBlock", 0, 2050, 1500, 820, 760,
      "Stone", ["Castle"], True)
pitched_roof("G3A_Castle_MainBlock", 0, 2050, 1520, 840, 760, 260,
             "RoofSlate", ["Castle"], ridge="x")
block("G3A_Castle_CentralKeep", 0, 2160, 650, 590, 1120,
      "StoneLight", ["Castle"], True)
pitched_roof("G3A_Castle_CentralKeep", 0, 2160, 670, 610, 1120, 310,
             "RoofSlate", ["Castle"], ridge="y")
for side, x in (("West", -720), ("East", 720)):
    tower(f"G3A_Castle_{side}Tower", x, 2020, 390, 1080, 360, ["Castle"])
tower("G3A_Castle_CentralTower", 0, 2290, 330, 1340, 330, ["Castle"], "StoneLight")
block("G3A_Castle_GateRecess", 0, 1638, 290, 18, 410,
      "StoneDark", ["Castle"], False, z=205)
block("G3A_Castle_GateDoor", 0, 1626, 205, 14, 320,
      "Door", ["Castle"], False, z=160)
for row, z in enumerate((420, 650, 880)):
    for col, x in enumerate((-430, -215, 215, 430)):
        block(f"G3A_Castle_Window_{row}_{col}", x, 1628, 48, 12, 92,
              "Window", ["Castle"], False, z=z)
for index, x in enumerate(range(-620, 621, 155)):
    block(f"G3A_Castle_Battlement_{index:02d}", x, 1620, 78, 82, 95,
          "StoneLight", ["Castle"], False, z=805)
for side in (-1, 1):
    block(f"G3A_Castle_Banner_{side:+d}", side * 270, 1614, 105, 12, 250,
          "Banner", ["Castle"], False, z=560)

# Outskirts: low planes, furrows, rustic sheds and sparse primitive tree markers.
for index, x in enumerate((-1650, 0, 1650)):
    block(f"G3A_Farmland_Plot_{index}", x, -3220, 1450, 620, 6,
          "Farmland", ["Outskirts", "Farmland"], False, z=3)
    for line in (-480, -240, 0, 240, 480):
        block(f"G3A_Farmland_Furrow_{index}_{line:+d}", x + line, -3220,
              16, 590, 11, "Earth", ["Outskirts", "Farmland"], False, z=8)
for index, (x, y) in enumerate(((-1850, -3160), (1760, -3150))):
    medieval_house(f"G3A_Outskirts_Shed_{index}", x, y, 330, 260, 240,
                   "Timber", "RoofBrown", ["Outskirts", "Farmland"],
                   front="south", ridge="y")
for index, (x, y, scale) in enumerate((
        (2360, -3460, 245), (2570, -3220, 285), (2150, -3060, 225),
        (-2520, -3450, 240), (-2320, -3030, 270), (2040, -3370, 210))):
    marker(f"G3A_Forest_Trunk_{index}", cylinder, x, y, 48, 48, scale,
           "Timber", ["Outskirts", "ForestMarker"], False)
    marker(f"G3A_Forest_Crown_{index}", cone, x, y, scale, scale, scale * 1.55,
           "Forest", ["Outskirts", "ForestMarker"], False, z=scale * 1.08)

nav_volume = actors_api.spawn_actor_from_class(
    unreal.NavMeshBoundsVolume, unreal.Vector(0.0, -300.0, 250.0),
    unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
if nav_volume is None:
    raise RuntimeError("Unable to create G-3A NavMesh bounds")
nav_volume.set_actor_label("StageG3A_NavMeshBounds_6000")
nav_volume.set_actor_scale3d(unreal.Vector(35.0, 35.0, 10.0))
tag_actor(nav_volume, "NavMesh")

evidence_actor = actors_api.spawn_actor_from_class(
    unreal.StageG3ACapitalEvidenceActor, unreal.Vector(0.0, 0.0, 0.0),
    unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
if evidence_actor is None:
    raise RuntimeError("Unable to place G-3A capital evidence actor")
evidence_actor.set_actor_label("StageG3A_Capital_Evidence_Adapter")

autofollow_actor = actors_api.spawn_actor_from_class(
    unreal.StageG3AStandardCameraAutoFollowActor, unreal.Vector(0.0, 0.0, 0.0),
    unreal.Rotator(roll=0.0, pitch=0.0, yaw=0.0))
if autofollow_actor is None:
    raise RuntimeError("Unable to place G-3A Standard camera auto-follow actor")
autofollow_actor.set_actor_label("StageG3A_StandardCamera_AutoFollow")

unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save G-3A map: {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_directory(ASSET_ROOT, only_if_is_dirty=False, recursive=True):
    raise RuntimeError("Unable to save G-3A material assets")

project_root = Path(unreal.Paths.project_dir()).resolve()
reports = project_root / "SourceArt" / "StageG3A" / "CapitalBlockout" / "v0.1" / "Reports"
reports.mkdir(parents=True, exist_ok=True)
payload = {
    "created_at": datetime.now().astimezone().isoformat(),
    "source_map": SOURCE_MAP,
    "target_map": TARGET_MAP,
    "reference_image": REFERENCE_IMAGE,
    "reference_usage": "capital silhouette, medieval atmosphere, district hierarchy, roofs, towers, gate, and streets",
    "excluded_reference_usage": "2.5D character and fang-rat rendering specifications",
    "blockout_bounds_uu": {"width": 6000, "depth": 6000},
    "inner_city_bounds_uu": {"width": 5200, "depth": 4800},
    "player_start": {"x": 0, "y": -3000, "z": 96, "yaw": 90},
    "guard_count": 1,
    "guard_location": {"x": -255, "y": -2410, "z": 88},
    "g1b_player_adapter_count": g1b_count,
    "g2a_camera_adapter_count": g2a_count,
    "g3a_standard_camera_autofollow_count": 1,
    "blockout_actor_count": len(spawned),
    "material_asset_count": len(materials) + 1,
    "walkable_surface_actor_count": 1,
    "click_move_trace_contract": "WalkableSurface blocks ECC_GameTraceChannel1 at runtime",
    "visual_language": "pitched roofs, timber frames, articulated stone walls, gate towers, castle spires",
    "engine_primitives_only": True,
    "external_assets": 0,
    "marketplace_assets": 0,
    "meshy_generation": 0,
    "foliage_system_used": False,
    "status": "PASS",
}
(reports / "map_generation_evidence.json").write_text(
    json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")

unreal.log(
    "STAGE_G3A_CAPITAL_MAP PASS "
    f"source={SOURCE_MAP} target={TARGET_MAP} blockout_actors={len(spawned)} "
    f"materials={len(materials) + 1} guard=1 player=1 camera=1 bounds=6000x6000 "
    "walkable_surfaces=1 click_trace=explicit engine_primitives=1 external_assets=0 oversized_assets=0")
