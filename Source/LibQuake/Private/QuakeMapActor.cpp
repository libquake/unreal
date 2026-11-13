#include "QuakeMapActor.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "QuakeEntityComponent.h"

AQuakeMapActor::AQuakeMapActor()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Static);
	RootComponent = Root;
}

TArray<AActor*> AQuakeMapActor::GetEntityActors() const
{
	TArray<AActor*> Attached;
	GetAttachedActors(Attached);
	Attached.RemoveAll([](const AActor* Actor) { return !Actor->FindComponentByClass<UQuakeEntityComponent>(); });
	return Attached;
}

void AQuakeMapActor::Destroyed()
{
	// still attached here, Unreal detaches the children after Destroyed
	UWorld* World = GetWorld();
	for (AActor* Actor : GetEntityActors())
	{
#if WITH_EDITOR
		if (World && !World->IsGameWorld())
		{
			World->EditorDestroyActor(Actor, true);
			continue;
		}
#endif
		Actor->Destroy();
	}
	Super::Destroyed();
}

#if WITH_EDITOR
void AQuakeMapActor::Rebuild()
{
	if (RebuildHandler)
	{
		RebuildHandler(this);
	}
}
#endif
