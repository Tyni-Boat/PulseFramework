// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PulseGameFramework : ModuleRules
{
	public PulseGameFramework(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[]
			{
				// ... add public include paths required here ...
			}
		);


		PrivateIncludePaths.AddRange(
			new string[]
			{
				// ... add other private include paths required here ...
			}
		);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"GameplayAbilities",
				"DeveloperSettings",
				"GameFeatures",
				"PakFile",
				"HTTP",
				"PhysicsCore",
				"Chaos", // For Chaos::FConvex and geometry queries
				// ... add other public dependencies that you statically link with here ...
			}
		);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"NetCore",
				"SlateCore",
				"UMG",
				"ApplicationCore",
				"InputCore",
				"Json", // Required for JSON serialization and parsing
				"JsonUtilities", // Required for JSON serialization and parsing
				"GeometryCore", // For TConvexHull3
				"GeometryCollectionEngine", // UGeometryCollection, UGeometryCollectionComponent
				"ChaosCore",
				// ... add private dependencies that you statically link with here ...	
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}