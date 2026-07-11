#include "StageEStateValidator.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace nation_sim_stage_e {
namespace {
using json = nlohmann::json;

json read_json(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open JSON: " + path.string());
    json value;
    input >> value;
    return value;
}

void issue(std::vector<ValidationIssue>& out, std::string severity, std::string npc,
           std::optional<int> state, std::string rule, std::string code,
           std::string message, const std::filesystem::path& source) {
    out.push_back({std::move(severity), std::move(npc), state, std::move(rule),
                   std::move(code), std::move(message), source.generic_string()});
}

std::string condition_signature(const json& rule) {
    json normalized;
    for (const char* key : {"trigger_event_type","subject_event_type","perception","required_actor_id",
                            "min_credibility","max_credibility","min_evidence","max_evidence",
                            "relationship_target","relationship_metric","min_relationship","max_relationship",
                            "country_parameter","min_country","max_country","min_player_evaluation",
                            "max_player_evaluation","min_state_minutes"}) {
        if (rule.contains(key)) normalized[key] = rule.at(key);
    }
    return normalized.dump();
}

bool interval_overlap(double left_min, double left_max, double right_min, double right_max) {
    return std::max(left_min, right_min) <= std::min(left_max, right_max);
}

bool input_overlap(const json& left, const json& right) {
    if (left.value("trigger_event_type", "") != right.value("trigger_event_type", "") ||
        left.value("subject_event_type", "") != right.value("subject_event_type", "") ||
        left.value("perception", "") != right.value("perception", "")) return false;
    const auto left_actor = left.value("required_actor_id", "");
    const auto right_actor = right.value("required_actor_id", "");
    if (!left_actor.empty() && !right_actor.empty() && left_actor != right_actor) return false;
    return interval_overlap(left.value("min_credibility", 0.0), left.value("max_credibility", 1.0),
                            right.value("min_credibility", 0.0), right.value("max_credibility", 1.0)) &&
           interval_overlap(left.value("min_evidence", 0.0), left.value("max_evidence", 1.0),
                            right.value("min_evidence", 0.0), right.value("max_evidence", 1.0)) &&
           interval_overlap(left.value("min_relationship", -100.0), left.value("max_relationship", 100.0),
                            right.value("min_relationship", -100.0), right.value("max_relationship", 100.0)) &&
           interval_overlap(left.value("min_country", -1000000.0), left.value("max_country", 1000000.0),
                            right.value("min_country", -1000000.0), right.value("max_country", 1000000.0)) &&
           interval_overlap(left.value("min_player_evaluation", -1000000.0), left.value("max_player_evaluation", 1000000.0),
                            right.value("min_player_evaluation", -1000000.0), right.value("max_player_evaluation", 1000000.0)) &&
           interval_overlap(left.value("min_state_minutes", 0.0), 1000000000.0,
                            right.value("min_state_minutes", 0.0), 1000000000.0);
}

std::vector<json> rules_for_state(const json& npc, const json& overlay, int state_id) {
    std::vector<json> rules;
    for (const auto& state : npc.at("states")) {
        if (state.value("state_id", 0) != state_id) continue;
        for (const auto& rule : state.at("transition_rules")) rules.push_back(rule);
        break;
    }
    for (const auto& rule : npc.at("legacy_entry_rules"))
        if (rule.value("source_state_id", 0) == state_id) rules.push_back(rule);
    for (const auto& rule : overlay.value("cross_npc_rules", json::array()))
        if (rule.value("npc_id", "") == npc.value("npc_id", "") && rule.value("source_state_id", 0) == state_id)
            rules.push_back(rule);
    return rules;
}
}

