# LibQuake for Unreal

Unreal Engine plugin that imports Quake 1 `.map`, `.bsp` and `.wad` files. A map becomes static meshes with
collision from its brushes, lights and one actor per entity, dragged into a level like any other asset. It is
built on
[libquake](https://github.com/libquake/libquake), which does the parsing, CSG and uvs.

Edit the map in [TrenchBroom](https://trenchbroom.github.io), save, and Unreal reimports it. Textures come from
the images in TrenchBroom's `textures` folder or from wads.

| | |
|---|---|
| Unreal Engine | 5.8 |
| Platforms | the importer runs in the editor, tested on macOS and Windows. Placed maps are plain Unreal assets and actors |
| Status | experimental |

## Install

Clone the repository into the `Plugins` folder of your project and enable the plugin:

```sh
git clone https://github.com/libquake/unreal Plugins/LibQuake
```

The project needs C++, Unreal compiles the plugin and libquake with it.

## What gets imported

| File | Becomes |
|---|---|
| `textures/*.png`, `.tga`, `.jpg` | a texture each in `/Game/QuakeTextures`, read when a map uses them |
| `.wad` | `Quake Wad` asset and a texture per texture and layer |
| `.map` | `Quake Map` asset, a static mesh per brush entity and a material instance per texture |
| `.bsp` | the same, with the textures from the bsp |

A placed map is a `Quake Map Actor` with the actors of its entities attached, a reimport updates them.

Start with [Getting started](guide/getting-started.md). The settings are in the
[project settings](reference/project-settings.md) and the [map settings](reference/map-settings.md).
