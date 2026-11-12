#include "QuakeWad.h"

#include "EditorFramework/AssetImportData.h"
#include "UObject/AssetRegistryTagsContext.h"

const FQuakeWadTexture* UQuakeWad::Find(const FString& Name) const
{
	return Textures.FindByPredicate([&Name](const FQuakeWadTexture& Texture)
	{
		return Texture.Name.Equals(Name, ESearchCase::IgnoreCase);
	});
}

#if WITH_EDITORONLY_DATA
void UQuakeWad::PostInitProperties()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		AssetImportData = NewObject<UAssetImportData>(this, TEXT("AssetImportData"));
	}
	Super::PostInitProperties();
}

void UQuakeWad::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	if (AssetImportData)
	{
		Context.AddTag(FAssetRegistryTag(SourceFileTagName(), AssetImportData->GetSourceData().ToJson(),
			FAssetRegistryTag::TT_Hidden));
	}
	Super::GetAssetRegistryTags(Context);
}
#endif
