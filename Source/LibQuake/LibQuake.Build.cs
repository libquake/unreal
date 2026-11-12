using UnrealBuildTool;

public class LibQuake : ModuleRules
{
	public LibQuake(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.Add("LibQuakeCore");
	}
}
