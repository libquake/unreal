#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class UQuakeMap;

namespace LibQuakeEditor
{
	/** Message log for import problems, shown under Window > Message Log > LibQuake. */
	inline const FName LogName = TEXT("LibQuake");
}

class FLibQuakeEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterEditorHooks();

	void OnAssetReimport(UObject* Asset);

	/** Updates the placed maps of the maps reimported since the last tick. */
	void SyncReimportedMaps();

	FDelegateHandle MenuCallback;
	TSet<TWeakObjectPtr<UQuakeMap>> ReimportedMaps;
};
