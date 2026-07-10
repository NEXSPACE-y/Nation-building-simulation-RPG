using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace NationSimulation.StageB;

public static class StageBAcceptance
{
    private sealed class TestFailure(string message) : Exception(message);

    private static string _fixture = "";
    private static string _output = "";
    private static JsonDocument? _contractDocument;
    private static JsonElement Contract => _contractDocument!.RootElement;

    public static int Run(string fixturePath, string contractPath, string outputDirectory)
    {
        _fixture = fixturePath;
        _output = outputDirectory;
        Directory.CreateDirectory(_output);
        _contractDocument = JsonDocument.Parse(File.ReadAllText(contractPath, Encoding.UTF8));
        var tests = new (string Name, Action Test)[]
        {
            ("Fixture contract: shared 1 country, 20 AI, 20 NON AI, 255 independent slots", TestFixtureContract),
            ("Scenario A: NON AI good deed reaches AI by rumor", TestScenarioA),
            ("Scenario B: guard observes violence and acts", TestScenarioB),
            ("Scenario C: skeptical AI investigates doubtful rumor", TestScenarioC),
            ("Scenario D: AI-to-AI report changes country state", TestScenarioD),
            ("Scenario E: offline elapsed time and seven-day cap", TestScenarioE),
            ("Scenario F: deterministic replay", TestScenarioF),
            ("Scenario G: mid-chain save/load equivalence with retained pending evidence", TestScenarioG),
            ("Chain limit emits an auditable error event", TestChainLimit),
            ("Configured player action surface is wired", TestPlayerActionSurface),
            ("Audit fields distinguish event actor and decision NPC", TestAuditIdentityFields)
        };

        var passed = 0;
        var report = new StringBuilder();
        foreach (var (name, test) in tests)
        {
            try
            {
                test();
                passed++;
                Godot.GD.Print($"[PASS] {name}");
                report.AppendLine($"PASS | {name}");
            }
            catch (Exception exception)
            {
                Godot.GD.PushError($"[FAIL] {name} | {exception.Message}");
                report.AppendLine($"FAIL | {name} | {exception.Message}");
            }
        }
        try
        {
            WriteEvidenceLogs();
        }
        catch (Exception exception)
        {
            Godot.GD.PushError($"[FAIL] Evidence generation | {exception.Message}");
            report.AppendLine($"FAIL | Evidence generation | {exception.Message}");
        }
        report.AppendLine($"SUMMARY | {passed}/{tests.Length} tests passed");
        File.WriteAllText(Path.Combine(_output, "acceptance_results.txt"), report.ToString(), new UTF8Encoding(false));
        Godot.GD.Print($"Stage B acceptance: {passed}/{tests.Length} tests passed");
        return passed == tests.Length ? 0 : 1;
    }

    private static void Check(bool condition, string message)
    {
        if (!condition) throw new TestFailure(message);
    }

    private static SimEvent FirstEvent(Simulation simulation, string type, string actor = "") =>
        simulation.EventHistory.FirstOrDefault(value => value.EventType == type &&
            (string.IsNullOrEmpty(actor) || value.ActorId == actor))
        ?? throw new TestFailure($"Event not found: {type}, actor={actor}");

    private static AuditEntry AuditFor(Simulation simulation, string decisionNpcId, string action = "") =>
        simulation.AuditLog.FirstOrDefault(value => value.DecisionNpcId == decisionNpcId &&
            (string.IsNullOrEmpty(action) || value.SelectedAction == action))
        ?? throw new TestFailure($"Audit not found: decision_npc_id={decisionNpcId}, action={action}");

    private static Simulation RunScenarioA()
    {
        var contract = Contract.GetProperty("scenario_a");
        var simulation = Simulation.FromFixture(_fixture);
        simulation.PlayerAction("MOVE", destinationId: "tavern");
        var help = simulation.PlayerAction("HELP", "non_ai_npc_001");
        Check(!simulation.Event(help).ObservedBy.Contains("ai_npc_003"), "Reacting AI must not directly witness Scenario A.");
        var before = simulation.AiNpc("ai_npc_003").PlayerEvaluation;
        simulation.ShareRumor("non_ai_npc_001", "ai_npc_003", help, 0.90);
        var merchant = simulation.AiNpc(contract.GetProperty("reacting_ai_npc_id").GetString()!);
        Check(merchant.CurrentStateId == contract.GetProperty("expected_state_id").GetInt32(), "Scenario A state mismatch.");
        Check(merchant.PlayerEvaluation - before >= contract.GetProperty("minimum_player_evaluation_delta").GetInt32(),
            "Scenario A player evaluation did not increase.");
        var audit = AuditFor(simulation, merchant.NpcId, contract.GetProperty("expected_action").GetString()!);
        Check(!string.IsNullOrEmpty(audit.SelectedDialogue), "Scenario A dialogue was not selected.");
        Check(audit.SelectedRule.Contains("help_heard", StringComparison.Ordinal), "Scenario A did not select heard-help rule.");
        return simulation;
    }

