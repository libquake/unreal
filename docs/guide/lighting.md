# Lighting

Maps are lit by Unreal. The lights of the map become static Unreal lights, they show after a lighting build
(Build > Build Lighting Only). For Lumen without a build, set them to Movable.

| Map | Unreal |
|---|---|
| `light` with `light`, `_color` | point light with inverse square falloff |
| `_sunlight`, `_sun_mangle`, `_sunlight_color` in the worldspawn | directional light, pointing where `_sun_mangle` says, straight down without it |

## Brightness

In Quake a surface at light level 128 shows its texture as it is. The light tools halve every light (`_range`
0.5), and a light fades linearly from its value to 0: `light 300` lights a surface to level 128 about 44 units
away. The point lights fade by the inverse square like any Unreal light and reach level 128 at about the same
distance, `sqrt(2π · light)` units:

| | |
|---|---|
| intensity | `light · 2π² · 1.2 / m²` candela, `m` = Quake units per meter (32 at the default scale) |
| attenuation radius | `(sqrt(1 + 1024 · light) - 1) / 2` Quake units, where the light adds less than 1/40 of the texture brightness |
| sun | `_sunlight · 0.5 / 128 · π · 1.2` lux |

Close to a light the surfaces get brighter than in Quake, further away darker. The factor 1.2 matches the exposure
the map brings along: the level gets an unbound post process volume `exposure` with manual exposure and a bias of
0. Without it Unreal's auto exposure brightens dark rooms and darkens lit ones, which flattens the
light of a Quake map. To use the exposure of your own level, delete the volume or turn off `bManualExposure` in the
[map settings](../reference/map-settings.md) before placing.

Before a lighting build the editor shows static lights as a preview, brighter and without shadows.

## Lightmap uvs

Unreal lays out the lightmap uvs (channel 1) of every mesh itself. Their resolution follows the size of the mesh,
about one texel per `LuxelSize` Quake units, 16 by default like Quake.

## Sky

Sky textures get `M_QuakeSky`: unlit, both layers projected along the view direction and scrolling like in Quake.
Sky faces cast no shadows, in Quake they are openings and the sun comes through them.

## Level Instances

Baking static light into Level Instances is experimental in Unreal. If a build leaves placed maps dark, break the
Level Instance into actors (right click > Level > Break) before baking.
