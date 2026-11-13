# LibQuake for Unreal

![LGBTQ+ Friendly][badge-lgbtq]
![Trans Rights][badge-trans]
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

Unreal Engine plugin that imports Quake 1 `.map` and `.wad` files. A map becomes static meshes with collision from
its brushes, lights and entities, placed in levels as a Level Instance. Built on
[libquake](https://github.com/libquake/libquake), which does the parsing, CSG and uvs.

Experimental. Unreal Engine 5.8, tested on macOS.

## Install

Clone the repository into the `Plugins` folder of your project and enable the plugin:

```sh
git clone https://github.com/libquake/unreal Plugins/LibQuake
```

The project needs C++ (Unreal compiles the plugin, libquake included, when the project opens).

## Use

1. Drag a `.wad` into the Content Browser.
2. Drag a `.map` into the Content Browser.
3. Right click the map > Place in Level.

The documentation is in [docs/](docs/index.md).

## libquake

`Source/ThirdParty/libquake` is a copy of the libquake sources, `VERSION` names the commit.
`Scripts/sync-libquake.sh` updates it from a checkout next to this repository or from a release tag.

[badge-trans]: https://pride-badges.pony.workers.dev/static/v1?label=trans%20rights&stripeWidth=6&stripeColors=5BCEFA,F5A9B8,FFFFFF,F5A9B8,5BCEFA
[badge-lgbtq]: https://pride-badges.pony.workers.dev/static/v1?label=lgbtq%2B%20friendly&stripeWidth=6&stripeColors=E40303,FF8C00,FFED00,008026,24408E,732982
