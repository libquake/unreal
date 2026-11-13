#include "LibQuakeSettings.h"

#include "QuakeEntityActors.h"

ULibQuakeSettings::ULibQuakeSettings()
{
	EntityClasses.Add(TEXT("light*"), AQuakePointLight::StaticClass());
	EntityClasses.Add(TEXT("info_player_start"), AQuakePlayerStart::StaticClass());
}
