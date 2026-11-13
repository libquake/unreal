#pragma once

#include "CoreMinimal.h"

class UQuakeMap;

namespace LibQuakeEditor
{
	/** The folder of the assets made from a map, "<map>_Generated" next to it. */
	FString GeneratedFolder(const UQuakeMap* Map);

	/**
	 * Deletes what earlier imports of the map left in its generated folder and the map no longer uses: meshes
	 * (SM_), material instances (MI_) and bsp textures (T_) of entities, groups or textures that are gone or
	 * renamed. Assets something else still refers to stay, other assets in the folder are not touched. Deleting
	 * runs a garbage collection, so this belongs after an import, not inside it. Returns the number deleted.
	 */
	int32 DeleteUnusedGeneratedAssets(UQuakeMap* Map);
}
