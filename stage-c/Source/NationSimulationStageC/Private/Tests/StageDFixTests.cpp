#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#include "StageD/StageDCoreBoundary.h"
#include "StageD/StageDSaveMetadata.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using nation_sim::Simulation;
using json = nlohmann::json;

void FixRequire(bool Condition, const std::string& Message)
{
    if (!Condition) throw std::runtime_error(Message);
}

struct FStageDFixContext
{
    FString RepositoryRoot;
    std::filesystem::path Fixture;
    std::filesystem::path Output;

    FStageDFixContext()
    {
        RepositoryRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
        Fixture = std::filesystem::path(*FPaths::Combine(RepositoryRoot, TEXT("data/stage_a_fixture.json")));
        Output = std::filesystem::path(*FPaths::Combine(RepositoryRoot, TEXT("out/stage-d/fix-output")));
        std::filesystem::create_directories(Output);
    }

    FString Path(const TCHAR* Name) const
    {
        return FString((Output / TCHAR_TO_UTF8(Name)).c_str());
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageDFixTest,
    "NationSimulation.StageD.Fixes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageDFixTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    FStageDFixContext Context;
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

    RunCase("Verified timestamp metadata binds epoch to the exact save JSON", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        const FString Save = Context.Path(TEXT("metadata_normal_save.json"));
        const FString Metadata = Save + TEXT(".utc");
        Sim.save(std::filesystem::path(*Save));
        FString Error;
        FixRequire(FStageDSaveMetadata::WriteVerified(Save, Metadata, 1700000000, Error), TCHAR_TO_UTF8(*Error));
        const auto Read = FStageDSaveMetadata::ReadValidated(Save, Metadata);
        FixRequire(Read.bValid && Read.SavedAtUtcEpoch == 1700000000 && !Read.SaveSha1.IsEmpty(), "valid metadata did not round-trip");
        Evidence["metadata_normal"] = {{"saved_at_utc_epoch", Read.SavedAtUtcEpoch}, {"save_sha1", TCHAR_TO_UTF8(*Read.SaveSha1)}};
    });

