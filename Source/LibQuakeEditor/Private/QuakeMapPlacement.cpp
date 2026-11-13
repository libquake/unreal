#include "QuakeMapPlacement.h"

#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "ImportUtils.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Misc/PackageName.h"
#include "QuakeConversion.h"
#include "QuakeEntityClasses.h"
#include "QuakeEntityComponent.h"
#include "QuakeMap.h"
#include "UObject/Package.h"

namespace
{
	/** "r g b" as 0..1 or 0..255. */
	FLinearColor ParseColor(const FString* Value)
	{
		FVector3f Rgb = LibQuakeEditor::ParseVector(Value);
		if (Rgb.GetMax() > 1.0f)
		{
			Rgb /= 255.0f;
		}
		return FLinearColor(Rgb.X, Rgb.Y, Rgb.Z);
	}

	// A lit surface shows albedo / pi * illuminance in Unreal, the manual exposure of the map divides by 1.2.
	// In Quake a surface at light level 128 shows its texture as it is, and the light tools halve every light
	// (_range 0.5).
	constexpr float ExposureScale = 1.2f;

	/** Illuminance in lux that shows a surface like Quake light level Level * 0.5 does. */
	float ToLux(float Level)
	{
		return Level * 0.5f / 128.0f * UE_PI * ExposureScale;
	}

	/**
	 * Quake's "light" fades linearly, a light of value L lights a surface to level 128 about sqrt(2 pi L) units
	 * away. An inverse square light reaches 128 at the same distance with L * 2 pi² * 1.2 / m² candela, m being
	 * Quake units per meter. It ends where it adds less than 1/40 of the texture brightness.
	 */
	void SetupLight(APointLight* Light, const FQuakeMapEntity& Entity, float Scale)
	{
		const FString* LightKey = Entity.Keys.Find(TEXT("light"));
		const float Value = FMath::Max(LightKey ? FCString::Atof(**LightKey) : 300.0f, 0.0f);
		const float UnitsPerMeter = 100.0f / Scale;

		// the setters ignore static lights once they are registered, the properties are set directly
		UPointLightComponent* Component = Light->PointLightComponent;
		Component->SetMobility(EComponentMobility::Static);
		Component->bUseInverseSquaredFalloff = true;
		Component->IntensityUnits = ELightUnits::Candelas;
		Component->Intensity = Value * 2.0f * UE_PI * UE_PI * ExposureScale / (UnitsPerMeter * UnitsPerMeter);
		Component->AttenuationRadius = (FMath::Sqrt(1.0f + 1024.0f * Value) - 1.0f) * 0.5f * Scale;
		if (const FString* Color = Entity.Keys.Find(TEXT("_color")))
		{
			Component->LightColor = ParseColor(Color).ToFColor(true);
		}
		Component->MarkRenderStateDirty();
	}

	AActor* Spawn(ULevel* Level, UClass* Class, const FString& Label, const FTransform& Transform)
	{
		FActorSpawnParameters Parameters;
		Parameters.OverrideLevel = Level;
		AActor* Actor = Level->GetWorld()->SpawnActor(Class, &Transform, Parameters);
		if (Actor)
		{
			// spawning combines the transform with the one the class template comes with
			Actor->SetActorTransform(Transform);
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

	/**
	 * The sun of the worldspawn ("_sunlight", "_sun_mangle" or "_sunlight_mangle" as yaw and pitch of the direction
	 * the light travels, straight down without one), null if the map has none.
	 */
	ADirectionalLight* SpawnSun(ULevel* Level, const FQuakeMapEntity& Worldspawn)
	{
		const FString* Sunlight = Worldspawn.Keys.Find(TEXT("_sunlight"));
		const float Value = Sunlight ? FCString::Atof(**Sunlight) : 0;
		if (Value <= 0)
		{
			return nullptr;
		}

		const FString* Mangle = Worldspawn.Keys.Find(TEXT("_sun_mangle"));
		Mangle = Mangle ? Mangle : Worldspawn.Keys.Find(TEXT("_sunlight_mangle"));
		FVector Direction(0, 0, -1);
		if (Mangle)
		{
			const FVector3f Angles = LibQuakeEditor::ParseVector(Mangle);
			const float Yaw = FMath::DegreesToRadians(Angles.X);
			const float Pitch = FMath::DegreesToRadians(Angles.Y);
			Direction = LibQuake::ToUnrealPosition(
				FVector3f(FMath::Cos(Yaw) * FMath::Cos(Pitch), FMath::Sin(Yaw) * FMath::Cos(Pitch), FMath::Sin(Pitch)),
				1.0f);
		}

		ADirectionalLight* Sun = Spawn<ADirectionalLight>(Level, TEXT("sun"), FTransform(Direction.Rotation()));
		UDirectionalLightComponent* Component = Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
		Component->SetMobility(EComponentMobility::Static);
		Component->Intensity = ToLux(Value);
		if (const FString* Color = Worldspawn.Keys.Find(TEXT("_sunlight_color")))
		{
			Component->LightColor = ParseColor(Color).ToFColor(true);
		}
		Component->MarkRenderStateDirty();
		return Sun;
	}

	/** Quake's origin is 24 units above the feet, the start is moved so its capsule stands on the same floor. */
	APlayerStart* SpawnPlayerStart(ULevel* Level, const FString& Label, FTransform Transform, float Scale)
	{
		APlayerStart* Start = Spawn<APlayerStart>(Level, Label, Transform);
		const float HalfHeight = Start->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		Transform.AddToTranslation(FVector(0, 0, HalfHeight - 24.0f * Scale));
		Start->SetActorTransform(Transform);
		return Start;
	}

	/** Fixed exposure for the whole level, the brightness of the lights is calibrated to it. */
	APostProcessVolume* SpawnExposure(ULevel* Level)
	{
		APostProcessVolume* Volume = Spawn<APostProcessVolume>(Level, TEXT("exposure"), FTransform::Identity);
		Volume->bUnbound = true;
		FPostProcessSettings& Settings = Volume->Settings;
		Settings.bOverride_AutoExposureMethod = true;
		Settings.AutoExposureMethod = AEM_Manual;
		Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		Settings.AutoExposureApplyPhysicalCameraExposure = false;
		Settings.bOverride_AutoExposureBias = true;
		Settings.AutoExposureBias = 0.0f;
		return Volume;
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
				Actor = SpawnPlayerStart(Level, Label, Entity.Transform, Map->Settings.Scale);
			}
			else
			{
				// unknown point entities keep their place and keys, Blueprints can be mapped to them later
				// an actor spawned without a root has no transform, it gets one once the root is there
				Actor = Spawn<AActor>(Level, Label, Entity.Transform);
				Actor->SetRootComponent(AddComponent<USceneComponent>(Actor, TEXT("Root")));
				Actor->GetRootComponent()->RegisterComponent();
				Actor->SetActorTransform(Entity.Transform);
			}

			if (Entity.ClassName == TEXT("worldspawn"))
			{
				if (ADirectionalLight* Sun = SpawnSun(Level, Entity))
				{
					Actors.Add(Sun);
				}
				if (Map->Settings.bManualExposure)
				{
					Actors.Add(SpawnExposure(Level));
				}
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
