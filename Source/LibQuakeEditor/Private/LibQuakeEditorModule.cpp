#include "LibQuakeEditorModule.h"

#include "MessageLogModule.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "LibQuakeEditor"

void FLibQuakeEditorModule::StartupModule()
{
	FMessageLogModule& MessageLog = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLog.RegisterLogListing(LibQuakeEditor::LogName, LOCTEXT("LogLabel", "LibQuake"));
}

void FLibQuakeEditorModule::ShutdownModule()
{
	if (FMessageLogModule* MessageLog = FModuleManager::GetModulePtr<FMessageLogModule>("MessageLog"))
	{
		MessageLog->UnregisterLogListing(LibQuakeEditor::LogName);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLibQuakeEditorModule, LibQuakeEditor);
