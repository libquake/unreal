# Project settings

Project Settings > Plugins > LibQuake (`ULibQuakeSettings`, saved in `Config/DefaultGame.ini`).

## New maps

A newly imported map starts with these, after that the [map settings](map-settings.md) of the map count. A
reimport keeps them.

| Setting | Default |
|---|---|
| `float DefaultScale` | 3.125 |
| `bool bDefaultCSG` | on |
| `EQuakePivot DefaultPivot` | BottomCenter |
| `float DefaultLuxelSize` | 16 |
| `EComponentMobility DefaultLightMobility` | Static |

## Entities

| Setting | |
|---|---|
| `TMap<FString, TSoftClassPtr<AActor>> EntityClasses` | actor class by entity class, see [Entities](../guide/entities.md). Comes with `light*` and `info_player_start` |

## Materials

| Setting | Default | |
|---|---|---|
| `SurfaceMaterial` | `M_Quake` | parent of the material instances, texture parameters `Albedo` and `Emissive`, scalar `EmissiveStrength` |
| `MaskedMaterial` | `M_QuakeMasked` | the same for `{` textures, transparent where the alpha of `Albedo` is 0 |
| `SkyMaterial` | `M_QuakeSky` | parent for sky textures, texture parameters `SkyBack` and `SkyFront` |
| `Materials` | empty | materials used as they are, by texture name or prefix, see [Maps](../guide/maps.md#materials) |

## Textures

| Setting | Default | |
|---|---|---|
| `MissingTexture` | `T_QuakeMissing` | for faces whose texture is in no imported wad, empty leaves them white |
