#include "QuakeMapPlacement.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/CollisionProfile.h"
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
	const TCHAR* ExposureId = TEXT("exposure");

	/** What an actor of a placed map is made from: an entity, the sun of the worldspawn or the exposure volume. */
	struct FEntitySpec
	{
		FString Id;
		UClass* Class = nullptr;
		FTransform Transform;
		/** Null for the exposure volume. The sun gets the keys of the worldspawn. */
		const FQuakeMapEntity* Entity = nullptr;
		bool bSun = false;
		UStaticMesh* Mesh = nullptr;
	};

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
		if (Map->Settings.bManualExposure)
		{
			FEntitySpec& Exposure = Specs.AddDefaulted_GetRef();
			Exposure.Id = ExposureId;
			Exposure.Class = APostProcessVolume::StaticClass();
		}
		return Specs;
	}

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

	/** Fixed exposure for the whole level, the brightness of the lights is calibrated to it. */
	void SetupExposure(APostProcessVolume* Volume)
	{
		Volume->Modify();
		Volume->bUnbound = true;
		FPostProcessSettings& Settings = Volume->Settings;
		Settings.bOverride_AutoExposureMethod = true;
		Settings.AutoExposureMethod = AEM_Manual;
		Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		Settings.AutoExposureApplyPhysicalCameraExposure = false;
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = 0.0f;
	}

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
		Keys->ClassName = Spec.Entity && !Spec.bSun ? Spec.Entity->ClassName : FString();
		Keys->Keys = Spec.Entity ? Spec.Entity->Keys : TMap<FString, FString>();

		if (Spec.Id == ExposureId)
		{
			SetupExposure(CastChecked<APostProcessVolume>(Actor));
		}
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

		TMap<FString, AActor*> Existing;
		for (AActor* Actor : MapActor->GetEntityActors())
		{
			Existing.Add(Actor->FindComponentByClass<UQuakeEntityComponent>()->EntityId, Actor);
		}

		for (const FEntitySpec& Spec : MakeSpecs(Map))
		{
			AActor* Actor = nullptr;
			Existing.RemoveAndCopyValue(Spec.Id, Actor);
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

		for (const auto& [Id, Actor] : Existing)
		{
			World->EditorDestroyActor(Actor, true);
		}
	}
}
