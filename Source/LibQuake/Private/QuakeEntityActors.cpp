#include "QuakeEntityActors.h"

#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "QuakeEntityComponent.h"

// A lit surface shows albedo / pi * the light in Unreal, a manual exposure with EV100 0 divides by 1.2.
float LibQuake::ToUnrealBrightness(float Level)
{
	return Level * 0.5f / 128.0f * UE_PI * 1.2f;
}

void AQuakePointLight::ApplyQuakeEntity_Implementation(UQuakeEntityComponent* Entity)
{
	const float Value = FMath::Max(Entity->GetFloat(TEXT("light"), 300.0f), 0.0f);

	// Unreal's setters ignore static lights once they are registered, the properties are set directly
	UPointLightComponent* Light = PointLightComponent;
	Light->Modify();
	Light->bUseInverseSquaredFalloff = false;
	Light->LightFalloffExponent = 2.0f;
	Light->IntensityUnits = ELightUnits::Unitless;
	Light->Intensity = LibQuake::ToUnrealBrightness(Value);
	Light->AttenuationRadius = Value * Entity->Scale;
	Light->LightColor = Entity->GetColor(TEXT("_color")).ToFColor(true);
	Light->MarkRenderStateDirty();
}

void AQuakePlayerStart::ApplyQuakeEntity_Implementation(UQuakeEntityComponent* Entity)
{
	// the start stands at the entity's origin already
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	AddActorWorldOffset(FVector(0, 0, HalfHeight - 24.0f * Entity->Scale));
}

void AQuakeSun::ApplyQuakeEntity_Implementation(UQuakeEntityComponent* Entity)
{
	FVector Direction(0, 0, -1);
	const FString Mangle = Entity->Keys.Contains(TEXT("_sun_mangle")) ? TEXT("_sun_mangle") : TEXT("_sunlight_mangle");
	if (Entity->Keys.Contains(Mangle))
	{
		const FVector Angles = Entity->GetVector(Mangle);
		const double Yaw = FMath::DegreesToRadians(Angles.X);
		const double Pitch = FMath::DegreesToRadians(Angles.Y);
		// Quake's y axis points the other way
		Direction = FVector(
			FMath::Cos(Yaw) * FMath::Cos(Pitch),
			-FMath::Sin(Yaw) * FMath::Cos(Pitch),
			FMath::Sin(Pitch));
	}
	SetActorRotation(Direction.Rotation());

	// like the point light, the properties are set directly
	UDirectionalLightComponent* Light = Cast<UDirectionalLightComponent>(GetLightComponent());
	Light->Modify();
	Light->Intensity = LibQuake::ToUnrealBrightness(Entity->GetFloat(TEXT("_sunlight")));
	Light->LightColor = Entity->GetColor(TEXT("_sunlight_color")).ToFColor(true);
	Light->MarkRenderStateDirty();
}

AQuakePointEntity::AQuakePointEntity()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
#if WITH_EDITORONLY_DATA
	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Sprite"));
	if (Sprite)
	{
		Sprite->SetupAttachment(RootComponent);
	}
#endif
}
