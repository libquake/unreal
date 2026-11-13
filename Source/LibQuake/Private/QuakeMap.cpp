#include "QuakeMap.h"

#include "EditorFramework/AssetImportData.h"
#include "UObject/AssetRegistryTagsContext.h"

bool UQuakeMap::IsMeshLibrary() const
{
	if (Settings.bMeshLibrary)
	{
		return true;
	}

	const FQuakeMapEntity* Worldspawn = Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("worldspawn"); });
	if (!Worldspawn)
	{
		return false;
	}
	for (const TCHAR* Key : {TEXT("_meshlib"), TEXT("qu_meshlib")})
	{
		if (const FString* Value = Worldspawn->Keys.Find(Key); Value && *Value != TEXT("0"))
		{
			return true;
		}
	}
	return false;
}

#if WITH_EDITORONLY_DATA
void UQuakeMap::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
	Super::PostInitProperties();
}

void UQuakeMap::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	if (AssetImportData)
	{
		Context.AddTag(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(),
			FAssetRegistryTag::TT_Hidden));
	}
	Super::GetAssetRegistryTags(Context);
}
#endif
