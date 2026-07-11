#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDGameMode.h"
#include "StageD/StageDHudWidget.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDPlayerCharacter.h"
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
#include <vector>

namespace
{
using nation_sim::AuditEntry;
using nation_sim::Simulation;
using json = nlohmann::json;

void StageDRequire(bool Condition, const std::string& Message)
{
    if (!Condition) throw std::runtime_error(Message);
}

std::string Sha256(const std::string& Value)
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
    std::vector<std::uint8_t> Bytes(Value.begin(), Value.end());
    const std::uint64_t BitLength = static_cast<std::uint64_t>(Bytes.size()) * 8;
    Bytes.push_back(0x80);
    while (Bytes.size() % 64 != 56) Bytes.push_back(0);
    for (int Shift = 56; Shift >= 0; Shift -= 8) Bytes.push_back(static_cast<std::uint8_t>(BitLength >> Shift));
    std::array<std::uint32_t, 8> Hash{
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    const auto RotateRight = [](std::uint32_t Input, unsigned Count) { return (Input >> Count) | (Input << (32 - Count)); };
    for (std::size_t Offset = 0; Offset < Bytes.size(); Offset += 64)
    {
        std::array<std::uint32_t, 64> Words{};
        for (std::size_t Index = 0; Index < 16; ++Index)
        {
            const auto Base = Offset + Index * 4;
            Words[Index] = (static_cast<std::uint32_t>(Bytes[Base]) << 24) |
                (static_cast<std::uint32_t>(Bytes[Base + 1]) << 16) |
                (static_cast<std::uint32_t>(Bytes[Base + 2]) << 8) | static_cast<std::uint32_t>(Bytes[Base + 3]);
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
    for (const auto Part : Hash) Output << std::setw(8) << Part;
    return Output.str();
}

const AuditEntry& FindAudit(const Simulation& Sim, const std::string& NpcId, const std::string& Action)
{
    const auto It = std::find_if(Sim.audit_log().begin(), Sim.audit_log().end(), [&](const AuditEntry& Entry) {
        return Entry.decision_npc_id == NpcId && Entry.selected_action == Action;
    });
    StageDRequire(It != Sim.audit_log().end(), "missing audit for " + NpcId + ":" + Action);
    return *It;
}

struct FStageDTestContext
{
    std::filesystem::path Fixture;
    std::filesystem::path Output;
    FString RepositoryRoot;

    FStageDTestContext()
    {
        RepositoryRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
        Fixture = std::filesystem::path(*FPaths::Combine(RepositoryRoot, TEXT("data/stage_a_fixture.json")));
        Output = std::filesystem::path(*FPaths::Combine(RepositoryRoot, TEXT("out/stage-d/test-output")));
        std::filesystem::create_directories(Output);
    }

    Simulation MarketHarm(bool bProcessAll = true) const
    {
        auto Sim = Simulation::from_fixture(Fixture);
        Sim.player_action("MOVE", "", "market");
        Sim.enqueue_player_action("HARM", "non_ai_npc_002");
        Sim.process_pending(bProcessAll ? static_cast<std::size_t>(-1) : 1);
        return Sim;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageDPlayableVerticalSliceTest,
    "NationSimulation.StageD.PlayableVerticalSlice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageDPlayableVerticalSliceTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FStageDTestContext Context;
    bool bPassed = true;
    std::vector<std::string> Report;
    json Evidence;

    const auto RunCase = [&](const char* Name, const std::function<void()>& Body)
    {
        try
        {
            Body();
            AddInfo(FString::Printf(TEXT("[PASS] %s"), UTF8_TO_TCHAR(Name)));
            Report.push_back(std::string("PASS | ") + Name);
        }
        catch (const std::exception& Exception)
        {
            bPassed = false;
            AddError(FString::Printf(TEXT("[FAIL] %s | %s"), UTF8_TO_TCHAR(Name), UTF8_TO_TCHAR(Exception.what())));
            Report.push_back(std::string("FAIL | ") + Name + " | " + Exception.what());
        }
    };

    RunCase("Playable fixture exposes five exact locations and 20 AI plus 20 NON AI", [&]
    {
        const auto Sim = Simulation::from_fixture(Context.Fixture);
        StageDRequire(Sim.ai_npcs().size() == 20 && Sim.non_ai_npcs().size() == 20, "NPC count mismatch");
        std::set<std::string> Locations;
        for (const auto& Npc : Sim.ai_npcs()) Locations.insert(Npc.current_location_id);
        for (const auto& Npc : Sim.non_ai_npcs()) Locations.insert(Npc.current_location_id);
        StageDRequire(Locations == std::set<std::string>{"capital", "market", "tavern", "residential", "gate"}, "location ids mismatch");
    });

    RunCase("Market HARM is witnessed by AI 001 007 012 and 017", [&]
    {
        auto Sim = Context.MarketHarm(false);
        const auto& Harm = Sim.event("evt_0000000003");
        const std::set<std::string> Witnesses(Harm.observed_by.begin(), Harm.observed_by.end());
        const std::set<std::string> Expected{"ai_npc_001", "ai_npc_007", "ai_npc_012", "ai_npc_017"};
        StageDRequire(Witnesses == Expected, "market witness set mismatch");
        Evidence["witnesses"] = Witnesses;
    });

    RunCase("Core selects REPORT WARN REFUSE_TRADE and FLEE", [&]
    {
        const auto Sim = Context.MarketHarm();
        StageDRequire(FindAudit(Sim, "ai_npc_001", "REPORT").selected_rule.find("harm_observed") != std::string::npos, "REPORT rule mismatch");
        FindAudit(Sim, "ai_npc_007", "WARN");
        FindAudit(Sim, "ai_npc_012", "REFUSE_TRADE");
        FindAudit(Sim, "ai_npc_017", "FLEE");
        Evidence["visible_actions"] = {"REPORT", "WARN", "REFUSE_TRADE", "FLEE"};
    });

    RunCase("AI 001 report is decided by AI 002 with unambiguous audit identity", [&]
    {
        const auto Sim = Context.MarketHarm();
        const auto& Audit = FindAudit(Sim, "ai_npc_002", "CHANGE_COUNTRY_STATE");
        StageDRequire(Audit.event_actor_id == "ai_npc_001", "report actor mismatch");
        StageDRequire(Audit.decision_npc_id == "ai_npc_002", "decision npc mismatch");
        StageDRequire(Audit.target_id == "ai_npc_002", "target mismatch");
    });

    RunCase("Country state changes security 50 to 48 and crime 10 to 12", [&]
    {
        const auto Sim = Context.MarketHarm();
        StageDRequire(Sim.country().security == 48, "security mismatch");
        StageDRequire(Sim.country().crime_level == 12, "crime level mismatch");
        Evidence["country"] = {{"security_before", 50}, {"security_after", 48}, {"crime_before", 10}, {"crime_after", 12}};
    });

    RunCase("Mid-chain save retains pending events and resumes deterministically", [&]
    {
        auto Continuous = Simulation::from_fixture(Context.Fixture);
        Continuous.player_action("MOVE", "", "market");
        Continuous.enqueue_player_action("HARM", "non_ai_npc_002");
        Continuous.process_pending();

        auto Interrupted = Simulation::from_fixture(Context.Fixture);
        Interrupted.player_action("MOVE", "", "market");
        Interrupted.enqueue_player_action("HARM", "non_ai_npc_002");
        Interrupted.process_pending(4);
        const auto Save = Context.Output / "mid_chain_pending_save.json";
        Interrupted.save(Save);
        const auto Pending = Interrupted.pending_events().size();
        StageDRequire(Pending > 0, "mid-chain save contains no pending events");
        auto Resumed = Simulation::load_save(Context.Fixture, Save);
        Resumed.process_pending();
        const std::string ContinuousHash = Sha256(Continuous.canonical_snapshot());
        const std::string ResumedHash = Sha256(Resumed.canonical_snapshot());
        StageDRequire(ContinuousHash == ResumedHash, "save/reload hash mismatch");
        Evidence["mid_chain"] = {{"pending_event_count", Pending}, {"continuous_sha256", ContinuousHash}, {"resumed_sha256", ResumedHash}, {"match", true}};
    });

    RunCase("Startup offline progress uses the core seven-day cap", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        const auto Result = Sim.advance_offline(700000);
        StageDRequire(Result.applied_real_seconds == 604800, "offline cap mismatch");
        StageDRequire(Result.elapsed_game_minutes == 604800, "offline game time mismatch");
        Evidence["offline"] = {{"requested_real_seconds", 700000}, {"applied_real_seconds", Result.applied_real_seconds}};
    });

    RunCase("Unreal presentation classes and capital map are present", [&]
    {
        StageDRequire(AStageDPlayerCharacter::StaticClass()->IsChildOf(ACharacter::StaticClass()), "third-person character missing");
        StageDRequire(AStageDNpcActor::StaticClass()->IsChildOf(AActor::StaticClass()), "NPC actor missing");
        StageDRequire(UStageDHudWidget::StaticClass()->IsChildOf(UUserWidget::StaticClass()), "UMG widget missing");
        StageDRequire(UNationSimulationGameInstanceSubsystem::StaticClass()->IsChildOf(UGameInstanceSubsystem::StaticClass()), "persistent subsystem missing");
        StageDRequire(FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/StageD_Capital.umap"))), "capital map missing");
    });

    RunCase("Actor and UI sources contain no duplicated causal-core decisions", [&]
    {
        const TArray<FString> PresentationSources = {
            TEXT("StageDNpcActor.cpp"), TEXT("StageDPlayerCharacter.cpp"), TEXT("StageDHudWidget.cpp"),
            TEXT("StageDGameMode.cpp"), TEXT("StageDLocationVolume.cpp")
        };
        for (const FString& File : PresentationSources)
        {
            const FString Path = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD"), File);
            FString Source;
            StageDRequire(FFileHelper::LoadFileToString(Source, *Path), "cannot inspect presentation source");
            StageDRequire(!Source.Contains(TEXT("nation_sim::")) && !Source.Contains(TEXT("simulation.hpp")), "core decision type leaked into " + std::string(TCHAR_TO_UTF8(*File)));
        }
    });

    Evidence["summary"] = bPassed ? "9/9 tests passed" : "FAILED";
    std::ofstream EvidenceOutput(Context.Output / "stage_d_scenario_evidence.json", std::ios::trunc);
    EvidenceOutput << Evidence.dump(2) << '\n';
    std::ofstream ResultOutput(Context.Output / "stage_d_acceptance_results.txt", std::ios::trunc);
    for (const auto& Line : Report) ResultOutput << Line << '\n';
    ResultOutput << "SUMMARY | " << (bPassed ? "9/9 tests passed" : "FAILED") << '\n';
    return bPassed;
}

#endif
