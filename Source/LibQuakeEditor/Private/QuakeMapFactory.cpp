#include "QuakeMapFactory.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "EditorFramework/AssetImportData.h"
#include "Engine/StaticMesh.h"
#include "GeneratedAssets.h"
#include "ImportUtils.h"
#include "LibQuakeSettings.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MeshDescription.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "QuakeConversion.h"
#include "QuakeMap.h"
#include "QuakeNameTable.h"
#include "QuakeWad.h"
#include "StaticMeshAttributes.h"
#include "TextureFiles.h"
#include "UObject/Package.h"

namespace
{
	FString ToFString(const std::string& Value)
	{
		return UTF8_TO_TCHAR(Value.c_str());
	}

	/** Placement of a point entity from "origin" and "angles" (pitch yaw roll), or the yaw "angle". */
	FTransform PointEntityTransform(const TMap<FString, FString>& Keys, float Scale)
	{
		const FVector3f Origin = LibQuakeEditor::ParseVector(Keys.Find(TEXT("origin")));

		FVector3f Angles = FVector3f::ZeroVector;
		if (const FString* AnglesKey = Keys.Find(TEXT("angles")))
		{
			Angles = LibQuakeEditor::ParseVector(AnglesKey);
		}
		else if (const FString* Angle = Keys.Find(TEXT("angle")))
		{
			Angles.Y = FCString::Atof(**Angle);
		}
		return FTransform(LibQuake::ToUnrealRotation(FVector(Angles)), LibQuake::ToUnrealPosition(Origin, Scale));
	}

	quakelib::PivotMode ToPivotMode(EQuakePivot Pivot)
	{
		switch (Pivot)
		{
		case EQuakePivot::Center:
			return quakelib::PivotMode::Center;
		case EQuakePivot::BottomCenter:
			return quakelib::PivotMode::BottomCenter;
		case EQuakePivot::BottomCorner:
			return quakelib::PivotMode::BottomCorner;
		default:
			return quakelib::PivotMode::MapOrigin;
		}
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
		/** Loaded wads, searched in this order. */
		TArray<UQuakeWad*> Wads;

		/** Wads of the project that are not loaded yet. */
		TArray<FAssetData> Others;
	};

	/**
	 * Assets of one class in content folders, by the Quake name of a texture. "base/wall1" is wall1 in the sub
	 * folder base, a name without a path is found in any sub folder. The folders are searched in the order added.
	 */
	template <typename T>
	class FFolderLookup
	{
	public:
		void Add(const FDirectoryPath& Folder)
		{
			const FString Root = Folder.Path.TrimChar(TEXT('/'));
			if (Root.IsEmpty())
			{
				return;
			}

			FARFilter Filter;
			Filter.PackagePaths.Add(*(TEXT("/") + Root));
			Filter.bRecursivePaths = true;
			Filter.ClassPaths.Add(T::StaticClass()->GetClassPathName());
			Filter.bRecursiveClasses = true;
			TArray<FAssetData> Found;
			IAssetRegistry::GetChecked().GetAssets(Filter, Found);

			// sorted, so a name in several sub folders always finds the same asset
			Found.Sort([](const FAssetData& A, const FAssetData& B)
			{
				return A.PackageName.LexicalLess(B.PackageName);
			});
			for (const FAssetData& Asset : Found)
			{
				const FString Name = Asset.AssetName.ToString();
				const FString SubFolder = Asset.PackagePath.ToString().RightChop(Root.Len() + 1).TrimChar(TEXT('/'));
				if (!SubFolder.IsEmpty() && !Assets.Contains(SubFolder / Name))
				{
					Assets.Add(SubFolder / Name, Asset);
				}
				if (!Assets.Contains(Name))
				{
					Assets.Add(Name, Asset);
				}
			}
		}

