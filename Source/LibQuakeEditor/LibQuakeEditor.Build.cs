using UnrealBuildTool;

public class LibQuakeEditor : ModuleRules
{
	public LibQuakeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[]
		{
			"AssetDefinition",
			"AssetRegistry",
			"LibQuake",
			"LibQuakeCore",
			"MessageLog",
			"Projects",
			"UnrealEd",
		});
	}
}
