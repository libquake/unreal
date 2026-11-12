#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "QuakeWad.h"
#include "QuakeWadFactory.h"
#include "UObject/Package.h"

namespace
{
	FString TestWad()
	{
		const FString Base = IPluginManager::Get().FindPlugin(TEXT("LibQuake"))->GetBaseDir();
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(Base, TEXT("Tests"), TEXT("Data"), TEXT("sky_alpha.wad")));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeWadImportTest, "LibQuake.Editor.ImportWad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeWadImportTest::RunTest(const FString& Parameters)
{
	UPackage* Package = CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha"));
	UQuakeWadFactory* Factory = NewObject<UQuakeWadFactory>();
	bool bCanceled = false;
	UQuakeWad* Wad = Cast<UQuakeWad>(Factory->FactoryCreateFile(UQuakeWad::StaticClass(), Package,
		TEXT("sky_alpha"), RF_Public | RF_Standalone, TestWad(), nullptr, GWarn, bCanceled));
	if (!TestNotNull(TEXT("wad imports"), Wad))
	{
		return false;
	}

	TestTrue(TEXT("wad has textures"), Wad->Textures.Num() > 0);
	for (const FQuakeWadTexture& Entry : Wad->Textures)
	{
		if (!TestNotNull(*FString::Printf(TEXT("%s has a texture"), *Entry.Name), Entry.Texture.Get()))
		{
			continue;
		}
		TestEqual(TEXT("point filtered"), Entry.Texture->Filter.GetValue(), TF_Nearest);
		TestTrue(TEXT("lives in sky_alpha_Textures"),
			Entry.Texture->GetPathName().StartsWith(TEXT("/Game/LibQuakeTests/sky_alpha_Textures/")));
	}

	const FQuakeWadTexture* Sky = Wad->Textures.FindByPredicate(
		[](const FQuakeWadTexture& Entry) { return Entry.SkyFront != nullptr; });
	TestNotNull(TEXT("sky has a cloud layer"), Sky);
	const FQuakeWadTexture* Fence = Wad->Textures.FindByPredicate(
		[](const FQuakeWadTexture& Entry) { return Entry.bAlpha; });
	if (TestNotNull(TEXT("fence texture is alpha"), Fence))
	{
		TestEqual(TEXT("found by its quake name"), Wad->Find(Fence->Name.ToUpper()), Fence);
	}

	TArray<UTexture2D*> Before;
	for (const FQuakeWadTexture& Entry : Wad->Textures)
	{
		Before.Add(Entry.Texture);
	}
	TestEqual(TEXT("reimport succeeds"), Factory->Reimport(Wad), EReimportResult::Succeeded);
	for (int32 i = 0; i < Wad->Textures.Num() && i < Before.Num(); i++)
	{
		TestTrue(TEXT("reimport keeps the texture objects"), Wad->Textures[i].Texture == Before[i]);
	}

	const int32 Count = Wad->Textures.Num();
	Wad->AssetImportData->UpdateFilenameOnly(TestWad() + TEXT(".missing"));
	AddExpectedError(TEXT("sky_alpha.wad.missing"), EAutomationExpectedErrorFlags::Contains, 0);
	TestEqual(TEXT("missing file fails"), Factory->Reimport(Wad), EReimportResult::Failed);
	TestEqual(TEXT("and leaves the wad alone"), Wad->Textures.Num(), Count);
	return true;
}
