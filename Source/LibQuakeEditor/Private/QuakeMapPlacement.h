#pragma once

#include "CoreMinimal.h"

class AActor;
class ULevel;
class UQuakeMap;
class UWorld;

namespace LibQuakeEditor
{
	/** Spawns the map into Level at the world origin: a static mesh actor per mesh, lights and player starts. */
	TArray<AActor*> SpawnMapActors(const UQuakeMap* Map, ULevel* Level);

	/**
	 * Places the map in the current level of World at Transform. The first time, the actors are spawned and handed to
	 * Unreal's Create Level Instance, which makes them the map's level. Later placements add a Level Instance of that
	 * level. Runs a garbage collection, so it belongs in a menu action, not in an import or a drag and drop.
	 */
	AActor* PlaceMap(UQuakeMap* Map, UWorld* World, const FTransform& Transform);

	/** Whether the level made by an earlier placement still exists. */
	bool HasLevel(const UQuakeMap* Map);
}
