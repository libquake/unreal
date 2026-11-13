# Lighting

Maps are lit by Unreal. The lights of the map become static Unreal lights, they show after a lighting build
(Build > Build Lighting Only). For Lumen without a build, set them to Movable.

| Map | Unreal |
|---|---|
| `light` with `light`, `_color` | point light, linear falloff, attenuation radius = `light` in Quake units |
| `_sunlight`, `_sun_mangle`, `_sunlight_color` in the worldspawn | directional light, pointing where `_sun_mangle` says, straight down without it |

The intensities are a first guess and may need adjusting in the scene.

## Lightmap uvs

Unreal lays out the lightmap uvs (channel 1) of every mesh itself. Their resolution follows the size of the mesh,
about one texel per `LuxelSize` Quake units, 16 by default like Quake.

## Sky

Sky textures get `M_QuakeSky`: unlit, both layers projected along the view direction and scrolling like in Quake.
Sky faces cast no shadows, in Quake they are openings and the sun comes through them.

## Level Instances

Baking static light into Level Instances is experimental in Unreal. If a build leaves placed maps dark, break the
Level Instance into actors (right click > Level > Break) before baking.
