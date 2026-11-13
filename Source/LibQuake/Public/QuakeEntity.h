#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "QuakeEntity.generated.h"

class UQuakeEntityComponent;

UINTERFACE(BlueprintType, MinimalAPI)
class UQuakeEntity : public UInterface
{
	GENERATED_BODY()
};

/**
 * For actor classes that read the keys of their entity. ApplyQuakeEntity is called when the map is placed and
 * after every reimport, once the actor stands at the entity's place and Entity holds its keys. Blueprints implement
 * it as an event, Blueprints of the plugin's classes call the parent to keep what those do.
 */
class LIBQUAKE_API IQuakeEntity
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Quake")
	void ApplyQuakeEntity(UQuakeEntityComponent* Entity);
};
