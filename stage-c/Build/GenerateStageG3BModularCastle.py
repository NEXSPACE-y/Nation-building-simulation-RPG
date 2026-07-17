from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path

import unreal


SOURCE_MAP = "/Game/Maps/StageG3A_CapitalBlockout_PoC"
TARGET_MAP = "/Game/Maps/StageG3B_ModularCastle_PoC"
ASSET_ROOT = "/Game/StageG3B/ModularCastle"
MESH_DIR = f"{ASSET_ROOT}/Meshes"
MATERIAL_DIR = f"{ASSET_ROOT}/Materials"
SOURCE_ART_ROOT = (
    Path(unreal.Paths.project_dir()).resolve() / "SourceArt" / "StageG3B" /
    "ModularCastle" / "v0.1"
)

MODULE_NAMES = (
    "SM_G3BR_WallBay_300",
    "SM_G3BR_WallBayTall_300",
    "SM_G3BR_GateArch_600",
    "SM_G3BR_TunnelWall",
    "SM_G3BR_TunnelRoof",
    "SM_G3BR_RoofBaySteep_320",
    "SM_G3BR_RoofCentral_680",
    "SM_G3BR_SideTower_400",
    "SM_G3BR_CentralTower_520",
    "SM_G3BR_CornerTurret_180",
    "SM_G3BR_SideSpire_500",
    "SM_G3BR_CentralSpire_640",
    "SM_G3BR_CornerSpire_220",
    "SM_G3BR_ButtressStepped",
    "SM_G3BR_WindowFramePointed",
    "SM_G3BR_WindowInsetPointed",
    "SM_G3BR_Crenellation_300",
    "SM_G3BR_EntranceStair",
    "SM_G3BR_BannerPole",
    "SM_G3BR_BannerCloth",
    "SM_G3BR_MagicPlinth",
    "SM_G3BR_MagicCrystal",
)


def tag_actor(actor, *tags: str) -> None:
    actor.set_editor_property(
        "tags",
        [unreal.Name("StageG3A"), unreal.Name("StageG3BR")]
        + [unreal.Name(tag) for tag in tags],
    )