    private static Simulation RunScenarioBAndD()
    {
        var scenarioB = Contract.GetProperty("scenario_b");
        var scenarioD = Contract.GetProperty("scenario_d");
        var simulation = Simulation.FromFixture(_fixture);
        simulation.PlayerAction("MOVE", destinationId: "market");
        var initialSecurity = simulation.Country.Security;
        var initialCrime = simulation.Country.CrimeLevel;
        var harm = simulation.PlayerAction("HARM", "non_ai_npc_002");
        var guard = simulation.AiNpc(scenarioB.GetProperty("guard_ai_npc_id").GetString()!);
        Check(guard.CurrentStateId == scenarioB.GetProperty("expected_state_id").GetInt32(), "Scenario B guard state mismatch.");
        Check(AuditFor(simulation, guard.NpcId, scenarioB.GetProperty("expected_action").GetString()!).SelectedRule.Contains("harm_observed", StringComparison.Ordinal),
            "Scenario B guard did not use observed-harm rule.");
        var authority = simulation.AiNpc(scenarioD.GetProperty("authority_ai_npc_id").GetString()!);
        Check(authority.CurrentStateId == scenarioD.GetProperty("expected_state_id").GetInt32(), "Scenario D authority state mismatch.");
        Check(simulation.Country.Security == initialSecurity + scenarioD.GetProperty("security_delta").GetInt32(), "Security delta mismatch.");
        Check(simulation.Country.CrimeLevel == initialCrime + scenarioD.GetProperty("crime_level_delta").GetInt32(), "Crime delta mismatch.");
        var countryEvent = FirstEvent(simulation, "COUNTRY_STATE_CHANGED", authority.NpcId);
        var path = simulation.CausalPath(countryEvent.EventId);
        Check(path[0].StartsWith($"{harm}:PLAYER_HARMED_NON_AI_NPC", StringComparison.Ordinal), "Scenario D root mismatch.");
        return simulation;
    }

    private static void TestFixtureContract()
    {
        var simulation = Simulation.FromFixture(_fixture);
        Check(simulation.Country.CountryId == Contract.GetProperty("country_id").GetString(), "Country id mismatch.");
        Check(simulation.WorldSeed == Contract.GetProperty("world_seed").GetUInt64(), "World seed mismatch.");
        Check(simulation.AiNpcs.Count == Contract.GetProperty("ai_npc_count").GetInt32(), "AI count mismatch.");
        Check(simulation.NonAiNpcs.Count == Contract.GetProperty("non_ai_npc_count").GetInt32(), "NON AI count mismatch.");
        var slots = Contract.GetProperty("state_slots_per_ai_npc").GetInt32();
        var signatures = new HashSet<string>(StringComparer.Ordinal);
        foreach (var npc in simulation.AiNpcs)
        {
            Check(npc.States.Count == slots, $"{npc.NpcId} state count mismatch.");
            Check(npc.States.Skip(5).All(state => state.Undefined && state.StateName == "UNDEFINED"),
                $"{npc.NpcId} undefined slots are not explicit.");
            var signature = string.Join('|', npc.States.Take(5).Select(state => state.StateName + state.StateDescription));
            Check(signatures.Add(signature), $"{npc.NpcId} reused another NPC state table.");
        }
    }

    private static void TestScenarioA() => _ = RunScenarioA();

    private static void TestScenarioB()
    {
        var simulation = RunScenarioBAndD();
        Check(FirstEvent(simulation, "AI_NPC_OBSERVED_EVENT", "ai_npc_001").Credibility == 1.0,
            "Direct observation credibility mismatch.");
    }

    private static void TestScenarioC()
    {
        var contract = Contract.GetProperty("scenario_c");
        var simulation = Simulation.FromFixture(_fixture);
        simulation.PlayerAction("MOVE", destinationId: "residential");
        var harm = simulation.PlayerAction("HARM", "non_ai_npc_003");
        var initialSecurity = simulation.Country.Security;
        simulation.ShareRumor("non_ai_npc_003", "ai_npc_004", harm, 0.50);
        var investigator = simulation.AiNpc(contract.GetProperty("investigator_ai_npc_id").GetString()!);
        Check(investigator.CurrentStateId == contract.GetProperty("expected_state_id").GetInt32(), "Scenario C state mismatch.");
        Check(AuditFor(simulation, investigator.NpcId, contract.GetProperty("expected_action").GetString()!).SelectedRule.Contains("harm_heard_doubt", StringComparison.Ordinal),
            "Scenario C did not select doubt rule.");
        Check(simulation.Country.Security == initialSecurity, "Scenario C doubtful rumor changed country state.");
    }

