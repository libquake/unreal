#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "LibQuakeSettings.generated.h"

class UTexture2D;

/** Project Settings > Plugins > LibQuake. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "LibQuake"))
class LIBQUAKE_API ULibQuakeSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	ULibQuakeSettings();

	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }

	/**
	 * Actor classes for entity classes in every map, see UQuakeEntityClasses for the names. A map's own
	 * EntityClasses come first. Comes with the plugin's lights and player start.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Entities")
	TMap<FString, TSoftClassPtr<AActor>> EntityClasses;

	/**
	 * For faces whose texture is in none of the imported wads, their uvs follow its size. Without one they are
	 * plain white. Used when a map is imported.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Textures")
	TSoftObjectPtr<UTexture2D> MissingTexture;
};
