#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "StageE/StageESaveMigration.h"
#include "nation_sim/simulation.hpp"

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
using nation_sim::Simulation;
using json = nlohmann::json;

struct FStageETestContext
{
    std::filesystem::path Fixture;
    std::filesystem::path Overlay;
    std::filesystem::path Output;
    FString OutputFString;

    FStageETestContext()
    {
        const FString RepositoryRoot = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
        Fixture = std::filesystem::path(*FPaths::Combine(RepositoryRoot, TEXT("data/stage_a_fixture.json")));
        Overlay = std::filesystem::path(*FPaths::Combine(
            FPaths::ProjectDir(), TEXT("Data/StageE/stage_e_state_definitions.json")));
        OutputFString = FPaths::Combine(RepositoryRoot, TEXT("out/stage-e/test-output"));
        IFileManager::Get().MakeDirectory(*OutputFString, true);
        Output = std::filesystem::path(*OutputFString);
    }
};

void Require(bool Condition, const std::string& Message)
{
    if (!Condition) throw std::runtime_error(Message);
}

Simulation NewSimulation(const FStageETestContext& Context)
{
    FString DefinitionSha256;
    FString Error;
    if (!FStageESaveMigration::Sha256File(FString(Context.Overlay.c_str()), DefinitionSha256, Error))
        throw std::runtime_error(TCHAR_TO_UTF8(*Error));
    return Simulation::from_fixture_with_overlay(Context.Fixture, Context.Overlay, TCHAR_TO_UTF8(*DefinitionSha256));
}

struct FRemoteIncident
{
    std::string SourceNpc;
    std::string EventId;
};

FRemoteIncident CreateRemoteIncident(Simulation& Sim)
{
    const std::set<std::string> TargetLocations{
        Sim.ai_npc("ai_npc_001").current_location_id,
        Sim.ai_npc("ai_npc_012").current_location_id,
        Sim.ai_npc("ai_npc_002").current_location_id};
    const auto It = std::find_if(Sim.non_ai_npcs().begin(), Sim.non_ai_npcs().end(),
        [&](const nation_sim::NonAiNpcState& Npc) { return !TargetLocations.contains(Npc.current_location_id); });
    Require(It != Sim.non_ai_npcs().end(), "remote incident NON AI NPC missing");
    if (Sim.player().current_location_id != It->current_location_id)
        Sim.player_action("MOVE", "", It->current_location_id);
    return {It->npc_id, Sim.player_action("HARM", It->npc_id)};
}

const nation_sim::AuditEntry& LastAudit(const Simulation& Sim, const std::string& NpcId)
{
    const auto It = std::find_if(Sim.audit_log().rbegin(), Sim.audit_log().rend(),
        [&](const nation_sim::AuditEntry& Entry) { return Entry.decision_npc_id == NpcId && !Entry.selected_rule.empty(); });
    if (It == Sim.audit_log().rend()) throw std::runtime_error("audit missing for " + NpcId);
    return *It;
}

