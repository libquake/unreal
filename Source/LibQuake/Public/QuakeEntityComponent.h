#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "QuakeEntityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuakeEntityTriggered, AActor*, Activator);

/**
 * The keys of a map entity on the actor placed for it. Entities find each other through "target" and
 * "targetname": Fire() triggers every entity whose targetname is this entity's target. Trigger brushes
 * (trigger_*) fire when something enters them, trigger_once only the first time.
 */
UCLASS(ClassGroup = (LibQuake), meta = (BlueprintSpawnableComponent))
class LIBQUAKE_API UQuakeEntityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quake")
	FString ClassName;

	/** All keys as written in the map, classname included. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quake")
	TMap<FString, FString> Keys;

	/** Called when another entity targets this one. */
	UPROPERTY(BlueprintAssignable, Category = "Quake")
	FQuakeEntityTriggered OnTriggered;

	UFUNCTION(BlueprintPure, Category = "Quake")
	FString GetKey(const FString& Key) const;

	UFUNCTION(BlueprintPure, Category = "Quake")
	float GetFloat(const FString& Key, float Fallback = 0) const;

	/** Triggers every entity in the world whose targetname is this entity's target. */
	UFUNCTION(BlueprintCallable, Category = "Quake")
	void Fire(AActor* Activator);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlap(AActor* OverlappedActor, AActor* OtherActor);

	bool bFired = false;
};
