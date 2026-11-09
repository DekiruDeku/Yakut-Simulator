using UnrealBuildTool;

public class YakutSimulator : ModuleRules
{
    public YakutSimulator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
            "HeadMountedDisplay", "XRBase", "UMG", "Slate", "SlateCore"
        });
        PrivateDependencyModuleNames.Add("EngineSettings");
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.Add("UnrealEd");
        }
    }
}
