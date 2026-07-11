#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "StageEStateValidator.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
using json = nlohmann::json;
using nation_sim_stage_e::StateDefinitionValidator;
using nation_sim_stage_e::ValidationIssue;

struct FValidationContext
{
    std::filesystem::path Fixture;
    std::filesystem::path Overlay;
    std::filesystem::path Output;

    FValidationContext()
    {
        const FString RepositoryRoot = FPaths::ConvertRelativePathToFull(
            FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
        Fixture = std::filesystem::path(*FPaths::Combine(RepositoryRoot, TEXT("data/stage_a_fixture.json")));
        Overlay = std::filesystem::path(*FPaths::Combine(
            FPaths::ProjectDir(), TEXT("Data/StageE/stage_e_state_definitions.json")));
        const FString OutputDirectory = FPaths::Combine(RepositoryRoot, TEXT("out/stage-e/validation-negative"));
        IFileManager::Get().MakeDirectory(*OutputDirectory, true);
        Output = std::filesystem::path(*OutputDirectory);
    }
};

json ReadJson(const std::filesystem::path& Path)
{
    std::ifstream Input(Path, std::ios::binary);
    if (!Input) throw std::runtime_error("cannot read validation fixture");
    json Value;
    Input >> Value;
    return Value;
}

void WriteJson(const std::filesystem::path& Path, const json& Value)
{
    std::ofstream Output(Path, std::ios::binary | std::ios::trunc);
    if (!Output) throw std::runtime_error("cannot write validation fixture");
    Output << Value.dump(2) << '\n';
}

bool HasIssue(const std::vector<ValidationIssue>& Issues, const std::string& Code, const std::string& Severity = {})
{
    return std::any_of(Issues.begin(), Issues.end(), [&](const ValidationIssue& Issue)
    {
        return Issue.error_code == Code && (Severity.empty() || Issue.severity == Severity);
    });
}

void RemoveIncomingTarget(json& Overlay, const std::string& NpcId, int TargetState)
{
    auto RemoveTarget = [TargetState](json& Rules)
    {
        Rules.erase(std::remove_if(Rules.begin(), Rules.end(), [TargetState](const json& Rule)
        {
            return Rule.value("target_state_id", 0) == TargetState;
        }), Rules.end());
    };
    for (auto& Npc : Overlay.at("npcs"))
    {
        if (Npc.value("npc_id", "") != NpcId) continue;
        for (auto& State : Npc.at("states"))
        {
            RemoveTarget(State.at("transition_rules"));
            RemoveTarget(State.at("time_based_rules"));
        }
        RemoveTarget(Npc.at("legacy_entry_rules"));
    }
    auto& CrossRules = Overlay.at("cross_npc_rules");
    CrossRules.erase(std::remove_if(CrossRules.begin(), CrossRules.end(), [&](const json& Rule)
    {
        return Rule.value("npc_id", "") == NpcId && Rule.value("target_state_id", 0) == TargetState;
    }), CrossRules.end());
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStageEStateValidationTest,
    "NationSimulation.StageE.StateDefinitionValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageEStateValidationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FValidationContext Context;
    const json Baseline = ReadJson(Context.Overlay);
    int Passed = 0;
    int Failed = 0;
    json Evidence = json::array();

    const auto Run = [&](const std::string& Name, const std::string& ExpectedCode,
                         const std::string& ExpectedSeverity, const std::function<void(json&)>& Mutate)
    {
        try
        {
            json Broken = Baseline;
            Mutate(Broken);
            const auto Path = Context.Output / (Name + ".json");
            WriteJson(Path, Broken);
            const auto Issues = StateDefinitionValidator::validate(Path, Context.Fixture);
            if (!HasIssue(Issues, ExpectedCode, ExpectedSeverity))
                throw std::runtime_error("expected " + ExpectedSeverity + " " + ExpectedCode + " was not emitted");
            Evidence.push_back({{"case", Name}, {"expected_code", ExpectedCode},
                                {"expected_severity", ExpectedSeverity}, {"issue_count", Issues.size()}});
            AddInfo(FString::Printf(TEXT("PASS | %s -> %s"), UTF8_TO_TCHAR(Name.c_str()), UTF8_TO_TCHAR(ExpectedCode.c_str())));
            ++Passed;
        }
        catch (const std::exception& Exception)
        {
            AddError(FString::Printf(TEXT("FAIL | %s | %s"), UTF8_TO_TCHAR(Name.c_str()), UTF8_TO_TCHAR(Exception.what())));
            ++Failed;
        }
    };

    try
    {
        const auto Issues = StateDefinitionValidator::validate(Context.Overlay, Context.Fixture);
        if (StateDefinitionValidator::error_count(Issues) != 0) throw std::runtime_error("baseline emitted ERROR");
        if (!HasIssue(Issues, "STATE_UNUSED_BY_SCENARIO", "INFO")) throw std::runtime_error("baseline INFO missing");
        for (const auto& Row : Issues)
        {
            const auto Parsed = json::parse(StateDefinitionValidator::json_lines({Row}));
            for (const char* Field : {"severity", "npc_id", "state_id", "rule_id", "error_code", "message", "source_file"})
                if (!Parsed.contains(Field)) throw std::runtime_error(std::string("JSONL field missing: ") + Field);
        }
        AddInfo(TEXT("PASS | baseline ERROR=0 and JSONL contract"));
        ++Passed;
    }
    catch (const std::exception& Exception)
    {
        AddError(FString::Printf(TEXT("FAIL | baseline | %s"), UTF8_TO_TCHAR(Exception.what())));
        ++Failed;
    }

    Run("state_count", "STATE_COUNT_INSUFFICIENT", "ERROR", [](json& Value) { Value["npcs"][0]["states"].erase(Value["npcs"][0]["states"].end() - 1); });
    Run("state_duplicate", "STATE_ID_DUPLICATE", "ERROR", [](json& Value) { Value["npcs"][0]["states"].push_back(Value["npcs"][0]["states"][0]); });
    Run("state_range", "STATE_ID_OUT_OF_RANGE", "ERROR", [](json& Value) { Value["npcs"][0]["states"][19]["state_id"] = 26; });
    Run("target_undefined", "TRANSITION_TARGET_UNDEFINED", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["transition_rules"][0]["target_state_id"] = 26; });
    Run("state_unreachable", "STATE_UNREACHABLE", "ERROR", [](json& Value) { RemoveIncomingTarget(Value, "ai_npc_001", 25); });
    Run("terminal_outgoing", "TERMINAL_OUTGOING_TRANSITION", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["is_terminal"] = true; });
    Run("condition_duplicate", "CONDITION_DUPLICATE", "ERROR", [](json& Value)
    {
        json Rule = Value["npcs"][0]["states"][0]["transition_rules"][0];
        Rule["rule_id"] = "negative_duplicate";
        Value["npcs"][0]["states"][0]["transition_rules"].push_back(Rule);
    });
    Run("priority_conflict", "PRIORITY_CONFLICT", "ERROR", [](json& Value)
    {
        json Rule = Value["npcs"][0]["states"][0]["transition_rules"][0];
        Rule["rule_id"] = "negative_priority";
        Rule["min_country"] = 20;
        Rule["max_country"] = 50;
        Value["npcs"][0]["states"][0]["transition_rules"].push_back(Rule);
    });
    Run("once_contradiction", "ONCE_ONLY_CONTRADICTION", "ERROR", [](json& Value)
    {
        Value["npcs"][0]["states"][0]["time_based_rules"].push_back({
            {"rule_id", "negative_once"}, {"after_game_minutes", 1}, {"target_state_id", 7},
            {"priority", 1}, {"once_only", true}, {"repeat_interval_minutes", 1}});
    });
    Run("cooldown_invalid", "COOLDOWN_INVALID", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["dialogue_candidates"][0]["cooldown"] = -1; });
    Run("dialogue_undefined", "DIALOGUE_UNDEFINED", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["dialogue_candidates"] = json::array(); });
    Run("action_undefined", "ACTION_UNDEFINED", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["action_candidates"][0]["type"] = "NOT_AN_ACTION"; });
    Run("immediate_loop", "IMMEDIATE_LOOP", "ERROR", [](json& Value)
    {
        Value["npcs"][0]["states"][0]["time_based_rules"].push_back({
            {"rule_id", "negative_loop_a"}, {"after_game_minutes", 0}, {"target_state_id", 7},
            {"priority", 1}, {"once_only", false}, {"repeat_interval_minutes", 0}});
        Value["npcs"][0]["states"][1]["time_based_rules"].push_back({
            {"rule_id", "negative_loop_b"}, {"after_game_minutes", 0}, {"target_state_id", 6},
            {"priority", 1}, {"once_only", false}, {"repeat_interval_minutes", 0}});
    });
    Run("npc_table_identical", "NPC_TABLE_IDENTICAL", "ERROR", [](json& Value) { Value["npcs"][1]["states"] = Value["npcs"][0]["states"]; });
    Run("unused_target", "UNUSED_STATE_TARGET", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["transition_rules"][0]["target_state_id"] = 26; });
    Run("event_unknown", "EVENT_TYPE_UNKNOWN", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["transition_rules"][0]["trigger_event_type"] = "UNKNOWN_EVENT"; });
    Run("npc_unknown", "NPC_REFERENCE_UNKNOWN", "ERROR", [](json& Value) { Value["npcs"][0]["states"][0]["action_candidates"][0]["target_id"] = "unknown_npc"; });
    Run("metric_unknown", "RELATIONSHIP_METRIC_UNKNOWN", "ERROR", [](json& Value)
    {
        Value["npcs"][0]["states"][0]["transition_rules"][0]["relationship_target"] = "player_001";
        Value["npcs"][0]["states"][0]["transition_rules"][0]["relationship_metric"] = "commercial_interest";
    });
    Run("overlay_mismatch", "OVERLAY_BASE_MISMATCH", "ERROR", [](json& Value) { Value["base_fixture_id"] = "not_stage_a"; });
    Run("legacy_undefined", "LEGACY_ENTRY_UNDEFINED", "ERROR", [](json& Value) { Value["npcs"][0]["legacy_entry_rules"].erase(Value["npcs"][0]["legacy_entry_rules"].begin()); });
    Run("cooldown_redundant", "COOLDOWN_REDUNDANT", "WARNING", [](json& Value)
    {
        Value["npcs"][0]["states"][0]["dialogue_candidates"][0]["once_only"] = true;
        Value["npcs"][0]["states"][0]["dialogue_candidates"][0]["cooldown"] = 10;
    });

    Evidence.push_back({{"summary", {{"passed", Passed}, {"failed", Failed}}}});
    WriteJson(Context.Output / "stage_e_validator_negative_evidence.json", Evidence);
    AddInfo(FString::Printf(TEXT("STAGE_E_VALIDATOR | %d/22 tests passed"), Passed));
    return Failed == 0 && Passed == 22;
}

#endif
