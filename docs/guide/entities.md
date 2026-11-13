# Entities

Placing a map spawns an actor per entity. Each one carries a `UQuakeEntityComponent` with the entity's class name
and all its keys.

| Entity | Actor |
|---|---|
| mapped in the map's `EntityClasses` | that class, brush entities get their mesh as an extra component |
| `worldspawn` and other brush entities | static mesh actor |
| `light*` | static point light |
| `info_player_start` | player start, raised so its capsule stands on the floor the Quake player stands on |
| `worldspawn` with `_sunlight` | an additional directional light, see [Lighting](lighting.md) |
| other point entities | an empty actor at the entity's origin |

## Mapping classes

Create a `Quake Entity Classes` data asset (Miscellaneous > Data Asset) and add class names with an actor class
or Blueprint, e.g. `func_door` to a door Blueprint. Set it as `EntityClasses` in the map's settings and place the
map again.

## Keys

`UQuakeEntityComponent` keeps the keys as text, as they are in the map. `GetKey` and `GetFloat` read them, from
C++ and from Blueprints. Positions and directions in keys are in Quake units and axes.

## Targets

`target` and `targetname` connect entities. `Fire` on an entity triggers every entity in the same placement whose
`targetname` matches its `target`, they receive the `OnTriggered` event with the actor that set it off.

Trigger brushes (`trigger_*`) fire when a pawn enters them, `trigger_once` only the first time. A Blueprint for
`func_door` that binds `OnTriggered` of its entity component to opening the door is enough for Quake's door
buttons and trigger fields.