		T* Find(const FString& QuakeName) const
		{
			const FString Path = FPaths::GetPath(QuakeName);
			const FString Name = LibQuakeEditor::AssetName(FPaths::GetCleanFilename(QuakeName));
			const FAssetData* Asset = Assets.Find(Path.IsEmpty() ? Name : Path / Name);
			return Asset ? Cast<T>(Asset->GetAsset()) : nullptr;
		}

	private:
		/** By name and by path below the folder, case does not matter as for Quake's names. */
		TMap<FString, FAssetData> Assets;
	};

	/**
	 * The textures of the texture folders as wad textures: "<name>_emission" is the emission, a sky gets
	 * "<name>_front" as cloud layer or the texture itself.
	 */
	class FTextureFolders
	{
	public:
		void Add(const FDirectoryPath& Folder)
		{
			Textures.Add(Folder);
		}

		const FQuakeWadTexture* Find(const FString& Name)
		{
			if (const TUniquePtr<FQuakeWadTexture>* Known = Entries.Find(Name))
			{
				return Known->Get();
			}

			TUniquePtr<FQuakeWadTexture> Entry;
			if (UTexture2D* Texture = Textures.Find(Name))
			{
				const FString BaseName = FPaths::GetCleanFilename(Name);
				Entry = MakeUnique<FQuakeWadTexture>();
				Entry->Name = Name;
				Entry->Texture = Texture;
				Entry->Emission = Textures.Find(Name + TEXT("_emission"));
				Entry->bAlpha = BaseName.StartsWith(TEXT("{"));
				if (BaseName.StartsWith(TEXT("sky")))
				{
					UTexture2D* Front = Textures.Find(Name + TEXT("_front"));
					Entry->SkyFront = Front ? Front : Texture;
				}
			}
			return Entries.Add(Name, MoveTemp(Entry)).Get();
		}

	private:
		FFolderLookup<UTexture2D> Textures;

		/** Every name asked for so far, null for the ones not in the folders. */
		TMap<FString, TUniquePtr<FQuakeWadTexture>> Entries;
	};

	/** State of one import. */
	struct FImport
	{
		UQuakeMap* Map = nullptr;
		FString Filename;

		/** Where the meshes, material instances and bsp textures go, "<map>_Generated". */
		FString Folder;

		/** The texture with a Quake name, from the wads for a map and from the file itself for a bsp. */
		TFunction<const FQuakeWadTexture*(const FString&)> FindTexture;

		/** The map's and the project's folders and the image files of a .map, they come before FindTexture. */
		FTextureFolders TextureFolders;
		LibQuakeEditor::FTextureFiles TextureFiles;
		FFolderLookup<UMaterialInterface> MaterialFolders;

		/** One material instance per texture name. */
		TMap<FString, UMaterialInterface*> Materials;

		TSet<FString> SkyTextures;

		/** Asset names given out so far, see UniqueAssetName. */
		TSet<FString> UsedNames;

		template <typename T>
		T* Asset(const FString& Name, bool& bOutCreated)
		{
			UPackage* Package = LibQuakeEditor::FindOrCreatePackage(Folder / Name);
			return LibQuakeEditor::FindOrCreateAsset<T>(Package, Name, bOutCreated);
		}

		/** The texture from the texture folders, an image file, the wads or the bsp. Null if it is in none of them. */
		const FQuakeWadTexture* Texture(const FString& Name)
		{
			if (const FQuakeWadTexture* InFolder = TextureFolders.Find(Name))
			{
				return InFolder;
			}
			const FQuakeWadTexture* File = TextureFiles.Find(Name);
			return File ? File : FindTexture(Name);
		}
	};

	/** The texture of the project settings for textures that are in no wad, null if there is none. */
	UTexture2D* MissingTexture()
	{
		return GetDefault<ULibQuakeSettings>()->MissingTexture.LoadSynchronous();
	}

