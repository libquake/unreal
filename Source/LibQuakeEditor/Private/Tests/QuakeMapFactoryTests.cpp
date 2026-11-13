#include "Engine/StaticMesh.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "QuakeMap.h"
#include "QuakeMapFactory.h"
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
