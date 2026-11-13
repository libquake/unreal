#pragma once

#include "CoreMinimal.h"
#include "EditorReimportHandler.h"
#include "Factories/Factory.h"

#include "QuakeMapFactory.generated.h"

class UQuakeMap;

/**
 * Imports .map files as a UQuakeMap with a static mesh per brush entity, collision from the brush hulls. Textures
 * come from the imported wads, the ones the map names first. Meshes and material instances go to "<map>_Generated",
 * a reimport updates them in place. A map libquake cannot read leaves everything as it was and reports to the
 * LibQuake message log.
 */
UCLASS()
class UQuakeMapFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UQuakeMapFactory();

	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
		const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;

	/** Reads the map and rebuilds everything generated from it. Returns false and changes nothing on errors. */
	static bool ImportMap(UQuakeMap* Map, const FString& Filename);
};