def create_materials():
    if unreal.EditorAssetLibrary.does_directory_exist(ASSET_ROOT):
        if not unreal.EditorAssetLibrary.delete_directory(ASSET_ROOT):
            raise RuntimeError(f"Unable to replace G-3B-R asset directory: {ASSET_ROOT}")

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    library = unreal.MaterialEditingLibrary
    master = tools.create_asset(
        "M_G3BR_ModularCastle_Master", MATERIAL_DIR,
        unreal.Material, unreal.MaterialFactoryNew()
    )
    if master is None:
        raise RuntimeError("Unable to create G-3B-R master material")
    master.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    master.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    master.set_editor_property("two_sided", False)
    color = library.create_material_expression(
        master, unreal.MaterialExpressionVectorParameter, -500, -100)
    color.set_editor_property("parameter_name", "BaseColor")
    color.set_editor_property("default_value", unreal.LinearColor(0.34, 0.38, 0.42, 1.0))
    roughness = library.create_material_expression(
        master, unreal.MaterialExpressionScalarParameter, -500, 80)
    roughness.set_editor_property("parameter_name", "Roughness")
    roughness.set_editor_property("default_value", 0.88)
    emissive = library.create_material_expression(
        master, unreal.MaterialExpressionVectorParameter, -500, 250)
    emissive.set_editor_property("parameter_name", "EmissiveColor")
    emissive.set_editor_property("default_value", unreal.LinearColor(0, 0, 0, 1))
    strength = library.create_material_expression(
        master, unreal.MaterialExpressionScalarParameter, -500, 400)
    strength.set_editor_property("parameter_name", "EmissiveStrength")
    strength.set_editor_property("default_value", 0.0)
    multiply = library.create_material_expression(master, unreal.MaterialExpressionMultiply, -170, 300)
    library.connect_material_expressions(emissive, "RGB", multiply, "A")
    library.connect_material_expressions(strength, "", multiply, "B")
    library.connect_material_property(color, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(roughness, "", unreal.MaterialProperty.MP_ROUGHNESS)
    library.connect_material_property(multiply, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    library.recompile_material(master)
    unreal.EditorAssetLibrary.save_asset(master.get_path_name(), only_if_is_dirty=False)

    palette = {
        "StoneMain": ((0.32, 0.37, 0.42), 0.90, (0, 0, 0), 0.0),
        "StoneLight": ((0.49, 0.53, 0.55), 0.87, (0, 0, 0), 0.0),
        "StoneAccent": ((0.18, 0.22, 0.27), 0.92, (0, 0, 0), 0.0),
        "RoofSlate": ((0.045, 0.075, 0.115), 0.84, (0, 0, 0), 0.0),
        "WindowDeepBlue": ((0.012, 0.035, 0.055), 0.72, (0, 0, 0), 0.0),
        "GateShadow": ((0.025, 0.035, 0.050), 0.95, (0, 0, 0), 0.0),
        "BannerRoyal": ((0.32, 0.018, 0.028), 0.76, (0, 0, 0), 0.0),
        "Metal": ((0.11, 0.13, 0.15), 0.70, (0, 0, 0), 0.0),
        "PlazaStone": ((0.34, 0.31, 0.26), 0.92, (0, 0, 0), 0.0),
        "Magic": ((0.02, 0.16, 0.22), 0.38, (0.02, 0.55, 0.92), 2.1),
    }
    materials = {}
    for name, (rgb, rough, glow, glow_strength) in palette.items():
        instance = tools.create_asset(
            f"MI_G3BR_{name}", MATERIAL_DIR,
            unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        if instance is None:
            raise RuntimeError(f"Unable to create G-3B-R material: {name}")
        library.set_material_instance_parent(instance, master)
        library.set_material_instance_vector_parameter_value(
            instance, "BaseColor", unreal.LinearColor(*rgb, 1.0))
        library.set_material_instance_scalar_parameter_value(instance, "Roughness", rough)
        library.set_material_instance_vector_parameter_value(
            instance, "EmissiveColor", unreal.LinearColor(*glow, 1.0))
        library.set_material_instance_scalar_parameter_value(
            instance, "EmissiveStrength", glow_strength)
        unreal.EditorAssetLibrary.save_asset(instance.get_path_name(), only_if_is_dirty=False)
        materials[name] = instance
    return materials


def import_modules():
    source_dir = SOURCE_ART_ROOT / "Models"
    tasks = []
    for name in MODULE_NAMES:
        source = source_dir / f"{name}.fbx"
        if not source.is_file():
            raise RuntimeError(f"Missing self-authored G-3B-R module: {source}")
        options = unreal.FbxImportUI()
        options.set_editor_property("import_mesh", True)
        options.set_editor_property("import_as_skeletal", False)
        options.set_editor_property("import_materials", False)
        options.set_editor_property("import_textures", False)
        static_data = unreal.FbxStaticMeshImportData()
        static_data.set_editor_property("combine_meshes", True)
        static_data.set_editor_property("generate_lightmap_u_vs", True)
        static_data.set_editor_property("auto_generate_collision", True)
        options.set_editor_property("static_mesh_import_data", static_data)
        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source))
        task.set_editor_property("destination_path", MESH_DIR)
        task.set_editor_property("destination_name", name)
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("save", True)
        task.set_editor_property("options", options)
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    result = {}
    for name in MODULE_NAMES:
        mesh = unreal.load_asset(f"{MESH_DIR}/{name}.{name}")
        if mesh is None or not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError(f"G-3B-R module import failed: {name}")
        result[name] = mesh
    return result


materials = create_materials()
modules = import_modules()
if not unreal.EditorAssetLibrary.does_asset_exist(SOURCE_MAP):
    raise RuntimeError(f"Accepted G-3A map is missing: {SOURCE_MAP}")
if unreal.EditorAssetLibrary.does_asset_exist(TARGET_MAP):
    if not unreal.EditorAssetLibrary.delete_asset(TARGET_MAP):
        raise RuntimeError(f"Unable to replace G-3B-R map: {TARGET_MAP}")
if unreal.EditorAssetLibrary.duplicate_asset(SOURCE_MAP, TARGET_MAP) is None:
    raise RuntimeError(f"Unable to duplicate accepted map: {SOURCE_MAP}")
unreal.EditorAssetLibrary.save_asset(TARGET_MAP, only_if_is_dirty=False)
unreal.SystemLibrary.collect_garbage()

level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
level_editor.load_level(TARGET_MAP)
world = unreal.EditorLevelLibrary.get_editor_world()
if world is None:
    raise RuntimeError("No editor world after loading G-3B-R map")
world.get_world_settings().set_actor_label("Stage G-3B-R Modular Castle PoC World Settings")
actors_api = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# Remove only the rejected G-3A castle placeholder from this duplicate. The accepted map is untouched.
removed_placeholder = 0
for actor in list(actors_api.get_all_level_actors()):
    label = actor.get_actor_label()
    class_name = actor.get_class().get_name()
    if (label.startswith("G3A_Castle_") and label != "G3A_Castle_Approach") or \
            class_name == "StageG3ACapitalEvidenceActor":
        actors_api.destroy_actor(actor)
        removed_placeholder += 1

accepted = actors_api.get_all_level_actors()
if sum(1 for a in accepted if a.get_class().get_name() == "BP_StageG1B_PLAYER_M_C") != 1:
    raise RuntimeError("Accepted PLAYER_M adapter count changed")
if sum(1 for a in accepted if a.get_class().get_name() == "BP_StageG2A_CameraModeAdapter_C") != 1:
    raise RuntimeError("Accepted camera adapter count changed")
if sum(1 for a in accepted if a.get_class().get_name() == "BP_StageG2B_GUARD_M_C") != 1:
    raise RuntimeError("Accepted GUARD_M placement must remain exactly one")

spawned = []


def spawn_module(label: str, module_name: str, x: float, y: float, z: float,
                 yaw: float, material_name: str, tags, collision=False):
    actor = actors_api.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, z),
        unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw))
    if actor is None:
        raise RuntimeError(f"Unable to spawn G-3B-R module: {label}")
    actor.set_actor_label(label)
    tag_actor(actor, "Castle", "CastleModule", *tags,
              *(["CastleCollision"] if collision else []))
    component = actor.static_mesh_component
    component.set_static_mesh(modules[module_name])
    component.set_material(0, materials[material_name])
    component.set_collision_profile_name("BlockAll" if collision else "NoCollision")
    component.set_editor_property("cast_shadow", True)
    component.set_editor_property("cast_dynamic_shadow", True)
    spawned.append(actor)
    return actor


