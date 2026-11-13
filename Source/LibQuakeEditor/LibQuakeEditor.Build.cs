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
			"ContentBrowser",
			"LibQuake",
			"LibQuakeCore",
			"MeshDescription",
			"MessageLog",
			"Projects",
			"RHI",
			"SlateCore",
			"StaticMeshDescription",
			"ToolMenus",
			"UnrealEd",
		});
	}
}
