#include "ActorFactoryQuakeMap.h"

#include "QuakeMap.h"
#include "QuakeMapActor.h"
#include "QuakeMapPlacement.h"

#define LOCTEXT_NAMESPACE "ActorFactoryQuakeMap"

UActorFactoryQuakeMap::UActorFactoryQuakeMap()
{
	DisplayName = LOCTEXT("DisplayName", "Quake Map");
	NewActorClass = AQuakeMapActor::StaticClass();
}

bool UActorFactoryQuakeMap::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	return AssetData.IsInstanceOf<UQuakeMap>();
}

void UActorFactoryQuakeMap::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);
	AQuakeMapActor* MapActor = CastChecked<AQuakeMapActor>(NewActor);
	MapActor->Map = Cast<UQuakeMap>(Asset);
	// the preview while dragging is transient, the entities come with the actor that is dropped
	if (!NewActor->HasAnyFlags(RF_Transient))
	{
		LibQuakeEditor::SyncMapActor(MapActor);
	}
}

UObject* UActorFactoryQuakeMap::GetAssetFromActorInstance(AActor* ActorInstance)
{
	const AQuakeMapActor* MapActor = Cast<AQuakeMapActor>(ActorInstance);
	return MapActor ? MapActor->Map : nullptr;
}

#undef LOCTEXT_NAMESPACE
