#pragma once

#include "CoreMinimal.h"
#include "EditorReimportHandler.h"
#include "Factories/Factory.h"

#include "QuakeWadFactory.generated.h"

class UQuakeWad;

/**
 * Imports .wad files as a UQuakeWad plus one texture asset per texture and layer in "<wad>_Textures". A reimport
 * updates the existing textures, so references to them survive. A wad libquake cannot read leaves the asset as it
 * was and reports to the LibQuake message log.
 */
UCLASS()
class UQuakeWadFactory : public UFactory, public FReimportHandler
{
	GENERATED_BODY()

public:
	UQuakeWadFactory();

	virtual UObject* FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
		const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled) override;

	virtual bool CanReimport(UObject* Obj, TArray<FString>& OutFilenames) override;
	virtual void SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths) override;
	virtual EReimportResult::Type Reimport(UObject* Obj) override;
	virtual int32 GetPriority() const override;

	/** Reads the wad and updates Wad from it. Returns false and leaves Wad alone if the file cannot be read. */
	static bool ImportWad(UQuakeWad* Wad, const FString& Filename);
};
