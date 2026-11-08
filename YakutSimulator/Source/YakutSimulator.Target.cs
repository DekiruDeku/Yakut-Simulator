// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class YakutSimulatorTarget : TargetRules
{
	public YakutSimulatorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// MetaXR precompiled libraries require MSVC 14.50 or newer.
			WindowsPlatform.Compiler = WindowsCompiler.VisualStudio2026;
			WindowsPlatform.CompilerVersion = "Latest";
		}

		ExtraModuleNames.AddRange( new string[] { "YakutSimulator" } );
	}
}
