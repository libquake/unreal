#include "QuakeMapPlacement.h"

#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "ImportUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Misc/PackageName.h"
#include "QuakeEntityClasses.h"
#include "QuakeEntityComponent.h"
#include "QuakeMap.h"
#include "UObject/Package.h"

namespace
{
	/** Quake's "light" is brightness and radius at once, it falls off linearly like in Quake. */
	void SetupLight(APointLight* Light, const FQuakeMapEntity& Entity, float Scale)
	{
		UPointLightComponent* Component = Light->PointLightComponent;
		Component->SetMobility(EComponentMobility::Static);

		const FString* LightKey = Entity.Keys.Find(TEXT("light"));
		const float Value = LightKey ? FCString::Atof(**LightKey) : 300.0f;
		Component->bUseInverseSquaredFalloff = false;
		Component->LightFalloffExponent = 1.0f;
		Component->SetAttenuationRadius(Value * Scale);
		Component->SetIntensity(Value / 300.0f * 5.0f);

		if (const FString* Color = Entity.Keys.Find(TEXT("_color")))
		{
			FVector3f Rgb = LibQuakeEditor::ParseVector(Color);
			if (Rgb.GetMax() > 1.0f)
			{
				Rgb /= 255.0f;
			}
			Component->SetLightColor(FLinearColor(Rgb.X, Rgb.Y, Rgb.Z));
		}
	}

	AActor* Spawn(ULevel* Level, UClass* Class, const FString& Label, const FTransform& Transform)
	{
		FActorSpawnParameters Parameters;
		Parameters.OverrideLevel = Level;
		AActor* Actor = Level->GetWorld()->SpawnActor(Class, &Transform, Parameters);
		if (Actor)
		{
			Actor->SetActorLabel(Label);
		}
		return Actor;
	}

	template <typename T>
	T* Spawn(ULevel* Level, const FString& Label, const FTransform& Transform)
	{
		return Cast<T>(Spawn(Level, T::StaticClass(), Label, Transform));
	}

	template <typename T>
	T* AddComponent(AActor* Actor, const TCHAR* Name)
	{
		T* Component = NewObject<T>(Actor, Name);
		Actor->AddInstanceComponent(Component);
		return Component;
	}

	UClass* MappedClass(const UQuakeMap* Map, const FString& ClassName)
	{
		const UQuakeEntityClasses* Mapping = Map->Settings.EntityClasses;
		const TSoftClassPtr<AActor>* Class = Mapping ? Mapping->Classes.Find(ClassName) : nullptr;
		return Class ? Class->LoadSynchronous() : nullptr;
	}

	/** Brushes without visible faces stay hidden, trigger_* ones only overlap, the others block. */
	void SetupMesh(UStaticMeshComponent* Component, UStaticMesh* Mesh, const FQuakeMapEntity& Entity)
	{
		Component->SetStaticMesh(Mesh);
		if (!Entity.bInvisible)
		{
			return;
		}
		Component->SetVisibility(false);
		Component->SetHiddenInGame(true);
		Component->SetCastShadow(false);
		if (Entity.ClassName.StartsWith(TEXT("trigger_")))
		{
			Component->SetCollisionProfileName(TEXT("Trigger"));
			Component->SetGenerateOverlapEvents(true);
		}
	}

	/** An actor of a mapped class, with the brush mesh as an extra component if the entity has one. */
	AActor* SpawnMapped(ULevel* Level, UClass* Class, const FString& Label, const FQuakeMapEntity& Entity,
		UStaticMesh* Mesh)
	{
		AActor* Actor = Spawn(Level, Class, Label, Entity.Transform);
		if (Actor && Mesh)
		{
			UStaticMeshComponent* Component = AddComponent<UStaticMeshComponent>(Actor, TEXT("QuakeMesh"));
			if (USceneComponent* Root = Actor->GetRootComponent())
			{
				Component->SetupAttachment(Root);
			}
			else
			{
				Actor->SetRootComponent(Component);
			}
			SetupMesh(Component, Mesh, Entity);
			Component->RegisterComponent();
		}
		return Actor;
	}

	FString LevelPackageName(const UQuakeMap* Map)
	{
		return FPackageName::GetLongPackagePath(Map->GetOutermost()->GetName()) / (Map->GetName() + TEXT("_Generated")) /
			(TEXT("L_") + Map->GetName());
	}
}

