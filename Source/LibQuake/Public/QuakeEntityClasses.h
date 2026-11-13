#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "QuakeEntityClasses.generated.h"

/**
 * Actor classes for Quake entity classes, e.g. "func_door" to a door Blueprint. A name ending in '*' is a prefix:
 * "light*" stands for "light", "light_torch_small_walltorch" and so on. An exact name wins over a prefix, a longer
 * prefix over a shorter one.
 */
UCLASS(BlueprintType)
class LIBQUAKE_API UQuakeEntityClasses : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entities")
	TMap<FString, TSoftClassPtr<AActor>> Classes;

	/** The class for an entity class name in Classes, null if no entry matches. */
	static TSoftClassPtr<AActor> Find(const TMap<FString, TSoftClassPtr<AActor>>& Classes, const FString& ClassName);

	/**
	 * The class for an entity: from MapClasses if it has an entry, from the project settings otherwise. Null if
	 * neither has one, the entity then gets a static mesh actor or a point entity.
	 */
	static UClass* Resolve(const UQuakeEntityClasses* MapClasses, const FString& ClassName);
};
