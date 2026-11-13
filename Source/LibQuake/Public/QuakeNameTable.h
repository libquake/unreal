#pragma once

#include "CoreMinimal.h"

namespace LibQuake
{
	/**
	 * The entry for a Quake name (an entity class or a texture) in a table of names and prefixes. A key ending in
	 * '*' is a prefix: "light*" stands for "light", "light_fluoro" and so on, "*" for every name. An exact name wins
	 * over a prefix, a longer prefix over a shorter one, case does not matter. Null if no entry matches.
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
			// StartsWith is false for an empty prefix, "*" alone has to match every name
			const FString Prefix = Key.LeftChop(1);
			const bool bMatches = Prefix.IsEmpty() || Name.StartsWith(Prefix);
			if (Key.EndsWith(TEXT("*")) && Prefix.Len() > BestLength && bMatches)
			{
				Best = &Value;
				BestLength = Prefix.Len();
			}
		}
		return Best;
	}
}
