# Getting started

## Textures

TrenchBroom takes its textures from a folder of images or from wad files, the plugin reads both.

### Image files

Keep the images in a folder `textures` of the game folder, next to `maps`, where TrenchBroom finds them:
`textures/base/wall1.png` is the texture `base/wall1`. Importing a map looks for `textures` in the map's folder and
the folders above it and reads `.png`, `.tga` and `.jpg` files. Nothing has to be imported by hand.

Each image becomes a texture in `/Game/QuakeTextures`, shared by all maps (`TextureFilesFolder` in the
[project settings](../reference/project-settings.md)): `/Game/QuakeTextures/base/wall1`, point filtered and
uncompressed like the wads. Images with transparent pixels get the masked material.

| Next to `wall1.png` | |
|---|---|
| `wall1_luma.png` or `wall1_glow.png` | glowing pixels, as in Quakespasm and ironwail |
| `sky1_front.png` | the cloud layer of the sky `sky1`, without it the sky is one layer |

Each texture remembers its image. A reimport of the map writes the textures whose images have changed and leaves
the others alone, Reimport on a texture reads its image again.

Keep the game folder outside of `Content`, otherwise Auto Reimport may import the images a second time as plain
textures.

### Wads

Drag the wad files the map uses into the Content Browser. Each becomes a `Quake Wad` asset next to a folder
`<wad>_Textures` with the textures: point filtered and uncompressed, so the pixels stay as they are. Textures with
fullbright pixels get a second texture `<name>_emission`, skies their cloud layer as `<name>_front`.

Quake texture names may contain characters Unreal does not allow in asset names, `*water1` becomes `_water1`.
Maps still find them, the wad asset keeps the Quake names.

## Maps

Drag the `.map` into the Content Browser. The import looks up the textures in the image files and the imported
wads, see [Textures and materials](maps.md#textures-and-materials), and writes to `<map>_Generated`:

- `SM_<class>_<n>`: a static mesh per brush entity, `SM_worldspawn_0` is the world. TrenchBroom groups give
  their name instead, `SM_<group>`
- `MI_<texture>`: a material instance per texture

Wads have to be imported before the map. Textures that are missing are reported in Window > Message Log >
LibQuake, their faces get a grey grid, `T_QuakeMissing`. Another texture can be set in Project Settings > Plugins >
LibQuake > Missing Texture, without one the faces are plain white.

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

Point TrenchBroom's game path at a folder inside the project, outside of `Content`, and keep maps, textures and
wads there. With Editor Preferences > Loading & Saving > Auto Reimport watching that folder, saving in TrenchBroom
reimports the map. A changed image alone does not, it needs a Reimport of the map or of its texture.

## Scale and axes

Quake and Unreal are both z up, Quake is right handed: `(x, y, z)` becomes `(x, -y, z)`. A Quake unit is
3.125 cm by default, which makes 32 units a meter and the player 1.75 m tall. The scale is a setting of the map,
see the [map settings](../reference/map-settings.md).
