# Lighting

Maps are lit by Unreal. The lights of the map become static Unreal lights, they show after a lighting build
(Build > Build Lighting Only). For Lumen without a build, set them to Movable.

| Map | Unreal |
|---|---|
| `light` with `light`, `_color` | point light, attenuation radius = `light` in Quake units, falloff exponent 2 |
| `_sunlight`, `_sun_mangle`, `_sunlight_color` in the worldspawn | directional light, pointing where `_sun_mangle` says, straight down without it |

## Brightness

In Quake a surface at light level 128 shows its texture as it is. The light tools halve every light (`_range`
0.5), and a light fades linearly from its value at the center to 0 at the radius, which is the value in Quake
units. Unreal fades by (1 - d²/r²)^exponent without inverse square falloff, an exponent of 2 is close to linear.
The intensity is `light · 0.5 / 128 · π · 1.2` (unitless), the sun gets the same in lux from `_sunlight`. A lit
surface shows albedo / π times the light in Unreal, and the factor 1.2 matches the exposure the map brings along:
the map gets an unbound post process volume `exposure` with manual exposure and a bias of 0. Without it
Unreal's auto exposure brightens dark rooms and darkens lit ones, which flattens the light of a Quake map. To use
the exposure of your own level, turn off `bManualExposure` in the [map settings](../reference/map-settings.md),
placed maps lose the volume with the next reimport or Rebuild.

Before a lighting build the editor shows static lights as a preview without shadows.

## Lightmap uvs

Unreal lays out the lightmap uvs (channel 1) of every mesh itself. Their resolution follows the size of the mesh,
about one texel per `LuxelSize` Quake units, 16 by default like Quake.

## Sky

Sky textures get `M_QuakeSky`: unlit, both layers projected along the view direction and scrolling like in Quake.
Sky faces cast no shadows, in Quake they are openings and the sun comes through them.
