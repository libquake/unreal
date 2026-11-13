#include "ActorFactoryQuakeMap.h"

#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "QuakeMap.h"
#include "QuakeMapPlacement.h"

#define LOCTEXT_NAMESPACE "ActorFactoryQuakeMap"

UActorFactoryQuakeMap::UActorFactoryQuakeMap()
{
	DisplayName = LOCTEXT("DisplayName", "Quake Map");
	NewActorClass = ALevelInstance::StaticClass();
}

bool UActorFactoryQuakeMap::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsInstanceOf<UQuakeMap>())
	{
		return false;
	}
	const UQuakeMap* Map = Cast<UQuakeMap>(AssetData.GetAsset());
	if (!Map || !LibQuakeEditor::HasLevel(Map))
	{
		OutErrorMsg = LOCTEXT("NoLevel", "Place the map once with right click > Place in Level, that makes its level.");
		return false;
	}
	return true;
}

void UActorFactoryQuakeMap::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);
	ILevelInstanceInterface* Instance = CastChecked<ILevelInstanceInterface>(NewActor);
	if (const UQuakeMap* Map = Cast<UQuakeMap>(Asset))
	{
		Instance->SetWorldAsset(Map->Level);
		Instance->LoadLevelInstance();
	}
}

#undef LOCTEXT_NAMESPACE
