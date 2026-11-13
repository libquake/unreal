#pragma once

#include "CoreMinimal.h"
#include "LibQuakeCore.h"

struct FKConvexElem;
struct FMeshDescription;

/**
 * libquake geometry in Unreal terms. Used by the editor import and by loading at runtime, so both end up with
 * the same meshes.
 */
namespace LibQuake
{
	/** Unreal units (cm) per Quake unit unless a map says otherwise: 32 Quake units are about a meter. */
	inline constexpr float DefaultScale = 3.125f;

	/** Both are z up, Quake is right handed and Unreal left handed: (x, y, z) -> (x, -y, z). */
	inline FVector3f ToUnrealDirection(const quakelib::Vec3& V)
	{
		return FVector3f(V.X, -V.Y, V.Z);
	}

	inline FVector3f ToUnrealPosition(const quakelib::Vec3& V, float Scale)
	{
		return ToUnrealDirection(V) * Scale;
	}

	/** For positions read from entity keys. */
	inline FVector ToUnrealPosition(const FVector3f& Quake, float Scale)
	{
		return FVector(Quake.X, -Quake.Y, Quake.Z) * Scale;
	}

	/**
	 * Quake angles (pitch yaw roll, degrees) as an Unreal rotation. A positive Quake pitch looks down and the yaw
	 * turns counterclockwise seen from above, the mirrored y axis turns all three around.
	 */
	inline FRotator ToUnrealRotation(const FVector& QuakeAngles)
	{
		return FRotator(-QuakeAngles.X, -QuakeAngles.Y, -QuakeAngles.Z);
	}

	/**
	 * The surfaces of an entity of one type, one polygon group per texture with the texture name as material slot.
	 * SOLID are the visible ones, CLIP the shape of triggers and clip brushes in a bsp, which has no brush hulls.
	 * UV0 is the texture in repeats, UV1 the lightmap. Returns false if the entity has no such surfaces.
	 */
	LIBQUAKE_API bool BuildMeshDescription(const quakelib::EntityGeometry& Geometry, float Scale,
		FMeshDescription& OutMesh, quakelib::SurfaceType Type = quakelib::SurfaceType::SOLID);

	/**
	 * The brush hulls as one mesh, for brush entities without visible faces like triggers: they need a shape for
	 * collision and overlaps but nothing to draw. Returns false if the entity has no hulls.
	 */
	LIBQUAKE_API bool BuildHullMeshDescription(const quakelib::EntityGeometry& Geometry, float Scale,
		FMeshDescription& OutMesh);

	/** One convex element per brush hull of the given types. */
	LIBQUAKE_API TArray<FKConvexElem> BuildConvexElements(const quakelib::EntityGeometry& Geometry, float Scale,
		TFunctionRef<bool(quakelib::CollisionHull::Type)> Include);
}
