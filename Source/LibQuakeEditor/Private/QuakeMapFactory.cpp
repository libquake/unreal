#include "QuakeMapFactory.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/StaticMesh.h"
#include "ImportUtils.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "QuakeConversion.h"
#include "QuakeMap.h"
#include "QuakeWad.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"

namespace
{
	const TCHAR* SurfaceMaterial = TEXT("/LibQuake/Materials/M_Quake.M_Quake");
	const TCHAR* MaskedMaterial = TEXT("/LibQuake/Materials/M_QuakeMasked.M_QuakeMasked");

	FString ToFString(const std::string& Value)
	{
		return UTF8_TO_TCHAR(Value.c_str());
	}

	/**
	 * The imported wads, the ones the map names first and in that order. The other wads of the project are only
	 * loaded when a texture is not in those.
	 */
	class FWadLookup
	{
	public:
		FWadLookup(const quakelib::MapDocument& Document, const FString& Filename)
		{
			IAssetRegistry::GetChecked().GetAssetsByClass(UQuakeWad::StaticClass()->GetClassPathName(), Others);
			for (const std::string& Wanted : Document.wads)
			{
				const FString Base = FPaths::GetBaseFilename(ToFString(Wanted));
				const int32 Match = Others.IndexOfByPredicate([&Base](const FAssetData& Asset)
				{
					return Asset.AssetName.ToString().Equals(Base, ESearchCase::IgnoreCase);
				});
				if (Match == INDEX_NONE)
				{
					LibQuakeEditor::ReportWarning(Filename, FString::Printf(
						TEXT("wad '%s' is not imported, looking in the other wads"), *Base));
					continue;
				}
				if (UQuakeWad* Wad = Cast<UQuakeWad>(Others[Match].GetAsset()))
				{
					Wads.AddUnique(Wad);
				}
				Others.RemoveAt(Match);
			}
		}

		const FQuakeWadTexture* Find(const FString& Name)
		{
			for (const UQuakeWad* Wad : Wads)
			{
				if (const FQuakeWadTexture* Texture = Wad->Find(Name))
				{
					return Texture;
				}
			}
			while (!Others.IsEmpty())
			{
				UQuakeWad* Wad = Cast<UQuakeWad>(Others.Pop().GetAsset());
				if (!Wad)
				{
					continue;
				}
				Wads.Add(Wad);
				if (const FQuakeWadTexture* Texture = Wad->Find(Name))
				{
					return Texture;
				}
			}
			return nullptr;
		}

	private:
		TArray<UQuakeWad*> Wads;
		TArray<FAssetData> Others;
	};

	/** State of one import. */
	struct FImport
	{
		UQuakeMap* Map = nullptr;
		FString Filename;
		FString Folder;
		FWadLookup* Wads = nullptr;
		TMap<FString, UMaterialInterface*> Materials;
		TSet<FString> UsedNames;

		template <typename T>
		T* Asset(const FString& Name, bool& bOutCreated)
		{
			UPackage* Package = LibQuakeEditor::FindOrCreatePackage(Folder / Name);
			return LibQuakeEditor::FindOrCreateAsset<T>(Package, Name, bOutCreated);
		}
	};

	UMaterialInterface* MaterialFor(FImport& Import, const FString& TextureName)
	{
		if (UMaterialInterface** Existing = Import.Materials.Find(TextureName))
		{
			return *Existing;
		}

		const FQuakeWadTexture* Texture = Import.Wads->Find(TextureName);
		if (!Texture)
		{
			LibQuakeEditor::ReportWarning(Import.Filename, FString::Printf(
				TEXT("texture '%s' not found in the imported wads"), *TextureName));
		}
		const bool bAlpha = Texture ? Texture->bAlpha : TextureName.StartsWith(TEXT("{"));

		bool bCreated = false;
		UMaterialInstanceConstant* Material = Import.Asset<UMaterialInstanceConstant>(
			LibQuakeEditor::UniqueAssetName(TEXT("MI_") + TextureName, Import.UsedNames), bCreated);
		Material->PreEditChange(nullptr);
		Material->SetParentEditorOnly(LoadObject<UMaterialInterface>(nullptr, bAlpha ? MaskedMaterial : SurfaceMaterial));
		Material->ClearParameterValuesEditorOnly();
		if (Texture)
		{
			Material->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Albedo")), Texture->Texture);
			if (Texture->Emission)
			{
				Material->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Emissive")), Texture->Emission);
			}
		}
		Material->PostEditChange();
		if (bCreated)
		{
			FAssetRegistryModule::AssetCreated(Material);
		}
		Import.Materials.Add(TextureName, Material);
		return Material;
	}