namespace LibQuakeEditor
{
	TArray<AActor*> SpawnMapActors(const UQuakeMap* Map, ULevel* Level)
	{
		TArray<AActor*> Actors;
		TMap<FString, int32> ClassCount;
		for (const FQuakeMapEntity& Entity : Map->Entities)
		{
			const FString Label = FString::Printf(TEXT("%s_%d"), *Entity.ClassName,
				ClassCount.FindOrAdd(Entity.ClassName)++);
			UStaticMesh* Mesh = Map->Meshes.IsValidIndex(Entity.Mesh) ? Map->Meshes[Entity.Mesh].Get() : nullptr;

			AActor* Actor = nullptr;
			if (UClass* Class = MappedClass(Map, Entity.ClassName))
			{
				Actor = SpawnMapped(Level, Class, Label, Entity, Mesh);
			}
			else if (Mesh)
			{
				AStaticMeshActor* MeshActor = Spawn<AStaticMeshActor>(Level, Label, Entity.Transform);
				SetupMesh(MeshActor->GetStaticMeshComponent(), Mesh, Entity);
				Actor = MeshActor;
			}
			else if (Entity.ClassName.StartsWith(TEXT("light")))
			{
				APointLight* Light = Spawn<APointLight>(Level, Label, Entity.Transform);
				SetupLight(Light, Entity, Map->Settings.Scale);
				Actor = Light;
			}
			else if (Entity.ClassName == TEXT("info_player_start"))
			{
				Actor = Spawn<APlayerStart>(Level, Label, Entity.Transform);
			}
			else
			{
				// unknown point entities keep their place and keys, Blueprints can be mapped to them later
				// an actor spawned without a root has no transform, it gets one before it is placed
				Actor = Spawn<AActor>(Level, Label, Entity.Transform);
				Actor->SetRootComponent(AddComponent<USceneComponent>(Actor, TEXT("Root")));
				Actor->GetRootComponent()->RegisterComponent();
				Actor->SetActorTransform(Entity.Transform);
			}

			if (Actor)
			{
				UQuakeEntityComponent* Keys = AddComponent<UQuakeEntityComponent>(Actor, TEXT("QuakeEntity"));
				Keys->ClassName = Entity.ClassName;
				Keys->Keys = Entity.Keys;
				Keys->RegisterComponent();
				Actors.Add(Actor);
			}
		}
		return Actors;
	}

	bool HasLevel(const UQuakeMap* Map)
	{
		return !Map->Level.IsNull() && FPackageName::DoesPackageExist(Map->Level.GetLongPackageName());
	}

	AActor* PlaceMap(UQuakeMap* Map, UWorld* World, const FTransform& Transform)
	{
		ULevelInstanceSubsystem* LevelInstances = World->GetSubsystem<ULevelInstanceSubsystem>();
		if (!LevelInstances)
		{
			return nullptr;
		}

		if (HasLevel(Map))
		{
			FActorSpawnParameters Parameters;
			Parameters.OverrideLevel = World->GetCurrentLevel();
			ALevelInstance* Instance = World->SpawnActor<ALevelInstance>(ALevelInstance::StaticClass(), Transform,
				Parameters);
			Instance->SetWorldAsset(Map->Level);
			Instance->LoadLevelInstance();
			return Instance;
		}

		const TArray<AActor*> Actors = SpawnMapActors(Map, World->GetCurrentLevel());
		if (Actors.IsEmpty())
		{
			return nullptr;
		}

		// the same settings Unreal's own "Create Level Instance" uses, without its dialogs
		FNewLevelInstanceParams Parameters;
		Parameters.Type = ELevelInstanceCreationType::LevelInstance;
		Parameters.PivotType = ELevelInstancePivotType::WorldOrigin;
		Parameters.LevelPackageName = LevelPackageName(Map);
		Parameters.HideCreationType();
		Parameters.SetExternalActors(false);
		Parameters.SetForceExternalActors(World->IsPartitionedWorld());

		ILevelInstanceInterface* Instance = LevelInstances->CreateLevelInstanceFrom(Actors, Parameters);
		if (!Instance)
		{
			ReportError(Map->GetName(), TEXT("Unreal could not create the level, see the output log"));
			NotifyIfReported();
			return nullptr;
		}

		AActor* InstanceActor = CastChecked<AActor>(Instance);
		InstanceActor->SetActorTransform(Transform);
		Map->Modify();
		Map->Level = Instance->GetWorldAsset();
		Map->MarkPackageDirty();
		return InstanceActor;
	}
}
