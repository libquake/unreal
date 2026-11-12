#include "LibQuakeCore.h"

#include "Modules/ModuleManager.h"

THIRD_PARTY_INCLUDES_START
#include <quakelib/wad/wad.h>
THIRD_PARTY_INCLUDES_END

#include <exception>

IMPLEMENT_MODULE(FDefaultModuleImpl, LibQuakeCore);

namespace LibQuake
{
	namespace
	{
		template <typename F>
		bool Guard(F&& Body, FString& OutError)
		{
			try
			{
				Body();
				return true;
			}
			catch (const std::exception& E)
			{
				OutError = UTF8_TO_TCHAR(E.what());
				return false;
			}
		}

		std::string ToStd(const FString& Value)
		{
			return TCHAR_TO_UTF8(*Value);
		}

		TArray<uint8> ToBytes(const quakelib::wad::cvec& Colors)
		{
			TArray<uint8> Bytes;
			Bytes.SetNumUninitialized(Colors.size() * 4);
			FMemory::Memcpy(Bytes.GetData(), Colors.data(), Bytes.Num());
			return Bytes;
		}
	}

	bool LoadBsp(const FString& Path, const quakelib::BspLoadOptions& Options, quakelib::Level& OutLevel,
		FString& OutError)
	{
		return Guard([&] { OutLevel = quakelib::LoadBsp(ToStd(Path), Options); }, OutError);
	}

	bool ParseMap(const FString& Path, quakelib::MapDocument& OutDocument, FString& OutError)
	{
		return Guard([&] { OutDocument = quakelib::ParseMap(ToStd(Path)); }, OutError);
	}

	bool BuildMap(const quakelib::MapDocument& Document, const quakelib::MapBuildOptions& Options,
		quakelib::Level& OutLevel, FString& OutError)
	{
		return Guard([&] { OutLevel = quakelib::BuildMap(Document, Options); }, OutError);
	}

	bool LoadWad(const FString& Path, TArray<FWadTexture>& OutTextures, FString& OutError)
	{
		return Guard([&]
		{
			auto Wad = quakelib::wad::QuakeWad::FromFile(ToStd(Path));
			if (!Wad)
			{
				throw std::runtime_error("cannot read " + ToStd(Path));
			}

			OutTextures.Reset();
			for (const auto& [Name, Entry] : Wad->Textures())
			{
				if (Entry.Type() != quakelib::wad::QuakeWadEntry::QWET_MipsTexture)
				{
					continue;
				}
				const quakelib::wad::QuakeTexture* Texture = Wad->GetTexture(Name);
				if (!Texture || Texture->raw.empty())
				{
					continue;
				}

				FWadTexture& Out = OutTextures.AddDefaulted_GetRef();
				Out.Name = UTF8_TO_TCHAR(Name.c_str());
				Out.Width = Texture->width;
				Out.Height = Texture->height;
				Out.Rgba = ToBytes(Texture->raw);
				Out.bAlpha = Texture->hasAlpha;
				if (Texture->hasEmission)
				{
					Out.Emission = ToBytes(Texture->emission);
				}
				if (!Texture->front.empty())
				{
					Out.SkyFront = ToBytes(Texture->front);
				}
			}
		}, OutError);
	}
}