	/**
	 * The material used as it is for a texture: from the map's or the project's material table, else from the
	 * material folders. Null if none has one.
	 */
	UMaterialInterface* MaterialOverride(const FImport& Import, const FString& TextureName)
	{
		const TSoftObjectPtr<UMaterialInterface>* Entry =
			LibQuake::FindByName(Import.Map->Settings.Materials, TextureName);
		if (!Entry)
		{
			Entry = LibQuake::FindByName(GetDefault<ULibQuakeSettings>()->Materials, TextureName);
		}
		return Entry ? Entry->LoadSynchronous() : Import.MaterialFolders.Find(TextureName);
	}

	/** The texture a material shows: its Albedo parameter, else the first texture it uses. */
	UTexture* MaterialTexture(const UMaterialInterface* Material)
	{
		UTexture* Albedo = nullptr;
		if (Material->GetTextureParameterValue(FHashedMaterialParameterInfo(TEXT("Albedo")), Albedo) && Albedo)
		{
			return Albedo;
		}
		TArray<UTexture*> Used;
		Material->GetUsedTextures(Used);
		return Used.IsEmpty() ? nullptr : Used[0];
	}

	/**
	 * Size in pixels for the uvs of a texture: of the texture, else of the material it gets, else of the missing
	 * texture. Faces without a size would have no uvs and no tangents, so there is always one.
	 */
	FIntPoint TextureSize(FImport& Import, const FString& TextureName)
	{
		const UTexture* Texture = nullptr;
		if (const FQuakeWadTexture* Found = Import.Texture(TextureName))
		{
			Texture = Found->Texture;
		}
		if (!Texture)
		{
			const UMaterialInterface* Override = MaterialOverride(Import, TextureName);
			Texture = Override ? MaterialTexture(Override) : nullptr;
		}
		if (!Texture)
		{
			Texture = MissingTexture();
		}
		return Texture ? FIntPoint(Texture->Source.GetSizeX(), Texture->Source.GetSizeY()) : FIntPoint(64, 64);
	}

	/**
	 * Opacity and brightness of a liquid texture: the map's worldspawn key ("_wateralpha", "_slimealpha",
	 * "_lavaalpha", "_telealpha") or the project's default for its kind, lava glows.
	 */
	void LiquidLook(const UQuakeMap* Map, const FString& TextureName, float& OutOpacity, float& OutBrightness)
	{
		const ULibQuakeSettings* Settings = GetDefault<ULibQuakeSettings>();
		const TCHAR* Key = TEXT("_wateralpha");
		OutOpacity = Settings->WaterAlpha;
		OutBrightness = 1.0f;
		if (TextureName.StartsWith(TEXT("*lava")))
		{
			Key = TEXT("_lavaalpha");
			OutOpacity = Settings->LavaAlpha;
			OutBrightness = Settings->LavaBrightness;
		}
		else if (TextureName.StartsWith(TEXT("*slime")))
		{
			Key = TEXT("_slimealpha");
			OutOpacity = Settings->SlimeAlpha;
		}
		else if (TextureName.StartsWith(TEXT("*tele")))
		{
			Key = TEXT("_telealpha");
			OutOpacity = Settings->TeleportAlpha;
		}

		// the worldspawn is the first entity, BuildAssets has written it already
		const FQuakeMapEntity* Worldspawn = Map->Entities.IsEmpty() ? nullptr : &Map->Entities[0];
		if (const FString* Value = Worldspawn ? Worldspawn->Keys.Find(Key) : nullptr)
		{
			OutOpacity = FMath::Clamp(FCString::Atof(**Value), 0.0f, 1.0f);
		}
	}