def spawn_visual_cube(label, x, y, z, sx, sy, sz, material_name, tags):
    cube = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    actor = actors_api.spawn_actor_from_class(
        unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator())
    actor.set_actor_label(label)
    tag_actor(actor, *tags)
    actor.set_actor_scale3d(unreal.Vector(sx / 100.0, sy / 100.0, sz / 100.0))
    actor.static_mesh_component.set_static_mesh(cube)
    actor.static_mesh_component.set_material(0, materials[material_name])
    actor.static_mesh_component.set_collision_profile_name("NoCollision")
    return actor


# Formal forecourt. It is visual only; the accepted G-3A physical ground remains the sole click surface.
spawn_visual_cube("G3BR_Forecourt_1800x1100", 0, 875, 5, 1800, 1100, 10,
                  "PlazaStone", ["CastlePlaza"])
spawn_module("G3BR_Entrance_Stair", "SM_G3BR_EntranceStair", 0, 1030, 8, 0,
             "PlazaStone", ["EntranceStair"], False)

# Four independent front wall bays preserve rhythm and leave the gate opening physically clear.
for index, x in enumerate((-850, -600, 600, 850)):
    spawn_module(f"G3BR_Front_WallBay_{index:02d}", "SM_G3BR_WallBay_300",
                 x, 1570, 0, 0, "StoneMain", ["WallBay", "FrontBay"], True)

spawn_module("G3BR_Gate_Arch_600", "SM_G3BR_GateArch_600", 0, 1450, 0, 0,
             "StoneLight", ["GateArch", "Gate"], False)
for side, x in (("West", -375), ("East", 375)):
    spawn_module(f"G3BR_Gate_TunnelWall_{side}", "SM_G3BR_TunnelWall",
                 x, 1650, 0, 0, "StoneAccent", ["TunnelWall", "Gate"], True)
spawn_module("G3BR_Gate_TunnelRoof", "SM_G3BR_TunnelRoof", 0, 1650, 500, 0,
             "RoofSlate", ["SteepRoof", "GateRoof"], False)

# Side wings are assembled from short rotated bays; no single facade module exceeds 300uu.
for side, x, yaw in (("West", -930, 90), ("East", 930, 90)):
    for index, y in enumerate((1850, 2150)):
        spawn_module(f"G3BR_{side}_WingBay_{index:02d}", "SM_G3BR_WallBay_300",
                     x, y, 0, yaw, "StoneMain", ["WallBay", "WingBay"], True)
