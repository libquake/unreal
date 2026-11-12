#pragma once

#include "CoreMinimal.h"

THIRD_PARTY_INCLUDES_START
#include <quakelib/bsp/bsp_level.h>
#include <quakelib/map/map_level.h>
THIRD_PARTY_INCLUDES_END

namespace LibQuake
{
	/** A texture of a wad file, RGBA rows from top to bottom. */
	struct FWadTexture
	{
		FString Name;
		int32 Width = 0;
		int32 Height = 0;
		TArray<uint8> Rgba;
		/** Fullbright pixels, black elsewhere. Empty if the texture has none. */
		TArray<uint8> Emission;
		/** Cloud layer of a sky, alpha 0 where the back layer (Rgba) shows. Empty for other textures. */
		TArray<uint8> SkyFront;
		/** '{' texture, transparent where alpha is 0. */
		bool bAlpha = false;
	};

	/** Loads a compiled bsp (version 29) and its .lit file. On failure returns false and sets OutError. */
	LIBQUAKECORE_API bool LoadBsp(const FString& Path, const quakelib::BspLoadOptions& Options,
		quakelib::Level& OutLevel, FString& OutError);

	/** Parses a .map file. OutDocument lists its textures and wads, which BuildMap needs first. */
	LIBQUAKECORE_API bool ParseMap(const FString& Path, quakelib::MapDocument& OutDocument, FString& OutError);

	/** Builds meshes, collision hulls and, if asked for, the baked lightmap of a parsed map. */
	LIBQUAKECORE_API bool BuildMap(const quakelib::MapDocument& Document, const quakelib::MapBuildOptions& Options,
		quakelib::Level& OutLevel, FString& OutError);

	/** Loads every texture of a WAD2 file. */
	LIBQUAKECORE_API bool LoadWad(const FString& Path, TArray<FWadTexture>& OutTextures, FString& OutError);
}
