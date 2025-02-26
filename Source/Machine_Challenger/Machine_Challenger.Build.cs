// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Machine_Challenger : ModuleRules
{
    public Machine_Challenger(ReadOnlyTargetRules Target) : base(Target)
    {
        PrivateDependencyModuleNames.AddRange(new string[] { "EnhancedInput", "Niagara", "AdvancedSessions", "AdvancedSessions", "RTune", "OnlineSubsystem", "OnlineSubsystemSteam", "OnlineSubsystemUtils", "CoreOnline", "GoogleTest" });

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG", "Slate", "SlateCore" });

        PrivateDependencyModuleNames.AddRange(new string[] { });
        PublicDependencyModuleNames.AddRange(new string[] { "GameplayAbilities", "GameplayTags", "GameplayTasks", "Sentry", "Chaos",  "GeometryCollectionEngine", "PhysicsCore" });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
