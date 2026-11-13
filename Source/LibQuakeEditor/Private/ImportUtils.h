#pragma once

#include "CoreMinimal.h"

class UPackage;

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

	/** Asset and package name for a Quake name, which may contain characters assets cannot have ('*', '{'). */
	FString UniqueAssetName(const FString& QuakeName, TSet<FString>& Used);

	void ReportError(const FString& File, const FString& Error);
	void ReportWarning(const FString& File, const FString& Warning);

	/** Shows the message log notification if this import reported anything. */
	void NotifyIfReported();
}
