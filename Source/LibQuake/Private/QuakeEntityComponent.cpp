#include "QuakeEntityComponent.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "QuakeConversion.h"

FString UQuakeEntityComponent::GetKey(const FString& Key) const
{
	return Keys.FindRef(Key);
}

float UQuakeEntityComponent::GetFloat(const FString& Key, float Fallback) const
{
	const FString* Value = Keys.Find(Key);
	return Value ? FCString::Atof(**Value) : Fallback;
}

FVector UQuakeEntityComponent::GetVector(const FString& Key) const
{
	FVector Result = FVector::ZeroVector;
	if (const FString* Value = Keys.Find(Key))
	{
		TArray<FString> Parts;
		Value->ParseIntoArrayWS(Parts);
		for (int32 i = 0; i < 3 && i < Parts.Num(); i++)
		{
			Result[i] = FCString::Atod(*Parts[i]);
		}
	}
	return Result;
}

FRotator UQuakeEntityComponent::GetRotation() const
{
	const FVector Angles = Keys.Contains(TEXT("angles")) ? GetVector(TEXT("angles"))
		: FVector(0, GetFloat(TEXT("angle")), 0);
	return LibQuake::ToUnrealRotation(Angles);
}

FVector UQuakeEntityComponent::GetMoveDirection() const
{
	if (!Keys.Contains(TEXT("angles")))
	{
		const float Angle = GetFloat(TEXT("angle"));
		if (Angle == -1)
		{
			return FVector::UpVector;
		}
		if (Angle == -2)
		{
			return FVector::DownVector;
		}
	}
	return GetRotation().Vector();
}

FLinearColor UQuakeEntityComponent::GetColor(const FString& Key, FLinearColor Fallback) const
{
	if (!Keys.Contains(Key))
	{
		return Fallback;
	}
	FVector Rgb = GetVector(Key);
	if (Rgb.GetMax() > 1.0)
	{
		Rgb /= 255.0;
	}
	return FLinearColor(Rgb.X, Rgb.Y, Rgb.Z);
}

void UQuakeEntityComponent::Fire(AActor* Activator)
{
	const FString Target = GetKey(TEXT("target"));
	if (Target.IsEmpty() || !GetWorld())
	{
		return;
	}

	// targets stay within the map the entity belongs to, the actors attached to the same map actor
	TArray<AActor*> Candidates;
	if (const AActor* Map = GetOwner()->GetAttachParentActor())
	{
		Map->GetAttachedActors(Candidates);
	}
	else
	{
		for (TActorIterator<AActor> It(GetWorld()); It; ++It)
		{
			Candidates.Add(*It);
		}
	}

	for (AActor* Candidate : Candidates)
	{
		UQuakeEntityComponent* Other = Candidate->FindComponentByClass<UQuakeEntityComponent>();
		if (Other && Other->GetKey(TEXT("targetname")) == Target)
		{
			Other->OnTriggered.Broadcast(Activator);
		}
	}
}

void UQuakeEntityComponent::BeginPlay()
{
	Super::BeginPlay();
	if (ClassName.StartsWith(TEXT("trigger_")))
	{
		GetOwner()->OnActorBeginOverlap.AddDynamic(this, &UQuakeEntityComponent::OnOverlap);
	}
}

void UQuakeEntityComponent::OnOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	// Quake's triggers react to players and monsters, both are pawns here
	if (!Cast<APawn>(OtherActor) || (bFired && ClassName == TEXT("trigger_once")))
	{
		return;
	}
	bFired = true;
	Fire(OtherActor);
}
