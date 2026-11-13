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
#include "QuakeMap.h"
#include "QuakeMapFactory.h"
#include "QuakeMapPlacement.h"
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

	UQuakeMap* ImportTestMap(const TCHAR* Name)
	{
		bool bCanceled = false;
		NewObject<UQuakeWadFactory>()->FactoryCreateFile(UQuakeWad::StaticClass(),
			CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha")), TEXT("sky_alpha"), RF_Public | RF_Standalone,
			TestFile(TEXT("sky_alpha.wad")), nullptr, GWarn, bCanceled);
		return Cast<UQuakeMap>(NewObject<UQuakeMapFactory>()->FactoryCreateFile(UQuakeMap::StaticClass(),
			CreatePackage(*(FString(TEXT("/Game/LibQuakeTests/")) + Name)), Name, RF_Public | RF_Standalone,
			TestFile(TEXT("sky_alpha.map")), nullptr, GWarn, bCanceled));
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
