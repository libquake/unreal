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

## Bsp files

A compiled `.bsp` is imported the same way. It brings its textures, they are written as `T_<texture>` next to the
meshes, no wad needed. A bsp has no brush hulls, its meshes are their own collision.

## Placing

Drag the map into the viewport, or right click it > **Place in Level** to put it at the origin. The level gets a
`Quake Map Actor` with an actor per entity attached to it: static mesh actors for the brushes, lights, a player
start and so on, see [Entities](entities.md). Moving the map actor moves the whole map, the entity actors can be
selected and edited one by one. Deleting the map actor deletes them with it.

The same map can be placed any number of times, each placement has its own actors.

## TrenchBroom

Point TrenchBroom's game path at a folder inside the project and keep maps and wads there. With Editor
Preferences > Loading & Saving > Auto Reimport watching that folder, saving in TrenchBroom reimports the map.

## Scale and axes

Quake and Unreal are both z up, Quake is right handed: `(x, y, z)` becomes `(x, -y, z)`. A Quake unit is
3.125 cm by default, which makes 32 units a meter and the player 1.75 m tall. The scale is a setting of the map,
see the [map settings](../reference/map-settings.md).
