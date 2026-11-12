#include "LibQuakeCore.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

namespace
{
	constexpr EAutomationTestFlags TestFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	FString TestFile(const TCHAR* Name)
	{
		const FString Base = IPluginManager::Get().FindPlugin(TEXT("LibQuake"))->GetBaseDir();
		return FPaths::Combine(Base, TEXT("Tests"), TEXT("Data"), Name);
	}

	const quakelib::EntityGeometry* WorldGeometry(const quakelib::Level& Level)
	{
		for (const quakelib::LevelEntity& Entity : Level.entities)
		{
			if (Entity.classname == "worldspawn" && Entity.geometry >= 0)
			{
				return &Level.geometry[Entity.geometry];
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLibQuakeLoadBspTest, "LibQuake.Core.LoadBsp", TestFlags)

bool FLibQuakeLoadBspTest::RunTest(const FString& Parameters)
{
	quakelib::Level Level;
	FString Error;
	if (!TestTrue(TEXT("bsp loads"), LibQuake::LoadBsp(TestFile(TEXT("lightstyles.bsp")), {}, Level, Error)))
	{
		AddError(Error);
		return false;
	}

	const quakelib::EntityGeometry* World = WorldGeometry(Level);
	TestNotNull(TEXT("worldspawn has geometry"), World);
	if (World)
	{
		TestTrue(TEXT("worldspawn has meshes"), !World->meshes.empty());
	}
	TestTrue(TEXT("embedded textures have pixels"), !Level.textures.empty() && !Level.textures[0].rgba.empty());
	TestTrue(TEXT("lightmap has more than one style"), Level.lightmap.layers.size() > 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLibQuakeLoadWadTest, "LibQuake.Core.LoadWad", TestFlags)

bool FLibQuakeLoadWadTest::RunTest(const FString& Parameters)
{
	TArray<LibQuake::FWadTexture> Textures;
	FString Error;
	if (!TestTrue(TEXT("wad loads"), LibQuake::LoadWad(TestFile(TEXT("sky_alpha.wad")), Textures, Error)))
	{
		AddError(Error);
		return false;
	}

	TestTrue(TEXT("wad has textures"), Textures.Num() > 0);
	for (const LibQuake::FWadTexture& Texture : Textures)
	{
		TestEqual(*FString::Printf(TEXT("%s is RGBA"), *Texture.Name), Texture.Rgba.Num(),
			Texture.Width * Texture.Height * 4);
	}
	TestTrue(TEXT("sky has a cloud layer"), Textures.ContainsByPredicate(
		[](const LibQuake::FWadTexture& Texture) { return !Texture.SkyFront.IsEmpty(); }));
	TestTrue(TEXT("fence texture has alpha"), Textures.ContainsByPredicate(
		[](const LibQuake::FWadTexture& Texture) { return Texture.bAlpha; }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLibQuakeBuildMapTest, "LibQuake.Core.BuildMap", TestFlags)

bool FLibQuakeBuildMapTest::RunTest(const FString& Parameters)
{
	FString Error;
	quakelib::MapDocument Document;
	if (!TestTrue(TEXT("map parses"), LibQuake::ParseMap(TestFile(TEXT("sky_alpha.map")), Document, Error)))
	{
		AddError(Error);
		return false;
	}
	TestTrue(TEXT("map names its wad"), Document.wads.size() == 1 && Document.wads[0] == "sky_alpha.wad");

	TArray<LibQuake::FWadTexture> Textures;
	TestTrue(TEXT("wad loads"), LibQuake::LoadWad(TestFile(TEXT("sky_alpha.wad")), Textures, Error));

	quakelib::MapBuildOptions Options;
	Options.textureSize = [&Textures](const std::string& Name) -> std::pair<int, int>
	{
		const FString Wanted = UTF8_TO_TCHAR(Name.c_str());
		for (const LibQuake::FWadTexture& Texture : Textures)
		{
			if (Texture.Name.Equals(Wanted, ESearchCase::IgnoreCase))
			{
				return {Texture.Width, Texture.Height};
			}
		}
		return {0, 0};
	};
	Options.bake = true;

	quakelib::Level Level;
	if (!TestTrue(TEXT("map builds"), LibQuake::BuildMap(Document, Options, Level, Error)))
	{
		AddError(Error);
		return false;
	}

	const quakelib::EntityGeometry* World = WorldGeometry(Level);
	TestNotNull(TEXT("worldspawn has geometry"), World);
	if (World)
	{
		TestTrue(TEXT("worldspawn has meshes"), !World->meshes.empty());
		TestTrue(TEXT("worldspawn has a hull per brush"), !World->hulls.empty());
	}
	TestFalse(TEXT("lightmap is baked"), Level.lightmap.layers.empty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLibQuakeErrorsTest, "LibQuake.Core.Errors", TestFlags)

bool FLibQuakeErrorsTest::RunTest(const FString& Parameters)
{
	FString Error;
	quakelib::Level Level;
	TestFalse(TEXT("missing bsp fails"), LibQuake::LoadBsp(TestFile(TEXT("missing.bsp")), {}, Level, Error));
	TestFalse(TEXT("with a message"), Error.IsEmpty());

	Error.Reset();
	quakelib::MapDocument Document;
	TestFalse(TEXT("a wad is not a map"), LibQuake::ParseMap(TestFile(TEXT("sky_alpha.wad")), Document, Error));
	TestFalse(TEXT("with a message"), Error.IsEmpty());

	Error.Reset();
	TArray<LibQuake::FWadTexture> Textures;
	TestFalse(TEXT("missing wad fails"), LibQuake::LoadWad(TestFile(TEXT("missing.wad")), Textures, Error));
	TestFalse(TEXT("with a message"), Error.IsEmpty());
	return true;
}
