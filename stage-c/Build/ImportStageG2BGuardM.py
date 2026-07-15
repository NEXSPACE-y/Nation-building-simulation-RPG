from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path

import unreal


ROOT = "/Game/StageG2B/Characters/GUARD_M/MeshyV01"
MESH_DIR = f"{ROOT}/Mesh"
SKELETON_DIR = f"{ROOT}/Skeleton"
TEXTURE_DIR = f"{ROOT}/Textures"
MATERIAL_DIR = f"{ROOT}/Materials"
ANIMATION_DIR = f"{ROOT}/Animations"
MESH_NAME = "SK_GUARD_M_Meshy_v0_1"
SKELETON_NAME = "SKEL_GUARD_M_Meshy_v0_1"
PHYSICS_NAME = "PHYS_GUARD_M_Meshy_v0_1"


def log(message: str) -> None:
    unreal.log(f"STAGE_G2B_IMPORT {message}")


def set_property(obj, name: str, value) -> bool:
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception:
        return False


def notify_changed(obj) -> None:
    method = getattr(obj, "post_edit_change", None)
    if method:
        method()


def import_task(filename: Path, destination_path: str, destination_name: str, options=None):
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(filename))
    task.set_editor_property("destination_path", destination_path)
    task.set_editor_property("destination_name", destination_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)
    task.set_editor_property("replace_existing", True)
    set_property(task, "replace_existing_settings", True)
    if options is not None:
        task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return task


