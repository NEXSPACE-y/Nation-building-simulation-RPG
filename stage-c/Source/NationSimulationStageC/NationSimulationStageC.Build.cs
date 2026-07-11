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
        PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty"));

        string Fixture = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../data/stage_a_fixture.json"));
        RuntimeDependencies.Add("$(TargetOutputDir)/Data/stage_a_fixture.json", Fixture, StagedFileType.NonUFS);
    }
}
