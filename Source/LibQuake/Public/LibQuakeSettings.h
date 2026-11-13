#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "QuakeMap.h"

#include "LibQuakeSettings.generated.h"

class APhysicsVolume;
class UMaterialInterface;
class UTexture2D;

/** Project Settings > Plugins > LibQuake. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LibQuake"))
class LIBQUAKE_API ULibQuakeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULibQuakeSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/** Writes the defaults for new maps into Settings, a newly imported map starts with them. */
	void ApplyDefaults(FQuakeMapSettings& Settings) const;

	// A newly imported map starts with these, after that its own settings count.

	UPROPERTY(Config, EditAnywhere, Category = "New Maps", meta = (ClampMin = "0.01"))
	float DefaultScale = 3.125f;

	UPROPERTY(Config, EditAnywhere, Category = "New Maps")
	bool bDefaultCSG = true;

	UPROPERTY(Config, EditAnywhere, Category = "New Maps")
	EQuakePivot DefaultPivot = EQuakePivot::BottomCenter;

	UPROPERTY(Config, EditAnywhere, Category = "New Maps", meta = (ClampMin = "1"))
	float DefaultLuxelSize = 16.0f;

	UPROPERTY(Config, EditAnywhere, Category = "New Maps")
	TEnumAsByte<EComponentMobility::Type> DefaultLightMobility = EComponentMobility::Static;

	/**
	 * Actor classes for entity classes in every map, names ending in '*' are prefixes. A map's own EntityClasses
	 * come first. An entry without a class leaves the entity out. Comes with the plugin's lights and player start.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Entities")
	TMap<FString, TSoftClassPtr<AActor>> EntityClasses;

	/**
	 * Volume classes for liquid brushes by texture name, names ending in '*' are prefixes ("*lava*" for lava). An
	 * entry without a class or no entry leaves the liquid without a volume. Comes with "*" for AQuakeLiquidVolume.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Entities")
	TMap<FString, TSoftClassPtr<APhysicsVolume>> LiquidClasses;

	/** Parent of the material instances of plain textures. Needs the texture parameters Albedo and Emissive. */
	UPROPERTY(Config, EditAnywhere, Category = "Materials")
	TSoftObjectPtr<UMaterialInterface> SurfaceMaterial;

	/** Parent for '{' textures, transparent where Albedo's alpha is 0. Same parameters as SurfaceMaterial. */
	UPROPERTY(Config, EditAnywhere, Category = "Materials")
	TSoftObjectPtr<UMaterialInterface> MaskedMaterial;

	/** Parent for sky textures. Needs the texture parameters SkyBack and SkyFront. */
	UPROPERTY(Config, EditAnywhere, Category = "Materials")
	TSoftObjectPtr<UMaterialInterface> SkyMaterial;

	/**
	 * Parent for liquid textures ('*'). Needs the texture parameter Albedo. The plugin's M_QuakeLiquid is opaque,
	 * M_QuakeLiquidTranslucent lets the floor show through.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Materials")
	TSoftObjectPtr<UMaterialInterface> LiquidMaterial;

	/**
	 * Materials used as they are instead of a material instance, by texture name, names ending in '*' are prefixes
	 * ("*water*" for every water texture). A map's own Materials come first.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Materials")
	TMap<FString, TSoftObjectPtr<UMaterialInterface>> Materials;

	/**
	 * For faces whose texture is in none of the imported wads, their uvs follow its size. Without one they are
	 * plain white. Used when a map is imported.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Textures")
	TSoftObjectPtr<UTexture2D> MissingTexture;
};
