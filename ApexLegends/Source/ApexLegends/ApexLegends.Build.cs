// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ApexLegends : ModuleRules
{
	public ApexLegends(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ApexLegends",
			"ApexLegends/Variant_Horror",
			"ApexLegends/Variant_Horror/UI",
			"ApexLegends/Variant_Shooter",
			"ApexLegends/Variant_Shooter/AI",
			"ApexLegends/Variant_Shooter/UI",
			"ApexLegends/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
