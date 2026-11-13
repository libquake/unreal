#include "LibQuakeEditorModule.h"

#include "ContentBrowserMenuContexts.h"
#include "Editor.h"
#include "MessageLogModule.h"
#include "Modules/ModuleManager.h"
#include "QuakeMap.h"
#include "QuakeMapPlacement.h"
#include "Selection.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "LibQuakeEditor"

namespace
{
	/** Places the selected maps at the world origin of the level being edited and selects them. */
	void PlaceSelectedMaps(const FToolMenuContext& MenuContext)
	{
		const UContentBrowserAssetContextMenuContext* Context =
			UContentBrowserAssetContextMenuContext::FindContextWithAssets(MenuContext);
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!Context || !World)
		{
			return;
		}

		GEditor->SelectNone(false, true);
		for (UQuakeMap* Map : Context->LoadSelectedObjects<UQuakeMap>())
		{
			if (AActor* Placed = LibQuakeEditor::PlaceMap(Map, World, FTransform::Identity))
			{
				GEditor->SelectActor(Placed, true, true);
			}
		}
	}

	void RegisterMenus()
	{
		UToolMenu* Menu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UQuakeMap::StaticClass());
		FToolMenuSection& Section = Menu->FindOrAddSection("GetAssetActions");

		FToolUIAction Action;
		Action.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&PlaceSelectedMaps);
		Section.AddMenuEntry("LibQuake_PlaceInLevel", LOCTEXT("PlaceInLevel", "Place in Level"),
			LOCTEXT("PlaceInLevelTooltip", "Places the map as a Level Instance. The first time this makes the map's "
				"level, after that the map can also be dragged into the viewport."),
			FSlateIcon(), Action);
	}
}

void FLibQuakeEditorModule::StartupModule()
{
	FMessageLogModule& MessageLog = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	MessageLog.RegisterLogListing(LibQuakeEditor::LogName, LOCTEXT("LogLabel", "LibQuake"));

	MenuCallback = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([this]
	{
		FToolMenuOwnerScoped Owner(this);
		RegisterMenus();
	}));
}

void FLibQuakeEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(MenuCallback);
	UToolMenus::UnregisterOwner(this);
	if (FMessageLogModule* MessageLog = FModuleManager::GetModulePtr<FMessageLogModule>("MessageLog"))
	{
		MessageLog->UnregisterLogListing(LibQuakeEditor::LogName);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLibQuakeEditorModule, LibQuakeEditor);
