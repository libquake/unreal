# Map settings

`Settings` of a `Quake Map` asset (`UQuakeMap`). A new import starts with the defaults of the
[project settings](project-settings.md). A reimport applies all of them, to the assets and to the placed maps.

| Setting | Default | |
|---|---|---|
| `float Scale` | 3.125 | Unreal units (cm) per Quake unit |
| `bool bCSG` | on | remove faces hidden inside other brushes of the same entity |
| `bool bMeshLibrary` | off | the map is a collection of meshes and is not placed, see [Maps](../guide/maps.md#mesh-libraries) |
| `EQuakePivot Pivot` | BottomCenter | origin of the brush entities' meshes: `MapOrigin`, `Center`, `BottomCenter` or `BottomCorner`, see [Maps](../guide/maps.md#pivots) |
| `float LuxelSize` | 16 | Quake units per lightmap texel, sets the lightmap resolution of the meshes |
| `EComponentMobility LightMobility` | Static | mobility of the plugin's lights in placed maps, Movable for Lumen |
| `TMap<FString, TSoftObjectPtr<UMaterialInterface>> Materials` | empty | materials by texture name or prefix, before the project's, see [Maps](../guide/maps.md#materials) |
| `FDirectoryPath MaterialFolder` | empty | materials named like the textures, before the project's, see [Maps](../guide/maps.md#textures-and-materials) |
| `FDirectoryPath TextureFolder` | empty | texture assets named like the textures, before the project's and the image files |
| `UQuakeEntityClasses* EntityClasses` | none | actor classes for entity classes of this map, before the project's, see [Entities](../guide/entities.md) |

## Generated

| Property | |
|---|---|
| `TArray<UStaticMesh*> Meshes` | a mesh per brush entity |
| `TArray<FQuakeMapEntity> Entities` | every entity: class name, transform, keys, index of its mesh |