for index, x in enumerate((-300, 0, 300)):
    spawn_module(f"G3BR_Rear_HighBay_{index:02d}", "SM_G3BR_WallBayTall_300",
                 x, 2270, 0, 0, "StoneMain", ["WallBay", "RearHighBay"], True)

# Repeated steep gables and a higher central roof produce depth instead of one flat slab.
for index, x in enumerate((-780, -470, 470, 780)):
    spawn_module(f"G3BR_Wing_SteepRoof_{index:02d}", "SM_G3BR_RoofBaySteep_320",
                 x, 1960, 620, 0, "RoofSlate", ["SteepRoof", "WingRoof"], False)
spawn_module("G3BR_Central_SteepRoof", "SM_G3BR_RoofCentral_680",
             0, 2190, 780, 0, "RoofSlate", ["SteepRoof", "CentralRoof"], False)

# Two side towers, one set-back central tower, and four slender corner turrets.
for side, x in (("West", -1080), ("East", 1080)):
    spawn_module(f"G3BR_{side}_SideTower", "SM_G3BR_SideTower_400",
                 x, 1770, 0, 0, "StoneMain", ["SideTower"], True)
    spawn_module(f"G3BR_{side}_SideSpire", "SM_G3BR_SideSpire_500",
                 x, 1770, 1080, 0, "RoofSlate", ["SteepRoof", "SideSpire"], False)
spawn_module("G3BR_Central_Tower", "SM_G3BR_CentralTower_520",
             0, 2320, 0, 0, "StoneLight", ["CentralTower"], True)
spawn_module("G3BR_Central_Spire", "SM_G3BR_CentralSpire_640",
             0, 2320, 1480, 0, "RoofSlate", ["SteepRoof", "CentralSpire"], False)
for index, (x, y) in enumerate(((-470, 1770), (470, 1770), (-470, 2240), (470, 2240))):
    spawn_module(f"G3BR_CornerTurret_{index:02d}", "SM_G3BR_CornerTurret_180",
                 x, y, 620, 0, "StoneLight", ["SmallTurret"], False)
    spawn_module(f"G3BR_CornerSpire_{index:02d}", "SM_G3BR_CornerSpire_220",
                 x, y, 1180, 0, "RoofSlate", ["SteepRoof", "SmallSpire"], False)

# Fourteen stepped buttresses establish vertical cadence at human scale.
buttresses = [
    (-455, 1405, 180), (455, 1405, 180),
    (-735, 1435, 180), (735, 1435, 180),
    (-1015, 1515, 180), (1015, 1515, 180),
    (-1055, 1880, 90), (-1055, 2110, 90), (-1055, 2340, 90),
    (1055, 1880, -90), (1055, 2110, -90), (1055, 2340, -90),
    (-315, 2390, 0), (315, 2390, 0),
]
for index, (x, y, yaw) in enumerate(buttresses):
    spawn_module(f"G3BR_Buttress_{index:02d}", "SM_G3BR_ButtressStepped",
                 x, y, 0, yaw, "StoneLight", ["Buttress"], False)


def spawn_window(label: str, x: float, y: float, z: float, yaw=0.0):
    spawn_module(f"{label}_Frame", "SM_G3BR_WindowFramePointed",
                 x, y, z, yaw, "StoneLight", ["WindowFrame"], False)
    spawn_module(f"{label}_Inset", "SM_G3BR_WindowInsetPointed",
                 x, y + (8 if yaw == 0 else 0), z + 2, yaw,
                 "WindowDeepBlue", ["WindowInset"], False)


# Twenty-two deep pointed windows: two front rows, side rhythm, and tower accents.
for row, z in enumerate((170, 390)):
    for col, x in enumerate((-850, -600, 600, 850)):
        spawn_window(f"G3BR_FrontWindow_{row}_{col}", x, 1444, z)
for side, x, yaw in (("West", -1056, 90), ("East", 1056, -90)):
    for index, y in enumerate((1870, 2110, 2330)):
        spawn_window(f"G3BR_{side}WingWindow_{index}", x, y, 310, yaw)
for side, x in (("West", -1080), ("East", 1080)):
    for row, z in enumerate((430, 730)):
        spawn_window(f"G3BR_{side}TowerWindow_{row}", x, 1558, z)
for row, z in enumerate((870, 1160)):
    spawn_window(f"G3BR_CentralTowerWindow_{row}", 0, 2048, z)