    RunCase("Timestamp metadata write failure fails the save result and reports an error", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        const FString Save = Context.Path(TEXT("metadata_failure_save.json"));
        Sim.save(std::filesystem::path(*Save));
        const FString ParentBlocker = Context.Path(TEXT("metadata_parent_is_file"));
        FixRequire(FFileHelper::SaveStringToFile(TEXT("blocker"), *ParentBlocker), "could not create failure fixture");
        FString Error;
        const bool bWritten = FStageDSaveMetadata::WriteVerified(
            Save, FPaths::Combine(ParentBlocker, TEXT("stage_d_save.json.utc")), 1700000001, Error);
        FixRequire(!bWritten, "metadata write unexpectedly succeeded");
        FixRequire(!Error.IsEmpty(), "metadata failure was not auditable");
        Evidence["metadata_failure"] = {{"save_result", false}, {"error", TCHAR_TO_UTF8(*Error)}};
    });

    RunCase("Stale or mismatched timestamp metadata is rejected instead of used offline", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        const FString Save = Context.Path(TEXT("metadata_mismatch_save.json"));
        const FString Metadata = Save + TEXT(".utc");
        Sim.save(std::filesystem::path(*Save));
        FString Error;
        FixRequire(FStageDSaveMetadata::WriteVerified(Save, Metadata, 1700000002, Error), TCHAR_TO_UTF8(*Error));
        FixRequire(FFileHelper::SaveStringToFile(TEXT("tampered"), *Save), "could not tamper save fixture");
        const auto Read = FStageDSaveMetadata::ReadValidated(Save, Metadata);
        FixRequire(!Read.bValid && Read.SavedAtUtcEpoch == 1700000002 && !Read.Error.IsEmpty(), "mismatch was not rejected");
        Evidence["metadata_mismatch"] = {{"offline_time_used", false}, {"error", TCHAR_TO_UTF8(*Read.Error)}};
    });

    RunCase("Pending MOVE to the same destination is suppressed", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        Sim.enqueue_player_action("MOVE", "", "market");
        const std::size_t Before = Sim.pending_events().size();
        FString Reason;
        FixRequire(!FStageDCoreBoundary::ShouldEnqueueMove(Sim, TEXT("market"), Reason), "duplicate MOVE was accepted");
        FixRequire(FStageDCoreBoundary::HasPendingMoveTo(Sim, TEXT("market")), "pending MOVE was not detected");
        FixRequire(Sim.pending_events().size() == Before, "duplicate check changed the queue");
        Evidence["move_duplicate"] = {{"destination", "market"}, {"pending_move_count", 1}, {"suppressed", true}};
    });

    RunCase("Different destination works and a previous destination can be revisited after leaving", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        Sim.player_action("MOVE", "", "market");
        FString Reason;
        FixRequire(FStageDCoreBoundary::ShouldEnqueueMove(Sim, TEXT("gate"), Reason), "different destination was rejected");
        Sim.player_action("MOVE", "", "gate");
        FixRequire(FStageDCoreBoundary::ShouldEnqueueMove(Sim, TEXT("market"), Reason), "revisit after leaving was rejected");
    });

    RunCase("Interaction boundary accepts only active same-location targets within 350 uu", [&]
    {
        FString Reason;
        FStageDInteractionCheck Check{true, true, true, 200.0f, 350.0f};
        FixRequire(FStageDCoreBoundary::ValidateInteraction(Check, Reason), "valid interaction was rejected");
        Check.bSameLocation = false;
        FixRequire(!FStageDCoreBoundary::ValidateInteraction(Check, Reason) && Reason.Contains(TEXT("different location")), "remote location was accepted");
        Check = {true, true, true, 351.0f, 350.0f};
        FixRequire(!FStageDCoreBoundary::ValidateInteraction(Check, Reason) && Reason.Contains(TEXT("out of interaction range")), "out-of-range target was accepted");
        Check = {true, true, false, 100.0f, 350.0f};
        FixRequire(!FStageDCoreBoundary::ValidateInteraction(Check, Reason) && Reason.Contains(TEXT("inactive")), "inactive target was accepted");
        Check = {false, false, false, 100.0f, 350.0f};
        FixRequire(!FStageDCoreBoundary::ValidateInteraction(Check, Reason) && Reason.Contains(TEXT("does not exist")), "missing target was accepted");
    });

    RunCase("Invalid remote interaction creates no causal event", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        Sim.player_action("MOVE", "", "market");
        const std::size_t BeforeHistory = Sim.event_history().size();
        const std::size_t BeforePending = Sim.pending_events().size();
        FString Reason;
        const FStageDInteractionCheck Remote{true, false, true, 100.0f, 350.0f};
        if (FStageDCoreBoundary::ValidateInteraction(Remote, Reason)) Sim.enqueue_player_action("HARM", "non_ai_npc_002");
        FixRequire(Sim.event_history().size() == BeforeHistory && Sim.pending_events().size() == BeforePending,
            "invalid interaction generated a causal event");
    });

    RunCase("Scenario D valid target still reaches the accepted country result", [&]
    {
        auto Sim = Simulation::from_fixture(Context.Fixture);
        Sim.player_action("MOVE", "", "market");
        FString Reason;
        const FStageDInteractionCheck Valid{true, true, true, 200.0f, 350.0f};
        FixRequire(FStageDCoreBoundary::ValidateInteraction(Valid, Reason), "Scenario D target failed interaction guard");
        Sim.player_action("HARM", "non_ai_npc_002");
        FixRequire(Sim.country().security == 48 && Sim.country().crime_level == 12, "Scenario D regression");
    });

    Evidence["summary"] = bPassed ? "8/8 tests passed" : "FAILED";
    std::ofstream EvidenceOutput(Context.Output / "stage_d_fix_evidence.json", std::ios::trunc);
    EvidenceOutput << Evidence.dump(2) << '\n';
    std::ofstream ResultOutput(Context.Output / "stage_d_fix_results.txt", std::ios::trunc);
    for (const auto& Line : Report) ResultOutput << Line << '\n';
    ResultOutput << "SUMMARY | " << (bPassed ? "8/8 tests passed" : "FAILED") << '\n';
    return bPassed;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageDTalkDialogueTest,
    "NationSimulation.StageD.TalkDialogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageDTalkDialogueTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FStageDFixContext Context;
    const auto Sim = Simulation::from_fixture(Context.Fixture);

    const FString AiDialogue = FStageDCoreBoundary::ResolveTalkDialogue(Sim, TEXT("ai_npc_001"));
    TestEqual(TEXT("AI TALK uses the current state's JSON dialogue candidate"),
        AiDialogue, FString(TEXT("Fixture baseline response for ai_npc_001")));

    const FString NonAiDialogue = FStageDCoreBoundary::ResolveTalkDialogue(Sim, TEXT("non_ai_npc_009"));
    TestEqual(TEXT("NON AI TALK uses the fixture basic_profile"),
        NonAiDialogue, FString(TEXT("Temporary Stage A profile 9")));

    TestTrue(TEXT("Unknown NPC cannot produce fabricated dialogue"),
        FStageDCoreBoundary::ResolveTalkDialogue(Sim, TEXT("missing_npc")).IsEmpty());
    return !HasAnyErrors();
}

#endif