	/** Square, power of two, about one texel per LuxelSize Quake units of surface. */
	int32 LightmapResolution(const quakelib::EntityGeometry& Geometry, float LuxelSize)
	{
		double Area = 0;
		for (const quakelib::RenderMesh& Mesh : Geometry.meshes)
		{
			for (size_t i = 0; i + 2 < Mesh.indices.size(); i += 3)
			{
				const quakelib::Vec3 A = Mesh.vertices[Mesh.indices[i]].point;
				const quakelib::Vec3 B = Mesh.vertices[Mesh.indices[i + 1]].point;
				const quakelib::Vec3 C = Mesh.vertices[Mesh.indices[i + 2]].point;
				const FVector3f AB(B.X - A.X, B.Y - A.Y, B.Z - A.Z);
				const FVector3f AC(C.X - A.X, C.Y - A.Y, C.Z - A.Z);
				Area += 0.5 * FVector3f::CrossProduct(AB, AC).Size();
			}
		}
		const int32 Side = FMath::CeilToInt32(FMath::Sqrt(Area) / LuxelSize);
		return FMath::Clamp(FMath::RoundUpToPowerOfTwo(FMath::Max(Side, 1)), 32, 1024);
	}

	UStaticMesh* BuildStaticMesh(FImport& Import, const FString& Name, const quakelib::EntityGeometry& Geometry)
	{
		const float Scale = Import.Map->Settings.Scale;
		FMeshDescription Description;
		if (!LibQuake::BuildMeshDescription(Geometry, Scale, Description))
		{
			return nullptr;
		}

		bool bCreated = false;
		UStaticMesh* Mesh = Import.Asset<UStaticMesh>(Name, bCreated);
		Mesh->PreEditChange(nullptr);

		// polygon groups are created in order, their index is the section's material index
		TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
		Materials.Reset();
		const TPolygonGroupAttributesConstRef<FName> Slots =
			FStaticMeshConstAttributes(Description).GetPolygonGroupMaterialSlotNames();
		for (const FPolygonGroupID Group : Description.PolygonGroups().GetElementIDs())
		{
			Materials.Emplace(MaterialFor(Import, Slots[Group].ToString()), Slots[Group], Slots[Group]);
		}

		// Unreal lays out its own lightmap uvs from the texture uvs, it knows what Lightmass needs
		const int32 Resolution = LightmapResolution(Geometry, Import.Map->Settings.LuxelSize);
		Mesh->SetNumSourceModels(1);
		FMeshBuildSettings& Build = Mesh->GetSourceModel(0).BuildSettings;
		Build.bRecomputeNormals = false;
		Build.bRecomputeTangents = true;
		Build.bUseMikkTSpace = true;
		Build.bRemoveDegenerates = true;
		Build.bGenerateLightmapUVs = true;
		Build.SrcLightmapIndex = 0;
		Build.DstLightmapIndex = 1;
		Build.MinLightmapResolution = Resolution;
		Mesh->SetLightMapCoordinateIndex(1);
		Mesh->SetLightMapResolution(Resolution);

		Mesh->CreateMeshDescription(0, MoveTemp(Description));
		Mesh->CommitMeshDescription(0);

		// brush hulls as simple collision, they are convex already and exact
		Mesh->CreateBodySetup();
		UBodySetup* Body = Mesh->GetBodySetup();
		Body->RemoveSimpleCollision();
		Body->AggGeom.ConvexElems = LibQuake::BuildConvexElements(Geometry, Scale,
			[](quakelib::CollisionHull::Type Type) { return Type != quakelib::CollisionHull::Type::Liquid; });
		Body->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		Body->InvalidatePhysicsData();
		Body->CreatePhysicsMeshes();

		Mesh->Build(true);
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();
		if (bCreated)
		{
			FAssetRegistryModule::AssetCreated(Mesh);
		}
		return Mesh;
	}
}