    private static void TestScenarioD()
    {
        var simulation = RunScenarioBAndD();
        var authorityId = Contract.GetProperty("scenario_d").GetProperty("authority_ai_npc_id").GetString()!;
        var countryEvent = FirstEvent(simulation, "COUNTRY_STATE_CHANGED", authorityId);
        var path = simulation.CausalPath(countryEvent.EventId);
        var actual = path.Select(value => value[(value.IndexOf(':') + 1)..]).ToArray();
        var expected = Contract.GetProperty("scenario_d").GetProperty("path_event_types")
            .EnumerateArray().Select(value => value.GetString()!).ToArray();
        Check(actual.SequenceEqual(expected, StringComparer.Ordinal), "Stage B Scenario D path differs from shared Stage A/B contract.");
        File.WriteAllLines(Path.Combine(_output, "scenario_d_causal_path.txt"), path, new UTF8Encoding(false));
    }

    private static void TestScenarioE()
    {
        var contract = Contract.GetProperty("scenario_e");
        var realSeconds = contract.GetProperty("real_seconds").GetInt64();
        var gameMinutes = contract.GetProperty("expected_game_minutes").GetInt64();
        var limit = contract.GetProperty("offline_limit_seconds").GetInt64();
        var simulation = Simulation.FromFixture(_fixture);
        var result = simulation.AdvanceOffline(realSeconds);
        Check(result.AppliedRealSeconds == realSeconds && result.ElapsedGameMinutes == gameMinutes,
            "Scenario E time conversion mismatch.");
        Check(simulation.CurrentWorldTimeMinutes == gameMinutes, "Scenario E world time mismatch.");
        var capped = Simulation.FromFixture(_fixture).AdvanceOffline(limit + 12345);
        Check(capped.AppliedRealSeconds == limit && capped.ElapsedGameMinutes == limit, "Scenario E cap mismatch.");
    }

    private static void TestScenarioF()
    {
        var first = RunScenarioA();
        var second = RunScenarioA();
        Check(first.CanonicalSnapshot() == second.CanonicalSnapshot(), "Stage B deterministic snapshots differ.");
        Check(first.AuditLogJsonLines() == second.AuditLogJsonLines(), "Stage B deterministic audit logs differ.");
        Check(first.CausalLogJsonLines() == second.CausalLogJsonLines(), "Stage B deterministic causal logs differ.");
    }

    private static void TestScenarioG()
    {
        var continuous = Simulation.FromFixture(_fixture);
        continuous.PlayerAction("MOVE", destinationId: "market");
        continuous.EnqueuePlayerAction("HARM", "non_ai_npc_002");
        continuous.ProcessPending();

        var interrupted = Simulation.FromFixture(_fixture);
        interrupted.PlayerAction("MOVE", destinationId: "market");
        interrupted.EnqueuePlayerAction("HARM", "non_ai_npc_002");
        interrupted.ProcessPending(2);
        Check(interrupted.PendingEvents.Count > 0, "Scenario G has no pending events at save point.");
        var pendingPath = Path.Combine(_output, "mid_chain_pending_save.json");
        var resumedPath = Path.Combine(_output, "mid_chain_resumed_save.json");
        interrupted.Save(pendingPath);
        using var pendingDocument = JsonDocument.Parse(File.ReadAllText(pendingPath, Encoding.UTF8));
        var pendingRoot = pendingDocument.RootElement;
        var pending = pendingRoot.GetProperty("pending_events");
        Check(pending.GetArrayLength() > 0, "Submitted Stage B pending save contains no pending events.");

        var resumed = Simulation.LoadSave(_fixture, pendingPath);
        Check(resumed.PendingEvents.Count > 0, "Loaded Stage B pending save contains no pending events.");
        resumed.ProcessPending();
        var continuousSnapshot = continuous.CanonicalSnapshot();
        var resumedSnapshot = resumed.CanonicalSnapshot();
        var continuousHash = Sha256(continuousSnapshot);
        var resumedHash = Sha256(resumedSnapshot);
        Check(continuousHash == resumedHash, "Scenario G continuous/resumed SHA-256 mismatch.");
        File.WriteAllText(Path.Combine(_output, "scenario_g_continuous_snapshot.json"), continuousSnapshot + "\n", new UTF8Encoding(false));
        File.WriteAllText(Path.Combine(_output, "scenario_g_resumed_snapshot.json"), resumedSnapshot + "\n", new UTF8Encoding(false));
        var evidence = new
        {
            pending_save_file = Path.GetFileName(pendingPath),
            pending_simulation_tick = pendingRoot.GetProperty("simulation_tick").GetInt64(),
            pending_next_event_sequence = pendingRoot.GetProperty("next_event_sequence").GetUInt64(),
            pending_event_count = pending.GetArrayLength(),
            pending_events = JsonSerializer.Deserialize<object>(pending.GetRawText()),
            hash_algorithm = "SHA-256",
            continuous_snapshot_sha256 = continuousHash,
            resumed_snapshot_sha256 = resumedHash,
            hashes_match = continuousHash == resumedHash,
            continuous_event_count = continuous.EventHistory.Count,
            resumed_event_count = resumed.EventHistory.Count
        };
        File.WriteAllText(Path.Combine(_output, "scenario_g_evidence.json"),
            JsonSerializer.Serialize(evidence, new JsonSerializerOptions { WriteIndented = true }) + "\n", new UTF8Encoding(false));
        resumed.Save(resumedPath);
        Check(Simulation.LoadSave(_fixture, resumedPath).CanonicalSnapshot() == resumed.CanonicalSnapshot(),
            "Completed Stage B save/load mismatch.");
    }

