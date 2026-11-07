using UnrealBuildTool;

public class DialogueMemoryTable : ModuleRules
{
    public DialogueMemoryTable(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        RuntimeDependencies.Add("$(PluginDir)/Resources/DialogueMemorySummarizerPrompt.md", StagedFileType.UFS);
        RuntimeDependencies.Add("$(ProjectDir)/Config/ForestCommissionKnowledge.json", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Config/ForestElementCodex.json", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Config/ForestPersonaFacts.json", StagedFileType.NonUFS);
        RuntimeDependencies.Add("$(ProjectDir)/Config/ForestDailyResourceValues.json", StagedFileType.NonUFS);

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "HTTP",
                "Json",
                "JsonUtilities"
            }
        );
    }
}
