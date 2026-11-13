using UnrealBuildTool;

public class LibQuake : ModuleRules
{
	public LibQuake(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "LibQuakeCore" });
		PrivateDependencyModuleNames.AddRange(new[] { "MeshDescription", "StaticMeshDescription" });
	}
}
