#include "ImportUtils.h"

#include "LibQuakeEditorModule.h"
#include "Logging/MessageLog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "ObjectTools.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "LibQuakeImport"

namespace LibQuakeEditor
{
	UPackage* FindOrCreatePackage(const FString& PackageName)
	{
		if (UPackage* Loaded = FindPackage(nullptr, *PackageName))
		{
			return Loaded;
		}
		if (FPackageName::DoesPackageExist(PackageName))
		{
			return LoadPackage(nullptr, *PackageName, LOAD_None);
		}
		return CreatePackage(*PackageName);
	}

	FString UniqueAssetName(const FString& QuakeName, TSet<FString>& Used)
	{
		// the name is used for the package too, which forbids more characters than objects do ('*' for liquids)
		const FString Base =
			ObjectTools::SanitizeInvalidChars(ObjectTools::SanitizeObjectName(QuakeName), INVALID_LONGPACKAGE_CHARACTERS);

		// collisions get a number in the order of the source, which is stable, so reimports keep their names
		FString Name = Base;
		for (int32 Suffix = 1; Used.Contains(Name); Suffix++)
		{
			Name = FString::Printf(TEXT("%s_%d"), *Base, Suffix);
		}
		Used.Add(Name);
		return Name;
	}

	FVector3f ParseVector(const FString* Value)
	{
		FVector3f Result = FVector3f::ZeroVector;
		if (Value)
		{
			TArray<FString> Parts;
			Value->ParseIntoArrayWS(Parts);
			for (int32 i = 0; i < 3 && i < Parts.Num(); i++)
			{
				Result[i] = FCString::Atof(*Parts[i]);
			}
		}
		return Result;
	}

	void ReportError(const FString& File, const FString& Error)
	{
		FMessageLog(LogName).Error(FText::Format(LOCTEXT("Message", "{0}: {1}"),
			FText::FromString(FPaths::GetCleanFilename(File)), FText::FromString(Error)));
	}

	void ReportWarning(const FString& File, const FString& Warning)
	{
		FMessageLog(LogName).Warning(FText::Format(LOCTEXT("Message", "{0}: {1}"),
			FText::FromString(FPaths::GetCleanFilename(File)), FText::FromString(Warning)));
	}

	void NotifyIfReported()
	{
		FMessageLog(LogName).Notify(LOCTEXT("Notify", "Quake import reported problems"), EMessageSeverity::Warning);
	}
}

#undef LOCTEXT_NAMESPACE
