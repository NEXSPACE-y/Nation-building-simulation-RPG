using UnrealBuildTool;
using System.IO;

public class NationSimulationStageC : ModuleRules
{
    public NationSimulationStageC(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp23;
        bEnableExceptions = true;
        bUseUnity = false;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "Json",
            "Projects",
            "Slate",
            "SlateCore",
            "UMG"
        });

        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../include")));
        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../Tools")));
        PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty"));

        string Fixture = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../data/stage_a_fixture.json"));
        RuntimeDependencies.Add("$(TargetOutputDir)/Data/stage_a_fixture.json", Fixture, StagedFileType.NonUFS);
        string StageEOverlay = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../stage-c/Data/StageE/stage_e_state_definitions.json"));
        RuntimeDependencies.Add("$(TargetOutputDir)/Data/StageE/stage_e_state_definitions.json", StageEOverlay, StagedFileType.NonUFS);
    }
}
