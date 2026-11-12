#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "QuakeWad.generated.h"

class UAssetImportData;
class UTexture2D;

USTRUCT(BlueprintType)
struct LIBQUAKE_API FQuakeWadTexture
{
	GENERATED_BODY()

	/** The name maps use, e.g. "*water1" or "{fence". */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	FString Name;

	/** For skies the back layer. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	TObjectPtr<UTexture2D> Texture;

	/** Fullbright pixels, black elsewhere. Null if the texture has none. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	TObjectPtr<UTexture2D> Emission;

	/** Cloud layer of a sky, alpha 0 where the back layer shows. Null for other textures. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	TObjectPtr<UTexture2D> SkyFront;

	/** '{' texture, transparent where alpha is 0. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Texture")
	bool bAlpha = false;
};

/** An imported Quake wad. Maps look textures up here by their Quake name. */
UCLASS(BlueprintType)
class LIBQUAKE_API UQuakeWad : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wad")
	TArray<FQuakeWadTexture> Textures;

	/** The texture with this Quake name, case insensitive. Null if the wad does not have it. */
	const FQuakeWadTexture* Find(const FString& Name) const;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Import")
	TObjectPtr<UAssetImportData> AssetImportData;

	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
#endif
};
