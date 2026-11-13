#include "GeneratedAssets.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "LibQuakeEditorModule.h"
#include "Logging/MessageLog.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Misc/PackageName.h"
#include "ObjectTools.h"
#include "QuakeMap.h"

#define LOCTEXT_NAMESPACE "LibQuakeGeneratedAssets"

namespace
{
	/** The assets the map uses: its meshes, their material instances and the textures of those. */
	TSet<FName> UsedPackages(const UQuakeMap* Map)
	{
		TSet<FName> Used;
		const auto Add = [&Used](const UObject* Object)
		{
			if (Object)
			{
				Used.Add(Object->GetOutermost()->GetFName());
			}
		};

		for (const UStaticMesh* Mesh : Map->Meshes)
		{
			Add(Mesh);
			if (!Mesh)
			{
				continue;
			}

			for (const FStaticMaterial& Slot : Mesh->GetStaticMaterials())
			{
				Add(Slot.MaterialInterface);
				if (const UMaterialInstance* Instance = Cast<UMaterialInstance>(Slot.MaterialInterface))
				{
					for (const FTextureParameterValue& Parameter : Instance->TextureParameterValues)
					{
						Add(Parameter.ParameterValue);
					}
				}
			}
		}
		return Used;
	}

	/** Only what the importer makes: meshes, material instances and bsp textures with its prefixes. */
	bool IsGenerated(const FAssetData& Asset)
	{
		const FString Name = Asset.AssetName.ToString();
		return (Asset.IsInstanceOf<UStaticMesh>() && Name.StartsWith(TEXT("SM_")))
			|| (Asset.IsInstanceOf<UMaterialInstanceConstant>() && Name.StartsWith(TEXT("MI_")))
			|| (Asset.IsInstanceOf<UTexture2D>() && Name.StartsWith(TEXT("T_")));
	}
}

namespace LibQuakeEditor
{
	FString GeneratedFolder(const UQuakeMap* Map)
	{
		return FPackageName::GetLongPackagePath(Map->GetOutermost()->GetName()) / (Map->GetName() + TEXT("_Generated"));
	}

	int32 DeleteUnusedGeneratedAssets(UQuakeMap* Map)
	{
		IAssetRegistry& Registry = IAssetRegistry::GetChecked();
		TArray<FAssetData> InFolder;
		Registry.GetAssetsByPath(FName(GeneratedFolder(Map)), InFolder, true);

		const TSet<FName> Used = UsedPackages(Map);
		TSet<FName> Unused;
		for (const FAssetData& Asset : InFolder)
		{
			if (IsGenerated(Asset) && !Used.Contains(Asset.PackageName))
			{
				Unused.Add(Asset.PackageName);
			}
		}

		// an asset something outside refers to stays, e.g. a mesh of an old placement in a saved level, and with it
		// what it refers to itself: repeat until nothing more is kept
		const FName MapPackage = Map->GetOutermost()->GetFName();
		for (bool bKeptAny = true; bKeptAny;)
		{
			bKeptAny = false;
			for (const FName Package : TSet<FName>(Unused))
			{
				TArray<FName> Referencers;
				Registry.GetReferencers(Package, Referencers);
				const bool bReferenced = Referencers.ContainsByPredicate([&](const FName& Referencer)
				{
					return Referencer != MapPackage && !Unused.Contains(Referencer);
				});
				if (bReferenced)
				{
					Unused.Remove(Package);
					bKeptAny = true;
				}
			}
		}

		TArray<UObject*> ToDelete;
		for (const FAssetData& Asset : InFolder)
		{
			UObject* Object = Unused.Contains(Asset.PackageName) ? Asset.GetAsset() : nullptr;
			if (Object)
			{
				ToDelete.Add(Object);
			}
		}
		if (ToDelete.IsEmpty())
		{
			return 0;
		}

		// without confirmation Unreal still checks the references and deletes nothing if one is left
		const int32 Deleted = ObjectTools::DeleteObjects(ToDelete, false,
			ObjectTools::EAllowCancelDuringDelete::CancelNotAllowed);
		if (Deleted > 0)
		{
			FMessageLog(LogName).Info(FText::Format(
				LOCTEXT("Deleted", "{0}: deleted {1} generated assets the map no longer uses"),
				FText::FromString(Map->GetName()), FText::AsNumber(Deleted)));
		}
		return Deleted;
	}
}

#undef LOCTEXT_NAMESPACE
