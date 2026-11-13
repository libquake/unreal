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
| `TMap<FString, TSoftClassPtr<APhysicsVolume>> LiquidClasses` | volume class by liquid texture, see [Maps](../guide/maps.md#liquids). Comes with `*` for `AQuakeLiquidVolume` |

## Materials

| Setting | Default | |
|---|---|---|
| `SurfaceMaterial` | `M_Quake` | parent of the material instances, texture parameters `Albedo` and `Emissive`, scalar `EmissiveStrength` |
| `MaskedMaterial` | `M_QuakeMasked` | the same for `{` textures, transparent where the alpha of `Albedo` is 0 |
| `SkyMaterial` | `M_QuakeSky` | parent for sky textures, texture parameters `SkyBack` and `SkyFront` |
| `LiquidMaterial` | `M_QuakeLiquidTranslucent` | parent for liquid textures (`*`), texture parameter `Albedo`, scalars `Opacity` and `Brightness`. `M_QuakeLiquid` is opaque |
| `Materials` | empty | materials used as they are, by texture name or prefix, see [Maps](../guide/maps.md#materials) |

## Liquids

How much a liquid covers what is behind it, 1 is opaque. A map overrides them with the worldspawn keys
`_wateralpha`, `_slimealpha`, `_lavaalpha` and `_telealpha`.

| Setting | Default |
|---|---|
| `float WaterAlpha` | 0.6 |
| `float SlimeAlpha` | 0.7 |
| `float LavaAlpha` | 0.85 |
| `float TeleportAlpha` | 1 |
| `float LavaBrightness` | 2, above 1 lava glows with bloom |

## Textures

| Setting | Default | |
|---|---|---|
| `MissingTexture` | `T_QuakeMissing` | for faces whose texture is in no imported wad, empty leaves them white |
