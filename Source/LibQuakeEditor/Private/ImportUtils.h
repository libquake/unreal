#pragma once

#include "CoreMinimal.h"

class UPackage;
class UTexture2D;

namespace LibQuakeEditor
{
	/** The package if it is loaded or on disk, a new one otherwise. Reimports reuse their assets this way. */
	UPackage* FindOrCreatePackage(const FString& PackageName);

	/** The object of that class and name in the package, or a new public one. */
	template <typename T>
	T* FindOrCreateAsset(UPackage* Package, const FString& Name, bool& bOutCreated)
	{
		T* Asset = FindObject<T>(Package, *Name);
		bOutCreated = Asset == nullptr;
		return bOutCreated ? NewObject<T>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional) : Asset;
	}

	/** Writes RGBA pixels (rows top to bottom) as a texture asset, point filtered and uncompressed like in Quake. */
	UTexture2D* WriteTexture(const FString& Folder, const FString& Name, int32 Width, int32 Height,
		const TArray<uint8>& Rgba);

	/** Asset and package name for a Quake name, which may contain characters assets cannot have ('*', '{'). */
	FString UniqueAssetName(const FString& QuakeName, TSet<FString>& Used);

	/** "x y z" from an entity key, missing values are 0. */
	FVector3f ParseVector(const FString* Value);

	void ReportError(const FString& File, const FString& Error);
	void ReportWarning(const FString& File, const FString& Warning);

	/** Shows the message log notification if this import reported anything. */
	void NotifyIfReported();
}
