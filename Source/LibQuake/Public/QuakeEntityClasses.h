#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "QuakeEntityClasses.generated.h"

/**
 * Actor classes for Quake entity classes, e.g. "func_door" to a door Blueprint. Placing a map spawns the class
 * listed here for an entity, brush entities get their mesh as an extra component. Entities without an entry get
 * the defaults: static meshes, point lights and player starts.
 */
UCLASS(BlueprintType)
class LIBQUAKE_API UQuakeEntityClasses : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entities")
	TMap<FString, TSoftClassPtr<AActor>> Classes;
};
