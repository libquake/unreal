#pragma once

#include "ActorFactories/ActorFactory.h"

#include "ActorFactoryQuakeMap.generated.h"

/**
 * Drag and drop of a Quake Map places a Level Instance of its level. The level itself is made by the first
 * "Place in Level" from the asset's context menu, which Unreal does not allow during a drag and drop.
 */
UCLASS()
class UActorFactoryQuakeMap : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryQuakeMap();

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
};
