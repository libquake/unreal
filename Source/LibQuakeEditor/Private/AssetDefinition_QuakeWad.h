#pragma once

#include "AssetDefinitionDefault.h"
#include "QuakeWad.h"

#include "AssetDefinition_QuakeWad.generated.h"

UCLASS()
class UAssetDefinition_QuakeWad : public UAssetDefinitionDefault
{
	GENERATED_BODY()

public:
	virtual FText GetAssetDisplayName() const override { return NSLOCTEXT("LibQuake", "QuakeWad", "Quake Wad"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(FColor(201, 125, 255)); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UQuakeWad::StaticClass(); }

	/** Imported from a file: offers Reimport and lets Auto Reimport pick up changes of the file. */
	virtual bool CanImport() const override { return true; }

	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override
	{
		static const FAssetCategoryPath Categories[] = { EAssetCategoryPaths::Misc };
		return Categories;
	}
};
