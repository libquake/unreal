#include "QuakeEntityClasses.h"

#include "LibQuakeSettings.h"
#include "QuakeNameTable.h"

bool UQuakeEntityClasses::Resolve(const UQuakeEntityClasses* MapClasses, const FString& ClassName, UClass*& OutClass)
{
	const TSoftClassPtr<AActor>* Entry = MapClasses ? LibQuake::FindByName(MapClasses->Classes, ClassName) : nullptr;
	if (!Entry)
	{
		Entry = LibQuake::FindByName(GetDefault<ULibQuakeSettings>()->EntityClasses, ClassName);
	}
	if (!Entry)
	{
		return false;
	}

	OutClass = Entry->IsNull() ? nullptr : Entry->LoadSynchronous();
	return true;
}