std::vector<ValidationIssue> StateDefinitionValidator::validate(
    const std::filesystem::path& overlay_path, const std::filesystem::path& fixture_path) {
    const json overlay = read_json(overlay_path);
    const json fixture = read_json(fixture_path);
    std::vector<ValidationIssue> out;
    const std::set<std::string> expected_npcs{"ai_npc_001","ai_npc_012","ai_npc_002"};
    const std::set<std::string> metrics(overlay.at("relationship_metrics").begin(), overlay.at("relationship_metrics").end());
    const std::map<std::string, std::set<std::string>> profile_metrics{
        {"ai_npc_001", {"trust", "fear", "loyalty"}},
        {"ai_npc_012", {"trust", "fear", "commercial_interest"}},
        {"ai_npc_002", {"trust", "fear", "utility"}}
    };
    const std::set<std::string> event_types(fixture.at("event_types").begin(), fixture.at("event_types").end());
    std::map<std::string, std::string> roles;
    std::set<std::string> known_npcs{"player_001","EVENT_ACTOR"};
    std::set<std::string> fixture_actions;
    for (const auto& npc : fixture.at("ai_npcs")) {
        roles[npc.at("npc_id").get<std::string>()] = npc.at("role").get<std::string>();
        known_npcs.insert(npc.at("npc_id").get<std::string>());
        for (const auto& state : npc.at("states")) {
            if (state.value("undefined", false)) continue;
            for (const auto& action : state.at("action_candidates")) {
                fixture_actions.insert(action.value("type", ""));
            }
        }
    }
    for (const auto& npc : fixture.at("non_ai_npcs")) known_npcs.insert(npc.at("non_ai_npc_id").get<std::string>());
    if (overlay.value("base_fixture_id", "") != fixture.value("fixture_id", ""))
        issue(out,"ERROR","",std::nullopt,"","OVERLAY_BASE_MISMATCH","base fixture id mismatch",overlay_path);
    const auto range = overlay.value("state_id_range", json::array());
    if (range != json::array({6, 25}))
        issue(out,"ERROR","",std::nullopt,"","OVERLAY_BASE_MISMATCH","state_id_range must be [6,25]",overlay_path);
    const std::set<std::string> declared_npcs(overlay.at("target_npc_ids").begin(), overlay.at("target_npc_ids").end());
    if (declared_npcs != expected_npcs)
        issue(out,"ERROR","",std::nullopt,"","OVERLAY_BASE_MISMATCH","target_npc_ids mismatch",overlay_path);
    for (const auto& action : overlay.at("allowed_action_types")) {
        const auto type = action.get<std::string>();
        if (!fixture_actions.contains(type))
            issue(out,"ERROR","",std::nullopt,"","ACTION_UNDEFINED","allowed action is absent from fixture: " + type,overlay_path);
    }

    std::map<std::string, std::string> canonical_tables;
    std::set<std::string> loaded_npcs;
    for (const auto& npc : overlay.at("npcs")) {
        const std::string npc_id = npc.value("npc_id", "");
        const auto profile = profile_metrics.find(npc_id);
        const auto metric_allowed = [&](const std::string& metric) {
            return metrics.contains(metric) && profile != profile_metrics.end() && profile->second.contains(metric);
        };
        loaded_npcs.insert(npc_id);
        if (!expected_npcs.contains(npc_id) || !roles.contains(npc_id) || roles[npc_id] != npc.value("expected_role", ""))
            issue(out,"ERROR",npc_id,std::nullopt,"","OVERLAY_BASE_MISMATCH","NPC id or fixture role mismatch",overlay_path);
        const auto& states = npc.at("states");
        std::set<int> state_ids;
        if (states.size() < 20)
            issue(out,"ERROR",npc_id,std::nullopt,"","STATE_COUNT_INSUFFICIENT","fewer than 20 Stage E states",overlay_path);
        for (const auto& state : states) {
            const int id = state.value("state_id", 0);
            if (!state_ids.insert(id).second) issue(out,"ERROR",npc_id,id,"","STATE_ID_DUPLICATE","duplicate state id",overlay_path);
            if (id < 6 || id > 25) issue(out,"ERROR",npc_id,id,"","STATE_ID_OUT_OF_RANGE","Stage E state id outside 6..25",overlay_path);
            const auto& dialogues = state.at("dialogue_candidates");
            std::set<std::string> dialogue_ids;
            if (dialogues.empty()) issue(out,"ERROR",npc_id,id,"","DIALOGUE_UNDEFINED","dialogue candidates are empty",overlay_path);
            for (const auto& dialogue : dialogues) {
                const std::string dialogue_id = dialogue.value("dialogue_id", "");
                if (dialogue_id.empty() || dialogue.value("text", "").empty() || dialogue.value("priority", 0) <= 0 ||
                    !dialogue_ids.insert(dialogue_id).second)
                    issue(out,"ERROR",npc_id,id,dialogue_id,"DIALOGUE_UNDEFINED","invalid dialogue definition",overlay_path);
                if (dialogue.value("cooldown", 0) < 0)
                    issue(out,"ERROR",npc_id,id,dialogue_id,"COOLDOWN_INVALID","negative dialogue cooldown",overlay_path);
                if (dialogue.value("once_only", false) && dialogue.value("cooldown", 0) > 0)
                    issue(out,"WARNING",npc_id,id,dialogue_id,"COOLDOWN_REDUNDANT","once-only dialogue has a redundant cooldown",overlay_path);
            }
            const auto& state_actions = state.at("action_candidates");
            if (state_actions.empty()) issue(out,"ERROR",npc_id,id,"","ACTION_UNDEFINED","action candidates are empty",overlay_path);
            for (const auto& action : state_actions) {
                const std::string type = action.value("type", "");
                if (!fixture_actions.contains(type)) issue(out,"ERROR",npc_id,id,"","ACTION_UNDEFINED","unknown action type: " + type,overlay_path);
                const std::string target = action.value("target_id", "");
                if (!target.empty() && !known_npcs.contains(target))
                    issue(out,"ERROR",npc_id,id,"","NPC_REFERENCE_UNKNOWN","unknown action target: " + target,overlay_path);
            }
            const auto rules = rules_for_state(npc, overlay, id);
            std::map<std::string, std::string> signatures;
            for (std::size_t index = 0; index < rules.size(); ++index) {
                const auto& rule = rules[index];
                const std::string rule_id = rule.value("rule_id", "");
                const int target = rule.value("target_state_id", 0);
                if (target < 6 || target > 25 || !std::any_of(states.begin(), states.end(), [&](const json& s){return s.value("state_id",0)==target;})) {
                    issue(out,"ERROR",npc_id,id,rule_id,"TRANSITION_TARGET_UNDEFINED","transition target is undefined",overlay_path);
                    issue(out,"ERROR",npc_id,id,rule_id,"UNUSED_STATE_TARGET","transition targets an unused slot",overlay_path);
                }
                for (const char* key : {"trigger_event_type","subject_event_type"}) {
                    const std::string type = rule.value(key, "");
                    if (!event_types.contains(type)) issue(out,"ERROR",npc_id,id,rule_id,"EVENT_TYPE_UNKNOWN","unknown event type: " + type,overlay_path);
                }
                const std::string actor = rule.value("required_actor_id", "");
                if (!actor.empty() && !known_npcs.contains(actor)) issue(out,"ERROR",npc_id,id,rule_id,"NPC_REFERENCE_UNKNOWN","unknown event actor",overlay_path);
                const std::string metric = rule.value("relationship_metric", "");
                if (!metric.empty() && !metric_allowed(metric))
                    issue(out,"ERROR",npc_id,id,rule_id,"RELATIONSHIP_METRIC_UNKNOWN","unknown relationship metric for NPC profile",overlay_path);
                if (rule.value("cooldown", 0) < 0) issue(out,"ERROR",npc_id,id,rule_id,"COOLDOWN_INVALID","negative cooldown",overlay_path);
                if (rule.value("once_only", false) && rule.value("cooldown", 0) > 0)
                    issue(out,"WARNING",npc_id,id,rule_id,"COOLDOWN_REDUNDANT","once-only rule has a redundant cooldown",overlay_path);
                const std::string signature = condition_signature(rule);
                if (signatures.contains(signature)) issue(out,"ERROR",npc_id,id,rule_id,"CONDITION_DUPLICATE","duplicate condition",overlay_path);
                signatures[signature] = rule_id;
                for (std::size_t other = 0; other < index; ++other) {
                    if (rule.value("priority",0) == rules[other].value("priority",0) && input_overlap(rule, rules[other]))
                        issue(out,"ERROR",npc_id,id,rule_id,"PRIORITY_CONFLICT","overlapping rules share priority",overlay_path);
                }
            }
            for (const auto& [metric, value] : state.value("relationship_modifiers", json::object()).items())
                if (!metric_allowed(metric))
                    issue(out,"ERROR",npc_id,id,"","RELATIONSHIP_METRIC_UNKNOWN","unknown state relationship modifier for NPC profile",overlay_path);
            const auto& time_rules = state.at("time_based_rules");
            if (state.value("is_terminal", false) && (!rules.empty() || !time_rules.empty()))
                issue(out,"ERROR",npc_id,id,"","TERMINAL_OUTGOING_TRANSITION","terminal state has outgoing transition",overlay_path);
            for (const auto& rule : time_rules) {
                const std::string rule_id = rule.value("rule_id", "");
                const auto after = rule.value("after_game_minutes", 0);
                const auto repeat = rule.value("repeat_interval_minutes", 0);
                const int target = rule.value("target_state_id", 0);
                if (!state_ids.contains(target) && !std::any_of(states.begin(), states.end(), [&](const json& value) {
                        return value.value("state_id", 0) == target;
                    })) {
                    issue(out,"ERROR",npc_id,id,rule_id,"TRANSITION_TARGET_UNDEFINED","time transition target is undefined",overlay_path);
                    issue(out,"ERROR",npc_id,id,rule_id,"UNUSED_STATE_TARGET","time transition targets an unused slot",overlay_path);
                }
                if (after < 0 || repeat < 0) issue(out,"ERROR",npc_id,id,rule_id,"COOLDOWN_INVALID","negative time value",overlay_path);
                if (rule.value("once_only",false) && repeat > 0)
                    issue(out,"ERROR",npc_id,id,rule_id,"ONCE_ONLY_CONTRADICTION","once-only time rule repeats",overlay_path);
                if (after == 0 && rule.value("target_state_id",0) == id)
                    issue(out,"ERROR",npc_id,id,rule_id,"IMMEDIATE_LOOP","immediate self transition",overlay_path);
            }
        }
        for (int id = 6; id <= 25; ++id) if (!state_ids.contains(id))
            issue(out,"ERROR",npc_id,id,"","STATE_COUNT_INSUFFICIENT","required state id missing",overlay_path);

        std::set<int> legacy_sources;
        for (const auto& rule : npc.at("legacy_entry_rules")) {
            const int source = rule.value("source_state_id", 0);
            const int target = rule.value("target_state_id", 0);
            legacy_sources.insert(source);
            if (source < 1 || source > 5 || !state_ids.contains(target))
                issue(out,"ERROR",npc_id,source,rule.value("rule_id",""),"LEGACY_ENTRY_UNDEFINED","legacy entry missing or undefined",overlay_path);
        }
        for (int source = 1; source <= 5; ++source) if (!legacy_sources.contains(source))
            issue(out,"ERROR",npc_id,source,"","LEGACY_ENTRY_UNDEFINED","legacy source has no explicit entry",overlay_path);
        for (int source = 1; source <= 5; ++source) {
            std::vector<json> entries;
            for (const auto& rule : npc.at("legacy_entry_rules"))
                if (rule.value("source_state_id", 0) == source) entries.push_back(rule);
            std::set<std::string> signatures;
            for (std::size_t index = 0; index < entries.size(); ++index) {
                const auto signature = condition_signature(entries[index]);
                const auto rule_id = entries[index].value("rule_id", "");
                if (!signatures.insert(signature).second)
                    issue(out,"ERROR",npc_id,source,rule_id,"CONDITION_DUPLICATE","duplicate legacy entry condition",overlay_path);
                for (std::size_t other = 0; other < index; ++other)
                    if (entries[index].value("priority", 0) == entries[other].value("priority", 0) && input_overlap(entries[index], entries[other]))
                        issue(out,"ERROR",npc_id,source,rule_id,"PRIORITY_CONFLICT","overlapping legacy entries share priority",overlay_path);
            }
        }

        std::map<int,std::set<int>> graph;
        for (const auto& state : states) {
            const int source = state.value("state_id",0);
            for (const auto& rule : state.at("transition_rules")) graph[source].insert(rule.value("target_state_id",0));
            for (const auto& rule : state.at("time_based_rules")) graph[source].insert(rule.value("target_state_id",0));
        }
        for (const auto& rule : overlay.value("cross_npc_rules", json::array()))
            if (rule.value("npc_id","") == npc_id) graph[rule.value("source_state_id",0)].insert(rule.value("target_state_id",0));
        std::set<int> reachable{6};
        std::queue<int> queue; queue.push(6);
        while (!queue.empty()) { const int current=queue.front(); queue.pop(); for (int target : graph[current]) if (reachable.insert(target).second) queue.push(target); }
        for (int id : state_ids) if (!reachable.contains(id)) issue(out,"ERROR",npc_id,id,"","STATE_UNREACHABLE","state unreachable from state 6",overlay_path);

        std::map<int, std::set<int>> immediate_graph;
        for (const auto& state : states) {
            const int source = state.value("state_id", 0);
            for (const auto& rule : state.at("time_based_rules"))
                if (rule.value("after_game_minutes", 0) == 0) immediate_graph[source].insert(rule.value("target_state_id", 0));
            for (const auto& rule : state.at("transition_rules"))
                if (rule.value("trigger_event_type", "") == "AI_NPC_CHANGED_STATE") immediate_graph[source].insert(rule.value("target_state_id", 0));
        }
        std::set<int> loop_reported;
        for (const int start : state_ids) {
            std::set<int> visited;
            std::vector<int> pending(immediate_graph[start].begin(), immediate_graph[start].end());
            bool loop = false;
            while (!pending.empty() && !loop) {
                const int current = pending.back();
                pending.pop_back();
                if (current == start) { loop = true; break; }
                if (!visited.insert(current).second) continue;
                pending.insert(pending.end(), immediate_graph[current].begin(), immediate_graph[current].end());
            }
            if (loop && loop_reported.insert(start).second)
                issue(out,"ERROR",npc_id,start,"","IMMEDIATE_LOOP","immediate transition cycle contains state",overlay_path);
        }

        const std::string canonical = states.dump();
        for (const auto& [other, table] : canonical_tables) if (table == canonical)
            issue(out,"ERROR",npc_id,std::nullopt,"","NPC_TABLE_IDENTICAL","state table identical to " + other,overlay_path);
        canonical_tables[npc_id] = canonical;
        for (const auto& [target, values] : npc.value("relationship_defaults", json::object()).items()) {
            if (!known_npcs.contains(target)) issue(out,"ERROR",npc_id,std::nullopt,"","NPC_REFERENCE_UNKNOWN","unknown relationship target",overlay_path);
            for (const auto& [metric, value] : values.items()) if (!metric_allowed(metric))
                issue(out,"ERROR",npc_id,std::nullopt,"","RELATIONSHIP_METRIC_UNKNOWN","unknown default metric for NPC profile",overlay_path);
        }

        const std::map<std::string, std::set<int>> scenario_states{
            {"ai_npc_001", {6,7,8,9,10,11,15,16,17,21}},
            {"ai_npc_012", {6,8,9,10,13,14,18}},
            {"ai_npc_002", {6,7,8,9,10,12,13,25}}
        };
        const auto direct_states = scenario_states.find(npc_id);
        if (direct_states != scenario_states.end())
            for (const int id : state_ids) if (!direct_states->second.contains(id))
                issue(out,"INFO",npc_id,id,"","STATE_UNUSED_BY_SCENARIO","state is not directly referenced by Scenario E-1 through E-7",overlay_path);
    }
    if (loaded_npcs != expected_npcs) issue(out,"ERROR","",std::nullopt,"","OVERLAY_BASE_MISMATCH","target NPC set mismatch",overlay_path);
    return out;
}

std::string StateDefinitionValidator::json_lines(const std::vector<ValidationIssue>& issues) {
    std::ostringstream output;
    for (const auto& value : issues) {
        json row{{"severity",value.severity},{"npc_id",value.npc_id},
                 {"state_id",value.state_id ? json(*value.state_id) : json(nullptr)},
                 {"rule_id",value.rule_id.empty() ? json(nullptr) : json(value.rule_id)},
                 {"error_code",value.error_code},{"message",value.message},{"source_file",value.source_file}};
        output << row.dump() << '\n';
    }
    return output.str();
}

int StateDefinitionValidator::error_count(const std::vector<ValidationIssue>& issues) {
    return static_cast<int>(std::count_if(issues.begin(), issues.end(),
        [](const ValidationIssue& issue){ return issue.severity == "ERROR"; }));
}
} // namespace nation_sim_stage_e
