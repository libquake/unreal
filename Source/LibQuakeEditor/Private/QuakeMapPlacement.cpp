#include "QuakeMapPlacement.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "QuakeEntity.h"
#include "QuakeEntityActors.h"
#include "QuakeEntityClasses.h"
#include "QuakeEntityComponent.h"
#include "QuakeMap.h"
#include "QuakeMapActor.h"
#include "UObject/Script.h"

namespace
{
	const TCHAR* SunId = TEXT("sun");

	/** What an actor of a placed map is made from: an entity or the sun of the worldspawn. */
	struct FEntitySpec
	{
		/** "<classname>_<n>" in map order, "sun" for the sun. */
		FString Id;

		UClass* Class = nullptr;

		/** Relative to the map actor. */
		FTransform Transform;

		/** The entity, for the sun the worldspawn. */
		const FQuakeMapEntity* Entity = nullptr;

		bool bSun = false;

		/** The mesh of a brush entity, null for point entities. */
		UStaticMesh* Mesh = nullptr;
	};

	/** Every actor the map should have, in map order. */
	TArray<FEntitySpec> MakeSpecs(const UQuakeMap* Map)
	{
		TArray<FEntitySpec> Specs;
		TMap<FString, int32> ClassCount;
		for (const FQuakeMapEntity& Entity : Map->Entities)
		{
			FEntitySpec& Spec = Specs.AddDefaulted_GetRef();
			Spec.Id = FString::Printf(TEXT("%s_%d"), *Entity.ClassName, ClassCount.FindOrAdd(Entity.ClassName)++);
			Spec.Transform = Entity.Transform;
			Spec.Entity = &Entity;
			Spec.Mesh = Map->Meshes.IsValidIndex(Entity.Mesh) ? Map->Meshes[Entity.Mesh].Get() : nullptr;
			Spec.Class = UQuakeEntityClasses::Resolve(Map->Settings.EntityClasses, Entity.ClassName);
			if (!Spec.Class)
			{
				Spec.Class = Spec.Mesh ? AStaticMeshActor::StaticClass() : AQuakePointEntity::StaticClass();
			}

			if (Entity.ClassName == TEXT("worldspawn") && FCString::Atof(*Entity.Keys.FindRef(TEXT("_sunlight"))) > 0)
			{
				FEntitySpec Sun;
				Sun.Id = SunId;
				Sun.Class = AQuakeSun::StaticClass();
				Sun.Entity = &Entity;
				Sun.bSun = true;
				Specs.Add(Sun);
			}
		}
		return Specs;
	}

	/** An instance component, saved with the actor and part of undo. Attach and register it afterwards. */
	template <typename T>
	T* AddComponent(AActor* Actor, const TCHAR* Name)
	{
		T* Component = NewObject<T>(Actor, Name, RF_Transactional);
		Actor->AddInstanceComponent(Component);
		return Component;
	}

