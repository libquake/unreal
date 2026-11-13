#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"

#include "QuakeEntityComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FQuakeEntityTriggered, AActor*, Activator);

/**
 * The keys of a map entity on the actor placed for it. Entities find each other through "target" and
 * "targetname": Fire() triggers every entity of the same map whose targetname is this entity's target. Trigger
 * brushes (trigger_*) fire when something enters them, trigger_once only the first time.
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

	/** Unreal units (cm) per Quake unit of the map. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quake")
	float Scale = 3.125f;

	/** Which entity of the map this is, "<classname>_<n>" in map order. A reimport finds the actor by it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quake")
	FString EntityId;

	/** Called when another entity targets this one. */
	UPROPERTY(BlueprintAssignable, Category = "Quake")
	FQuakeEntityTriggered OnTriggered;

	UFUNCTION(BlueprintPure, Category = "Quake")
	FString GetKey(const FString& Key) const;

	UFUNCTION(BlueprintPure, Category = "Quake")
	float GetFloat(const FString& Key, float Fallback = 0) const;

	/** "x y z" as written, in Quake units and axes, missing values are 0. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	FVector GetVector(const FString& Key) const;

	/** "r g b" as 0..1 or 0..255, Fallback if the entity has no such key. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	FLinearColor GetColor(const FString& Key, FLinearColor Fallback = FLinearColor::White) const;

	/** Triggers every entity of the same map whose targetname is this entity's target. */
	UFUNCTION(BlueprintCallable, Category = "Quake")
	void Fire(AActor* Activator);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnOverlap(AActor* OverlappedActor, AActor* OtherActor);

	bool bFired = false;
};
