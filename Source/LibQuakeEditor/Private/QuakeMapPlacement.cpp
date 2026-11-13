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

	template <typename T>
	T* Spawn(ULevel* Level, const FString& Label, const FTransform& Transform)
	{
		FActorSpawnParameters Parameters;
		Parameters.OverrideLevel = Level;
		T* Actor = Level->GetWorld()->SpawnActor<T>(T::StaticClass(), Transform, Parameters);
		Actor->SetActorLabel(Label);
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
			if (Map->Meshes.IsValidIndex(Entity.Mesh))
			{
				AStaticMeshActor* Actor = Spawn<AStaticMeshActor>(Level, Label, Entity.Transform);
				Actor->GetStaticMeshComponent()->SetStaticMesh(Map->Meshes[Entity.Mesh]);
				Actors.Add(Actor);
			}
			else if (Entity.Mesh == INDEX_NONE && Entity.ClassName.StartsWith(TEXT("light")))
			{
				APointLight* Light = Spawn<APointLight>(Level, Label, Entity.Transform);
				SetupLight(Light, Entity, Map->Settings.Scale);
				Actors.Add(Light);
			}
			else if (Entity.ClassName == TEXT("info_player_start"))
			{
				Actors.Add(Spawn<APlayerStart>(Level, Label, Entity.Transform));
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