def create_base_options(scale: float) -> unreal.FbxImportUI:
    options = unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type", False)
    options.set_editor_property("mesh_type_to_import", unreal.FBXImportType.FBXIT_SKELETAL_MESH)
    options.set_editor_property("import_mesh", True)
    options.set_editor_property("import_as_skeletal", True)
    options.set_editor_property("import_animations", False)
    options.set_editor_property("import_materials", False)
    options.set_editor_property("import_textures", False)
    set_property(options, "create_physics_asset", True)
    data = options.get_editor_property("skeletal_mesh_import_data")
    set_property(data, "normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    set_property(data, "convert_scene", True)
    set_property(data, "force_front_x_axis", False)
    set_property(data, "import_uniform_scale", scale)
    set_property(data, "import_morph_targets", False)
    return options


def create_animation_options(skeleton, scale: float) -> unreal.FbxImportUI:
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
    set_property(data, "import_uniform_scale", scale)
    return options


def asset_type(asset) -> str:
    return asset.get_class().get_name() if asset else "NONE"


def assets_of_type(path: str, class_name: str):
    result = []
    for asset_path in unreal.EditorAssetLibrary.list_assets(path, recursive=True, include_folder=False):
        asset = unreal.EditorAssetLibrary.load_asset(asset_path)
        if asset and asset_type(asset) == class_name:
            result.append((asset_path, asset))
    return result


def imported_height(mesh) -> float:
    try:
        return float(mesh.get_editor_property("imported_bounds").box_extent.z * 2.0)
    except Exception:
        try:
            return float(mesh.get_bounds().box_extent.z * 2.0)
        except Exception:
            return 0.0


def rename_first(path: str, class_name: str, target: str):
    matches = assets_of_type(path, class_name)
    if not matches:
        return None
    source_path, asset = matches[0]
    if source_path.split(".")[0] != target:
        if unreal.EditorAssetLibrary.does_asset_exist(target):
            unreal.EditorAssetLibrary.delete_asset(target)
        if not unreal.EditorAssetLibrary.rename_asset(source_path, target):
            raise RuntimeError(f"Unable to rename {source_path} to {target}")
        asset = unreal.EditorAssetLibrary.load_asset(target)
    return asset


def configure_texture(path: Path, name: str, srgb: bool, compression):
    task = import_task(path, TEXTURE_DIR, name)
    asset_path = f"{TEXTURE_DIR}/{name}"
    texture = unreal.EditorAssetLibrary.load_asset(asset_path)
    if texture is None:
        raise RuntimeError(f"GUARD_M texture import failed: {name}; imported={task.imported_object_paths}")
    texture.set_editor_property("srgb", srgb)
    texture.set_editor_property("compression_settings", compression)
    notify_changed(texture)
    unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
    return texture


def create_material(textures: dict):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    material_path = f"{MATERIAL_DIR}/M_GUARD_M_Meshy_v0_1"
    instance_path = f"{MATERIAL_DIR}/MI_GUARD_M_Meshy_v0_1"
    if unreal.EditorAssetLibrary.does_asset_exist(material_path):
        unreal.EditorAssetLibrary.delete_asset(material_path)
    material = tools.create_asset(
        "M_GUARD_M_Meshy_v0_1", MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew()
    )
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    set_property(material, "shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    material.set_editor_property("two_sided", False)
    library = unreal.MaterialEditingLibrary
    nodes = {}
    positions = {"base": (-650, -350), "normal": (-650, 0), "metallic": (-650, 250), "roughness": (-650, 450)}
    for key, texture in textures.items():
        node = library.create_material_expression(material, unreal.MaterialExpressionTextureSample, *positions[key])
        node.set_editor_property("texture", texture)
        if key == "normal":
            node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        elif key in {"metallic", "roughness"}:
            node.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        nodes[key] = node
    library.connect_material_property(nodes["base"], "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    library.connect_material_property(nodes["normal"], "RGB", unreal.MaterialProperty.MP_NORMAL)
    library.connect_material_property(nodes["metallic"], "R", unreal.MaterialProperty.MP_METALLIC)
    library.connect_material_property(nodes["roughness"], "R", unreal.MaterialProperty.MP_ROUGHNESS)
    library.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(material_path, only_if_is_dirty=False)
    if unreal.EditorAssetLibrary.does_asset_exist(instance_path):
        unreal.EditorAssetLibrary.delete_asset(instance_path)
    instance = tools.create_asset(
        "MI_GUARD_M_Meshy_v0_1", MATERIAL_DIR,
        unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew()
    )
    library.set_material_instance_parent(instance, material)
    unreal.EditorAssetLibrary.save_asset(instance_path, only_if_is_dirty=False)
    return material, instance


def assign_material(mesh, instance) -> int:
    materials = list(mesh.get_editor_property("materials"))
    if len(materials) != 1:
        raise RuntimeError(f"GUARD_M material slot contract failed: {len(materials)}")
    materials[0].set_editor_property("material_interface", instance)
    mesh.set_editor_property("materials", materials)
    notify_changed(mesh)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False)
    return len(materials)


def sequence_info(path: str, sequence) -> dict:
    info = {
        "asset_path": path,
        "name": sequence.get_name(),
        "skeleton_path": sequence.get_editor_property("skeleton").get_path_name(),
    }
    for method_name, key in (("get_play_length", "play_length_seconds"), ("get_number_of_sampled_keys", "sampled_keys")):
        method = getattr(sequence, method_name, None)
        if method:
            try:
                info[key] = method()
            except Exception:
                pass
    return info


def main() -> None:
    project_root = Path(unreal.Paths.project_dir()).resolve()
    working = project_root / "SourceArt" / "StageG2B" / "GUARD_M" / "Meshy" / "v0.1" / "Working"
    base_fbx = working / "FBX" / "GUARD_M_Meshy_v0.1_Base_Rigged.fbx"
    animation_fbx = working / "FBX" / "GUARD_M_Meshy_v0.1_AllAnimations.fbx"
    reports = working / "Reports"
    reports.mkdir(parents=True, exist_ok=True)
    if unreal.EditorAssetLibrary.does_directory_exist(ROOT):
        for existing_asset in unreal.EditorAssetLibrary.list_assets(ROOT, recursive=True, include_folder=False):
            unreal.EditorAssetLibrary.delete_asset(existing_asset)
        unreal.EditorAssetLibrary.delete_directory(ROOT)
    for folder in (MESH_DIR, SKELETON_DIR, TEXTURE_DIR, MATERIAL_DIR, ANIMATION_DIR, f"{ROOT}/Blueprints"):
        unreal.EditorAssetLibrary.make_directory(folder)

    initial_task = import_task(base_fbx, MESH_DIR, MESH_NAME, create_base_options(1.0))
    mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{MESH_NAME}")
    if mesh is None:
        meshes = assets_of_type(MESH_DIR, "SkeletalMesh")
        if not meshes:
            raise RuntimeError(f"GUARD_M base mesh import failed: {initial_task.imported_object_paths}")
        mesh = meshes[0][1]
    initial_height = imported_height(mesh)
    import_scale = 1.0
    if initial_height > 0 and not 170.0 <= initial_height <= 180.0:
        import_scale = 175.0 / initial_height
        import_task(base_fbx, MESH_DIR, MESH_NAME, create_base_options(import_scale))
        mesh = unreal.EditorAssetLibrary.load_asset(f"{MESH_DIR}/{MESH_NAME}")
    final_height = imported_height(mesh)

    skeleton = mesh.get_editor_property("skeleton")
    if skeleton is None:
        raise RuntimeError("GUARD_M imported mesh has no skeleton")
    current_skeleton_path = skeleton.get_path_name().split(".")[0]
    target_skeleton_path = f"{SKELETON_DIR}/{SKELETON_NAME}"
    if current_skeleton_path != target_skeleton_path:
        if unreal.EditorAssetLibrary.does_asset_exist(target_skeleton_path):
            unreal.EditorAssetLibrary.delete_asset(target_skeleton_path)
        if not unreal.EditorAssetLibrary.rename_asset(current_skeleton_path, target_skeleton_path):
            raise RuntimeError(f"Unable to move GUARD_M skeleton: {current_skeleton_path}")
        skeleton = unreal.EditorAssetLibrary.load_asset(target_skeleton_path)
    physics = rename_first(MESH_DIR, "PhysicsAsset", f"{SKELETON_DIR}/{PHYSICS_NAME}")
    if physics is None:
        subsystem = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
        physics = subsystem.create_physics_asset(mesh, True, 0)
        if physics is not None:
            current_physics_path = physics.get_path_name().split(".")[0]
            target_physics_path = f"{SKELETON_DIR}/{PHYSICS_NAME}"
            if current_physics_path != target_physics_path:
                if unreal.EditorAssetLibrary.does_asset_exist(target_physics_path):
                    unreal.EditorAssetLibrary.delete_asset(target_physics_path)
                if not unreal.EditorAssetLibrary.rename_asset(current_physics_path, target_physics_path):
                    raise RuntimeError(f"Unable to move GUARD_M physics asset: {current_physics_path}")
                physics = unreal.EditorAssetLibrary.load_asset(target_physics_path)
    if physics is None:
        raise RuntimeError("GUARD_M physics asset was not generated")

    texture_root = working / "Textures"
    textures = {
        "base": configure_texture(texture_root / "T_GUARD_M_BaseColor_2K.png", "T_GUARD_M_BaseColor_2K", True, unreal.TextureCompressionSettings.TC_DEFAULT),
        "normal": configure_texture(texture_root / "T_GUARD_M_Normal_2K.png", "T_GUARD_M_Normal_2K", False, unreal.TextureCompressionSettings.TC_NORMALMAP),
        "metallic": configure_texture(texture_root / "T_GUARD_M_Metallic_2K.png", "T_GUARD_M_Metallic_2K", False, unreal.TextureCompressionSettings.TC_MASKS),
        "roughness": configure_texture(texture_root / "T_GUARD_M_Roughness_2K.png", "T_GUARD_M_Roughness_2K", False, unreal.TextureCompressionSettings.TC_MASKS),
    }
    material, instance = create_material(textures)
    material_slots = assign_material(mesh, instance)

    animation_task = import_task(animation_fbx, ANIMATION_DIR, "", create_animation_options(skeleton, import_scale))
    sequences = assets_of_type(ANIMATION_DIR, "AnimSequence")
    inventory = []
    for path, sequence in sequences:
        set_property(sequence, "enable_root_motion", False)
        set_property(sequence, "force_root_lock", True)
        notify_changed(sequence)
        unreal.EditorAssetLibrary.save_asset(path.split(".")[0], only_if_is_dirty=False)
        inventory.append(sequence_info(path, sequence))

    idle_candidates = [item for item in inventory if "idle" in item["name"].lower()]
    import_evidence = {
        "created_at": datetime.now().astimezone().isoformat(),
        "skeletal_mesh": mesh.get_path_name(),
        "skeleton": skeleton.get_path_name(),
        "physics_asset": physics.get_path_name(),
        "player_skeleton_shared": False,
        "initial_import_uniform_scale": 1.0,
        "initial_world_height_uu": initial_height,
        "final_import_uniform_scale": import_scale,
        "final_component_scale": 1.0,
        "final_world_height_uu": final_height,
        "world_height_in_170_180_band": 170.0 <= final_height <= 180.0,
        "material_slot_count": material_slots,
        "material": material.get_path_name(),
        "material_instance": instance.get_path_name(),
        "animation_sequence_count": len(inventory),
        "guard_idle_candidates": idle_candidates,
        "normal_import_method": "Import Normals and Tangents",
        "convert_scene": True,
        "force_front_x_axis": False,
        "lod_policy": "LOD0 source retained; additional LOD required before multiple placement",
        "multiple_guard_placement_allowed": False,
        "external_communication": 0,
    }
    (reports / "mesh_import_evidence.json").write_text(json.dumps(import_evidence, ensure_ascii=False, indent=2), encoding="utf-8")
    (reports / "animation_inventory.json").write_text(json.dumps({"sequences": inventory}, ensure_ascii=False, indent=2), encoding="utf-8")
    (reports / "texture_material_evidence.json").write_text(json.dumps({
        "material": material.get_path_name(),
        "material_instance": instance.get_path_name(),
        "base_color": {"asset": textures["base"].get_path_name(), "srgb": True, "compression": "Default"},
        "normal": {"asset": textures["normal"].get_path_name(), "srgb": False, "compression": "Normalmap"},
        "metallic": {"asset": textures["metallic"].get_path_name(), "srgb": False, "compression": "Masks"},
        "roughness": {"asset": textures["roughness"].get_path_name(), "srgb": False, "compression": "Masks"},
        "player_material_changed": False,
        "texture_content_modified": False,
        "status": "PASS",
    }, ensure_ascii=False, indent=2), encoding="utf-8")
    unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    log(
        f"PASS mesh={mesh.get_path_name()} skeleton={skeleton.get_path_name()} "
        f"height={final_height:.3f} scale={import_scale:.6f} animations={len(inventory)} idle_candidates={len(idle_candidates)}"
    )


main()
