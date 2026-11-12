using System.IO;
using UnrealBuildTool;

// libquake as a snapshot of its sources (Source/ThirdParty/libquake, see Scripts/sync-libquake.sh) plus a small
// exported interface. libquake's C++ API has no export macros, reports errors with exceptions and needs RTTI, all
// of that stays in this module.
public class LibQuakeCore : ModuleRules
{
	public LibQuakeCore(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp20;
		PCHUsage = PCHUsageMode.NoPCHs;
		bUseUnity = false;
		bEnableExceptions = true;
		bUseRTTI = true;

		string LibQuake = Path.Combine(PluginDirectory, "Source", "ThirdParty", "libquake");
		PublicIncludePaths.Add(Path.Combine(LibQuake, "include"));
		PrivateIncludePaths.Add(Path.Combine(LibQuake, "src"));

		PublicDependencyModuleNames.Add("Core");
	}
}
