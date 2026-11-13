#include "LibQuakeSettings.h"

#include "Engine/Texture2D.h"
#include "QuakeEntityActors.h"

ULibQuakeSettings::ULibQuakeSettings()
{
	EntityClasses.Add(TEXT("light*"), AQuakePointLight::StaticClass());
	EntityClasses.Add(TEXT("info_player_start"), AQuakePlayerStart::StaticClass());
	MissingTexture = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(TEXT("/LibQuake/Textures/T_QuakeMissing.T_QuakeMissing")));
}
