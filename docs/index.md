# LibQuake for Unreal

Unreal Engine plugin that imports Quake 1 `.map` and `.wad` files. A map becomes static meshes with collision from
its brushes, lights and one actor per entity, placed in levels as a Level Instance. It is built on
[libquake](https://github.com/libquake/libquake), which does the parsing, CSG and uvs.

Edit the map in [TrenchBroom](https://trenchbroom.github.io), save, and Unreal reimports it.

| | |
|---|---|
| Unreal Engine | 5.8 |
| Platforms | the importer runs in the editor, tested on macOS. Placed maps are plain Unreal assets and actors |
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
| `.wad` | `Quake Wad` asset and a texture per texture and layer |
| `.map` | `Quake Map` asset, a static mesh per brush entity and a material instance per texture |

Placing a map the first time makes its level, every placement is a Level Instance of it.

Start with [Getting started](guide/getting-started.md).
