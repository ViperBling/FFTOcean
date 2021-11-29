
namespace UnrealBuildTool.Rules
{
	public class ShaderDeclaration : ModuleRules
	{
		public ShaderDeclaration(ReadOnlyTargetRules Target) : base(Target)
		{
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

			PrivateIncludePaths.AddRange(new string[]
			{
				"ShaderDeclaration/Public"
			});

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Renderer",
				"RenderCore",
				"RHI",
				"Projects"
			});
		}
	}
}