	/**
	 * The material for a texture: the one of the material tables, or a material instance of the project's sky,
	 * liquid ('*'), masked ('{') or surface material with the layers of the texture. A texture that is in no wad
	 * gets the missing texture as albedo.
	 */
	UMaterialInterface* MaterialFor(FImport& Import, const FString& TextureName)
	{
		if (UMaterialInterface** Existing = Import.Materials.Find(TextureName))
		{
			return *Existing;
		}

		const FQuakeWadTexture* Texture = Import.Texture(TextureName);
		const bool bSky = Texture ? Texture->SkyFront != nullptr : TextureName.StartsWith(TEXT("sky"));
		const bool bAlpha = Texture ? Texture->bAlpha : TextureName.StartsWith(TEXT("{"));
		const bool bLiquid = TextureName.StartsWith(TEXT("*"));
		if (bSky)
		{
			Import.SkyTextures.Add(TextureName);
		}

		if (UMaterialInterface* Override = MaterialOverride(Import, TextureName))
		{
			Import.Materials.Add(TextureName, Override);
			return Override;
		}

		if (!Texture)
		{
			LibQuakeEditor::ReportWarning(Import.Filename,
				FString::Printf(TEXT("texture '%s' not found"), *TextureName));
		}

		const ULibQuakeSettings* Settings = GetDefault<ULibQuakeSettings>();
		const TSoftObjectPtr<UMaterialInterface>& Parent = bSky ? Settings->SkyMaterial
			: bLiquid ? Settings->LiquidMaterial
			: bAlpha ? Settings->MaskedMaterial
			: Settings->SurfaceMaterial;

		bool bCreated = false;
		UMaterialInstanceConstant* Material = Import.Asset<UMaterialInstanceConstant>(
			LibQuakeEditor::UniqueAssetName(TEXT("MI_") + TextureName, Import.UsedNames), bCreated);
		Material->PreEditChange(nullptr);
		Material->SetParentEditorOnly(Parent.LoadSynchronous());
		Material->ClearParameterValuesEditorOnly();

		const auto SetTexture = [Material](const TCHAR* Parameter, UTexture* Value)
		{
			Material->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(Parameter), Value);
		};
		if (bSky)
		{
			if (Texture)
			{
				SetTexture(TEXT("SkyBack"), Texture->Texture);
				SetTexture(TEXT("SkyFront"), Texture->SkyFront);
			}
		}
		else
		{
			if (UTexture* Albedo = Texture ? Texture->Texture.Get() : MissingTexture())
			{
				SetTexture(TEXT("Albedo"), Albedo);
			}
			if (Texture && Texture->Emission)
			{
				SetTexture(TEXT("Emissive"), Texture->Emission);
			}
		}

