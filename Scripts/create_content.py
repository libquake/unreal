"""Creates the plugin's base materials in Content/Materials and the texture for missing textures in
Content/Textures. Run in a project that has the plugin and Python enabled, from the plugin folder:

    UnrealEditor-Cmd <project>.uproject -run=pythonscript -script="$PWD/Scripts/create_content.py"

Existing materials keep their asset but get rebuilt from scratch, so the script is the source of truth for them
and instances of them stay valid.
"""

import os

import unreal

FOLDER = "/LibQuake/Materials"
lib = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary


def new_material(name, shading_model, blend_mode):
    # loaded directly, the asset registry may not have scanned the plugin yet when this runs as a commandlet
    material = unreal.load_asset(f"{FOLDER}/{name}")
    if material:
        lib.delete_all_material_expressions(material)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())
    material.set_editor_property("shading_model", shading_model)
    material.set_editor_property("blend_mode", blend_mode)
    return material


def texture_parameter(material, name, default, x, y):
    node = lib.create_material_expression(material, unreal.MaterialExpressionTextureSampleParameter2D, x, y)
    node.set_editor_property("parameter_name", name)
    node.set_editor_property("texture", unreal.load_asset(default))
    return node


def constant(material, value, x, y):
    node = lib.create_material_expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def quake_surface(name, masked):
    """Lit surface: Albedo as base color, Emissive (the fullbright pixels) on top, matte like Quake."""
    blend_mode = unreal.BlendMode.BLEND_MASKED if masked else unreal.BlendMode.BLEND_OPAQUE
    material = new_material(name, unreal.MaterialShadingModel.MSM_DEFAULT_LIT, blend_mode)
    albedo = texture_parameter(material, "Albedo", "/Engine/EngineResources/WhiteSquareTexture", -600, -200)
    emissive = texture_parameter(material, "Emissive", "/Engine/EngineResources/Black", -600, 100)

    strength = lib.create_material_expression(material, unreal.MaterialExpressionScalarParameter, -600, 350)
    strength.set_editor_property("parameter_name", "EmissiveStrength")
    strength.set_editor_property("default_value", 1.0)
    glow = lib.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 150)
    lib.connect_material_expressions(emissive, "RGB", glow, "A")
    lib.connect_material_expressions(strength, "", glow, "B")

    lib.connect_material_property(albedo, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    lib.connect_material_property(glow, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.connect_material_property(constant(material, 1.0, -250, 400), "", unreal.MaterialProperty.MP_ROUGHNESS)
    # Quake has no specular at all, 0 also turns off the fresnel sheen at grazing angles
    lib.connect_material_property(constant(material, 0.0, -250, 500), "", unreal.MaterialProperty.MP_SPECULAR)

    if masked:
        lib.connect_material_property(albedo, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    lib.recompile_material(material)
    assets.save_loaded_asset(material)


def quake_sky():
    """Unlit sky: two layers projected along the view direction like Quake's, the back one scrolling at 8 units
    per second and the clouds (SkyFront, alpha 0 where the back shows) at 16."""
    material = new_material("M_QuakeSky", unreal.MaterialShadingModel.MSM_UNLIT, unreal.BlendMode.BLEND_OPAQUE)

    # Quake normalizes the view direction after squashing z, so the direction alone is enough: no world positions
    view = lib.create_material_expression(material, unreal.MaterialExpressionCameraVectorWS, -1100, 0)
    time = lib.create_material_expression(material, unreal.MaterialExpressionTime, -1100, 200)

    layers = []
    for index, speed in enumerate((8.0, 16.0)):
        uv = lib.create_material_expression(material, unreal.MaterialExpressionCustom, -800, index * 300)
        uv.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT2)
        uv.set_editor_property("code", (
            "float3 dir = -ToCamera;\n"
            "dir.z *= 3;\n"
            "dir *= 6 * 63 / length(dir);\n"
            f"return (Time * {speed} + dir.xy) / 128;"))
        inputs = []
        for input_name in ("ToCamera", "Time"):
            custom_input = unreal.CustomInput()
            custom_input.set_editor_property("input_name", input_name)
            inputs.append(custom_input)
        uv.set_editor_property("inputs", inputs)
        lib.connect_material_expressions(view, "", uv, "ToCamera")
        lib.connect_material_expressions(time, "", uv, "Time")
        name = ("SkyBack", "SkyFront")[index]
        default = ("/Engine/EngineResources/WhiteSquareTexture", "/Engine/EngineResources/Black")[index]
        layer = texture_parameter(material, name, default, -500, index * 300)
        lib.connect_material_expressions(uv, "", layer, "UVs")
        layers.append(layer)

    back, front = layers
    blend = lib.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -200, 150)
    lib.connect_material_expressions(back, "RGB", blend, "A")
    lib.connect_material_expressions(front, "RGB", blend, "B")
    lib.connect_material_expressions(front, "A", blend, "Alpha")
    lib.connect_material_property(blend, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    lib.recompile_material(material)
    assets.save_loaded_asset(material)


def missing_texture():
    """The grid for faces whose texture is not in any imported wad, see the MissingTexture project setting."""
    task = unreal.AssetImportTask()
    task.filename = os.path.join(os.path.dirname(os.path.abspath(__file__)), "MissingTexture.jpg")
    task.destination_path = "/LibQuake/Textures"
    task.destination_name = "T_QuakeMissing"
    task.replace_existing = True
    task.automated = True
    task.save = False
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    texture = unreal.load_asset("/LibQuake/Textures/T_QuakeMissing")
    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    assets.save_loaded_asset(texture)


missing_texture()
quake_surface("M_Quake", masked=False)
quake_surface("M_QuakeMasked", masked=True)
quake_sky()
