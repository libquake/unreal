#include "QuakeMapPlacement.h"

#include "BSPOps.h"
#include "Components/BrushComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Editor/EditorEngine.h"
#include "Engine/Polys.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/PhysicsVolume.h"
#include "LibQuakeSettings.h"
#include "Model.h"
#include "QuakeNameTable.h"
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

	/** What an actor of a placed map is made from: an entity, the sun of the worldspawn or a liquid brush. */
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

		/** A liquid brush of the entity, its volume. */
		const FQuakeLiquid* Liquid = nullptr;
	};

	/** The volume class for a liquid texture, null for none. */
	UClass* LiquidClass(const FString& Texture)
	{
		const TSoftClassPtr<APhysicsVolume>* Entry =
			LibQuake::FindByName(GetDefault<ULibQuakeSettings>()->LiquidClasses, Texture);
		return Entry && !Entry->IsNull() ? Entry->LoadSynchronous() : nullptr;
	}

	/** Every actor the map should have, in map order. Entities mapped to no class are left out. */
	TArray<FEntitySpec> MakeSpecs(const UQuakeMap* Map)
	{
		TArray<FEntitySpec> Specs;
		TMap<FString, int32> ClassCount;
		for (const FQuakeMapEntity& Entity : Map->Entities)
		{
			// counted before skipping, so the ids of the other entities stay the same when an entry is added
			const FString Id =
				FString::Printf(TEXT("%s_%d"), *Entity.ClassName, ClassCount.FindOrAdd(Entity.ClassName)++);
			UStaticMesh* Mesh = Map->Meshes.IsValidIndex(Entity.Mesh) ? Map->Meshes[Entity.Mesh].Get() : nullptr;

			UClass* Class = nullptr;
			if (!UQuakeEntityClasses::Resolve(Map->Settings.EntityClasses, Entity.ClassName, Class))
			{
				Class = Mesh ? AStaticMeshActor::StaticClass() : AQuakePointEntity::StaticClass();
			}

			if (Class)
			{
				FEntitySpec& Spec = Specs.AddDefaulted_GetRef();
				Spec.Id = Id;
				Spec.Class = Class;
				Spec.Transform = Entity.Transform;
				Spec.Entity = &Entity;
				Spec.Mesh = Mesh;
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

			for (int32 Index = 0; Index < Entity.Liquids.Num(); Index++)
			{
				const FQuakeLiquid& Liquid = Entity.Liquids[Index];
				if (UClass* VolumeClass = LiquidClass(Liquid.Texture))
				{
					FEntitySpec& Spec = Specs.AddDefaulted_GetRef();
					Spec.Id = FString::Printf(TEXT("%s_liquid_%d"), *Id, Index);
					Spec.Class = VolumeClass;
					Spec.Transform = Entity.Transform;
					Spec.Entity = &Entity;
					Spec.Liquid = &Liquid;
				}
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

	/** Gives a volume the convex shape of a liquid brush, rebuilt from scratch on every update. */
	void SetupLiquidBrush(AVolume* Volume, const FQuakeLiquid& Liquid)
	{
		Volume->PreEditChange(nullptr);

		UModel* Brush = NewObject<UModel>(Volume, NAME_None, RF_Transactional);
		Brush->Initialize(nullptr, true);
		Brush->Polys = NewObject<UPolys>(Brush, NAME_None, RF_Transactional);
		for (int32 i = 0; i + 2 < Liquid.Indices.Num(); i += 3)
		{
			// brush polygons wind the other way round than mesh triangles, their normals point outside
			FPoly Poly;
			Poly.Init();
			for (int32 Corner = 2; Corner >= 0; Corner--)
			{
				Poly.Vertices.Emplace(FVector3f(Liquid.Points[Liquid.Indices[i + Corner]]));
			}
			Poly.Base = Poly.Vertices[0];
			if (Poly.Finalize(Volume, 1) == 0)
			{
				Brush->Polys->Element.Add(Poly);
			}
		}

		// the hull comes as triangles, a face of the brush is one polygon
		GEditor->bspMergeCoplanars(Brush, false, true);
		Brush->Linked = 1;
		FBSPOps::bspValidateBrush(Brush, false, false);
		Brush->BuildBound();

		Volume->Brush = Brush;
		Volume->GetBrushComponent()->Brush = Brush;
		FBSPOps::csgPrepMovingBrush(Volume);
		Volume->PostEditChange();
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
		Keys->ClassName = Spec.bSun || Spec.Liquid ? FString() : Spec.Entity->ClassName;
		Keys->Keys = Spec.Entity->Keys;
		if (Spec.Liquid)
		{
			Keys->Keys = {
				{TEXT("texture"), Spec.Liquid->Texture},
				{TEXT("liquid"), LibQuake::LiquidKind(Spec.Liquid->Texture)},
			};
			SetupLiquidBrush(CastChecked<AVolume>(Actor), *Spec.Liquid);
		}

		// the plugin's lights follow the map, the lights of own classes keep what their Blueprint says
		if (Actor->IsA<AQuakePointLight>() || Actor->IsA<AQuakeSun>())
		{
			TInlineComponentArray<ULightComponent*> Lights(Actor);
			for (ULightComponent* Light : Lights)
			{
				Light->Modify();
				Light->SetMobility(Map->Settings.LightMobility);
			}
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
	APostProcessVolume* AddQuakeExposure(UWorld* World)
	{
		FActorSpawnParameters Parameters;
		Parameters.OverrideLevel = World->GetCurrentLevel();
		APostProcessVolume* Volume = World->SpawnActor<APostProcessVolume>(Parameters);
		if (!Volume)
		{
			return nullptr;
		}

		Volume->SetActorLabel(TEXT("QuakeExposure"));
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
