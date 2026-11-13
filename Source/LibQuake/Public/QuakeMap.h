#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/Object.h"

#include "QuakeMap.generated.h"

class UAssetImportData;
class UMaterialInterface;
class UQuakeEntityClasses;
class UStaticMesh;

/** Where the mesh of a brush entity has its origin, and so its actor. */
UENUM(BlueprintType)
enum class EQuakePivot : uint8
{
	/** The origin of the map, the mesh stays where it is in the map. */
	MapOrigin,
	/** The center of the entity's bounds. */
	Center,
	/** The center of the bottom of the entity's bounds, a prop dragged into a level stands on the floor. */
	BottomCenter,
};

/** Settings of one map. A new import starts with the defaults of the project settings (ULibQuakeSettings). */
USTRUCT(BlueprintType)
struct LIBQUAKE_API FQuakeMapSettings
{
	GENERATED_BODY()

	/** Unreal units (cm) per Quake unit. 3.125 makes 32 Quake units a meter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import", meta = (ClampMin = "0.01"))
	float Scale = 3.125f;

	/** Remove faces hidden inside other brushes of the same entity. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import")
	bool bCSG = true;

	/**
	 * Origin of the brush entities' meshes. An "origin" brush in an entity wins, then its key "_pivot" ("origin",
	 * "center" or "bottom"). The worldspawn stays at the map origin.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import")
	EQuakePivot Pivot = EQuakePivot::BottomCenter;

	/**
	 * The map is a collection of meshes, e.g. props, not a level: it cannot be placed, its meshes are dragged into
	 * levels one by one. The worldspawn key "_meshlib" "1" does the same from TrenchBroom.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import")
	bool bMeshLibrary = false;

	/** Quake units per lightmap texel. 16 matches Quake, smaller values give sharper shadows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting", meta = (ClampMin = "1"))
	float LuxelSize = 16.0f;

	/** Mobility of the lights of placed maps. Static for baked light, Movable for Lumen. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Lighting")
	TEnumAsByte<EComponentMobility::Type> LightMobility = EComponentMobility::Static;

	/**
	 * Materials used as they are instead of a material instance, by texture name, names ending in '*' are prefixes.
	 * They come before the ones in the project settings.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Materials")
	TMap<FString, TSoftObjectPtr<UMaterialInterface>> Materials;

	/** Actor classes for entity classes of this map, they come before the ones in the project settings. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Entities")
	TObjectPtr<UQuakeEntityClasses> EntityClasses;
};

/** An entity of the map, in Unreal units relative to the map's origin. */
USTRUCT(BlueprintType)
struct LIBQUAKE_API FQuakeMapEntity
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entity")
	FString ClassName;

	/** origin and angle, for brush entities the pivot of their mesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entity")
	FTransform Transform;

	/** Index into UQuakeMap::Meshes, INDEX_NONE for point entities. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entity")
	int32 Mesh = INDEX_NONE;

	/** A brush entity without visible faces, like a trigger. Its mesh is the brush hulls, for collision only. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entity")
	bool bInvisible = false;

	/** All keys as written in the map, classname included. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Entity")
	TMap<FString, FString> Keys;
};

/**
 * An imported .map or .bsp file with a static mesh per brush entity and its entities. Dragged into a level it
 * becomes an AQuakeMapActor with an actor per entity. The file stays the source: everything in Generated is
 * rebuilt on every reimport, and placed maps follow.
 */
UCLASS(BlueprintType)
class LIBQUAKE_API UQuakeMap : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Import")
	FQuakeMapSettings Settings;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated")
	TArray<TObjectPtr<UStaticMesh>> Meshes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Generated")
	TArray<FQuakeMapEntity> Entities;

	/** bMeshLibrary or the worldspawn key "_meshlib" ("qu_meshlib" in older maps). */
	UFUNCTION(BlueprintPure, Category = "Quake")
	bool IsMeshLibrary() const;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Instanced, Category = "Import")
	TObjectPtr<UAssetImportData> AssetImportData;

	virtual void PostInitProperties() override;
	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
#endif
};
