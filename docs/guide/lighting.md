# Lighting

Maps are lit by Unreal. The lights of the map become static Unreal lights, they show after a lighting build
(Build > Build Lighting Only). For Lumen without a build, set them to Movable.

| Map | Unreal |
|---|---|
| `light` with `light`, `_color` | point light, attenuation radius = `light` in Quake units, falloff exponent 2 |
| `_sunlight`, `_sun_mangle`, `_sunlight_color` in the worldspawn | directional light, pointing where `_sun_mangle` says, straight down without it |

## Brightness

Quake lights fade linearly from their value at the center to 0 at the radius, and a surface at light level 128
shows its texture as it is. Unreal fades by (1 - d²/r²)^exponent, an exponent of 2 is close to linear. The
intensity is `light / 128 * π * 1.2`, so a surface at level 128 shows its texture as it is in Unreal too, under the
exposure the map brings along: the level gets an unbound post process volume `exposure` with manual exposure and
a bias of 0. Without it Unreal's auto exposure brightens dark rooms and darkens lit ones, which flattens the
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
