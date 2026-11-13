#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "QuakeEntityClasses.generated.h"

/**
 * Actor classes for Quake entity classes, e.g. "func_door" to a door Blueprint. Names ending in '*' are prefixes,
 * see LibQuake::FindByName. An entry without a class leaves the entity out, it gets no actor.
 */
UCLASS(BlueprintType)
class LIBQUAKE_API UQuakeEntityClasses : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entities")
	TMap<FString, TSoftClassPtr<AActor>> Classes;

	/**
	 * The class for an entity, from MapClasses if it has an entry and from the project settings otherwise. Returns
	 * false if neither has one, the entity then gets a static mesh actor or a point entity. OutClass is null for an
	 * entry without a class.
	 */
	static bool Resolve(const UQuakeEntityClasses* MapClasses, const FString& ClassName, UClass*& OutClass);
};
