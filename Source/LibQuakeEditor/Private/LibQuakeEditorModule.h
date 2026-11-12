#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

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
};
