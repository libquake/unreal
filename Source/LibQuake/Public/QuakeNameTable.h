#pragma once

#include "CoreMinimal.h"

namespace LibQuake
{
	/**
	 * The entry for a Quake name (an entity class or a texture) in a table of names and prefixes. A key ending in
	 * '*' is a prefix: "light*" stands for "light", "light_fluoro" and so on. An exact name wins over a prefix, a
	 * longer prefix over a shorter one, case does not matter. Null if no entry matches.
	 */
	template <typename ValueType>
	const ValueType* FindByName(const TMap<FString, ValueType>& Table, const FString& Name)
	{
		if (const ValueType* Exact = Table.Find(Name))
		{
			return Exact;
		}

		const ValueType* Best = nullptr;
		int32 BestLength = -1;
		for (const auto& [Key, Value] : Table)
		{
			const int32 Length = Key.Len() - 1;
			if (Key.EndsWith(TEXT("*")) && Length > BestLength && Name.StartsWith(Key.LeftChop(1)))
			{
				Best = &Value;
				BestLength = Length;
			}
		}
		return Best;
	}
}
