// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class Machine_ChallengerTarget : TargetRules
{
	public Machine_ChallengerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        // Update build settings
        DefaultBuildSettings = BuildSettingsVersion.Latest;

        // Update include order version
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

		ExtraModuleNames.AddRange( new string[] { "Machine_Challenger" } );
	}
}
