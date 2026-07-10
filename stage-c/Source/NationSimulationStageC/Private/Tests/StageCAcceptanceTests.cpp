#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"

#include "nation_sim/simulation.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

using nation_sim::AuditEntry;
using nation_sim::Event;
using nation_sim::Simulation;
using json = nlohmann::json;

class FStageCTestFailure final : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

void Require(bool Condition, const std::string& Message)
{
    if (!Condition)
    {
        throw FStageCTestFailure(Message);
    }
}

std::filesystem::path ToFilesystemPath(const FString& Path)
{
    return std::filesystem::path(*Path);
}

std::string Sha256(std::string_view Input)
{
    constexpr std::array<std::uint32_t, 64> Constants{
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    std::vector<std::uint8_t> Bytes(Input.begin(), Input.end());
    const std::uint64_t BitLength = static_cast<std::uint64_t>(Bytes.size()) * 8;
    Bytes.push_back(0x80);
    while (Bytes.size() % 64 != 56) Bytes.push_back(0);
    for (int Shift = 56; Shift >= 0; Shift -= 8) Bytes.push_back(static_cast<std::uint8_t>(BitLength >> Shift));
    std::array<std::uint32_t, 8> Hash{
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    const auto RotateRight = [](std::uint32_t Value, unsigned Count) {
        return (Value >> Count) | (Value << (32 - Count));
    };
    for (std::size_t Offset = 0; Offset < Bytes.size(); Offset += 64)
    {
        std::array<std::uint32_t, 64> Words{};
        for (std::size_t Index = 0; Index < 16; ++Index)
        {
            const auto Base = Offset + Index * 4;
            Words[Index] = (static_cast<std::uint32_t>(Bytes[Base]) << 24) |
                           (static_cast<std::uint32_t>(Bytes[Base + 1]) << 16) |
                           (static_cast<std::uint32_t>(Bytes[Base + 2]) << 8) |
                           static_cast<std::uint32_t>(Bytes[Base + 3]);
        }
        for (std::size_t Index = 16; Index < 64; ++Index)
        {
            const auto S0 = RotateRight(Words[Index - 15], 7) ^ RotateRight(Words[Index - 15], 18) ^ (Words[Index - 15] >> 3);
            const auto S1 = RotateRight(Words[Index - 2], 17) ^ RotateRight(Words[Index - 2], 19) ^ (Words[Index - 2] >> 10);
            Words[Index] = Words[Index - 16] + S0 + Words[Index - 7] + S1;
        }
        auto A=Hash[0], B=Hash[1], C=Hash[2], D=Hash[3], E=Hash[4], F=Hash[5], G=Hash[6], H=Hash[7];
        for (std::size_t Index = 0; Index < 64; ++Index)
        {
            const auto Sum1 = RotateRight(E, 6) ^ RotateRight(E, 11) ^ RotateRight(E, 25);
            const auto Choice = (E & F) ^ (~E & G);
            const auto Temp1 = H + Sum1 + Choice + Constants[Index] + Words[Index];
            const auto Sum0 = RotateRight(A, 2) ^ RotateRight(A, 13) ^ RotateRight(A, 22);
            const auto Majority = (A & B) ^ (A & C) ^ (B & C);
            const auto Temp2 = Sum0 + Majority;
            H=G; G=F; F=E; E=D+Temp1; D=C; C=B; B=A; A=Temp1+Temp2;
        }
        Hash[0]+=A; Hash[1]+=B; Hash[2]+=C; Hash[3]+=D;
        Hash[4]+=E; Hash[5]+=F; Hash[6]+=G; Hash[7]+=H;
    }
    std::ostringstream Output;
    Output << std::hex << std::setfill('0');
    for (const auto Value : Hash) Output << std::setw(8) << Value;
    return Output.str();
}

const Event& FirstEvent(const Simulation& Sim, const std::string& Type, const std::string& Actor = {})
{
    const auto It = std::find_if(Sim.event_history().begin(), Sim.event_history().end(), [&](const Event& Value) {
        return Value.event_type == Type && (Actor.empty() || Value.actor_id == Actor);
    });
    if (It == Sim.event_history().end()) throw FStageCTestFailure("Event not found: " + Type + ", actor=" + Actor);
    return *It;
}

const AuditEntry& AuditFor(const Simulation& Sim, const std::string& DecisionNpcId, const std::string& Action = {})
{
    const auto It = std::find_if(Sim.audit_log().begin(), Sim.audit_log().end(), [&](const AuditEntry& Value) {
        return Value.decision_npc_id == DecisionNpcId && (Action.empty() || Value.selected_action == Action);
    });
    if (It == Sim.audit_log().end()) throw FStageCTestFailure("Audit not found: decision_npc_id=" + DecisionNpcId + ", action=" + Action);
    return *It;
}

struct FStageCTestContext
{
    std::filesystem::path Fixture;
    std::filesystem::path ContractPath;
    std::filesystem::path Output;
    json Contract;

    FStageCTestContext()
    {
        const FString RepositoryRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
        Fixture = ToFilesystemPath(FPaths::Combine(RepositoryRoot, TEXT("data"), TEXT("stage_a_fixture.json")));
        ContractPath = ToFilesystemPath(FPaths::Combine(RepositoryRoot, TEXT("data"), TEXT("stage_parity_contract.json")));
        Output = ToFilesystemPath(FPaths::Combine(RepositoryRoot, TEXT("out"), TEXT("stage-c"), TEXT("test-output")));
        std::filesystem::create_directories(Output);
        std::ifstream Input(ContractPath, std::ios::binary);
        if (!Input) throw FStageCTestFailure("Unable to open shared Stage A/B/C contract.");
        Input >> Contract;
    }

    Simulation RunScenarioA() const
    {
        const auto& Scenario = Contract.at("scenario_a");
        auto Sim = Simulation::from_fixture(Fixture);
        Sim.player_action("MOVE", "", "tavern");
        const std::string Help = Sim.player_action("HELP", "non_ai_npc_001");
        const auto& HelpEvent = Sim.event(Help);
        Require(std::find(HelpEvent.observed_by.begin(), HelpEvent.observed_by.end(), "ai_npc_003") == HelpEvent.observed_by.end(),
                "Scenario A reacting AI directly witnessed the event.");
        const int Before = Sim.ai_npc("ai_npc_003").player_evaluation;
        Sim.share_rumor("non_ai_npc_001", "ai_npc_003", Help, 0.90);
        const auto& Npc = Sim.ai_npc(Scenario.at("reacting_ai_npc_id").get<std::string>());
        Require(Npc.current_state_id == Scenario.at("expected_state_id").get<int>(), "Scenario A state mismatch.");
        Require(Npc.player_evaluation - Before >= Scenario.at("minimum_player_evaluation_delta").get<int>(),
                "Scenario A evaluation did not increase.");
        const auto& Audit = AuditFor(Sim, Npc.npc_id, Scenario.at("expected_action").get<std::string>());
        Require(!Audit.selected_dialogue.empty(), "Scenario A selected no dialogue.");
        Require(Audit.selected_rule.find("help_heard") != std::string::npos, "Scenario A selected the wrong rule.");
        return Sim;
    }

    Simulation RunScenarioBAndD() const
    {
        const auto& ScenarioB = Contract.at("scenario_b");
        const auto& ScenarioD = Contract.at("scenario_d");
        auto Sim = Simulation::from_fixture(Fixture);
        Sim.player_action("MOVE", "", "market");
        const int InitialSecurity = Sim.country().security;
        const int InitialCrime = Sim.country().crime_level;
        const std::string Harm = Sim.player_action("HARM", "non_ai_npc_002");
        const auto& Guard = Sim.ai_npc(ScenarioB.at("guard_ai_npc_id").get<std::string>());
        Require(Guard.current_state_id == ScenarioB.at("expected_state_id").get<int>(), "Scenario B guard state mismatch.");
        Require(AuditFor(Sim, Guard.npc_id, ScenarioB.at("expected_action").get<std::string>()).selected_rule.find("harm_observed") != std::string::npos,
                "Scenario B selected the wrong rule.");
        const auto& Authority = Sim.ai_npc(ScenarioD.at("authority_ai_npc_id").get<std::string>());
        Require(Authority.current_state_id == ScenarioD.at("expected_state_id").get<int>(), "Scenario D authority state mismatch.");
        Require(Sim.country().security == InitialSecurity + ScenarioD.at("security_delta").get<int>(), "Scenario D security mismatch.");
        Require(Sim.country().crime_level == InitialCrime + ScenarioD.at("crime_level_delta").get<int>(), "Scenario D crime mismatch.");
        const auto& CountryEvent = FirstEvent(Sim, "COUNTRY_STATE_CHANGED", Authority.npc_id);
        const auto Path = Sim.causal_path(CountryEvent.event_id);
        Require(Path.front().starts_with(Harm + ":PLAYER_HARMED_NON_AI_NPC"), "Scenario D root mismatch.");
        return Sim;
    }
};

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageCAcceptanceTest,
    "NationSimulation.StageC.Acceptance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageCAcceptanceTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    bool bAllPassed = true;
    std::vector<std::string> Report;
    FStageCTestContext Context;

    const auto RunCase = [&](const char* Name, const std::function<void()>& Body) {
        try
        {
            Body();
            AddInfo(FString::Printf(TEXT("[PASS] %s"), UTF8_TO_TCHAR(Name)));
            Report.push_back(std::string("PASS | ") + Name);
        }
        catch (const std::exception& Exception)
        {
            bAllPassed = false;
            AddError(FString::Printf(TEXT("[FAIL] %s | %s"), UTF8_TO_TCHAR(Name), UTF8_TO_TCHAR(Exception.what())));
            Report.push_back(std::string("FAIL | ") + Name + " | " + Exception.what());
        }
    };

    RunCase("Fixture contract: shared 1 country, 20 AI, 20 NON AI, 255 independent slots", [&] {
        const auto Sim = Simulation::from_fixture(Context.Fixture);
        Require(Sim.country().country_id == Context.Contract.at("country_id").get<std::string>(), "Country id mismatch.");
        Require(Sim.world_seed() == Context.Contract.at("world_seed").get<std::uint64_t>(), "World seed mismatch.");
        Require(Sim.ai_npcs().size() == Context.Contract.at("ai_npc_count").get<std::size_t>(), "AI count mismatch.");
        Require(Sim.non_ai_npcs().size() == Context.Contract.at("non_ai_npc_count").get<std::size_t>(), "NON AI count mismatch.");
        const auto Slots = Context.Contract.at("state_slots_per_ai_npc").get<std::size_t>();
        std::set<std::string> Signatures;
        for (const auto& Npc : Sim.ai_npcs())
        {
            Require(Npc.states.size() == Slots, Npc.npc_id + " state count mismatch.");
            for (std::size_t Index = 5; Index < Npc.states.size(); ++Index)
                Require(Npc.states[Index].undefined && Npc.states[Index].state_name == "UNDEFINED", Npc.npc_id + " undefined slot mismatch.");
            std::string Signature;
            for (std::size_t Index = 0; Index < 5; ++Index) Signature += Npc.states[Index].state_name + Npc.states[Index].state_description;
            Require(Signatures.insert(Signature).second, Npc.npc_id + " reused another state table.");
        }
    });

    RunCase("Scenario A: NON AI good deed reaches AI by rumor", [&] { Context.RunScenarioA(); });

    RunCase("Scenario B: guard observes violence and acts", [&] {
        const auto Sim = Context.RunScenarioBAndD();
        Require(FirstEvent(Sim, "AI_NPC_OBSERVED_EVENT", "ai_npc_001").credibility == 1.0, "Observed credibility mismatch.");
    });

    RunCase("Scenario C: skeptical AI investigates doubtful rumor", [&] {
        const auto& Scenario = Context.Contract.at("scenario_c");
        auto Sim = Simulation::from_fixture(Context.Fixture);
        Sim.player_action("MOVE", "", "residential");
        const std::string Harm = Sim.player_action("HARM", "non_ai_npc_003");
        const int InitialSecurity = Sim.country().security;
        Sim.share_rumor("non_ai_npc_003", "ai_npc_004", Harm, 0.50);
        const auto& Npc = Sim.ai_npc(Scenario.at("investigator_ai_npc_id").get<std::string>());
        Require(Npc.current_state_id == Scenario.at("expected_state_id").get<int>(), "Scenario C state mismatch.");
        Require(AuditFor(Sim, Npc.npc_id, Scenario.at("expected_action").get<std::string>()).selected_rule.find("harm_heard_doubt") != std::string::npos,
                "Scenario C selected the wrong rule.");
        Require(Sim.country().security == InitialSecurity, "Doubtful rumor changed country state.");
    });

    RunCase("Scenario D: AI-to-AI report changes country state", [&] {
        const auto Sim = Context.RunScenarioBAndD();
        const auto& Scenario = Context.Contract.at("scenario_d");
        const auto& CountryEvent = FirstEvent(Sim, "COUNTRY_STATE_CHANGED", Scenario.at("authority_ai_npc_id").get<std::string>());
        const auto Path = Sim.causal_path(CountryEvent.event_id);
        const auto Expected = Scenario.at("path_event_types").get<std::vector<std::string>>();
        std::vector<std::string> Actual;
        for (const auto& Entry : Path) Actual.push_back(Entry.substr(Entry.find(':') + 1));
        Require(Actual == Expected, "Stage C Scenario D path differs from shared contract.");
        std::ofstream Output(Context.Output / "scenario_d_causal_path.txt", std::ios::trunc);
        for (const auto& Entry : Path) Output << Entry << '\n';
    });

    RunCase("Scenario E: offline elapsed time and seven-day cap", [&] {
        const auto& Scenario = Context.Contract.at("scenario_e");
        const auto RealSeconds = Scenario.at("real_seconds").get<std::int64_t>();
        const auto GameMinutes = Scenario.at("expected_game_minutes").get<std::int64_t>();
        const auto Limit = Scenario.at("offline_limit_seconds").get<std::int64_t>();
        auto Sim = Simulation::from_fixture(Context.Fixture);
        const auto Result = Sim.advance_offline(RealSeconds);
        Require(Result.applied_real_seconds == RealSeconds && Result.elapsed_game_minutes == GameMinutes, "Time conversion mismatch.");
        Require(Sim.current_world_time_minutes() == GameMinutes, "World time mismatch.");
        auto CappedSim = Simulation::from_fixture(Context.Fixture);
        const auto Capped = CappedSim.advance_offline(Limit + 12345);
        Require(Capped.applied_real_seconds == Limit && Capped.elapsed_game_minutes == Limit, "Offline cap mismatch.");
    });

    RunCase("Scenario F: deterministic replay", [&] {
        const auto First = Context.RunScenarioA();
        const auto Second = Context.RunScenarioA();
        Require(First.canonical_snapshot() == Second.canonical_snapshot(), "Deterministic snapshots differ.");
        Require(First.audit_log_json_lines() == Second.audit_log_json_lines(), "Deterministic audits differ.");
        Require(First.causal_log_json_lines() == Second.causal_log_json_lines(), "Deterministic causal logs differ.");
    });

    RunCase("Scenario G: retained pending save and resume hash equivalence", [&] {
        auto Continuous = Simulation::from_fixture(Context.Fixture);
        Continuous.player_action("MOVE", "", "market");
        Continuous.enqueue_player_action("HARM", "non_ai_npc_002");
        Continuous.process_pending();

        auto Interrupted = Simulation::from_fixture(Context.Fixture);
        Interrupted.player_action("MOVE", "", "market");
        Interrupted.enqueue_player_action("HARM", "non_ai_npc_002");
        Interrupted.process_pending(2);
        Require(!Interrupted.pending_events().empty(), "No pending events at Stage C save point.");
        const auto PendingPath = Context.Output / "mid_chain_pending_save.json";
        const auto ResumedPath = Context.Output / "mid_chain_resumed_save.json";
        Interrupted.save(PendingPath);
        json Pending;
        { std::ifstream Input(PendingPath, std::ios::binary); Input >> Pending; }
        Require(!Pending.at("pending_events").empty(), "Submitted Stage C save contains no pending events.");

        auto Resumed = Simulation::load_save(Context.Fixture, PendingPath);
        Require(!Resumed.pending_events().empty(), "Loaded Stage C save contains no pending events.");
        Resumed.process_pending();
        const std::string ContinuousSnapshot = Continuous.canonical_snapshot();
        const std::string ResumedSnapshot = Resumed.canonical_snapshot();
        const std::string ContinuousHash = Sha256(ContinuousSnapshot);
        const std::string ResumedHash = Sha256(ResumedSnapshot);
        Require(ContinuousHash == ResumedHash, "Stage C continuous/resumed SHA-256 mismatch.");
        { std::ofstream Output(Context.Output / "scenario_g_continuous_snapshot.json", std::ios::trunc); Output << ContinuousSnapshot << '\n'; }
        { std::ofstream Output(Context.Output / "scenario_g_resumed_snapshot.json", std::ios::trunc); Output << ResumedSnapshot << '\n'; }
        const json Evidence{
            {"pending_save_file", PendingPath.filename().string()},
            {"pending_simulation_tick", Pending.at("world").at("simulation_tick")},
            {"pending_next_event_sequence", Pending.at("world").at("next_event_sequence")},
            {"pending_event_count", Pending.at("pending_events").size()},
            {"pending_events", Pending.at("pending_events")},
            {"hash_algorithm", "SHA-256"},
            {"continuous_snapshot_sha256", ContinuousHash},
            {"resumed_snapshot_sha256", ResumedHash},
            {"hashes_match", ContinuousHash == ResumedHash},
            {"continuous_event_count", Continuous.event_history().size()},
            {"resumed_event_count", Resumed.event_history().size()}
        };
        { std::ofstream Output(Context.Output / "scenario_g_evidence.json", std::ios::trunc); Output << Evidence.dump(2) << '\n'; }
        Resumed.save(ResumedPath);
        Require(Simulation::load_save(Context.Fixture, ResumedPath).canonical_snapshot() == Resumed.canonical_snapshot(),
                "Completed Stage C save/load mismatch.");
    });

    RunCase("Chain limit emits an auditable error event", [&] {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        Sim.set_chain_limit_for_test(1);
        Sim.player_action("MOVE", "", "market");
        const auto& Limit = FirstEvent(Sim, "CHAIN_LIMIT_REACHED");
        Require(Limit.payload.at("processed_event_count") == "1" && Sim.pending_events().empty(), "Chain limit evidence mismatch.");
    });

    RunCase("Configured player action surface is wired", [&] {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        Sim.player_action("MOVE", "", "market");
        Sim.player_action("TALK", "non_ai_npc_002");
        Sim.player_action("TRADE", "non_ai_npc_002");
        Sim.player_action("STEAL", "non_ai_npc_002");
        Sim.player_action("WAIT");
        Require(FirstEvent(Sim, "PLAYER_TALKED").target_id == "non_ai_npc_002", "TALK not wired.");
        Require(FirstEvent(Sim, "PLAYER_TRADED").target_id == "non_ai_npc_002", "TRADE not wired.");
        Require(FirstEvent(Sim, "PLAYER_STOLE").target_id == "non_ai_npc_002", "STEAL not wired.");
        auto HelpAi = Simulation::from_fixture(Context.Fixture);
        HelpAi.player_action("MOVE", "", "gate");
        HelpAi.player_action("HELP", "ai_npc_004");
        Require(HelpAi.ai_npc("ai_npc_004").current_state_id == 2, "Direct HELP to AI not wired.");
        auto HarmAi = Simulation::from_fixture(Context.Fixture);
        HarmAi.player_action("MOVE", "", "gate");
        HarmAi.player_action("HARM", "ai_npc_004");
        Require(HarmAi.ai_npc("ai_npc_004").current_state_id == 3, "Direct HARM to AI not wired.");
    });

    RunCase("Audit fields distinguish event actor and decision NPC", [&] {
        const auto Sim = Context.RunScenarioBAndD();
        const auto& Audit = AuditFor(Sim, "ai_npc_002", "CHANGE_COUNTRY_STATE");
        Require(Audit.event_actor_id == "ai_npc_001", "event_actor_id mismatch.");
        Require(Audit.decision_npc_id == "ai_npc_002", "decision_npc_id mismatch.");
        Require(Audit.target_id == "ai_npc_002", "target_id mismatch.");
    });

    try
    {
        const auto Sim = Context.RunScenarioBAndD();
        { std::ofstream Output(Context.Output / "stage_c_audit.jsonl", std::ios::trunc); Output << Sim.audit_log_json_lines(); }
        { std::ofstream Output(Context.Output / "stage_c_causal.jsonl", std::ios::trunc); Output << Sim.causal_log_json_lines(); }
        { std::ofstream Output(Context.Output / "stage_c_reproducible_snapshot.json", std::ios::trunc); Output << Sim.canonical_snapshot() << '\n'; }
    }
    catch (const std::exception& Exception)
    {
        bAllPassed = false;
        AddError(FString::Printf(TEXT("Evidence generation failed: %s"), UTF8_TO_TCHAR(Exception.what())));
    }

    const std::size_t Passed = static_cast<std::size_t>(std::count_if(Report.begin(), Report.end(), [](const std::string& Entry) {
        return Entry.starts_with("PASS | ");
    }));
    Report.push_back("SUMMARY | " + std::to_string(Passed) + "/11 tests passed");
    {
        std::ofstream Output(Context.Output / "acceptance_results.txt", std::ios::trunc);
        for (const auto& Entry : Report) Output << Entry << '\n';
    }
    return bAllPassed;
}

#endif // WITH_DEV_AUTOMATION_TESTS
