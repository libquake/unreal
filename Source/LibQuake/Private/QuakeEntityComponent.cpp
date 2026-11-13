#include "QuakeEntityComponent.h"

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

FString UQuakeEntityComponent::GetKey(const FString& Key) const
{
	return Keys.FindRef(Key);
}

float UQuakeEntityComponent::GetFloat(const FString& Key, float Fallback) const
{
	const FString* Value = Keys.Find(Key);
	return Value ? FCString::Atof(**Value) : Fallback;
}

void UQuakeEntityComponent::Fire(AActor* Activator)
{
	const FString Target = GetKey(TEXT("target"));
	if (Target.IsEmpty() || !GetWorld())
	{
		return;
	}

	// each placement of a map is its own level, targets stay within it
	const ULevel* Level = GetOwner()->GetLevel();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetLevel() != Level)
		{
			continue;
		}
		UQuakeEntityComponent* Other = It->FindComponentByClass<UQuakeEntityComponent>();
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
