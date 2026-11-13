#include "QuakeEntityClasses.h"

#include "LibQuakeSettings.h"

TSoftClassPtr<AActor> UQuakeEntityClasses::Find(const TMap<FString, TSoftClassPtr<AActor>>& Classes,
	const FString& ClassName)
{
	if (const TSoftClassPtr<AActor>* Exact = Classes.Find(ClassName))
	{
		return *Exact;
	}

	TSoftClassPtr<AActor> Best;
	int32 BestLength = -1;
	for (const auto& [Name, Class] : Classes)
	{
		const int32 Length = Name.Len() - 1;
		if (Name.EndsWith(TEXT("*")) && Length > BestLength && ClassName.StartsWith(Name.LeftChop(1)))
		{
			Best = Class;
			BestLength = Length;
		}
	}
	return Best;
}

UClass* UQuakeEntityClasses::Resolve(const UQuakeEntityClasses* MapClasses, const FString& ClassName)
{
	TSoftClassPtr<AActor> Class = MapClasses ? Find(MapClasses->Classes, ClassName) : nullptr;
	if (Class.IsNull())
	{
		Class = Find(GetDefault<ULibQuakeSettings>()->EntityClasses, ClassName);
	}
	return Class.IsNull() ? nullptr : Class.LoadSynchronous();
}
