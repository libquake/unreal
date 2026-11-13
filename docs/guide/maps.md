# Maps

## Geometry

libquake builds the meshes, the plugin only converts them. With CSG on, faces hidden inside other brushes of the
same entity are removed. `func_group` and `func_detail` are separate entities and get their own meshes.

| Texture | Surface |
|---|---|
| `clip`, `trigger` | collision only |
| `skip`, `hint`, `hintskip`, `nodraw`, `caulk`, `origin` | neither drawn nor solid |
| `sky...` | `M_QuakeSky`, see [Lighting](lighting.md) |
| `*...` | `M_QuakeLiquid`, see [Liquids](#liquids) |
| `{...` | `M_QuakeMasked`, transparent where the palette index is 255 |
| everything else | `M_Quake`, fullbright pixels as emissive, matte (roughness 1, specular 0) like in Quake |

Brush entities without visible faces, like triggers, get a mesh made of their brush hulls. It is never drawn, it
is there for collision and overlaps.

## Materials

Each texture gets a material instance `MI_<texture>` of the project's surface, masked or sky material, see the
[project settings](../reference/project-settings.md). Artists who want their own look set their own parent
materials there, with the same parameters.

A material table replaces the instance for some textures, e.g. `*water*` with a water material or `sky*` with a
sky of the game. Names ending in `*` are prefixes as for the entity classes. The `Materials` of the
[map settings](../reference/map-settings.md) come first, then the ones of the project settings. A reimport
applies changes to the tables.

## Liquids

A brush with a texture starting with `*` is a liquid, like in Quake: `*lava...` is lava, `*slime...` slime, every
other one water, `*teleport` included. It is no wall, the collision leaves it out.

Its faces get `M_QuakeLiquid`: unlit like in Quake, where liquids have no lightmap, two sided so they show from
below, and warped by Quake's turbulence. Its parameters are `Brightness`, `WarpAmount` (in texels, 8 like Quake) and
`WarpSpeed`. `M_QuakeLiquidTranslucent` is the same with `Opacity`, set it as `LiquidMaterial` in the
[project settings](../reference/project-settings.md) to see the floor through the water.

Each liquid brush of a placed map also gets a volume of its shape, an `AQuakeLiquidVolume`: a water volume,
characters swim in it. Its `QuakeEntity` component has the keys `texture` and `liquid` (`water`, `slime` or
`lava`), a Blueprint based on it can hurt in slime and lava. `LiquidClasses` in the project settings choose the
volume class by texture name, `*lava*` to a lava volume for example. An entry without a class leaves those liquids
without a volume.

## Mesh libraries

A map can be a collection of meshes instead of a level, e.g. a map full of props. Set the worldspawn key
`_meshlib` to `1` in TrenchBroom (`qu_meshlib` from older maps works too) or turn on `bMeshLibrary` in the
[map settings](../reference/map-settings.md). Each TrenchBroom group becomes one mesh, brush entities inside it
and nested groups included: it is named after the group, `SM_crate` for a group "crate", with its pivot at the
bottom center of the whole group (see below). Brush entities outside of groups get their own mesh, loose brushes
of the worldspawn end up in `SM_worldspawn_0`.

A mesh library is not placed: dragging it into a level or Place in Level refuse it, its meshes are dragged into
levels like any other static mesh. A reimport updates them in place, as long as the group names stay.

## Pivots

Every brush entity except the worldspawn has its mesh and its actor at its own pivot, by default the center of
the bottom of its bounds (`Pivot` in the [map settings](../reference/map-settings.md)). An `origin` brush in the
entity wins, as in the Quake tools: its center is the pivot, the brush itself is neither drawn nor solid. The key
`_pivot` on an entity (`origin`, `center`, `bottom` or `corner`) overrides the map's setting for that entity.

`BottomCorner` (`corner`) puts the pivot at the lowest corner of the bounds. Modular pieces like wall segments
then snap to the grid in Unreal whatever their size, with the bottom center that only works when half of their
size is on the grid.

## Collision

Every brush is convex, the plugin uses the brush hulls as the simple collision of the static mesh, one convex
element per brush, and traces against them as well (Use Simple Collision As Complex). Clip brushes block like
walls, liquids are left out of the collision.

A bsp has no brush hulls. Its meshes collide with their own triangles (Use Complex Collision As Simple), triggers
with their clip faces.

## Reimport

A reimport updates the meshes and material instances in place, and every placed map follows: entities keep their
actor and get its place, mesh and keys again, new entities get an actor, the actors of removed ones are deleted.
Properties the plugin does not set stay as they were edited in Unreal. Meshes, material instances and bsp
textures the map no longer uses, e.g. of a renamed group or a texture that now has a material from the material
table, are deleted from `<map>_Generated` afterwards, unless something else still refers to them, like a saved
level that is not open. Other assets in that folder are not touched. An actor detached from its map actor is
no longer part of the map. **Rebuild** in the details of a map actor does the same without a reimport, for
example after changing the entity classes.

Asset names and the ids that connect entities to their actors (`<classname>_<n>`) depend on the class and the
order of the entities in the map, a reimport keeps them as long as that order stays.