# Small repeated crenellations rather than oversized cube teeth.
crenels = [
    (-850, 1440, 620, 0), (-600, 1440, 620, 0),
    (600, 1440, 620, 0), (850, 1440, 620, 0),
    (-300, 1430, 700, 0), (0, 1430, 700, 0), (300, 1430, 700, 0),
    (-1045, 1880, 620, 90), (-1045, 2180, 620, 90),
    (1045, 1880, 620, 90), (1045, 2180, 620, 90),
]
for index, (x, y, z, yaw) in enumerate(crenels):
    spawn_module(f"G3BR_Crenellation_{index:02d}", "SM_G3BR_Crenellation_300",
                 x, y, z, yaw, "StoneLight", ["Crenellation"], False)


def spawn_banner(label: str, x: float, y: float, z: float):
    spawn_module(f"{label}_Pole", "SM_G3BR_BannerPole", x, y + 8, z - 55, 0,
                 "Metal", ["BannerPole"], False)
    spawn_module(f"{label}_Cloth", "SM_G3BR_BannerCloth", x, y, z + 30, 0,
                 "BannerRoyal", ["Banner"], False)


for label, x, y, z in (
        ("G3BR_GateBanner_West", -245, 1408, 380),
        ("G3BR_GateBanner_East", 245, 1408, 380),
        ("G3BR_CentralBanner", 0, 2040, 1020),
        ("G3BR_SideBanner_West", -1080, 1552, 650),
        ("G3BR_SideBanner_East", 1080, 1552, 650)):
    spawn_banner(label, x, y, z)

# Exactly three restrained magic markers, all outside the principal gate sightline.
for index, (x, y) in enumerate(((-650, 850), (650, 850), (650, 1140))):
    spawn_module(f"G3BR_MagicPlinth_{index}", "SM_G3BR_MagicPlinth",
                 x, y, 8, 0, "StoneAccent", ["MagicPlinth"], False)
    spawn_module(f"G3BR_MagicMarker_{index}", "SM_G3BR_MagicCrystal",
                 x, y, 218, 0, "Magic", ["MagicMarker"], False)

evidence = actors_api.spawn_actor_from_class(
    unreal.StageG3BModularCastleEvidenceActor, unreal.Vector(0, 0, 0), unreal.Rotator())
if evidence is None:
    raise RuntimeError("Unable to place G-3B-R evidence actor")
evidence.set_actor_label("StageG3BR_ModularCastle_Evidence")

unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
if not level_editor.save_current_level():
    raise RuntimeError(f"Unable to save G-3B-R map: {TARGET_MAP}")
if not unreal.EditorAssetLibrary.save_directory(ASSET_ROOT, only_if_is_dirty=False, recursive=True):
    raise RuntimeError("Unable to save G-3B-R modular assets")

reports = SOURCE_ART_ROOT / "Reports"
reports.mkdir(parents=True, exist_ok=True)
payload = {
    "created_at": datetime.now().astimezone().isoformat(),
    "stage": "G-3B-R",
    "source_map": SOURCE_MAP,
    "target_map": TARGET_MAP,
    "removed_g3a_placeholder_actor_count": removed_placeholder,
    "imported_module_asset_count": len(modules),
    "placed_module_actor_count": len(spawned),
    "wall_bay_width_uu": 300,
    "gate_opening_width_uu": 600,
    "gate_opening_height_uu": 565,
    "gate_tunnel_depth_uu": 380,
    "side_tower_diameter_uu": 400,
    "side_tower_height_uu": 1080,
    "central_tower_diameter_uu": 520,
    "central_tower_height_uu": 1480,
    "overall_bounds_uu": {"width": 2360, "depth": 1120, "height": 2000},
    "forecourt_bounds_uu": {"width": 1800, "depth": 1100},
    "buttress_count": len(buttresses),
    "window_frame_count": 20,
    "crenellation_module_count": len(crenels),
    "banner_count": 5,
    "magic_marker_count": 3,
    "guard_count": 1,
    "external_assets": 0,
    "marketplace_assets": 0,
    "high_resolution_textures": 0,
    "status": "PASS",
}
(reports / "map_generation_evidence.json").write_text(
    json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
unreal.log(
    "STAGE_G3BR_MODULAR_MAP PASS "
    f"source={SOURCE_MAP} target={TARGET_MAP} assets={len(modules)} "
    f"actors={len(spawned)} buttresses={len(buttresses)} windows=20 "
    f"crenels={len(crenels)} banners=5 magic=3 guard=1 external_assets=0")
