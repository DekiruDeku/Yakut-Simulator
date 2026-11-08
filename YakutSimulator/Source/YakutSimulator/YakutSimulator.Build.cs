// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class YakutSimulator : ModuleRules
{
	public YakutSimulator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "GameplayTags", "StateTreeModule", "GameplayStateTreeModule", "UMG", "AIModule", "GameplayTasks" });

		PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "HeadMountedDisplay", "Slate", "SlateCore" });

	}
}
