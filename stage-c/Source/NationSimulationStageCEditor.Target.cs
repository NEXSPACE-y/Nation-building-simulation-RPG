using UnrealBuildTool;
using System.Collections.Generic;

public class NationSimulationStageCEditorTarget : TargetRules
{
    public NationSimulationStageCEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("NationSimulationStageC");
    }
}
