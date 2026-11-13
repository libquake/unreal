#pragma once

#include "CoreMinimal.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PhysicsVolume.h"
#include "GameFramework/PlayerStart.h"
#include "QuakeEntity.h"

#include "QuakeEntityActors.generated.h"

class UBillboardComponent;

/**
 * Quake's "light": brightness and radius at once, fading linearly to 0 at the radius. The light tools halve every
 * light (_range 0.5) and a surface at level 128 shows its texture as it is. Unreal fades by (1 - d²/r²)^exponent
 * without inverse square falloff, 2 comes closest to linear.
 */
UCLASS(Blueprintable)
class LIBQUAKE_API AQuakePointLight : public APointLight, public IQuakeEntity
{
	GENERATED_BODY()

public:
	virtual void ApplyQuakeEntity_Implementation(UQuakeEntityComponent* Entity) override;
};

/** Quake's origin is 24 units above the feet, the start is raised so its capsule stands on the same floor. */
UCLASS(Blueprintable)
class LIBQUAKE_API AQuakePlayerStart : public APlayerStart, public IQuakeEntity
{
	GENERATED_BODY()

public:
	virtual void ApplyQuakeEntity_Implementation(UQuakeEntityComponent* Entity) override;
};

/**
 * The sun of a map, from the keys of its worldspawn: "_sunlight", "_sunlight_color" and "_sun_mangle" or
 * "_sunlight_mangle" as yaw and pitch of the direction the light travels, straight down without one.
 */
UCLASS(Blueprintable)
class LIBQUAKE_API AQuakeSun : public ADirectionalLight, public IQuakeEntity
{
	GENERATED_BODY()

public:
	virtual void ApplyQuakeEntity_Implementation(UQuakeEntityComponent* Entity) override;
};

/**
 * A liquid brush as a water volume: characters swim in it. Its QuakeEntity component has the keys "texture" (e.g.
 * "*lava1") and "liquid" ("water", "slime" or "lava"), Blueprints based on it can add damage for slime and lava.
 */
UCLASS(Blueprintable)
class LIBQUAKE_API AQuakeLiquidVolume : public APhysicsVolume, public IQuakeEntity
{
	GENERATED_BODY()

public:
	virtual void ApplyQuakeEntity_Implementation(UQuakeEntityComponent* Entity) override;
};

/** A point entity without a class of its own, keeps its place and keys for Blueprints mapped to it later. */
UCLASS(Blueprintable)
class LIBQUAKE_API AQuakePointEntity : public AActor
{
	GENERATED_BODY()

public:
	AQuakePointEntity();

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	TObjectPtr<UBillboardComponent> Sprite;
#endif
};

namespace LibQuake
{
	/**
	 * Unreal brightness that shows a surface like the Quake light level Level * 0.5 does under a manual exposure
	 * with EV100 0: lux for a directional light, the intensity at the center of a point light without inverse
	 * square falloff.
	 */
	LIBQUAKE_API float ToUnrealBrightness(float Level);

	/** "lava", "slime" or "water" for a liquid texture, by its name as in Quake. */
	LIBQUAKE_API FString LiquidKind(const FString& Texture);
}
