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
            "Json",
            "Projects"
        });

        PublicIncludePaths.Add(Path.GetFullPath(Path.Combine(ModuleDirectory, "../../../include")));
        PublicSystemIncludePaths.Add(Path.Combine(ModuleDirectory, "ThirdParty"));
    }
}
