#include "ActorFactoryQuakeMap.h"
#include "AssetToolsModule.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Editor.h"
#include "EditorReimportHandler.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/BodySetup.h"
#include "QuakeEntityActors.h"
#include "QuakeEntityClasses.h"
#include "QuakeEntityComponent.h"
#include "QuakeMap.h"
#include "QuakeMapActor.h"
#include "QuakeMapFactory.h"
#include "QuakeMapPlacement.h"
#include "QuakeTriggerProbe.h"
#include "QuakeWad.h"
#include "QuakeWadFactory.h"
#include "UObject/Package.h"

namespace
{
	FString TestFile(const TCHAR* Name)
	{
		const FString Base = IPluginManager::Get().FindPlugin(TEXT("LibQuake"))->GetBaseDir();
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(Base, TEXT("Tests"), TEXT("Data"), Name));
	}

	UQuakeMap* ImportTestMap(const TCHAR* Name, const TCHAR* File = TEXT("sky_alpha.map"))
	{
		bool bCanceled = false;
		NewObject<UQuakeWadFactory>()->FactoryCreateFile(UQuakeWad::StaticClass(),
			CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha")), TEXT("sky_alpha"), RF_Public | RF_Standalone,
			TestFile(TEXT("sky_alpha.wad")), nullptr, GWarn, bCanceled);
		return Cast<UQuakeMap>(NewObject<UQuakeMapFactory>()->FactoryCreateFile(UQuakeMap::StaticClass(),
			CreatePackage(*(FString(TEXT("/Game/LibQuakeTests/")) + Name)), Name, RF_Public | RF_Standalone,
			TestFile(File), nullptr, GWarn, bCanceled));
	}

	AActor* FindEntity(const TArray<AActor*>& Actors, const TCHAR* ClassName)
	{
		AActor* const* Found = Actors.FindByPredicate([ClassName](const AActor* Actor)
		{
			const UQuakeEntityComponent* Entity = Actor->FindComponentByClass<UQuakeEntityComponent>();
			return Entity && Entity->ClassName == ClassName;
		});
		return Found ? *Found : nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeMapImportTest, "LibQuake.Editor.ImportMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeMapImportTest::RunTest(const FString& Parameters)
{
	bool bCanceled = false;
	UQuakeWad* Wad = Cast<UQuakeWad>(NewObject<UQuakeWadFactory>()->FactoryCreateFile(UQuakeWad::StaticClass(),
		CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha")), TEXT("sky_alpha"), RF_Public | RF_Standalone,
		TestFile(TEXT("sky_alpha.wad")), nullptr, GWarn, bCanceled));
	if (!TestNotNull(TEXT("wad imports"), Wad))
	{
		return false;
	}

	UQuakeMapFactory* Factory = NewObject<UQuakeMapFactory>();
	UQuakeMap* Map = Cast<UQuakeMap>(Factory->FactoryCreateFile(UQuakeMap::StaticClass(),
		CreatePackage(TEXT("/Game/LibQuakeTests/sky_alpha_map")), TEXT("sky_alpha_map"), RF_Public | RF_Standalone,
		TestFile(TEXT("sky_alpha.map")), nullptr, GWarn, bCanceled));
	if (!TestNotNull(TEXT("map imports"), Map))
	{
		return false;
	}

	TestTrue(TEXT("worldspawn and the fence have meshes"), Map->Meshes.Num() >= 2);
	for (const UStaticMesh* Mesh : Map->Meshes)
	{
		TestTrue(TEXT("mesh has convex collision"), Mesh->GetBodySetup()->AggGeom.ConvexElems.Num() > 0);
		TestEqual(TEXT("lightmap uvs in channel 1"), Mesh->GetLightMapCoordinateIndex(), 1);
		for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
		{
			TestNotNull(TEXT("every slot has a material"), Material.MaterialInterface.Get());
		}
	}

	bool bSkyFound = false;
	for (const UStaticMesh* Mesh : Map->Meshes)
	{
		const TArray<FStaticMaterial>& Materials = Mesh->GetStaticMaterials();
		for (int32 Section = 0; Section < Materials.Num(); Section++)
		{
			const UMaterialInterface* Material = Materials[Section].MaterialInterface;
			if (!Material || Material->GetMaterial()->GetName() != TEXT("M_QuakeSky"))
			{
				continue;
			}
			bSkyFound = true;
			TestFalse(TEXT("sky casts no shadow"), Mesh->GetSectionInfoMap().Get(0, Section).bCastShadow);
		}
	}
	TestTrue(TEXT("sky1 gets the sky material"), bSkyFound);

	// Auto Reimport and the Reimport menu only handle assets whose type says they are imported
	const TSharedPtr<IAssetTypeActions> Actions = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools")
		.Get().GetAssetTypeActionsForClass(UQuakeMap::StaticClass()).Pin();
	TestTrue(TEXT("the map type is imported"), Actions.IsValid() && Actions->IsImportedAsset());
	TestTrue(TEXT("Unreal finds the reimport"), FReimportManager::Instance()->CanReimport(Map));

	const TArray<TObjectPtr<UStaticMesh>> Before = Map->Meshes;
	TestEqual(TEXT("reimport succeeds"), Factory->Reimport(Map), EReimportResult::Succeeded);
	TestTrue(TEXT("reimport keeps the meshes"), Map->Meshes == Before);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeMapPlaceTest, "LibQuake.Editor.PlaceMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeMapPlaceTest::RunTest(const FString& Parameters)
{
	UQuakeMap* Map = ImportTestMap(TEXT("sky_alpha_place"));
	if (!TestNotNull(TEXT("map imports"), Map))
	{
		return false;
	}

	FText Error;
	TestTrue(TEXT("a map can be dragged into a level"),
		GetMutableDefault<UActorFactoryQuakeMap>()->CanCreateActorFrom(FAssetData(Map), Error));

	UWorld* World = GEditor->GetEditorWorldContext().World();
	const FVector Offset(1000, 2000, 300);
	AQuakeMapActor* MapActor = LibQuakeEditor::PlaceMap(Map, World, FTransform(Offset));
	if (!TestNotNull(TEXT("map is placed"), MapActor))
	{
		return false;
	}
	const TArray<AActor*> Actors = MapActor->GetEntityActors();
	int32 Meshes = 0;
	int32 Lights = 0;
	for (const AActor* Actor : Actors)
	{
		Meshes += Actor->IsA<AStaticMeshActor>();
		Lights += Actor->IsA<AQuakePointLight>();
	}
	TestEqual(TEXT("an actor per mesh"), Meshes, Map->Meshes.Num());
	TestTrue(TEXT("lights get the plugin's light"), Lights > 0);
	TestEqual(TEXT("an actor per entity"), Actors.Num(), Map->Entities.Num());
	for (const AActor* Actor : Actors)
	{
		const UQuakeEntityComponent* Entity = Actor->FindComponentByClass<UQuakeEntityComponent>();
		TestFalse(TEXT("every actor has an id"), Entity->EntityId.IsEmpty());
		TestEqual(TEXT("classname is among the keys"), Entity->GetKey(TEXT("classname")), Entity->ClassName);
	}

	const FQuakeMapEntity* Light = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("light"); });
	const AActor* LightActor = FindEntity(Actors, TEXT("light"));
	TestEqual(TEXT("light sits at its origin, relative to the map"), LightActor->GetActorLocation(),
		Light->Transform.GetLocation() + Offset);

	// "light" "300": the static light keeps the values even though Unreal's setters ignore static lights
	const UPointLightComponent* LightComponent = Cast<APointLight>(LightActor)->PointLightComponent;
	TestFalse(TEXT("fades like quake, not by the inverse square"), LightComponent->bUseInverseSquaredFalloff);
	TestEqual(TEXT("level 150 at the center after quake's range scale"), (double)LightComponent->Intensity,
		150.0 / 128.0 * UE_DOUBLE_PI * 1.2, 0.001);
	TestEqual(TEXT("radius is the light value"), (double)LightComponent->AttenuationRadius,
		300.0 * Map->Settings.Scale, 0.01);

	const FQuakeMapEntity* Start = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("info_player_start"); });
	const APlayerStart* StartActor = Cast<APlayerStart>(FindEntity(Actors, TEXT("info_player_start")));
	if (TestNotNull(TEXT("player start"), StartActor))
	{
		const double Feet = StartActor->GetActorLocation().Z -
			StartActor->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		TestEqual(TEXT("player start stands where the Quake player's feet are"), Feet,
			Start->Transform.GetLocation().Z + Offset.Z - 24.0 * Map->Settings.Scale, 0.01);
	}

	World->EditorDestroyActor(MapActor, true);
	TestTrue(TEXT("deleting the map deletes its entities"),
		!Actors.ContainsByPredicate([](const AActor* Actor) { return IsValid(Actor); }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeMapSyncTest, "LibQuake.Editor.SyncMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeMapSyncTest::RunTest(const FString& Parameters)
{
	UQuakeMap* Map = ImportTestMap(TEXT("sky_alpha_sync"));
	if (!TestNotNull(TEXT("map imports"), Map))
	{
		return false;
	}
	UWorld* World = GEditor->GetEditorWorldContext().World();
	AQuakeMapActor* MapActor = LibQuakeEditor::PlaceMap(Map, World, FTransform::Identity);
	AActor* Light = FindEntity(MapActor->GetEntityActors(), TEXT("light"));
	AActor* Start = FindEntity(MapActor->GetEntityActors(), TEXT("info_player_start"));
	const int32 Before = MapActor->GetEntityActors().Num();

	// what a reimport after editing the map does: the light moved, the start is gone, a new entity came
	FQuakeMapEntity* LightEntity = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("light"); });
	LightEntity->Transform.AddToTranslation(FVector(0, 0, 100));
	Map->Entities.RemoveAll(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("info_player_start"); });
	FQuakeMapEntity& Added = Map->Entities.AddDefaulted_GetRef();
	Added.ClassName = TEXT("info_null");
	Added.Keys.Add(TEXT("classname"), Added.ClassName);
	LightEntity = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("light"); });
	LibQuakeEditor::SyncMapActor(MapActor);

	const TArray<AActor*> After = MapActor->GetEntityActors();
	TestEqual(TEXT("the light keeps its actor"), FindEntity(After, TEXT("light")), Light);
	TestEqual(TEXT("and follows the map"), Light->GetActorLocation(), LightEntity->Transform.GetLocation());
	TestFalse(TEXT("the removed start loses its actor"), IsValid(Start));
	TestNotNull(TEXT("the new entity gets one"), Cast<AQuakePointEntity>(FindEntity(After, TEXT("info_null"))));
	TestEqual(TEXT("one gone, one new"), After.Num(), Before);

	// mapping the light to another class replaces its actor
	UQuakeEntityClasses* Classes = NewObject<UQuakeEntityClasses>();
	Classes->Classes.Add(TEXT("light"), AQuakePointEntity::StaticClass());
	Map->Settings.EntityClasses = Classes;
	LibQuakeEditor::SyncMapActor(MapActor);
	TestFalse(TEXT("the old light is gone"), IsValid(Light));
	TestNotNull(TEXT("the light is a point entity now"),
		Cast<AQuakePointEntity>(FindEntity(MapActor->GetEntityActors(), TEXT("light"))));

	World->EditorDestroyActor(MapActor, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeEntityClassesTest, "LibQuake.Editor.EntityClasses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeEntityClassesTest::RunTest(const FString& Parameters)
{
	TMap<FString, TSoftClassPtr<AActor>> Classes;
	Classes.Add(TEXT("light*"), AQuakePointLight::StaticClass());
	Classes.Add(TEXT("light_torch*"), AQuakePointEntity::StaticClass());
	Classes.Add(TEXT("light_torch_small_walltorch"), APlayerStart::StaticClass());
	TestTrue(TEXT("prefix"),
		UQuakeEntityClasses::Find(Classes, TEXT("light_fluoro")).Get() == AQuakePointLight::StaticClass());
	TestTrue(TEXT("the prefix without a suffix"),
		UQuakeEntityClasses::Find(Classes, TEXT("light")).Get() == AQuakePointLight::StaticClass());
	TestTrue(TEXT("the longer prefix wins"),
		UQuakeEntityClasses::Find(Classes, TEXT("light_torch_x")).Get() == AQuakePointEntity::StaticClass());
	TestTrue(TEXT("an exact name wins"),
		UQuakeEntityClasses::Find(Classes, TEXT("light_torch_small_walltorch")).Get() == APlayerStart::StaticClass());
	TestTrue(TEXT("no match"), UQuakeEntityClasses::Find(Classes, TEXT("info_null")).IsNull());
	TestTrue(TEXT("the project settings bring the lights"),
		UQuakeEntityClasses::Resolve(nullptr, TEXT("light")) == AQuakePointLight::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeTriggerTest, "LibQuake.Editor.Triggers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeTriggerTest::RunTest(const FString& Parameters)
{
	UQuakeMap* Map = ImportTestMap(TEXT("trigger_map"), TEXT("trigger.map"));
	if (!TestNotNull(TEXT("map imports"), Map))
	{
		return false;
	}
	const FQuakeMapEntity* TriggerEntity = Map->Entities.FindByPredicate(
		[](const FQuakeMapEntity& Entity) { return Entity.ClassName == TEXT("trigger_once"); });
	if (!TestNotNull(TEXT("trigger is kept"), TriggerEntity))
	{
		return false;
	}
	TestTrue(TEXT("trigger has a hull mesh"), Map->Meshes.IsValidIndex(TriggerEntity->Mesh));
	TestTrue(TEXT("and is invisible"), TriggerEntity->bInvisible);

	UWorld* World = GEditor->GetEditorWorldContext().World();
	AQuakeMapActor* MapActor = LibQuakeEditor::PlaceMap(Map, World, FTransform::Identity);
	const TArray<AActor*> Actors = MapActor->GetEntityActors();
	AActor* Trigger = FindEntity(Actors, TEXT("trigger_once"));
	AActor* Wall = FindEntity(Actors, TEXT("func_wall"));
	AActor* Null = FindEntity(Actors, TEXT("info_null"));
	if (!TestNotNull(TEXT("trigger"), Trigger) || !TestNotNull(TEXT("wall"), Wall) || !TestNotNull(TEXT("null"), Null))
	{
		return false;
	}

	const UStaticMeshComponent* Shape = Trigger->FindComponentByClass<UStaticMeshComponent>();
	TestFalse(TEXT("trigger is not drawn"), Shape->IsVisible());
	TestEqual(TEXT("trigger only overlaps"), Shape->GetCollisionProfileName(), FName(TEXT("Trigger")));
	TestTrue(TEXT("trigger reports overlaps"), Shape->GetGenerateOverlapEvents());
	TestTrue(TEXT("the wall stays visible"), Wall->FindComponentByClass<UStaticMeshComponent>()->IsVisible());

	// brush entities have their origin at the bottom center by default, the actor stands there
	const FBox Bounds = Shape->GetStaticMesh()->GetBoundingBox();
	TestEqual(TEXT("mesh starts at the actor's height"), Bounds.Min.Z, 0.0, 0.01);
	TestTrue(*FString::Printf(TEXT("mesh is centered on the actor, not %s"), *Bounds.GetCenter().ToString()),
		FVector2D(Bounds.GetCenter()).Equals(FVector2D::ZeroVector, 0.01));
	// the wall spans x 96..112 in Quake units, its bottom center is at 104 0 0
	TestEqual(TEXT("the wall stands at its bottom center"), Wall->GetActorLocation(),
		FVector(104 * Map->Settings.Scale, 0, 0));

	UQuakeTriggerProbe* WallProbe = NewObject<UQuakeTriggerProbe>();
	UQuakeTriggerProbe* NullProbe = NewObject<UQuakeTriggerProbe>();
	Wall->FindComponentByClass<UQuakeEntityComponent>()->OnTriggered.AddDynamic(WallProbe,
		&UQuakeTriggerProbe::OnTriggered);
	Null->FindComponentByClass<UQuakeEntityComponent>()->OnTriggered.AddDynamic(NullProbe,
		&UQuakeTriggerProbe::OnTriggered);
	Trigger->FindComponentByClass<UQuakeEntityComponent>()->Fire(nullptr);
	TestEqual(TEXT("the wall is triggered"), WallProbe->Count, 1);
	TestEqual(TEXT("the info_null is triggered too"), NullProbe->Count, 1);
	TestEqual(TEXT("info_null keeps its origin"), Null->GetActorLocation(),
		FVector(-96 * Map->Settings.Scale, 0, 32 * Map->Settings.Scale));

	// "90 -45 0": the sun shines towards Quake's +y at 45 degrees, which is Unreal's -y
	const AActor* const* Sun = Actors.FindByPredicate([](const AActor* Actor) { return Actor->IsA<AQuakeSun>(); });
	if (TestNotNull(TEXT("worldspawn sun"), Sun))
	{
		const FVector Direction = (*Sun)->GetActorForwardVector();
		TestTrue(*FString::Printf(TEXT("sun points down and to -y, not %s"), *Direction.ToString()),
			Direction.Equals(FVector(0, -1, -1).GetSafeNormal(), 0.01));
		// "_sunlight" "200" is level 100 after Quake's range scale of 0.5
		TestEqual(TEXT("sun brightness"), (double)Cast<ADirectionalLight>(*Sun)->GetLightComponent()->Intensity,
			100.0 / 128.0 * UE_DOUBLE_PI * 1.2, 0.001);
	}

	World->EditorDestroyActor(MapActor, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FQuakeBspImportTest, "LibQuake.Editor.ImportBsp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FQuakeBspImportTest::RunTest(const FString& Parameters)
{
	bool bCanceled = false;
	UQuakeMap* Map = Cast<UQuakeMap>(NewObject<UQuakeMapFactory>()->FactoryCreateFile(UQuakeMap::StaticClass(),
		CreatePackage(TEXT("/Game/LibQuakeTests/lightstyles")), TEXT("lightstyles"), RF_Public | RF_Standalone,
		TestFile(TEXT("lightstyles.bsp")), nullptr, GWarn, bCanceled));
	if (!TestNotNull(TEXT("bsp imports"), Map))
	{
		return false;
	}

	TestTrue(TEXT("bsp has meshes"), Map->Meshes.Num() > 0);
	TestTrue(TEXT("bsp has entities"), Map->Entities.Num() > 1);
	int32 Textured = 0;
	for (const UStaticMesh* Mesh : Map->Meshes)
	{
		TestEqual(TEXT("a bsp mesh is its own collision"), Mesh->GetBodySetup()->CollisionTraceFlag.GetValue(),
			CTF_UseComplexAsSimple);
		for (const FStaticMaterial& Material : Mesh->GetStaticMaterials())
		{
			UTexture* Albedo = nullptr;
			if (Material.MaterialInterface && Material.MaterialInterface->GetTextureParameterValue(
				FHashedMaterialParameterInfo(TEXT("Albedo")), Albedo) && Albedo)
			{
				TestTrue(TEXT("textures come from the bsp"),
					Albedo->GetPathName().StartsWith(TEXT("/Game/LibQuakeTests/lightstyles_Generated/T_")));
				Textured++;
			}
		}
	}
	TestTrue(TEXT("materials have the bsp textures"), Textured > 0);
	return true;
}
