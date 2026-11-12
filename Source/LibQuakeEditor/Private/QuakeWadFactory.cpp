#include "QuakeWadFactory.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "LibQuakeCore.h"
#include "LibQuakeEditorModule.h"
#include "Logging/MessageLog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "QuakeWad.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "QuakeWadFactory"

namespace
{
	/**
	 * Quake names may contain characters assets cannot have ('*', '{', '+'). Collisions after replacing them get a
	 * number, in the wad's (sorted) order, so a reimport ends up with the same names.
	 */
	FString UniqueAssetName(const FString& QuakeName, TSet<FString>& Used)
	{
		const FString Base = ObjectTools::SanitizeObjectName(QuakeName);
		FString Name = Base;
		for (int32 Suffix = 1; Used.Contains(Name); Suffix++)
		{
			Name = FString::Printf(TEXT("%s_%d"), *Base, Suffix);
		}
		Used.Add(Name);
		return Name;
	}

	UPackage* FindOrCreatePackage(const FString& PackageName)
	{
		if (UPackage* Loaded = FindPackage(nullptr, *PackageName))
		{
			return Loaded;
		}
		if (FPackageName::DoesPackageExist(PackageName))
		{
			return LoadPackage(nullptr, *PackageName, LOAD_None);
		}
		return CreatePackage(*PackageName);
	}

	/** Point filtered and uncompressed, so the pixels stay as they are in the wad. */
	UTexture2D* WriteTexture(const FString& Folder, const FString& Name, int32 Width, int32 Height,
		const TArray<uint8>& Rgba)
	{
		UPackage* Package = FindOrCreatePackage(Folder / Name);
		UTexture2D* Texture = FindObject<UTexture2D>(Package, *Name);
		const bool bCreated = Texture == nullptr;
		if (bCreated)
		{
			Texture = NewObject<UTexture2D>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
		}
		else
		{
			Texture->PreEditChange(nullptr);
		}

		TArray<uint8> Bgra = Rgba;
		for (int32 i = 0; i + 3 < Bgra.Num(); i += 4)
		{
			Swap(Bgra[i], Bgra[i + 2]);
		}
		Texture->Source.Init(Width, Height, 1, 1, TSF_BGRA8, Bgra.GetData());
		Texture->SRGB = true;
		Texture->Filter = TF_Nearest;
		Texture->CompressionNone = true;
		Texture->PostEditChange();

		if (bCreated)
		{
			FAssetRegistryModule::AssetCreated(Texture);
		}
		Package->MarkPackageDirty();
		return Texture;
	}
}

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
		FMessageLog(LibQuakeEditor::LogName).Error(FText::Format(LOCTEXT("WadFailed", "{0}: {1}"),
			FText::FromString(FPaths::GetCleanFilename(Filename)), FText::FromString(Error)));
		FMessageLog(LibQuakeEditor::LogName).Notify(LOCTEXT("ImportFailed", "Quake import failed"));
		return false;
	}

	Wad->Modify();
	const FString Folder = FPackageName::GetLongPackagePath(Wad->GetOutermost()->GetName()) /
		(Wad->GetName() + TEXT("_Textures"));
	TSet<FString> Used;
	Wad->Textures.Reset(Loaded.Num());
	for (const LibQuake::FWadTexture& Source : Loaded)
	{
		const FString Name = UniqueAssetName(Source.Name, Used);
		FQuakeWadTexture& Entry = Wad->Textures.AddDefaulted_GetRef();
		Entry.Name = Source.Name;
		Entry.bAlpha = Source.bAlpha;
		Entry.Texture = WriteTexture(Folder, Name, Source.Width, Source.Height, Source.Rgba);
		if (!Source.Emission.IsEmpty())
		{
			Entry.Emission = WriteTexture(Folder, UniqueAssetName(Name + TEXT("_emission"), Used), Source.Width,
				Source.Height, Source.Emission);
		}
		if (!Source.SkyFront.IsEmpty())
		{
			Entry.SkyFront = WriteTexture(Folder, UniqueAssetName(Name + TEXT("_front"), Used), Source.Width,
				Source.Height, Source.SkyFront);
		}
	}

	Wad->AssetImportData->Update(Filename);
	Wad->MarkPackageDirty();
	return true;
}

#undef LOCTEXT_NAMESPACE
