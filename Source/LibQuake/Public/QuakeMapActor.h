#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "QuakeMapActor.generated.h"

class UQuakeMap;

/**
 * A placed Quake map. The actors of its entities are attached to it, moving it moves the whole map. A reimport of
 * the map updates them: entities keep their actor, new ones get one, removed ones lose it. Properties the plugin
 * does not set stay as they were edited. An actor detached from the map is no longer part of it.
 */
UCLASS(NotBlueprintable)
class LIBQUAKE_API AQuakeMapActor : public AActor
{
	GENERATED_BODY()

public:
	AQuakeMapActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quake")
	TObjectPtr<UQuakeMap> Map;

	/** The actors of the entities, in no particular order. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	TArray<AActor*> GetEntityActors() const;

	/** Deleting the map deletes the actors of its entities. */
	virtual void Destroyed() override;

#if WITH_EDITOR
	/** Set by the editor module, rebuilds the entity actors from the map. */
	static inline TFunction<void(AQuakeMapActor*)> RebuildHandler;

	/** Brings the entity actors in line with the map again, like a reimport does. */
	UFUNCTION(CallInEditor, Category = "Quake")
	void Rebuild();
#endif
};
