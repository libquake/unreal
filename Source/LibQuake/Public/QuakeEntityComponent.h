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
	bool HasKey(const FString& Key) const;

	/** The value as written, empty if the entity has no such key. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	FString GetKey(const FString& Key) const;

	UFUNCTION(BlueprintPure, Category = "Quake")
	float GetFloat(const FString& Key, float Fallback = 0) const;

	UFUNCTION(BlueprintPure, Category = "Quake")
	int32 GetInt(const FString& Key, int32 Fallback = 0) const;

	/** "1" or "true" is true, "0" or "false" false, case does not matter. Fallback otherwise. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	bool GetBool(const FString& Key, bool Fallback = false) const;

	/** Whether a bit of "spawnflags" is set, e.g. 1 for a door's START_OPEN. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	bool HasSpawnFlag(int32 Flag) const;

	/** "x y z" as written, in Quake units and axes, missing values are 0. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	FVector GetVector(const FString& Key) const;

	/**
	 * "x y z" as a position in Unreal units and axes, scaled like the map. Positions in the map, like "origin", are
	 * relative to the map actor, the actor's own place is GetActorLocation.
	 */
	UFUNCTION(BlueprintPure, Category = "Quake")
	FVector GetPosition(const FString& Key) const;

	/** "angles" (pitch yaw roll) or the yaw "angle" as an Unreal rotation. */
	UFUNCTION(BlueprintPure, Category = "Quake")
	FRotator GetRotation() const;

	/**
	 * The direction a brush entity moves in, e.g. a door or a button: "angle" -1 is up, -2 down, other values and
	 * "angles" are a direction like GetRotation. Brush entities are not turned by their angle, it is only this.
	 */
	UFUNCTION(BlueprintPure, Category = "Quake")
	FVector GetMoveDirection() const;

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
