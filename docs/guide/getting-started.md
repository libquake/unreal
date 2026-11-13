# Getting started

## Textures

Drag the wad files the map uses into the Content Browser. Each becomes a `Quake Wad` asset next to a folder
`<wad>_Textures` with the textures: point filtered and uncompressed, so the pixels stay as they are. Textures with
fullbright pixels get a second texture `<name>_emission`, skies their cloud layer as `<name>_front`.

Quake texture names may contain characters Unreal does not allow in asset names, `*water1` becomes `_water1`.
Maps still find them, the wad asset keeps the Quake names.

## Maps

Drag the `.map` into the Content Browser. The import looks up the textures in the imported wads, the ones named in
the map's `wad` key first, and writes to `<map>_Generated`:

- `SM_<class>_<n>`: a static mesh per brush entity, `SM_worldspawn_0` is the world
- `MI_<texture>`: a material instance per texture

Import the wads first. Textures that are missing are reported in Window > Message Log > LibQuake, their faces are
plain white.

## Placing

Right click the map > **Place in Level**. The first time, the meshes, lights and entities are spawned and handed to
Unreal's Create Level Instance: Unreal asks where to save the new level (`<map>_Generated/L_<map>` is suggested)
and puts a Level Instance of it into the open level. After that the map can also be dragged into the viewport like
any other asset, every placement is another instance of the same level.

## TrenchBroom

Point TrenchBroom's game path at a folder inside the project and keep maps and wads there. With Editor
Preferences > Loading & Saving > Auto Reimport watching that folder, saving in TrenchBroom reimports the map.

## Scale and axes

Quake and Unreal are both z up, Quake is right handed: `(x, y, z)` becomes `(x, -y, z)`. A Quake unit is
3.125 cm by default, which makes 32 units a meter and the player 1.75 m tall. The scale is a setting of the map,
see the [map settings](../reference/map-settings.md).