	/** Brushes without visible faces stay hidden, trigger_* ones only overlap, the others block. */
	void SetupMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FQuakeMapEntity& Entity)
	{
		Component->Modify();
		Component->SetStaticMesh(Mesh);
		Component->SetVisibility(!Entity.bInvisible);
		Component->SetHiddenInGame(Entity.bInvisible);
		Component->SetCastShadow(!Entity.bInvisible);
		const bool bTrigger = Entity.bInvisible && Entity.ClassName.StartsWith(TEXT("trigger_"));
		Component->SetCollisionProfileName(bTrigger ? TEXT("Trigger") : UCollisionProfile::BlockAll_ProfileName);
		Component->SetGenerateOverlapEvents(bTrigger);
	}

	/** The mesh component of a brush entity: the actor's own on a static mesh actor, an added one otherwise. */
	UStaticMeshComponent* FindOrAddMeshComponent(AActor* Actor)
	{
		if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(Actor))
		{
			return MeshActor->GetStaticMeshComponent();
		}
		for (UActorComponent* Component : Actor->GetInstanceComponents())
		{
			if (Component->GetFName() == TEXT("QuakeMesh"))
			{
				return Cast<UStaticMeshComponent>(Component);
			}
		}

		UStaticMeshComponent* Mesh = AddComponent<UStaticMeshComponent>(Actor, TEXT("QuakeMesh"));
		if (USceneComponent* Root = Actor->GetRootComponent())
		{
			Mesh->SetupAttachment(Root);
		}
		else
		{
			Actor->SetRootComponent(Mesh);
		}
		Mesh->RegisterComponent();
		return Mesh;
	}

	/** A new actor for the spec, attached to the map actor. ApplySpec gives it its place and keys. */
	AActor* SpawnEntityActor(AQuakeMapActor* MapActor, const FEntitySpec& Spec)
	{
		FActorSpawnParameters Parameters;
		Parameters.OverrideLevel = MapActor->GetLevel();
		const FTransform Transform = Spec.Transform * MapActor->GetActorTransform();
		AActor* Actor = MapActor->GetWorld()->SpawnActor(Spec.Class, &Transform, Parameters);
		if (!Actor)
		{
			return nullptr;
		}
		Actor->SetActorLabel(Spec.Id);

		if (Spec.Mesh)
		{
			FindOrAddMeshComponent(Actor);
		}
		if (!Actor->GetRootComponent())
		{
			// an actor class without components, it needs a root to have a place
			USceneComponent* Root = AddComponent<USceneComponent>(Actor, TEXT("Root"));
			Actor->SetRootComponent(Root);
			Root->RegisterComponent();
		}

		Actor->AttachToActor(MapActor, FAttachmentTransformRules::KeepWorldTransform);
		return Actor;
	}

	/**
	 * Puts the actor at its place relative to the map and gives it its mesh and keys, then lets IQuakeEntity actors
	 * apply the keys. Runs on new actors and on every update, everything it sets comes from the map.
	 */
	void ApplySpec(AActor* Actor, const FEntitySpec& Spec, const UQuakeMap* Map)
	{
		Actor->Modify();
		Actor->SetActorRelativeTransform(Spec.Transform);
		if (Spec.Mesh)
		{
			SetupMesh(FindOrAddMeshComponent(Actor), Spec.Mesh, *Spec.Entity);
		}

		UQuakeEntityComponent* Keys = Actor->FindComponentByClass<UQuakeEntityComponent>();
		if (!Keys)
		{
			Keys = AddComponent<UQuakeEntityComponent>(Actor, TEXT("QuakeEntity"));
			Keys->RegisterComponent();
		}
		Keys->Modify();
		Keys->EntityId = Spec.Id;
		Keys->Scale = Map->Settings.Scale;
		Keys->ClassName = Spec.bSun ? FString() : Spec.Entity->ClassName;
		Keys->Keys = Spec.Entity->Keys;

		if (Actor->Implements<UQuakeEntity>())
		{
			// without the guard Unreal drops events on actors of the editor world
			FEditorScriptExecutionGuard ScriptGuard;
			IQuakeEntity::Execute_ApplyQuakeEntity(Actor, Keys);
		}
	}
}

namespace LibQuakeEditor
{
	AQuakeMapActor* PlaceMap(UQuakeMap* Map, UWorld* World, const FTransform& Transform)
	{
		FActorSpawnParameters Parameters;
		Parameters.OverrideLevel = World->GetCurrentLevel();
		AQuakeMapActor* MapActor = World->SpawnActor<AQuakeMapActor>(AQuakeMapActor::StaticClass(), Transform,
			Parameters);
		if (!MapActor)
		{
			return nullptr;
		}

		MapActor->SetActorLabel(Map->GetName());
		MapActor->Map = Map;
		SyncMapActor(MapActor);
		return MapActor;
	}

	void SyncMapActor(AQuakeMapActor* MapActor)
	{
		const UQuakeMap* Map = MapActor->Map;
		UWorld* World = MapActor->GetWorld();
		if (!Map || !World)
		{
			return;
		}

		// an entity actor copied in the editor has the same id, the map keeps updating one of them
		TMap<FString, AActor*> Existing;
		for (AActor* Actor : MapActor->GetEntityActors())
		{
			Existing.FindOrAdd(Actor->FindComponentByClass<UQuakeEntityComponent>()->EntityId, Actor);
		}

		for (const FEntitySpec& Spec : MakeSpecs(Map))
		{
			AActor* Actor = nullptr;
			Existing.RemoveAndCopyValue(Spec.Id, Actor);
			// a different class for the entity, e.g. after changing the entity classes, needs a new actor
			if (Actor && Actor->GetClass() != Spec.Class)
			{
				World->EditorDestroyActor(Actor, true);
				Actor = nullptr;
			}

			if (!Actor)
			{
				Actor = SpawnEntityActor(MapActor, Spec);
			}
			if (Actor)
			{
				ApplySpec(Actor, Spec, Map);
			}
		}

		// what is left belongs to entities that are no longer in the map
		for (const auto& [Id, Actor] : Existing)
		{
			World->EditorDestroyActor(Actor, true);
		}
	}
}
