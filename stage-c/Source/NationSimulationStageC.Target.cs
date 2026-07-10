using UnrealBuildTool;
using System.Collections.Generic;

public class NationSimulationStageCTarget : TargetRules
{
    public NationSimulationStageCTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("NationSimulationStageC");
    }
}
