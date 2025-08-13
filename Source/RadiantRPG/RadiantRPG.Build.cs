// Source/RadiantRPG/RadiantRPG.Build.cs

using UnrealBuildTool;

public class RadiantRPG : ModuleRules
{
	public RadiantRPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		
		// Ensure we're building for the correct target
		bLegacyPublicIncludePaths = false;
		
		// Core dependencies - these MUST be available
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"AIModule",
			"NavigationSystem"
		});
		
		// Enhanced Input System
		PublicDependencyModuleNames.AddRange(new string[] {
			"EnhancedInput"
		});
		
		// Gameplay Tags
		PublicDependencyModuleNames.AddRange(new string[] {
			"GameplayTags"
		});
		
		// UI Dependencies
		PublicDependencyModuleNames.AddRange(new string[] {
			"UMG"
		});
		
		// Networking
		PublicDependencyModuleNames.AddRange(new string[] {
			"NetCore"
		});
		
		// Private dependencies (optional systems)
		PrivateDependencyModuleNames.AddRange(new string[] {
			"GameplayAbilities",
			"GameplayTasks",
			
		});
		
		// Physics
		PrivateDependencyModuleNames.AddRange(new string[] {
			"PhysicsCore"
		});
		
		// Additional editor-only dependencies
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd",
				"ToolMenus",
				"EditorStyle",
				"EditorWidgets"
			});
		}
		
		// Platform-specific settings
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			// Windows-specific settings if needed
		}
		
		// Optimization settings
		OptimizeCode = CodeOptimization.Default;
		
		// Include path settings for UE 5.5+
		PublicIncludePaths.AddRange(new string[] {
			"RadiantRPG/Public"
		});
		
		PrivateIncludePaths.AddRange(new string[] {
			"RadiantRPG/Private"
		});
	}
}