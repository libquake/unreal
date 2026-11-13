#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "QuakeMap.generated.h"

class UAssetImportData;
class UStaticMesh;

USTRUCT(BlueprintType)
struct LIBQUAKE_API FQuakeMapSettings
{
	GENERATED_BODY()

	/** Unreal units (cm) per Quake unit. 3.125 makes 32 Quake units a meter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import", meta = (ClampMin = "0.01"))
	float Scale = 3.125f;

	/** Remove faces hidden inside other brushes of the same entity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import")
	bool bCSG = true;

	/** Quake units per lightmap texel. 16 matches Quake, smaller values give sharper shadows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting", meta = (ClampMin = "1"))
	float LuxelSize = 16.0f;
};

/**
 * An imported .map file with a static mesh per brush entity. The .map stays the source: everything in Generated is
 * rebuilt on every reimport.
 */
UCLASS(BlueprintType)
class LIBQUAKE_API UQuakeMap : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import")
	FQuakeMapSettings Settings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated")
	TArray<TObjectPtr<UStaticMesh>> Meshes;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Import")
	TObjectPtr<UAssetImportData> AssetImportData;

	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
#endif
};