    private static void TestChainLimit()
    {
        var simulation = Simulation.FromFixture(_fixture);
        simulation.SetChainLimitForTest(1);
        simulation.PlayerAction("MOVE", destinationId: "market");
        var limit = FirstEvent(simulation, "CHAIN_LIMIT_REACHED");
        Check(limit.Payload["processed_event_count"] == "1" && simulation.PendingEvents.Count == 0,
            "CHAIN_LIMIT_REACHED evidence mismatch.");
    }

    private static void TestPlayerActionSurface()
    {
        var simulation = Simulation.FromFixture(_fixture);
        simulation.PlayerAction("MOVE", destinationId: "market");
        simulation.PlayerAction("TALK", "non_ai_npc_002");
        simulation.PlayerAction("TRADE", "non_ai_npc_002");
        simulation.PlayerAction("STEAL", "non_ai_npc_002");
        simulation.PlayerAction("WAIT");
        Check(FirstEvent(simulation, "PLAYER_TALKED").TargetId == "non_ai_npc_002", "TALK not wired.");
        Check(FirstEvent(simulation, "PLAYER_TRADED").TargetId == "non_ai_npc_002", "TRADE not wired.");
        Check(FirstEvent(simulation, "PLAYER_STOLE").TargetId == "non_ai_npc_002", "STEAL not wired.");

        var helpAi = Simulation.FromFixture(_fixture);
        helpAi.PlayerAction("MOVE", destinationId: "gate");
        helpAi.PlayerAction("HELP", "ai_npc_004");
        Check(helpAi.AiNpc("ai_npc_004").CurrentStateId == 2, "Direct HELP to AI not wired.");
        var harmAi = Simulation.FromFixture(_fixture);
        harmAi.PlayerAction("MOVE", destinationId: "gate");
        harmAi.PlayerAction("HARM", "ai_npc_004");
        Check(harmAi.AiNpc("ai_npc_004").CurrentStateId == 3, "Direct HARM to AI not wired.");
    }

    private static void TestAuditIdentityFields()
    {
        var simulation = RunScenarioBAndD();
        var audit = AuditFor(simulation, "ai_npc_002", "CHANGE_COUNTRY_STATE");
        Check(audit.EventActorId == "ai_npc_001", "Audit event_actor_id mismatch.");
        Check(audit.DecisionNpcId == "ai_npc_002", "Audit decision_npc_id mismatch.");
        Check(audit.TargetId == "ai_npc_002", "Audit target_id mismatch.");
    }

    private static void WriteEvidenceLogs()
    {
        var simulation = RunScenarioBAndD();
        File.WriteAllText(Path.Combine(_output, "stage_b_audit.jsonl"), simulation.AuditLogJsonLines(), new UTF8Encoding(false));
        File.WriteAllText(Path.Combine(_output, "stage_b_causal.jsonl"), simulation.CausalLogJsonLines(), new UTF8Encoding(false));
        File.WriteAllText(Path.Combine(_output, "stage_b_reproducible_snapshot.json"), simulation.CanonicalSnapshot() + "\n", new UTF8Encoding(false));
    }

    private static string Sha256(string value) => Convert.ToHexString(
        SHA256.HashData(Encoding.UTF8.GetBytes(value))).ToLowerInvariant();
}
