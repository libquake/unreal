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

A reimport updates the meshes and material instances in place, and every placed map follows: entities keep their
actor and get its place, mesh and keys again, new entities get an actor, the actors of removed ones are deleted.
Properties the plugin does not set stay as they were edited in Unreal. An actor detached from its map actor is
no longer part of the map. **Rebuild** in the details of a map actor does the same without a reimport, for
example after changing the entity classes.

Asset names and the ids that connect entities to their actors (`<classname>_<n>`) depend on the class and the
order of the entities in the map, a reimport keeps them as long as that order stays.
