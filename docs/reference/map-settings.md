# Map settings

`Settings` of a `Quake Map` asset (`UQuakeMap`). A reimport applies all of them, to the assets and to the placed
maps.

| Setting | Default | |
|---|---|---|
| `float Scale` | 3.125 | Unreal units (cm) per Quake unit |
| `bool bCSG` | on | remove faces hidden inside other brushes of the same entity |
| `bool bMeshLibrary` | off | the map is a collection of meshes and is not placed, see [Maps](../guide/maps.md#mesh-libraries) |
| `EQuakePivot Pivot` | BottomCenter | origin of the brush entities' meshes: `MapOrigin`, `Center` or `BottomCenter`, see [Maps](../guide/maps.md#pivots) |
| `float LuxelSize` | 16 | Quake units per lightmap texel, sets the lightmap resolution of the meshes |
| `UQuakeEntityClasses* EntityClasses` | none | actor classes for entity classes of this map, before the project's, see [Entities](../guide/entities.md) |

## Generated

| Property | |
|---|---|
| `TArray<UStaticMesh*> Meshes` | a mesh per brush entity |
| `TArray<FQuakeMapEntity> Entities` | every entity: class name, transform, keys, index of its mesh |