		if (bLiquid)
		{
			float Opacity = 1.0f;
			float Brightness = 1.0f;
			LiquidLook(Import.Map, TextureName, Opacity, Brightness);
			Material->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Opacity")), Opacity);
			Material->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Brightness")), Brightness);
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

	/**
	 * The mesh of a brush entity. Entities without visible faces (triggers) get their hulls as mesh, or in a bsp
	 * their clip faces. bOutInvisible tells them apart: they are only there for collision and overlaps. Null if the
	 * entity has none of these.
	 */
	UStaticMesh* BuildStaticMesh(FImport& Import, const FString& Name, const quakelib::EntityGeometry& Geometry,
		bool& bOutInvisible)
	{
		const float Scale = Import.Map->Settings.Scale;
		FMeshDescription Description;
		bOutInvisible = !LibQuake::BuildMeshDescription(Geometry, Scale, Description);
		if (bOutInvisible)
		{
			Description = FMeshDescription();
			if (!LibQuake::BuildHullMeshDescription(Geometry, Scale, Description))
			{
				Description = FMeshDescription();
				if (!LibQuake::BuildMeshDescription(Geometry, Scale, Description, quakelib::SurfaceType::CLIP))
				{
					return nullptr;
				}
			}
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
			UMaterialInterface* Material = bOutInvisible ? nullptr : MaterialFor(Import, Slots[Group].ToString());
			Materials.Emplace(Material, Slots[Group], Slots[Group]);
		}

		// Unreal lays out its own lightmap uvs from the texture uvs, it knows what Lightmass needs
		const int32 Resolution = LightmapResolution(Geometry, Import.Map->Settings.LuxelSize);
		Mesh->SetNumSourceModels(1);
		FMeshBuildSettings& Build = Mesh->GetSourceModel(0).BuildSettings;
		Build.bRecomputeNormals = bOutInvisible;
		Build.bRecomputeTangents = true;
		Build.bUseMikkTSpace = true;
		Build.bRemoveDegenerates = true;
		Build.bGenerateLightmapUVs = !bOutInvisible;
		Build.SrcLightmapIndex = 0;
		Build.DstLightmapIndex = 1;
		Build.MinLightmapResolution = Resolution;
		Mesh->SetLightMapCoordinateIndex(1);
		Mesh->SetLightMapResolution(Resolution);

		Mesh->CreateMeshDescription(0, MoveTemp(Description));
		Mesh->CommitMeshDescription(0);

		// sky is an opening in Quake, it must not keep the sun out when Unreal bakes the light
		for (int32 Section = 0; Section < Materials.Num(); Section++)
		{
			FMeshSectionInfo Info = Mesh->GetSectionInfoMap().Get(0, Section);
			Info.bCastShadow = !Import.SkyTextures.Contains(Materials[Section].MaterialSlotName.ToString());
			Mesh->GetSectionInfoMap().Set(0, Section, Info);
		}

		// brush hulls as simple collision, they are convex already and exact. A bsp has none, there the mesh itself
		// is the collision
		Mesh->CreateBodySetup();
		UBodySetup* Body = Mesh->GetBodySetup();
		Body->RemoveSimpleCollision();
		Body->AggGeom.ConvexElems = LibQuake::BuildConvexElements(Geometry, Scale,
			[](quakelib::CollisionHull::Type Type) { return Type != quakelib::CollisionHull::Type::Liquid; });
		Body->CollisionTraceFlag =
			Body->AggGeom.ConvexElems.IsEmpty() ? CTF_UseComplexAsSimple : CTF_UseSimpleAsComplex;
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

	/** A key of a libquake entity, empty if it has none. */
	FString KeyOf(const quakelib::LevelEntity& Entity, const char* Key)
	{
		const auto Found = Entity.attributes.find(Key);
		return Found != Entity.attributes.end() ? ToFString(Found->second) : FString();
	}

	/** The liquid brushes of an entity, relative to its pivot like its mesh. */
	TArray<FQuakeLiquid> Liquids(const quakelib::EntityGeometry& Geometry, float Scale)
	{
		TArray<FQuakeLiquid> Liquids;
		for (const quakelib::CollisionHull& Hull : Geometry.hulls)
		{
			if (Hull.type != quakelib::CollisionHull::Type::Liquid)
			{
				continue;
			}

			FQuakeLiquid& Liquid = Liquids.AddDefaulted_GetRef();
			Liquid.Texture = ToFString(Hull.textureName);
			for (const quakelib::Vec3& Point : Hull.points)
			{
				Liquid.Points.Add(FVector(LibQuake::ToUnrealPosition(Point, Scale)));
			}
			// mirrored like the meshes, the second and third corner swap places
			for (size_t i = 0; i + 2 < Hull.indices.size(); i += 3)
			{
				Liquid.Indices.Append({(int32)Hull.indices[i], (int32)Hull.indices[i + 2], (int32)Hull.indices[i + 1]});
			}
		}
		return Liquids;
	}

	/** TrenchBroom group names by group id ("_tb_id"), from the func_group entities that are groups. */
	TMap<FString, FString> GroupNames(const quakelib::Level& Level)
	{
		TMap<FString, FString> Names;
		for (const quakelib::LevelEntity& Entity : Level.entities)
		{
			if (KeyOf(Entity, "_tb_type") == TEXT("_tb_group"))
			{
				Names.Add(KeyOf(Entity, "_tb_id"), KeyOf(Entity, "_tb_name"));
			}
		}
		return Names;
	}

	/**
	 * The name of a brush entity's mesh: "SM_<name>" after the TrenchBroom group it is or it is in, "SM_<class>_<n>"
	 * for entities outside of groups.
	 */
	FString MeshName(const quakelib::LevelEntity& Entity, const TMap<FString, FString>& Groups,
		TMap<FString, int32>& ClassCount)
	{
		FString GroupName = KeyOf(Entity, "_tb_name");
		if (GroupName.IsEmpty())
		{
			GroupName = Groups.FindRef(KeyOf(Entity, "_tb_group"));
		}
		if (!GroupName.IsEmpty())
		{
			return TEXT("SM_") + GroupName;
		}

		const FString ClassName = ToFString(Entity.classname);
		return FString::Printf(TEXT("SM_%s_%d"), *ClassName, ClassCount.FindOrAdd(ClassName)++);
	}

	/** Meshes, materials and entities of a built level, written into the map asset. */
	void BuildAssets(FImport& Import, const quakelib::Level& Level)
	{
		UQuakeMap* Map = Import.Map;
		const float Scale = Map->Settings.Scale;
		Map->Meshes.Reset();
		Map->Entities.Reset();

		const TMap<FString, FString> Groups = GroupNames(Level);
		TMap<FString, int32> ClassCount;
		for (const quakelib::LevelEntity& Entity : Level.entities)
		{
			FQuakeMapEntity& Out = Map->Entities.AddDefaulted_GetRef();
			Out.ClassName = ToFString(Entity.classname);
			for (const auto& [Key, Value] : Entity.attributes)
			{
				Out.Keys.Add(ToFString(Key), ToFString(Value));
			}

			if (Entity.geometry < 0)
			{
				Out.Transform = PointEntityTransform(Out.Keys, Scale);
				continue;
			}

			// a brush entity stands at the pivot of its mesh
			const quakelib::EntityGeometry& Geometry = Level.geometry[Entity.geometry];
			Out.Transform = FTransform(FVector(LibQuake::ToUnrealPosition(Geometry.pivot, Scale)));
			Out.Liquids = Liquids(Geometry, Scale);

			const FString Name =
				LibQuakeEditor::UniqueAssetName(MeshName(Entity, Groups, ClassCount), Import.UsedNames);
			if (UStaticMesh* Mesh = BuildStaticMesh(Import, Name, Geometry, Out.bInvisible))
			{
				Out.Mesh = Map->Meshes.Add(Mesh);
			}
		}
	}

	/** A mesh library joins the brush entities of each TrenchBroom group into one mesh. */
	bool IsMeshLibrary(const UQuakeMap* Map, const quakelib::MapDocument& Document)
	{
		if (Map->Settings.bMeshLibrary)
		{
			return true;
		}

		for (const quakelib::MapEntity& Entity : Document.entities)
		{
			if (Entity.properties.classname == "worldspawn")
			{
				TMap<FString, FString> Keys;
				for (const auto& [Key, Value] : Entity.properties.attributes)
				{
					Keys.Add(ToFString(Key), ToFString(Value));
				}
				return UQuakeMap::HasMeshLibraryKey(Keys);
			}
		}
		return false;
	}

	/** Parses and builds a .map with the textures of the imported wads. */
	bool BuildFromMap(FImport& Import, quakelib::Level& OutLevel, FString& OutError)
	{
		quakelib::MapDocument Document;
		if (!LibQuake::ParseMap(Import.Filename, Document, OutError))
		{
			return false;
		}

		TSharedRef<FWadLookup> Wads = MakeShared<FWadLookup>(Document, Import.Filename);
		Import.FindTexture = [Wads](const FString& Name) { return Wads->Find(Name); };

		quakelib::MapBuildOptions Options;
		Options.csg = Import.Map->Settings.bCSG;
		Options.pivot = ToPivotMode(Import.Map->Settings.Pivot);
		Options.mergeGroups = IsMeshLibrary(Import.Map, Document);
		Options.textureSize = [&Import](const std::string& Name) -> std::pair<int, int>
		{
			const FIntPoint Size = TextureSize(Import, ToFString(Name));
			return {Size.X, Size.Y};
		};
		return LibQuake::BuildMap(Document, Options, OutLevel, OutError);
	}

	/** Loads a .bsp and writes the textures it carries next to the meshes, as T_<name>. */
	bool BuildFromBsp(FImport& Import, TArray<FQuakeWadTexture>& OutTextures, quakelib::Level& OutLevel,
		FString& OutError)
	{
		if (!LibQuake::LoadBsp(Import.Filename, {}, OutLevel, OutError))
		{
			return false;
		}

		OutTextures.Reset(OutLevel.textures.size());
		for (const quakelib::TextureInfo& Info : OutLevel.textures)
		{
			if (Info.rgba.empty())
			{
				continue;
			}
			const FString Name = LibQuakeEditor::UniqueAssetName(TEXT("T_") + ToFString(Info.name), Import.UsedNames);
			const auto Write = [&](const FString& AssetName, const std::vector<uint8_t>& Rgba)
			{
				return LibQuakeEditor::WriteTexture(Import.Folder, AssetName, Info.width, Info.height,
					TArray<uint8>(Rgba.data(), Rgba.size()));
			};

			FQuakeWadTexture& Entry = OutTextures.AddDefaulted_GetRef();
			Entry.Name = ToFString(Info.name);
			Entry.bAlpha = Info.hasAlpha;
			Entry.Texture = Write(Name, Info.rgba);
			if (Info.hasEmission)
			{
				Entry.Emission = Write(LibQuakeEditor::UniqueAssetName(Name + TEXT("_emission"), Import.UsedNames),
					Info.emission);
			}
			if (!Info.skyFront.empty())
			{
				Entry.SkyFront = Write(LibQuakeEditor::UniqueAssetName(Name + TEXT("_front"), Import.UsedNames),
					Info.skyFront);
			}
		}

		Import.FindTexture = [&OutTextures](const FString& Name)
		{
			return OutTextures.FindByPredicate([&Name](const FQuakeWadTexture& Texture)
			{
				return Texture.Name.Equals(Name, ESearchCase::IgnoreCase);
			});
		};
		return true;
	}
}

UQuakeMapFactory::UQuakeMapFactory()
{
	Formats.Add(TEXT("map;Quake Map"));
	Formats.Add(TEXT("bsp;Quake BSP"));
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
		GetDefault<ULibQuakeSettings>()->ApplyDefaults(Map->Settings);
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
	FImport Import;
	Import.Map = Map;
	Import.Filename = Filename;
	Import.Folder = LibQuakeEditor::GeneratedFolder(Map);

	const ULibQuakeSettings* Settings = GetDefault<ULibQuakeSettings>();
	Import.TextureFolders.Add(Map->Settings.TextureFolder);
	Import.TextureFolders.Add(Settings->TextureFolder);
	Import.MaterialFolders.Add(Map->Settings.MaterialFolder);
	Import.MaterialFolders.Add(Settings->MaterialFolder);

	FString Error;
	quakelib::Level Level;
	// the textures of a bsp, Import.FindTexture looks them up while the meshes are built
	TArray<FQuakeWadTexture> BspTextures;
	const bool bBsp = FPaths::GetExtension(Filename).Equals(TEXT("bsp"), ESearchCase::IgnoreCase);
	if (!bBsp)
	{
		Import.TextureFiles.Init(Filename, Settings->TextureFilesFolder.Path);
	}
	if (!(bBsp ? BuildFromBsp(Import, BspTextures, Level, Error) : BuildFromMap(Import, Level, Error)))
	{
		LibQuakeEditor::ReportError(Filename, Error);
		LibQuakeEditor::NotifyIfReported();
		return false;
	}

	Map->Modify();
	BuildAssets(Import, Level);
	Map->AssetImportData->Update(Filename);
	Map->MarkPackageDirty();
	LibQuakeEditor::NotifyIfReported();
	return true;
}
