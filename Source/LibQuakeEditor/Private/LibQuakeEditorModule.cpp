#include "LibQuakeEditorModule.h"

#include "ContentBrowserMenuContexts.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Logging/MessageLog.h"
#include "MessageLogModule.h"
#include "Misc/CoreDelegates.h"
#include "Modules/ModuleManager.h"
#include "QuakeMap.h"
#include "QuakeMapActor.h"
#include "QuakeMapPlacement.h"
#include "ScopedTransaction.h"
#include "Selection.h"
#include "Subsystems/ImportSubsystem.h"
#include "TimerManager.h"
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

		const FScopedTransaction Transaction(LOCTEXT("PlaceMaps", "Place Quake Map"));
		GEditor->SelectNone(false, true);
		for (UQuakeMap* Map : Context->LoadSelectedObjects<UQuakeMap>())
		{
			if (Map->IsMeshLibrary())
			{
				FMessageLog(LibQuakeEditor::LogName).Warning(FText::Format(
					LOCTEXT("MeshLibrary", "{0} is a mesh library, drag its meshes into the level"),
					FText::FromString(Map->GetName())));
				FMessageLog(LibQuakeEditor::LogName).Notify(LOCTEXT("NotPlaced", "Quake map not placed"));
				continue;
			}

			if (AActor* Placed = LibQuakeEditor::PlaceMap(Map, World, FTransform::Identity))
			{
				GEditor->SelectActor(Placed, true, true);
			}
		}
	}

	/** Tools > LibQuake > Add Quake Exposure: the exposure the lights are calibrated to, selected afterwards. */
	void AddQuakeExposure(const FToolMenuContext&)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World)
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("AddExposure", "Add Quake Exposure"));
		if (AActor* Volume = LibQuakeEditor::AddQuakeExposure(World))
		{
			GEditor->SelectNone(false, true);
			GEditor->SelectActor(Volume, true, true);
		}
	}

	void RegisterMenus()
	{
		UToolMenu* AssetMenu = UE::ContentBrowser::ExtendToolMenu_AssetContextMenu(UQuakeMap::StaticClass());
		FToolUIAction Place;
		Place.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&PlaceSelectedMaps);
		AssetMenu->FindOrAddSection("GetAssetActions").AddMenuEntry("LibQuake_PlaceInLevel",
			LOCTEXT("PlaceInLevel", "Place in Level"),
			LOCTEXT("PlaceInLevelTooltip", "Places the map at the origin of the level. Dragging it into the viewport "
				"does the same where it is dropped."),
			FSlateIcon(), Place);

		FToolUIAction AddExposure;
		AddExposure.ExecuteAction = FToolMenuExecuteAction::CreateStatic(&AddQuakeExposure);
		UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		ToolsMenu->FindOrAddSection("LibQuake", LOCTEXT("ToolsSection", "LibQuake")).AddMenuEntry(
			"LibQuake_AddExposure",
			LOCTEXT("AddExposure", "Add Quake Exposure"),
			LOCTEXT("AddExposureTooltip", "Adds a post process volume with the manual exposure the lights of Quake "
				"maps are calibrated to. It belongs to the level, change or delete it like any other volume."),
			FSlateIcon(), AddExposure);
	}

	void RebuildMapActor(AQuakeMapActor* MapActor)
	{
		const FScopedTransaction Transaction(LOCTEXT("RebuildMap", "Rebuild Quake Map"));
		LibQuakeEditor::SyncMapActor(MapActor);
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

	AQuakeMapActor::RebuildHandler = &RebuildMapActor;

	// the editor is made after the modules of the default loading phase
	if (GEditor)
	{
		RegisterEditorHooks();
	}
	else
	{
		FCoreDelegates::GetOnPostEngineInit().AddRaw(this, &FLibQuakeEditorModule::RegisterEditorHooks);
	}
}

void FLibQuakeEditorModule::RegisterEditorHooks()
{
	if (!GEditor)
	{
		return;
	}
	GEditor->GetEditorSubsystem<UImportSubsystem>()->OnAssetReimport.AddRaw(this,
		&FLibQuakeEditorModule::OnAssetReimport);
}

void FLibQuakeEditorModule::ShutdownModule()
{
	AQuakeMapActor::RebuildHandler = nullptr;
	FCoreDelegates::GetOnPostEngineInit().RemoveAll(this);

	if (GEditor)
	{
		if (UImportSubsystem* Imports = GEditor->GetEditorSubsystem<UImportSubsystem>())
		{
			Imports->OnAssetReimport.RemoveAll(this);
		}
	}

	UToolMenus::UnRegisterStartupCallback(MenuCallback);
	UToolMenus::UnregisterOwner(this);
	if (FMessageLogModule* MessageLog = FModuleManager::GetModulePtr<FMessageLogModule>("MessageLog"))
	{
		MessageLog->UnregisterLogListing(LibQuakeEditor::LogName);
	}
}

void FLibQuakeEditorModule::OnAssetReimport(UObject* Asset)
{
	UQuakeMap* Map = Cast<UQuakeMap>(Asset);
	if (!Map)
	{
		return;
	}

	// the reimport is still running, the placed maps follow once it is done
	ReimportedMaps.Add(Map);
	GEditor->GetTimerManager()->SetTimerForNextTick(FTimerDelegate::CreateRaw(this,
		&FLibQuakeEditorModule::SyncReimportedMaps));
}

void FLibQuakeEditorModule::SyncReimportedMaps()
{
	TSet<TWeakObjectPtr<UQuakeMap>> Maps = MoveTemp(ReimportedMaps);
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (Maps.IsEmpty() || !World)
	{
		return;
	}

	for (TActorIterator<AQuakeMapActor> It(World); It; ++It)
	{
		if (Maps.Contains(TWeakObjectPtr<UQuakeMap>(It->Map.Get())))
		{
			LibQuakeEditor::SyncMapActor(*It);
		}
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FLibQuakeEditorModule, LibQuakeEditor);
