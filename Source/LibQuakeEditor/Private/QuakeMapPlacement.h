#pragma once

#include "CoreMinimal.h"

class AQuakeMapActor;
class UQuakeMap;
class UWorld;

namespace LibQuakeEditor
{
	/** Places the map in the current level of World at Transform: an AQuakeMapActor and its entity actors. */
	AQuakeMapActor* PlaceMap(UQuakeMap* Map, UWorld* World, const FTransform& Transform);

	/**
	 * Brings the entity actors of a placed map in line with its map. Every entity gets an actor of the class mapped
	 * to its class name, or a static mesh actor for brush entities and an AQuakePointEntity otherwise. An entity
	 * keeps its actor while the class stays the same, actors of entities that are gone are deleted. The actor's
	 * place, mesh and keys are set and actors implementing IQuakeEntity apply the keys. The worldspawn adds a sun
	 * ("_sunlight") and the map an exposure volume (bManualExposure).
	 */
	void SyncMapActor(AQuakeMapActor* MapActor);
}
