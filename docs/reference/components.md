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
| `void Fire(AActor* Activator)` | triggers the entities of the same placement whose `targetname` is this entity's `target` |

Trigger brushes (class names starting with `trigger_`) fire by themselves when a pawn enters them,
`trigger_once` only the first time.

## UQuakeEntityClasses

Data asset with `TMap<FString, TSoftClassPtr<AActor>> Classes`, entity class name to actor class.

## UQuakeWad

The imported wad.

| Member | |
|---|---|
| `TArray<FQuakeWadTexture> Textures` | `Name` as in the wad, `Texture`, `Emission`, `SkyFront`, `bAlpha` |
| `const FQuakeWadTexture* Find(Name)` | by Quake name, case insensitive, null if missing |
