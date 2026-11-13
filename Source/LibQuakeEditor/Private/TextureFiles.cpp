#include "TextureFiles.h"

#include "EditorFramework/AssetImportData.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "ImportUtils.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace LibQuakeEditor
{
	namespace
	{
		const TCHAR* const Extensions[] = {TEXT("png"), TEXT("tga"), TEXT("jpg"), TEXT("jpeg")};

		/** The asset path for a texture name with each part a valid asset name, "*water1" becomes "_water1". */
		FString AssetPath(const FString& Name)
		{
			TArray<FString> Parts;
			Name.ParseIntoArray(Parts, TEXT("/"));
			for (FString& Part : Parts)
			{
				Part = AssetName(Part);
			}
			return FString::Join(Parts, TEXT("/"));
		}

		bool HasAlpha(const FImage& Image)
		{
			for (const FColor& Pixel : Image.AsBGRA8())
			{
				if (Pixel.A < 255)
				{
					return true;
				}
			}
			return false;
		}

		/** Whether the texture was written from this file with this content. */
		bool IsFrom(const UTexture2D* Texture, const FString& File, const FMD5Hash& Hash)
		{
			const UAssetImportData* Data = Texture ? Texture->AssetImportData.Get() : nullptr;
			if (!Data || Data->GetSourceFileCount() != 1)
			{
				return false;
			}
			return Data->GetFirstFilename() == File && Data->GetSourceData().SourceFiles[0].FileHash == Hash;
		}
	}

	void FTextureFiles::Init(const FString& InMapFilename, const FString& InAssetFolder)
	{
		MapFilename = InMapFilename;
		AssetFolder = InAssetFolder.TrimChar(TEXT('/'));
		if (AssetFolder.IsEmpty())
		{
			return;
		}
		AssetFolder = TEXT("/") + AssetFolder;

		FString Folder = FPaths::GetPath(FPaths::ConvertRelativePathToFull(MapFilename));
		while (!Folder.IsEmpty())
		{
			const FString Textures = Folder / TEXT("textures");
			if (IFileManager::Get().DirectoryExists(*Textures))
			{
				Roots.Add(Textures);
			}

			const FString Parent = FPaths::GetPath(Folder);
			if (Parent == Folder)
			{
				break;
			}
			Folder = Parent;
		}
	}

	const FQuakeWadTexture* FTextureFiles::Find(const FString& Name)
	{
		if (const TUniquePtr<FQuakeWadTexture>* Known = Entries.Find(Name))
		{
			return Known->Get();
		}

		TUniquePtr<FQuakeWadTexture> Entry;
		const FString File = FindFile(Name);
		bool bAlpha = false;
		if (UTexture2D* Texture = File.IsEmpty() ? nullptr : Load(File, Name, bAlpha))
		{
			const FString BaseName = FPaths::GetCleanFilename(Name);
			Entry = MakeUnique<FQuakeWadTexture>();
			Entry->Name = Name;
			Entry->Texture = Texture;
			Entry->bAlpha = bAlpha || BaseName.StartsWith(TEXT("{"));

			FString Emission = FindFile(Name + TEXT("_luma"));
			if (Emission.IsEmpty())
			{
				Emission = FindFile(Name + TEXT("_glow"));
			}
			bool bIgnored = false;
			Entry->Emission = Emission.IsEmpty() ? nullptr : Load(Emission, Name + TEXT("_emission"), bIgnored);

			if (BaseName.StartsWith(TEXT("sky")))
			{
				const FString Front = FindFile(Name + TEXT("_front"));
				UTexture2D* FrontTexture = Front.IsEmpty() ? nullptr : Load(Front, Name + TEXT("_front"), bIgnored);
				Entry->SkyFront = FrontTexture ? FrontTexture : Texture;
			}
		}
		return Entries.Add(Name, MoveTemp(Entry)).Get();
	}

	FString FTextureFiles::FindFile(const FString& Name) const
	{
		// Quake's '*' is not allowed in file names on Windows, engines with external textures use '#' for it
		TArray<FString, TInlineAllocator<2>> Candidates = {Name};
		if (Name.Contains(TEXT("*")))
		{
			Candidates.Add(Name.Replace(TEXT("*"), TEXT("#")));
		}

		for (const FString& Root : Roots)
		{
			for (const FString& Candidate : Candidates)
			{
				for (const TCHAR* Extension : Extensions)
				{
					const FString File = Root / Candidate + TEXT(".") + Extension;
					if (IFileManager::Get().FileExists(*File))
					{
						return File;
					}
				}
			}
		}
		return FString();
	}

	UTexture2D* FTextureFiles::Load(const FString& File, const FString& Name, bool& bOutAlpha)
	{
		FImage Image;
		if (!FImageUtils::LoadImage(*File, Image))
		{
			ReportWarning(MapFilename, FString::Printf(TEXT("cannot read the image '%s'"), *File));
			return nullptr;
		}
		Image.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		bOutAlpha = HasAlpha(Image);

		const FString Path = AssetFolder / AssetPath(Name);
		const FString Folder = FPaths::GetPath(Path);
		const FString TextureName = FPaths::GetCleanFilename(Path);

		// an unchanged file keeps its asset as it is, so reimports leave the shared textures alone
		const FMD5Hash Hash = FMD5Hash::HashFile(*File);
		UTexture2D* Existing = FindObject<UTexture2D>(FindOrCreatePackage(Path), *TextureName);
		if (IsFrom(Existing, File, Hash))
		{
			return Existing;
		}

		UTexture2D* Texture = WriteTexture(Folder, TextureName, Image);
		Texture->AssetImportData->Update(File, Hash);
		return Texture;
	}
}
