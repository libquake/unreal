#include "ImportUtils.h"
#include "Misc/AutomationTest.h"
#include "Misc/PackageName.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeAssetNameTest, "LibQuake.Editor.AssetNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeAssetNameTest::RunTest(const FString& Parameters)
{
	TSet<FString> Used;
	for (const TCHAR* QuakeName : {TEXT("*lava1"), TEXT("{fence"), TEXT("+0button"), TEXT("+abutton"), TEXT("sky1")})
	{
		const FString Name = LibQuakeEditor::UniqueAssetName(QuakeName, Used);
		FText Reason;
		TestTrue(*FString::Printf(TEXT("'%s' gives the valid package name '%s'"), QuakeName, *Name),
			FPackageName::IsValidLongPackageName(TEXT("/Game/Textures/") + Name, false, &Reason));
	}

	TestNotEqual(TEXT("names that sanitize the same stay apart"), LibQuakeEditor::UniqueAssetName(TEXT("*water"), Used),
		LibQuakeEditor::UniqueAssetName(TEXT("{water"), Used));
	return true;
}
