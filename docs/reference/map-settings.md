# Map settings

`Settings` of a `Quake Map` asset (`UQuakeMap`). A reimport applies Scale, CSG and LuxelSize, EntityClasses is used
when the map is placed.

| Setting | Default | |
|---|---|---|
| `float Scale` | 3.125 | Unreal units (cm) per Quake unit |
| `bool bCSG` | on | remove faces hidden inside other brushes of the same entity |
| `float LuxelSize` | 16 | Quake units per lightmap texel, sets the lightmap resolution of the meshes |
| `UQuakeEntityClasses* EntityClasses` | none | actor classes for entity classes, see [Entities](../guide/entities.md) |

## Generated

| Property | |
|---|---|
| `TArray<UStaticMesh*> Meshes` | a mesh per brush entity |
| `TArray<FQuakeMapEntity> Entities` | every entity: class name, transform, keys, index of its mesh |
| `TSoftObjectPtr<UWorld> Level` | the level made by the first placement |
