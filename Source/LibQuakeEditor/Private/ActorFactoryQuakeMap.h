#pragma once

#include "ActorFactories/ActorFactory.h"

#include "ActorFactoryQuakeMap.generated.h"

/**
 * Drag and drop of a Quake Map places an AQuakeMapActor with the actors of its entities. Mesh libraries are refused,
 * their meshes are placed instead.
 */
UCLASS()
class UActorFactoryQuakeMap : public UActorFactory
{
	GENERATED_BODY()

public:
	UActorFactoryQuakeMap();

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
};
