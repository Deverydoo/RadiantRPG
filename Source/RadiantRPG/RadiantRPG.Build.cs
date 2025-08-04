// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class RadiantRPG : ModuleRules
{
	public RadiantRPG(ReadOnlyTargetRules Target) : base(Target)
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
			"GameplayTags"  // Added for FGameplayTag and FGameplayTagContainer
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Slate",      // Uncommented for UI support
			"SlateCore"   // Uncommented for UI support
		});


		PublicIncludePaths.AddRange(new string[] {
			"RadiantRPG",
		});

	}
}