UQuakeMapFactory::UQuakeMapFactory()
{
	Formats.Add(TEXT("map;Quake Map"));
	SupportedClass = UQuakeMap::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
}

UObject* UQuakeMapFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	UQuakeMap* Map = FindObject<UQuakeMap>(InParent, *InName.ToString());
	if (!Map)
	{
		Map = NewObject<UQuakeMap>(InParent, InClass, InName, Flags | RF_Transactional);
	}
	return ImportMap(Map, Filename) ? Map : nullptr;
}

bool UQuakeMapFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	const UQuakeMap* Map = Cast<UQuakeMap>(Obj);
	if (!Map || !Map->AssetImportData)
	{
		return false;
	}
	Map->AssetImportData->ExtractFilenames(OutFilenames);
	return true;
}

void UQuakeMapFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	UQuakeMap* Map = Cast<UQuakeMap>(Obj);
	if (Map && Map->AssetImportData && NewReimportPaths.Num() > 0)
	{
		Map->AssetImportData->UpdateFilenameOnly(NewReimportPaths[0]);
	}
}

EReimportResult::Type UQuakeMapFactory::Reimport(UObject* Obj)
{
	UQuakeMap* Map = Cast<UQuakeMap>(Obj);
	if (!Map || !Map->AssetImportData)
	{
		return EReimportResult::Failed;
	}
	return ImportMap(Map, Map->AssetImportData->GetFirstFilename()) ? EReimportResult::Succeeded
		: EReimportResult::Failed;
}

int32 UQuakeMapFactory::GetPriority() const
{
	return ImportPriority;
}

bool UQuakeMapFactory::ImportMap(UQuakeMap* Map, const FString& Filename)
{
	FString Error;
	quakelib::MapDocument Document;
	if (!LibQuake::ParseMap(Filename, Document, Error))
	{
		LibQuakeEditor::ReportError(Filename, Error);
		LibQuakeEditor::NotifyIfReported();
		return false;
	}

	FWadLookup Wads(Document, Filename);
	quakelib::MapBuildOptions Options;
	Options.csg = Map->Settings.bCSG;
	Options.textureSize = [&Wads](const std::string& Name) -> std::pair<int, int>
	{
		const FQuakeWadTexture* Texture = Wads.Find(ToFString(Name));
		return Texture && Texture->Texture
			? std::pair<int, int>(Texture->Texture->Source.GetSizeX(), Texture->Texture->Source.GetSizeY())
			: std::pair<int, int>(0, 0);
	};

	quakelib::Level Level;
	if (!LibQuake::BuildMap(Document, Options, Level, Error))
	{
		LibQuakeEditor::ReportError(Filename, Error);
		LibQuakeEditor::NotifyIfReported();
		return false;
	}

	Map->Modify();
	FImport Import;
	Import.Map = Map;
	Import.Filename = Filename;
	Import.Folder = FPackageName::GetLongPackagePath(Map->GetOutermost()->GetName()) /
		(Map->GetName() + TEXT("_Generated"));
	Import.Wads = &Wads;

	TMap<FString, int32> ClassCount;
	Map->Meshes.Reset();
	for (int32 Index = 0; Index < (int32)Level.entities.size(); Index++)
	{
		const quakelib::LevelEntity& Entity = Level.entities[Index];
		if (Entity.geometry < 0)
		{
			continue;
		}
		const FString ClassName = ToFString(Entity.classname);
		const FString Name = LibQuakeEditor::UniqueAssetName(
			FString::Printf(TEXT("SM_%s_%d"), *ClassName, ClassCount.FindOrAdd(ClassName)++), Import.UsedNames);
		if (UStaticMesh* Mesh = BuildStaticMesh(Import, Name, Level.geometry[Entity.geometry]))
		{
			Map->Meshes.Add(Mesh);
		}
	}

	Map->AssetImportData->Update(Filename);
	Map->MarkPackageDirty();
	LibQuakeEditor::NotifyIfReported();
	return true;
}
