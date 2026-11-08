using UnrealBuildTool;

public class YakutSimulatorEditor : ModuleRules
{
    public YakutSimulatorEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new[] { "PropertyBindingUtils", "HeadMountedDisplay" });
        PrivateDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UnrealEd", "YakutSimulator", "AssetRegistry", "AssetTools", "BlueprintGraph", "KismetCompiler", "Kismet", "StateTreeEditorModule", "StateTreeModule", "GameplayStateTreeModule", "UMG", "UMGEditor", "Slate", "SlateCore", "EnhancedInput", "InputBlueprintNodes", "InputCore", "GameplayTags" });
    }
}