bool HasAction(const Simulation& Sim, const std::string& NpcId, const std::string& Action)
{
    return std::any_of(Sim.audit_log().begin(), Sim.audit_log().end(),
        [&](const nation_sim::AuditEntry& Entry) { return Entry.decision_npc_id == NpcId && Entry.selected_action == Action; });
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStageEBehavioralDepthTest,
    "NationSimulation.StageE.BehavioralDepth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageEBehavioralDepthTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FStageETestContext Context;
    json Evidence;
    int Passed = 0;
    int Failed = 0;
    auto Run = [&](const char* Name, auto&& Body)
    {
        try { Body(); AddInfo(FString::Printf(TEXT("PASS | %s"), UTF8_TO_TCHAR(Name))); ++Passed; }
        catch (const std::exception& Exception)
        {
            AddError(FString::Printf(TEXT("FAIL | %s | %s"), UTF8_TO_TCHAR(Name), UTF8_TO_TCHAR(Exception.what())));
            ++Failed;
        }
    };

    Run("60 unique states and 15 legacy entries are active", [&]
    {
        const auto Sim = NewSimulation(Context);
        for (const std::string Id : {"ai_npc_001", "ai_npc_012", "ai_npc_002"})
        {
            const auto& Npc = Sim.ai_npc(Id);
            Require(Npc.current_state_id == 6, Id + " initial Stage E state mismatch");
            for (int StateId = 6; StateId <= 25; ++StateId)
                Require(!Npc.states[static_cast<std::size_t>(StateId - 1)].undefined, Id + " missing state");
            for (int StateId = 1; StateId <= 5; ++StateId)
                Require(!Npc.states[static_cast<std::size_t>(StateId - 1)].transition_rules.empty(), Id + " legacy entry missing");
        }
        const auto HashVector = Context.Output / "sha256_abc.bin";
        { std::ofstream Out(HashVector, std::ios::binary | std::ios::trunc); Out << "abc"; }
        FString Hash, HashError;
        Require(FStageESaveMigration::Sha256File(FString(HashVector.c_str()), Hash, HashError), TCHAR_TO_UTF8(*HashError));
        Require(Hash == TEXT("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"), "SHA-256 known vector mismatch");
        Evidence["state_count"] = 60;
        Evidence["legacy_entry_count"] = 15;
    });

    Run("Scenario E-1 same violence produces distinct roles", [&]
    {
        auto Sim = NewSimulation(Context);
        Sim.set_relationship_metric("ai_npc_001", "player_001", "trust", 30);
        Sim.set_relationship_metric("ai_npc_012", "player_001", "trust", 30);
        if (Sim.player().current_location_id != "market") Sim.player_action("MOVE", "", "market");
        const std::string Root = Sim.player_action("HARM", "non_ai_npc_002");
        Require(Sim.ai_npc("ai_npc_001").current_state_id == 16, "guard did not prepare detention");
        Require(HasAction(Sim, "ai_npc_012", "REFUSE_TRADE"), "broker did not refuse trade");
        Sim.player_action("HARM", "non_ai_npc_002");
        Require(HasAction(Sim, "ai_npc_001", "ARREST"), "guard did not arrest after repeated violence");
        Require(HasAction(Sim, "ai_npc_002", "INVESTIGATE"), "captain did not consider accusation");
        Sim.player_action("WAIT");
        Require(Sim.ai_npc("ai_npc_002").current_state_id == 10, "captain did not execute accusation after deliberation");
        Require(HasAction(Sim, "ai_npc_002", "REPORT"), "captain accusation did not produce an authority report");
        Evidence["E1"] = {{"root_event_id", Root}, {"guard_action", "ARREST"},
                           {"broker_action", "REFUSE_TRADE"}, {"captain_branch", "EXECUTE_ACCUSATION"},
                           {"captain_action", "REPORT"}};
    });

    Run("Scenario E-2 low evidence defers judgment", [&]
    {
        auto Sim = NewSimulation(Context);
        const auto Incident = CreateRemoteIncident(Sim);
        Sim.share_rumor(Incident.SourceNpc, "ai_npc_001", Incident.EventId, 0.2);
        Sim.share_rumor(Incident.SourceNpc, "ai_npc_012", Incident.EventId, 0.2);
        Sim.share_rumor(Incident.SourceNpc, "ai_npc_002", Incident.EventId, 0.2);
        Require(Sim.ai_npc("ai_npc_001").current_state_id == 21, "guard low evidence state mismatch");
        Require(Sim.ai_npc("ai_npc_012").current_state_id == 13, "broker low evidence state mismatch");
        Require(Sim.ai_npc("ai_npc_002").current_state_id == 7, "captain low evidence state mismatch");
        Evidence["E2"] = {21, 13, 7};
    });

    Run("Scenario E-3 relationship reverses guard decision", [&]
    {
        auto Trusted = NewSimulation(Context);
        Trusted.set_relationship_metric("ai_npc_001", "player_001", "trust", 70);
        Trusted.player_action("MOVE", "", "market");
        Trusted.player_action("HARM", "non_ai_npc_002");
        auto Distrusted = NewSimulation(Context);
        Distrusted.set_relationship_metric("ai_npc_001", "player_001", "trust", 30);
        Distrusted.player_action("MOVE", "", "market");
        Distrusted.player_action("HARM", "non_ai_npc_002");
        Require(Trusted.ai_npc("ai_npc_001").current_state_id == 9, "trusted guard did not warn");
        Require(Distrusted.ai_npc("ai_npc_001").current_state_id == 16, "distrusted guard did not prepare detention");
        Evidence["E3"] = {{"high_trust", 9}, {"low_trust", 16}};
    });

    Run("Scenario E-4 country security changes guard policy", [&]
    {
        auto High = NewSimulation(Context);
        High.set_country_parameter("security", 70);
        High.set_relationship_metric("ai_npc_001", "player_001", "trust", 30);
        High.player_action("MOVE", "", "market"); High.player_action("HARM", "non_ai_npc_002");
        auto Low = NewSimulation(Context);
        Low.set_country_parameter("security", 30);
        Low.set_relationship_metric("ai_npc_001", "player_001", "trust", 30);
        Low.player_action("MOVE", "", "market"); Low.player_action("HARM", "non_ai_npc_002");
        Require(High.ai_npc("ai_npc_001").current_state_id == 16, "high security procedure mismatch");
        Require(Low.ai_npc("ai_npc_001").current_state_id == 8, "low security force response mismatch");
        Evidence["E4"] = {{"security_high", 16}, {"security_low", 8}};
    });

    Run("Scenario E-5 timed decay preserves major incidents", [&]
    {
        auto Minor = NewSimulation(Context);
        const auto Incident = CreateRemoteIncident(Minor);
        Minor.share_rumor(Incident.SourceNpc, "ai_npc_001", Incident.EventId, 0.8);
        Require(Minor.ai_npc("ai_npc_001").current_state_id == 7, "minor alert not entered");
        for (int Index = 0; Index < 6; ++Index) Minor.player_action("WAIT");
        Require(Minor.ai_npc("ai_npc_001").current_state_id == 6, "minor alert did not decay");
        auto Major = NewSimulation(Context);
        Major.set_country_parameter("security", 30);
        Major.player_action("MOVE", "", "market"); Major.player_action("HARM", "non_ai_npc_002");
        for (int Index = 0; Index < 6; ++Index) Major.player_action("WAIT");
        Require(Major.ai_npc("ai_npc_001").current_state_id == 8, "major incident was forgotten");
        Evidence["E5"] = {{"minor_after_360", 6}, {"major_after_360", 8}};
    });

    Run("Scenario E-6 broker guard captain chain", [&]
    {
        auto Sim = NewSimulation(Context);
        Sim.set_country_parameter("security", 30);
        Sim.set_relationship_metric("ai_npc_012", "player_001", "commercial_interest", 80);
        Sim.set_relationship_metric("ai_npc_001", "player_001", "trust", 30);
        const auto Incident = CreateRemoteIncident(Sim);
        Sim.share_rumor(Incident.SourceNpc, "ai_npc_012", Incident.EventId, 0.9);
        Require(HasAction(Sim, "ai_npc_012", "REPORT"), "broker did not report guard");
        Require(HasAction(Sim, "ai_npc_001", "REPORT"), "guard did not report captain");
        Require(HasAction(Sim, "ai_npc_002", "REPORT"), "captain did not issue order");
        Require(Sim.ai_npc("ai_npc_001").current_state_id == 16, "guard policy did not change after order");
        Evidence["E6"] = {"ai_npc_012:REPORT", "ai_npc_001:REPORT", "ai_npc_002:REPORT", "ai_npc_001:16"};
        { std::ofstream Out(Context.Output / "stage_e_audit.jsonl"); Out << Sim.audit_log_json_lines(); }
        { std::ofstream Out(Context.Output / "stage_e_causal.jsonl"); Out << Sim.causal_log_json_lines(); }
    });

    Run("Scenario E-7 save resume is deterministic", [&]
    {
        auto Continuous = NewSimulation(Context);
        Continuous.set_country_parameter("security", 30);
        Continuous.set_relationship_metric("ai_npc_012", "player_001", "commercial_interest", 80);
        const auto Incident = CreateRemoteIncident(Continuous);
        Continuous.enqueue_rumor(Incident.SourceNpc, "ai_npc_012", Incident.EventId, 0.9);
        Continuous.process_pending(5);
        auto Interrupted = Continuous;
        Continuous.process_pending();
        const auto SavePath = Context.Output / "stage_e_mid_chain_save.json";
        Interrupted.save(SavePath);
        FString DefinitionSha256;
        FString DefinitionHashError;
        Require(FStageESaveMigration::Sha256File(FString(Context.Overlay.c_str()), DefinitionSha256, DefinitionHashError),
            TCHAR_TO_UTF8(*DefinitionHashError));
        auto Resumed = Simulation::load_save_with_overlay(
            Context.Fixture, Context.Overlay, SavePath, TCHAR_TO_UTF8(*DefinitionSha256));
        Resumed.process_pending();
        Require(Continuous.canonical_snapshot() == Resumed.canonical_snapshot(), "Stage E save/resume snapshot mismatch");
        const auto ContinuousPath = Context.Output / "stage_e_continuous_snapshot.json";
        const auto ResumedPath = Context.Output / "stage_e_resumed_snapshot.json";
        { std::ofstream Out(ContinuousPath); Out << Continuous.canonical_snapshot() << '\n'; }
        { std::ofstream Out(ResumedPath); Out << Resumed.canonical_snapshot() << '\n'; }
        FString ContinuousSha, ResumedSha, Error;
        Require(FStageESaveMigration::Sha256File(FString(ContinuousPath.c_str()), ContinuousSha, Error), TCHAR_TO_UTF8(*Error));
        Require(FStageESaveMigration::Sha256File(FString(ResumedPath.c_str()), ResumedSha, Error), TCHAR_TO_UTF8(*Error));
        Require(ContinuousSha == ResumedSha, "Stage E SHA-256 mismatch");
        json Saved;
        { std::ifstream Input(SavePath, std::ios::binary); Input >> Saved; }
        json Runtime = json::array();
        for (const auto& Npc : Saved.at("ai_runtime"))
        {
            const auto Id = Npc.at("npc_id").get<std::string>();
            if (Id != "ai_npc_001" && Id != "ai_npc_012" && Id != "ai_npc_002") continue;
            Runtime.push_back({{"npc_id", Id}, {"current_state_id", Npc.at("current_state_id")},
                               {"state_entered_at", Npc.at("stage_e_runtime").at("state_entered_at")},
                               {"timed_transition_at", Npc.at("stage_e_runtime").at("timed_transition_at")}});
        }
        Evidence["E7"] = {{"pending_at_save", Interrupted.pending_events().size()},
                           {"definition_sha256", Saved.at("stage_e_overlay").at("definition_sha256")},
                           {"npc_runtime", Runtime},
                           {"continuous_sha256", TCHAR_TO_UTF8(*ContinuousSha)},
                           {"resumed_sha256", TCHAR_TO_UTF8(*ResumedSha)}};
    });

    Evidence["summary"] = {{"passed", Passed}, {"failed", Failed}};
    { std::ofstream Out(Context.Output / "stage_e_scenario_evidence.json"); Out << Evidence.dump(2) << '\n'; }
    AddInfo(FString::Printf(TEXT("STAGE_E_BEHAVIOR | %d/8 tests passed"), Passed));
    return Failed == 0;
}

#endif
