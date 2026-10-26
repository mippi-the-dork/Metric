using UnrealBuildTool;

public class Metric : ModuleRules
{
    public Metric(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "EditorSubsystem"
            });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd",
                "LevelEditor",
                "Slate",
                "SlateCore",
                "InputCore",
                "DeveloperSettings",
                "RenderCore",
                "Projects",
                "AssetRegistry",
                "PhysicsCore",
                "AssetDefinition",
                "EngineAssetDefinitions",
                "PropertyEditor"
            });
    }
}
