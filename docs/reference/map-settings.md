# Map settings

`Settings` of a `Quake Map` asset (`UQuakeMap`). A reimport applies all of them, to the assets and to the placed
maps.

| Setting | Default | |
|---|---|---|
| `float Scale` | 3.125 | Unreal units (cm) per Quake unit |
| `bool bCSG` | on | remove faces hidden inside other brushes of the same entity |
| `float LuxelSize` | 16 | Quake units per lightmap texel, sets the lightmap resolution of the meshes |
| `bool bManualExposure` | on | put a post process volume with a fixed exposure into the map, see [Lighting](../guide/lighting.md) |
| `UQuakeEntityClasses* EntityClasses` | none | actor classes for entity classes of this map, before the project's, see [Entities](../guide/entities.md) |

## Generated

| Property | |
|---|---|
| `TArray<UStaticMesh*> Meshes` | a mesh per brush entity |
| `TArray<FQuakeMapEntity> Entities` | every entity: class name, transform, keys, index of its mesh |
