#pragma once

#include "AssetDefinitionDefault.h"
#include "QuakeMap.h"

#include "AssetDefinition_QuakeMap.generated.h"

UCLASS()
class UAssetDefinition_QuakeMap : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("LibQuake", "QuakeMap", "Quake Map"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(255, 110, 199)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UQuakeMap::StaticClass(); }

	/** Imported from a file: offers Reimport and lets Auto Reimport pick up changes of the file. */
	virtual bool CanImport() const override { return true; }

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Misc };
		return Categories;
	}
};
