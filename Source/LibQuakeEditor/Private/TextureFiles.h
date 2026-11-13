#pragma once

#include "CoreMinimal.h"
#include "QuakeWad.h"

class UTexture2D;

namespace LibQuakeEditor
{
	/**
	 * Textures from image files, found like TrenchBroom does: "base/wall1" is textures/base/wall1.png (or .tga,
	 * .jpg) in the folder of the map or a folder above it. Each file becomes a texture asset in a folder shared by
	 * all maps, with the file as its source, and is written again when the file has changed. "<name>_luma" or
	 * "<name>_glow" next to it is the emission, "<name>_front" the cloud layer of a sky.
	 */
	class FTextureFiles
	{
	public:
		/** Looks for "textures" folders from the map's folder up. AssetFolder is where the textures are written. */
		void Init(const FString& MapFilename, const FString& AssetFolder);

		/** The texture of an image file, null if there is no file or it cannot be read. */
		const FQuakeWadTexture* Find(const FString& Name);

	private:
		/** The image file for a texture name, empty if there is none. */
		FString FindFile(const FString& Name) const;

		/** The texture asset of an image file, written if the file is new or has changed. */
		UTexture2D* Load(const FString& File, const FString& Name, bool& bOutAlpha);

		FString MapFilename;
		FString AssetFolder;

		/** The "textures" folders, the nearest first. */
		TArray<FString> Roots;

		/** Every name asked for so far, null for the ones without a file. */
		TMap<FString, TUniquePtr<FQuakeWadTexture>> Entries;
	};
}
