using Godot;

namespace NationSimulation.StageB;

public partial class Main : Node
{
    public override void _Ready()
    {
        try
        {
            var projectRoot = ProjectSettings.GlobalizePath("res://");
            var repositoryRoot = Path.GetFullPath(Path.Combine(projectRoot, ".."));
            var fixture = Path.Combine(repositoryRoot, "data", "stage_a_fixture.json");
            var parityContract = Path.Combine(repositoryRoot, "data", "stage_parity_contract.json");
            var output = System.Environment.GetEnvironmentVariable("STAGE_B_EVIDENCE_DIR");
            if (string.IsNullOrWhiteSpace(output)) output = Path.Combine(repositoryRoot, "out", "stage-b", "test-output");

            var arguments = OS.GetCmdlineUserArgs();
            if (arguments.Contains("--acceptance", StringComparer.Ordinal))
            {
                var exitCode = StageBAcceptance.Run(fixture, parityContract, output);
                GetTree().Quit(exitCode);
                return;
            }

            var simulation = Simulation.FromFixture(fixture);
            GD.Print("Nation Simulation Stage B loaded in Godot 4.7 .NET.");
            GD.Print($"fixture=stage_a_fixture_v1 ai_npcs={simulation.AiNpcs.Count} non_ai_npcs={simulation.NonAiNpcs.Count} world_seed={simulation.WorldSeed}");
            GD.Print("Run with '-- --acceptance' to execute the Godot headless acceptance suite.");
            if (DisplayServer.GetName() == "headless") GetTree().Quit(0);
        }
        catch (Exception exception)
        {
            GD.PushError($"Stage B startup failed: {exception}");
            GetTree().Quit(1);
        }
    }
}
