#include "LibQuakeSettings.h"

#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "QuakeEntityActors.h"

ULibQuakeSettings::ULibQuakeSettings()
{
	EntityClasses.Add(TEXT("light*"), AQuakePointLight::StaticClass());
	EntityClasses.Add(TEXT("info_player_start"), AQuakePlayerStart::StaticClass());

	SurfaceMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/LibQuake/Materials/M_Quake.M_Quake")));
	MaskedMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/LibQuake/Materials/M_QuakeMasked.M_QuakeMasked")));
	SkyMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/LibQuake/Materials/M_QuakeSky.M_QuakeSky")));

	MissingTexture = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(TEXT("/LibQuake/Textures/T_QuakeMissing.T_QuakeMissing")));
}

void ULibQuakeSettings::ApplyDefaults(FQuakeMapSettings& Settings) const
{
	Settings.Scale = DefaultScale;
	Settings.bCSG = bDefaultCSG;
	Settings.Pivot = DefaultPivot;
	Settings.LuxelSize = DefaultLuxelSize;
	Settings.LightMobility = DefaultLightMobility;
}
