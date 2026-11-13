"""Creates the plugin's base materials in Content/Materials. Run in a project that has the plugin and Python
enabled, from the plugin folder:

    UnrealEditor-Cmd <project>.uproject -run=pythonscript -script="$PWD/Scripts/create_content.py"

Existing materials are replaced, so the script is the source of truth for them.
"""

import unreal

FOLDER = "/LibQuake/Materials"
lib = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary


def new_material(name):
    path = f"{FOLDER}/{name}"
    if assets.does_asset_exist(path):
        assets.delete_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    return tools.create_asset(name, FOLDER, unreal.Material, unreal.MaterialFactoryNew())


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
    material = new_material(name)
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
    lib.connect_material_property(constant(material, 0.2, -250, 500), "", unreal.MaterialProperty.MP_SPECULAR)

    if masked:
        material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED)
        lib.connect_material_property(albedo, "A", unreal.MaterialProperty.MP_OPACITY_MASK)

    lib.recompile_material(material)
    assets.save_loaded_asset(material)


quake_surface("M_Quake", masked=False)
quake_surface("M_QuakeMasked", masked=True)
