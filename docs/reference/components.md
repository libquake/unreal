# Components

Runtime types in the `LibQuake` module, they work in packaged games. Importers and placement are in
`LibQuakeEditor`.

## UQuakeEntityComponent

On every actor placed for an entity.

| Member | |
|---|---|
| `FString ClassName` | the entity's class name |
| `TMap<FString, FString> Keys` | all keys as in the map, `classname` included |
| `FQuakeEntityTriggered OnTriggered` | called with the activator when another entity targets this one |
| `FString GetKey(Key)` | the value, empty if the entity has no such key |
| `float GetFloat(Key, Fallback)` | the value as a number |
| `FVector GetVector(Key)` | "x y z" as written, in Quake units and axes |
| `FLinearColor GetColor(Key, Fallback)` | "r g b" as 0..1 or 0..255 |
| `FRotator GetRotation()` | `angles` (pitch yaw roll) or `angle` as an Unreal rotation |
| `FVector GetMoveDirection()` | the direction a brush entity moves in: `angle`, `-1` up, `-2` down, or `angles` |
| `float Scale` | Unreal units per Quake unit of the map |
| `FString EntityId` | `<classname>_<n>` in map order, a reimport finds the actor by it |
| `void Fire(AActor* Activator)` | triggers the entities of the same placed map whose `targetname` is this entity's `target` |

Trigger brushes (class names starting with `trigger_`) fire by themselves when a pawn enters them,
`trigger_once` only the first time.

## IQuakeEntity

Interface for actors placed for an entity. `ApplyQuakeEntity(UQuakeEntityComponent* Entity)` is a Blueprint native
event, called when the map is placed and after every reimport.

## AQuakeMapActor

A placed map. `Map` is the `UQuakeMap`, `GetEntityActors()` the attached entity actors. Destroying it destroys
them. `Rebuild` (editor only) brings them in line with the map.

## Entity actors

| Class | |
|---|---|
| `AQuakePointLight` | point light from `light` and `_color` |
| `AQuakePlayerStart` | player start standing on the floor |
| `AQuakeSun` | directional light from `_sunlight`, `_sunlight_color`, `_sun_mangle` of the worldspawn |
| `AQuakePointEntity` | empty actor with a sprite, for point entities without a class |

## UQuakeEntityClasses

Data asset with `TMap<FString, TSoftClassPtr<AActor>> Classes`, entity class name (or prefix ending in `*`) to
actor class, an entry without a class leaves the entity out. `Resolve` looks up a map's classes, then the project
settings (`ULibQuakeSettings::EntityClasses`). `LibQuake::FindByName` (`QuakeNameTable.h`) is the lookup by name
or prefix the class and material tables share.

## UQuakeWad

The imported wad.

| Member | |
|---|---|
| `TArray<FQuakeWadTexture> Textures` | `Name` as in the wad, `Texture`, `Emission`, `SkyFront`, `bAlpha` |
| `const FQuakeWadTexture* Find(Name)` | by Quake name, case insensitive, null if missing |
