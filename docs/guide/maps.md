# Maps

## Geometry

libquake builds the meshes, the plugin only converts them. With CSG on, faces hidden inside other brushes of the
same entity are removed. `func_group` and `func_detail` are separate entities and get their own meshes.

| Texture | Surface |
|---|---|
| `clip`, `trigger` | collision only |
| `skip`, `hint`, `hintskip`, `nodraw`, `caulk`, `origin` | neither drawn nor solid |
| `sky...` | `M_QuakeSky`, see [Lighting](lighting.md) |
| `{...` | `M_QuakeMasked`, transparent where the palette index is 255 |
| everything else | `M_Quake`, fullbright pixels as emissive |

Brush entities without visible faces, like triggers, get a mesh made of their brush hulls. It is never drawn, it
is there for collision and overlaps.

## Collision

Every brush is convex, the plugin uses the brush hulls as the simple collision of the static mesh, one convex
element per brush, and traces against them as well (Use Simple Collision As Complex). Clip brushes block like
walls, liquids are left out of the collision.

A bsp has no brush hulls. Its meshes collide with their own triangles (Use Complex Collision As Simple), triggers
with their clip faces.

## Reimport

A reimport updates the meshes and material instances in place, every placed instance shows the new geometry.
The level made by the first placement is not rebuilt yet: lights and entities that were added or moved in the map
show up after deleting the level and placing the map again.

Asset names depend on the class and the order of the entities in the map, a reimport keeps them as long as that
order stays.
