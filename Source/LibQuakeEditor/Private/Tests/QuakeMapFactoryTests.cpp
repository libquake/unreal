#include "ActorFactoryQuakeMap.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "QuakeEntityComponent.h"
#include "QuakeMap.h"
#include "QuakeMapFactory.h"
#include "QuakeMapPlacement.h"
#include "QuakeTriggerProbe.h"
#include "QuakeWad.h"
#include "QuakeWadFactory.h"
#include "UObject/Package.h"

namespace
{
	FString TestFile(const TCHAR* Name)
	{
		const FString Base = IPluginManager::Get().FindPlugin(TEXT("LibQuake"))->GetBaseDir();
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(Base, TEXT("Tests"), TEXT("Data"), Name));
	}

	UQuakeMap* ImportTestMap(const TCHAR* Name, const TCHAR* File = TEXT("sky_alpha.map"))
	{
		bool bCanceled = false;
		NewObject<UQuakeWadFactory>()->FactoryCreateFile(UQuakeWad::StaticClass(),
			CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha")), TEXT("sky_alpha"), RF_Public | RF_Standalone,
			TestFile(TEXT("sky_alpha.wad")), nullptr, GWarn, bCanceled);
		return Cast<UQuakeMap>(NewObject<UQuakeMapFactory>()->FactoryCreateFile(UQuakeMap::StaticClass(),
			CreatePackage(*(FString(TEXT("/Game/LibQuakeTests/")) + Name)), Name, RF_Public | RF_Standalone,
			TestFile(File), nullptr, GWarn, bCanceled));
	}

	AActor* FindEntity(const TArray<AActor*>& Actors, const TCHAR* ClassName)
	{
		AActor* const* Found = Actors.FindByPredicate([ClassName](const AActor* Actor)
		{
			const UQuakeEntityComponent* Entity = Actor->FindComponentByClass<UQuakeEntityComponent>();
			return Entity && Entity->ClassName == ClassName;
		});
		return Found ? *Found : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeMapImportTest, "LibQuake.Editor.ImportMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeMapImportTest::RunTest(const FString& Parameters)
{
	bool bCanceled = false;
	UQuakeWad* Wad = Cast<UQuakeWad>(NewObject<UQuakeWadFactory>()->FactoryCreateFile(UQuakeWad::StaticClass(),
		CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha")), TEXT("sky_alpha"), RF_Public | RF_Standalone,
		TestFile(TEXT("sky_alpha.wad")), nullptr, GWarn, bCanceled));
	if (!TestNotNull(TEXT("wad imports"), Wad))
	{
		return false;
	}

	UQuakeMapFactory* Factory = NewObject<UQuakeMapFactory>();
	UQuakeMap* Map = Cast<UQuakeMap>(Factory->FactoryCreateFile(UQuakeMap::StaticClass(),
		CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha_map")), TEXT("sky_alpha_map"), RF_Public | RF_Standalone,
		TestFile(TEXT("sky_alpha.map")), nullptr, GWarn, bCanceled));
	if (!TestNotNull(TEXT("map imports"), Map))
	{
		return false;
	}

	TestTrue(TEXT("worldspawn and the fence have meshes"), Map->Meshes.Num() >= 2);
	for (const UStaticMesh* Mesh : Map->Meshes)
	{
		TestTrue(TEXT("mesh has convex collision"), Mesh->GetBodySetup()->AggGeom.ConvexElems.Num() > 0);
		TestEqual(TEXT("lightmap uvs in channel 1"), Mesh->GetLightMapCoordinateIndex(), 1);
		for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
		{
			TestNotNull(TEXT("every slot has a material"), Material.MaterialInterface.Get());
		}
	}

	bool bSkyFound = false;
	for (const UStaticMesh* Mesh : Map->Meshes)
	{
		const TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
		for (int32 Section = 0; Section < Materials.Num(); Section++)
		{
			const UMaterialInterface* Material = Materials[Section].MaterialInterface;
			if (!Material || Material->GetMaterial()->GetName() != TEXT("M_QuakeSky"))
			{
				continue;
			}
			bSkyFound = true;
			TestFalse(TEXT("sky casts no shadow"), Mesh->GetSectionInfoMap().Get(0, Section).bCastShadow);
		}
	}
	TestTrue(TEXT("sky1 gets the sky material"), bSkyFound);

	const TArray<TObjectPtr<UStaticMesh>> Before = Map->Meshes;
	TestEqual(TEXT("reimport succeeds"), Factory->Reimport(Map), EReimportResult::Succeeded);
	TestTrue(TEXT("reimport keeps the meshes"), Map->Meshes == Before);
	return true;
}

// Making the level goes through Unreal's Save As dialog, which a headless run cannot answer. This covers what is
// handed to it, and that dragging a map without a level is refused.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeMapSpawnTest, "LibQuake.Editor.SpawnMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeMapSpawnTest::RunTest(const FString& Parameters)
{
	UQuakeMap* Map = ImportTestMap(TEXT("sky_alpha_spawn"));
	if (!TestNotNull(TEXT("map imports"), Map))
	{
		return false;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	const TArray<AActor*> Actors = LibQuakeEditor::SpawnMapActors(Map, World->GetCurrentLevel());
	int32 Meshes = 0;
	int32 Lights = 0;
	int32 Exposures = 0;
	for (const AActor* Actor : Actors)
	{
		Meshes += Actor->IsA<AStaticMeshActor>();
		Lights += Actor->IsA<APointLight>();
		Exposures += Actor->IsA<APostProcessVolume>();
	}
	TestEqual(TEXT("an actor per mesh"), Meshes, Map->Meshes.Num());
	TestTrue(TEXT("lights are spawned"), Lights > 0);
	TestEqual(TEXT("one exposure volume"), Exposures, 1);
	TestEqual(TEXT("an actor per entity"), Actors.Num() - Exposures, Map->Entities.Num());
	for (const AActor* Actor : Actors)
	{
		if (Actor->IsA<APostProcessVolume>())
		{
			continue;
		}
		const UQuakeEntityComponent* Entity = Actor->FindComponentByClass<UQuakeEntityComponent>();
		if (TestNotNull(TEXT("every actor carries its keys"), Entity))
		{
			TestEqual(TEXT("classname is among the keys"), Entity->GetKey(TEXT("classname")), Entity->ClassName);
		}
	}

	const FQuakeMapEntity* Light = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("light"); });
	const AActor* LightActor = *Actors.FindByPredicate([](const AActor* Actor) { return Actor->IsA<APointLight>(); });
	TestEqual(TEXT("light sits at its origin"), LightActor->GetActorLocation(), Light->Transform.GetLocation());

	// "light" "300": the static light keeps the values even though Unreal's setters ignore static lights
	const UPointLightComponent* LightComponent = Cast<APointLight>(LightActor)->PointLightComponent;
	TestFalse(TEXT("fades like quake, not by the inverse square"), LightComponent->bUseInverseSquaredFalloff);
	TestEqual(TEXT("level 150 at the center after quake's range scale"), (double)LightComponent->Intensity,
		150.0 / 128.0 * UE_DOUBLE_PI * 1.2, 0.001);
	TestEqual(TEXT("radius is the light value"), (double)LightComponent->AttenuationRadius,
		300.0 * Map->Settings.Scale, 0.01);

	const FQuakeMapEntity* Start = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("info_player_start"); });
	const APlayerStart* StartActor = Cast<APlayerStart>(
		*Actors.FindByPredicate([](const AActor* Actor) { return Actor->IsA<APlayerStart>(); }));
	const double Feet = StartActor->GetActorLocation().Z - StartActor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	TestEqual(TEXT("player start stands where the Quake player's feet are"), Feet,
		Start->Transform.GetLocation().Z - 24.0 * Map->Settings.Scale, 0.01);

	for (AActor* Actor : Actors)
	{
		World->EditorDestroyActor(Actor, false);
	}

	FText Error;
	TestFalse(TEXT("dragging is refused before the level exists"),
		GetMutableDefault<UActorFactoryQuakeMap>()->CanCreateActorFrom(FAssetData(Map), Error));
	TestFalse(TEXT("with a hint"), Error.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeTriggerTest, "LibQuake.Editor.Triggers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeTriggerTest::RunTest(const FString& Parameters)
{
	UQuakeMap* Map = ImportTestMap(TEXT("trigger_map"), TEXT("trigger.map"));
	if (!TestNotNull(TEXT("map imports"), Map))
	{
		return false;
	}
	const FQuakeMapEntity* TriggerEntity = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("trigger_once"); });
	if (!TestNotNull(TEXT("trigger is kept"), TriggerEntity))
	{
		return false;
	}
	TestTrue(TEXT("trigger has a hull mesh"), Map->Meshes.IsValidIndex(TriggerEntity->Mesh));
	TestTrue(TEXT("and is invisible"), TriggerEntity->bInvisible);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	const TArray<AActor*> Actors = LibQuakeEditor::SpawnMapActors(Map, World->GetCurrentLevel());
	AActor* Trigger = FindEntity(Actors, TEXT("trigger_once"));
	AActor* Wall = FindEntity(Actors, TEXT("func_wall"));
	AActor* Null = FindEntity(Actors, TEXT("info_null"));
	if (!TestNotNull(TEXT("trigger"), Trigger) || !TestNotNull(TEXT("wall"), Wall) || !TestNotNull(TEXT("null"), Null))
	{
		return false;
	}

	const UStaticMeshComponent* Shape = Trigger->FindComponentByClass<UStaticMeshComponent>();
	TestFalse(TEXT("trigger is not drawn"), Shape->IsVisible());
	TestEqual(TEXT("trigger only overlaps"), Shape->GetCollisionProfileName(), FName(TEXT("Trigger")));
	TestTrue(TEXT("trigger reports overlaps"), Shape->GetGenerateOverlapEvents());
	TestTrue(TEXT("the wall stays visible"), Wall->FindComponentByClass<UStaticMeshComponent>()->IsVisible());

	UQuakeTriggerProbe* WallProbe = NewObject<UQuakeTriggerProbe>();
	UQuakeTriggerProbe* NullProbe = NewObject<UQuakeTriggerProbe>();
	Wall->FindComponentByClass<UQuakeEntityComponent>()->OnTriggered.AddDynamic(WallProbe,
		&UQuakeTriggerProbe::OnTriggered);
	Null->FindComponentByClass<UQuakeEntityComponent>()->OnTriggered.AddDynamic(NullProbe,
		&UQuakeTriggerProbe::OnTriggered);
	Trigger->FindComponentByClass<UQuakeEntityComponent>()->Fire(nullptr);
	TestEqual(TEXT("the wall is triggered"), WallProbe->Count, 1);
	TestEqual(TEXT("the info_null is triggered too"), NullProbe->Count, 1);
	TestEqual(TEXT("info_null keeps its origin"), Null->GetActorLocation(),
		FVector(-96 * Map->Settings.Scale, 0, 32 * Map->Settings.Scale));

	// "90 -45 0": the sun shines towards Quake's +y at 45 degrees, which is Unreal's -y
	const AActor* const* Sun = Actors.FindByPredicate([](const AActor* Actor) { return Actor->IsA<ADirectionalLight>(); });
	if (TestNotNull(TEXT("worldspawn sun"), Sun))
	{
		const FVector Direction = (*Sun)->GetActorForwardVector();
		TestTrue(*FString::Printf(TEXT("sun points down and to -y, not %s"), *Direction.ToString()),
			Direction.Equals(FVector(0, -1, -1).GetSafeNormal(), 0.01));
		// "_sunlight" "200" is level 100 after Quake's range scale of 0.5
		TestEqual(TEXT("sun brightness"), (double)Cast<ADirectionalLight>(*Sun)->GetLightComponent()->Intensity,
			100.0 / 128.0 * UE_DOUBLE_PI * 1.2, 0.001);
	}

	for (AActor* Actor : Actors)
	{
		World->EditorDestroyActor(Actor, false);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeBspImportTest, "LibQuake.Editor.ImportBsp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeBspImportTest::RunTest(const FString& Parameters)
{
	bool bCanceled = false;
	UQuakeMap* Map = Cast<UQuakeMap>(NewObject<UQuakeMapFactory>()->FactoryCreateFile(UQuakeMap::StaticClass(),
		CreatePackage(TEXT("/Game/LibQuakeTests/lightstyles")), TEXT("lightstyles"), RF_Public | RF_Standalone,
		TestFile(TEXT("lightstyles.bsp")), nullptr, GWarn, bCanceled));
	if (!TestNotNull(TEXT("bsp imports"), Map))
	{
		return false;
	}

	TestTrue(TEXT("bsp has meshes"), Map->Meshes.Num() > 0);
	TestTrue(TEXT("bsp has entities"), Map->Entities.Num() > 1);
	int32 Textured = 0;
	for (const UStaticMesh* Mesh : Map->Meshes)
	{
		TestEqual(TEXT("a bsp mesh is its own collision"), Mesh->GetBodySetup()->CollisionTraceFlag.GetValue(),
			CTF_UseComplexAsSimple);
		for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
		{
			UTexture* Albedo = nullptr;
			if (Material.MaterialInterface && Material.MaterialInterface->GetTextureParameterValue(
				FHashedMaterialParameterInfo(TEXT("Albedo")), Albedo) && Albedo)
			{
				TestTrue(TEXT("textures come from the bsp"),
					Albedo->GetPathName().StartsWith(TEXT("/Game/LibQuakeTests/lightstyles_Generated/T_")));
				Textured++;
			}
		}
	}
	TestTrue(TEXT("materials have the bsp textures"), Textured > 0);
	return true;
}
