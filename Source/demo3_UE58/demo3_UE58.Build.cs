// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class demo3_UE58 : ModuleRules
{
	public demo3_UE58(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		RuntimeDependencies.Add("$(ProjectDir)/Config/ForestZones.json", StagedFileType.NonUFS);
		// File-backed Slate brush must be staged as a loose file in packaged builds.
		RuntimeDependencies.Add("$(ProjectDir)/Content/UI/Textures/WoodlandPaper.png", StagedFileType.NonUFS);
		RuntimeDependencies.Add("$(ProjectDir)/Content/UI/Textures/ElementEmblems.png", StagedFileType.NonUFS);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "NavigationSystem", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });
		PrivateDependencyModuleNames.Add("AnimGraphRuntime");
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "Json", "JsonUtilities", "Projects" });
		PublicDependencyModuleNames.Add("DialogueMemoryTable");
		PublicDependencyModuleNames.Add("Niagara");
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "AnimGraph", "BlueprintGraph" });
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
