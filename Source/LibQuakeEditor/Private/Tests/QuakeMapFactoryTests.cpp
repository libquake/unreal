#include "ActorFactoryQuakeMap.h"
#include "Editor.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
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
	for (const AActor* Actor : Actors)
	{
		Meshes += Actor->IsA<AStaticMeshActor>();
		Lights += Actor->IsA<APointLight>();
	}
	TestEqual(TEXT("an actor per mesh"), Meshes, Map->Meshes.Num());
	TestTrue(TEXT("lights are spawned"), Lights > 0);
	TestEqual(TEXT("an actor per entity"), Actors.Num(), Map->Entities.Num());
	for (const AActor* Actor : Actors)
	{
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

	for (AActor* Actor : Actors)
	{
		World->EditorDestroyActor(Actor, false);
	}
	return true;
}
