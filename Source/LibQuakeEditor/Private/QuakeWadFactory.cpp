#include "QuakeWadFactory.h"

#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "ImportUtils.h"
#include "LibQuakeCore.h"
#include "Misc/PackageName.h"
#include "QuakeWad.h"
#include "UObject/Package.h"

UQuakeWadFactory::UQuakeWadFactory()
{
	Formats.Add(TEXT("wad;Quake WAD"));
	SupportedClass = UQuakeWad::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UQuakeWadFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UQuakeWad* Wad = FindObject<UQuakeWad>(InParent, *InName.ToString());
	if (!Wad)
	{
		Wad = NewObject<UQuakeWad>(InParent, InClass, InName, Flags | RF_Transactional);
	}
	return ImportWad(Wad, Filename) ? Wad : nullptr;
}

bool UQuakeWadFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	const UQuakeWad* Wad = Cast<UQuakeWad>(Obj);
	if (!Wad || !Wad->AssetImportData)
	{
		return false;
	}
	Wad->AssetImportData->ExtractFilenames(OutFilenames);
	return true;
}

void UQuakeWadFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UQuakeWad* Wad = Cast<UQuakeWad>(Obj);
	if (Wad && Wad->AssetImportData && NewReimportPaths.Num() > 0)
	{
		Wad->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UQuakeWadFactory::Reimport(UObject* Obj)
{
	UQuakeWad* Wad = Cast<UQuakeWad>(Obj);
	if (!Wad || !Wad->AssetImportData)
	{
		return EReimportResult::Failed;
	}
	return ImportWad(Wad, Wad->AssetImportData->GetFirstFilename()) ? EReimportResult::Succeeded
		: EReimportResult::Failed;
}

int32 UQuakeWadFactory::GetPriority() const
{
	return ImportPriority;
}

bool UQuakeWadFactory::ImportWad(UQuakeWad* Wad, const FString& Filename)
{
	TArray<LibQuake::FWadTexture> Loaded;
	FString Error;
	if (!LibQuake::LoadWad(Filename, Loaded, Error))
	{
		LibQuakeEditor::ReportError(Filename, Error);
		LibQuakeEditor::NotifyIfReported();
		return false;
	}

	Wad->Modify();
	const FString Folder = FPackageName::GetLongPackagePath(Wad->GetOutermost()->GetName()) /
		(Wad->GetName() + TEXT("_Textures"));
	TSet<FString> Used;
	Wad->Textures.Reset(Loaded.Num());
	for (const LibQuake::FWadTexture& Source : Loaded)
	{
		const FString Name = LibQuakeEditor::UniqueAssetName(Source.Name, Used);
		const auto Write = [&](const FString& AssetName, const TArray<uint8>& Rgba)
		{
			return LibQuakeEditor::WriteTexture(Folder, LibQuakeEditor::UniqueAssetName(AssetName, Used), Source.Width,
				Source.Height, Rgba);
		};

		FQuakeWadTexture& Entry = Wad->Textures.AddDefaulted_GetRef();
		Entry.Name = Source.Name;
		Entry.bAlpha = Source.bAlpha;
		Entry.Texture = LibQuakeEditor::WriteTexture(Folder, Name, Source.Width, Source.Height, Source.Rgba);
		if (!Source.Emission.IsEmpty())
		{
			Entry.Emission = Write(Name + TEXT("_emission"), Source.Emission);
		}
		if (!Source.SkyFront.IsEmpty())
		{
			Entry.SkyFront = Write(Name + TEXT("_front"), Source.SkyFront);
		}
	}

	Wad->AssetImportData->Update(Filename);
	Wad->MarkPackageDirty();
	return true;
}
