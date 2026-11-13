# Entities

Placing a map spawns an actor per entity, attached to the map actor. Each one carries a `UQuakeEntityComponent`
with the entity's class name and all its keys.

## Entity classes

One table says which actor class an entity class becomes: Project Settings > Plugins > LibQuake > Entity Classes.
It comes with the plugin's classes:

| Entity | Actor |
|---|---|
| `light*` | `AQuakePointLight`, a static point light, see [Lighting](lighting.md) |
| `info_player_start` | `AQuakePlayerStart`, raised so its capsule stands on the floor the Quake player stands on |

Entities without an entry get a static mesh actor if they are brush entities and an `AQuakePointEntity`, an empty
actor with a sprite, otherwise. A worldspawn with `_sunlight` adds an `AQuakeSun`.

A name ending in `*` is a prefix: `light*` covers `light`, `light_fluoro`, `light_torch_small_walltorch` and so on,
`*` alone every class.
An exact name wins over a prefix, a longer prefix over a shorter one. Brush entities mapped to a class get their
mesh as an extra component `QuakeMesh`.

An entry without a class leaves the entity out, e.g. `info_player_deathmatch` or `ambient_*` when a game has no
use for them. Brush entities left out keep their mesh asset, they just get no actor.

To change the classes for one map only, create a `Quake Entity Classes` data asset (Miscellaneous > Data Asset)
and set it as `EntityClasses` in the map's settings. Its entries come before the project's.

## Own classes

Map an entity class to any actor class or Blueprint, e.g. `func_door` to a door Blueprint or `trigger_teleport`
and `info_teleport_destination` to teleporter Blueprints. The keys are in the actor's `QuakeEntity` component.

Actors that implement the `Quake Entity` interface get `ApplyQuakeEntity` when the map is placed and after every
reimport, once they stand at the entity's place and the component holds the keys. A Blueprint based on
`AQuakePointLight` that implements it and calls the parent keeps the plugin's light and can add to it, e.g. a
flicker for the `style` key.

## Keys

`UQuakeEntityComponent` keeps the keys as text, as they are in the map, custom keys included. From C++ and from
Blueprints, `HasKey`, `GetKey`, `GetFloat`, `GetInt`, `GetBool` and `HasSpawnFlag` read them. `GetVector` gives
"x y z" as written, in Quake units and axes, `GetPosition` the same in Unreal units and axes, scaled like the map.

Point entities are turned by `angles` (pitch yaw roll) or the yaw `angle`. Brush entities are not: their brushes
are already where they belong, `angle` is the direction they move in, like in Quake. `GetMoveDirection` gives it
as an Unreal direction (`-1` up, `-2` down), for door, button or push Blueprints. `GetRotation` turns `angles` or
`angle` into an Unreal rotation.

## Targets

`target` and `targetname` connect entities. `Fire` on an entity triggers every entity of the same placed map whose
`targetname` matches its `target`, they receive the `OnTriggered` event with the actor that set it off.

Trigger brushes (`trigger_*`) fire when a pawn enters them, `trigger_once` only the first time. A Blueprint for
`func_door` that binds `OnTriggered` of its entity component to opening the door is enough for Quake's door
buttons and trigger fields.
