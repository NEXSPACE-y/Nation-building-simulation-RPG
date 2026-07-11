#include "nation_sim/simulation.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace nation_sim {
namespace {

using json = nlohmann::json;

template <typename T>
T required(const json& value, const char* key) {
    if (!value.contains(key)) {
        throw std::runtime_error(std::string("missing required JSON field: ") + key);
    }
    return value.at(key).get<T>();
}

json event_to_json(const Event& event) {
    return {
        {"event_id", event.event_id}, {"event_type", event.event_type},
        {"actor_id", event.actor_id}, {"actor_type", event.actor_type},
        {"target_id", event.target_id}, {"target_type", event.target_type},
        {"location_id", event.location_id}, {"country_id", event.country_id},
        {"occurred_at", event.occurred_at}, {"simulation_tick", event.simulation_tick},
        {"observed_by", event.observed_by}, {"heard_by", event.heard_by},
        {"participants", event.participants}, {"evidence_level", event.evidence_level},
        {"credibility", event.credibility}, {"source_event_id", event.source_event_id},
        {"root_event_id", event.root_event_id}, {"payload", event.payload},
        {"processed_by", event.processed_by}, {"resulting_events", event.resulting_events}
    };
}

Event event_from_json(const json& value) {
    Event event;
    event.event_id = required<std::string>(value, "event_id");
    event.event_type = required<std::string>(value, "event_type");
    event.actor_id = value.value("actor_id", "");
    event.actor_type = value.value("actor_type", "");
    event.target_id = value.value("target_id", "");
    event.target_type = value.value("target_type", "");
    event.location_id = value.value("location_id", "");
    event.country_id = value.value("country_id", "");
    event.occurred_at = value.value("occurred_at", std::int64_t{});
    event.simulation_tick = value.value("simulation_tick", std::int64_t{});
    event.observed_by = value.value("observed_by", std::vector<std::string>{});
    event.heard_by = value.value("heard_by", std::vector<std::string>{});
    event.participants = value.value("participants", std::vector<std::string>{});
    event.evidence_level = value.value("evidence_level", 1.0);
    event.credibility = value.value("credibility", 1.0);
    event.source_event_id = value.value("source_event_id", "");
    event.root_event_id = value.value("root_event_id", event.event_id);
    event.payload = value.value("payload", std::map<std::string, std::string>{});
    event.processed_by = value.value("processed_by", std::vector<std::string>{});
    event.resulting_events = value.value("resulting_events", std::vector<std::string>{});
    return event;
}

json audit_to_json(const AuditEntry& audit) {
    return {
        {"timestamp", audit.timestamp}, {"simulation_tick", audit.simulation_tick}, {"event_id", audit.event_id},
        {"root_event_id", audit.root_event_id}, {"event_actor_id", audit.event_actor_id},
        {"decision_npc_id", audit.decision_npc_id}, {"target_id", audit.target_id},
        {"current_state", audit.current_state}, {"matched_rules", audit.matched_rules},
        {"selected_rule", audit.selected_rule}, {"next_state", audit.next_state},
        {"selected_dialogue", audit.selected_dialogue},
        {"selected_action", audit.selected_action},
        {"generated_events", audit.generated_events},
        {"country_state_changes", audit.country_state_changes},
        {"rejected_rules", audit.rejected_rules},
        {"random_seed", audit.random_seed}
    };
}

AuditEntry audit_from_json(const json& value) {
    AuditEntry audit;
    audit.timestamp = value.value("timestamp", std::int64_t{});
    audit.simulation_tick = value.value("simulation_tick", std::int64_t{});
    audit.event_id = value.value("event_id", "");
    audit.root_event_id = value.value("root_event_id", "");
    audit.event_actor_id = value.value("event_actor_id", "");
    audit.decision_npc_id = value.value("decision_npc_id", "");
    audit.target_id = value.value("target_id", "");
    audit.current_state = value.value("current_state", 0);
    audit.matched_rules = value.value("matched_rules", std::vector<std::string>{});
    audit.selected_rule = value.value("selected_rule", "");
    audit.next_state = value.value("next_state", 0);
    audit.selected_dialogue = value.value("selected_dialogue", "");
    audit.selected_action = value.value("selected_action", "");
    audit.generated_events = value.value("generated_events", std::vector<std::string>{});
    audit.country_state_changes = value.value("country_state_changes", std::vector<std::string>{});
    audit.rejected_rules = value.value("rejected_rules", std::vector<std::string>{});
    audit.random_seed = value.value("random_seed", std::uint64_t{});
    return audit;
}

json country_to_json(const CountryState& country) {
    return {
        {"country_id", country.country_id}, {"name", country.name},
        {"stability", country.stability}, {"security", country.security},
        {"economy", country.economy}, {"food", country.food},
        {"military", country.military}, {"public_support", country.public_support},
        {"authority", country.authority}, {"crime_level", country.crime_level},
        {"active_issues", country.active_issues}, {"updated_at", country.updated_at}
    };
}

void load_country_runtime(CountryState& country, const json& value) {
    country.country_id = required<std::string>(value, "country_id");
    country.name = required<std::string>(value, "name");
    country.stability = required<int>(value, "stability");
    country.security = required<int>(value, "security");
    country.economy = required<int>(value, "economy");
    country.food = required<int>(value, "food");
    country.military = required<int>(value, "military");
    country.public_support = required<int>(value, "public_support");
    country.authority = required<int>(value, "authority");
    country.crime_level = required<int>(value, "crime_level");
    country.active_issues = value.value("active_issues", std::vector<std::string>{});
    country.updated_at = value.value("updated_at", std::int64_t{});
}

json player_to_json(const PlayerState& player) {
    return {
        {"player_id", player.player_id}, {"country_id", player.country_id},
        {"current_location_id", player.current_location_id}, {"status", player.status},
        {"inventory", player.inventory}, {"funds", player.funds},
        {"reputation", player.reputation}, {"crime_record", player.crime_record},
        {"achievement_tags", player.achievement_tags}, {"action_history", player.action_history},
        {"last_login_at", player.last_login_at}, {"created_at", player.created_at},
        {"updated_at", player.updated_at}
    };
}

void load_player_runtime(PlayerState& player, const json& value) {
    player.player_id = required<std::string>(value, "player_id");
    player.country_id = required<std::string>(value, "country_id");
    player.current_location_id = required<std::string>(value, "current_location_id");
    player.status = value.value("status", "ACTIVE");
    player.inventory = value.value("inventory", std::vector<std::string>{});
    player.funds = value.value("funds", 0);
    player.reputation = value.value("reputation", 0);
    player.crime_record = value.value("crime_record", 0);
    player.achievement_tags = value.value("achievement_tags", std::vector<std::string>{});
    player.action_history = value.value("action_history", std::vector<std::string>{});
    player.last_login_at = value.value("last_login_at", std::int64_t{});
    player.created_at = value.value("created_at", std::int64_t{});
    player.updated_at = value.value("updated_at", std::int64_t{});
}

json ai_runtime_to_json(const AiNpcState& npc) {
    return {
        {"npc_id", npc.npc_id}, {"current_location_id", npc.current_location_id},
        {"current_state_id", npc.current_state_id},
        {"player_evaluation", npc.player_evaluation}, {"relationships", npc.relationships},
        {"current_goal", npc.current_goal}, {"known_events", npc.known_events},
        {"active_action", npc.active_action}, {"status", npc.status},
        {"used_rules", npc.used_rules}, {"rule_last_used_tick", npc.rule_last_used_tick},
        {"used_dialogues", npc.used_dialogues},
        {"dialogue_last_used_tick", npc.dialogue_last_used_tick},
        {"updated_at", npc.updated_at}
    };
}

json non_ai_runtime_to_json(const NonAiNpcState& npc) {
    return {
        {"npc_id", npc.npc_id}, {"current_location_id", npc.current_location_id},
        {"current_activity", npc.current_activity}, {"temporary_memory", npc.temporary_memory},
        {"spawned_at", npc.spawned_at}, {"despawn_policy", npc.despawn_policy}
    };
}

std::string action_event_type(const std::string& action, bool target_is_ai) {
    if (action == "HELP") return target_is_ai ? "PLAYER_HELPED_AI_NPC" : "PLAYER_HELPED_NON_AI_NPC";
    if (action == "HARM") return target_is_ai ? "PLAYER_HARMED_AI_NPC" : "PLAYER_HARMED_NON_AI_NPC";
    if (action == "TALK") return "PLAYER_TALKED";
    if (action == "TRADE") return "PLAYER_TRADED";
    if (action == "STEAL") return "PLAYER_STOLE";
    if (action == "MOVE") return "PLAYER_LEFT_LOCATION";
    if (action == "WAIT") return "TIME_ELAPSED";
    throw std::invalid_argument("unsupported player action: " + action);
}

double event_salience(const std::string& event_type) {
    if (event_type == "PLAYER_HARMED_NON_AI_NPC" || event_type == "PLAYER_HARMED_AI_NPC") return 0.95;
    if (event_type == "PLAYER_STOLE") return 0.85;
    if (event_type == "PLAYER_HELPED_NON_AI_NPC" || event_type == "PLAYER_HELPED_AI_NPC") return 0.65;
    if (event_type == "PLAYER_TRADED") return 0.50;
    if (event_type == "PLAYER_TALKED") return 0.35;
    return 0.40;
}

bool contains_string(const std::vector<std::string>& values, const std::string& value) {
    return std::find(values.begin(), values.end(), value) != values.end();
}

TransitionRule stage_e_rule_from_json(const json& value) {
    TransitionRule rule;
    rule.rule_id = required<std::string>(value, "rule_id");
    rule.trigger_event_type = required<std::string>(value, "trigger_event_type");
    rule.subject_event_type = required<std::string>(value, "subject_event_type");
    rule.perception = required<std::string>(value, "perception");
    rule.min_credibility = value.value("min_credibility", 0.0);
    rule.max_credibility = value.value("max_credibility", 1.0);
    rule.min_evidence = value.value("min_evidence", 0.0);
    rule.max_evidence = value.value("max_evidence", 1.0);
    rule.required_actor_id = value.value("required_actor_id", "");
    rule.target_state_id = required<int>(value, "target_state_id");
    rule.priority = required<int>(value, "priority");
    rule.cooldown = value.value("cooldown", std::int64_t{});
    rule.once_only = value.value("once_only", false);
    rule.player_evaluation_delta = value.value("player_evaluation_delta", 0);
    rule.relationship_metric = value.value("relationship_metric", "");
    rule.relationship_delta = value.value("relationship_delta", 0);
    rule.relationship_target = value.value("relationship_target", "");
    rule.min_relationship = value.value("min_relationship", -1000000);
    rule.max_relationship = value.value("max_relationship", 1000000);
    rule.country_parameter = value.value("country_parameter", "");
    rule.min_country = value.value("min_country", -1000000);
    rule.max_country = value.value("max_country", 1000000);
    rule.min_player_evaluation = value.value("min_player_evaluation", -1000000);
    rule.max_player_evaluation = value.value("max_player_evaluation", 1000000);
    rule.min_state_minutes = value.value("min_state_minutes", std::int64_t{});
    return rule;
}

StateDefinition stage_e_state_from_json(const json& value) {
    StateDefinition state;
    state.state_id = required<int>(value, "state_id");
    state.state_name = required<std::string>(value, "state_name");
    state.state_description = required<std::string>(value, "state_description");
    state.undefined = false;
    state.current_goal = required<std::string>(value, "current_goal");
    state.goal_modifier = state.current_goal;
    for (const auto& dialogue_value : value.at("dialogue_candidates")) {
        state.dialogue_candidates.push_back({
            required<std::string>(dialogue_value, "dialogue_id"),
            required<std::string>(dialogue_value, "text"),
            required<std::string>(dialogue_value, "tone"),
            required<int>(dialogue_value, "priority"),
            dialogue_value.value("once_only", false),
            dialogue_value.value("cooldown", std::int64_t{})
        });
    }
    for (const auto& action_value : value.at("action_candidates")) {
        ActionCandidate action;
        action.action_type = required<std::string>(action_value, "type");
        action.target_id = action_value.value("target_id", "");
        action.priority = required<int>(action_value, "priority");
        action.credibility_factor = action_value.value("credibility_factor", 1.0);
        for (const auto& effect_value : action_value.at("country_effects")) {
            action.country_effects.push_back({required<std::string>(effect_value, "parameter"),
                                              required<int>(effect_value, "delta"),
                                              required<std::string>(effect_value, "reason")});
        }
        state.action_candidates.push_back(std::move(action));
    }
    for (const auto& rule_value : value.at("transition_rules")) {
        state.transition_rules.push_back(stage_e_rule_from_json(rule_value));
    }
    for (const auto& time_value : value.at("time_based_rules")) {
        state.time_based_rules.push_back({required<std::string>(time_value, "rule_id"),
                                          required<std::int64_t>(time_value, "after_game_minutes"),
                                          required<int>(time_value, "target_state_id"),
                                          required<int>(time_value, "priority"),
                                          time_value.value("once_only", false),
                                          time_value.value("repeat_interval_minutes", std::int64_t{})});
    }
    state.player_evaluation_modifier = value.value("player_evaluation_modifier", 0);
    state.relationship_modifiers = value.value("relationship_modifiers", std::map<std::string, int>{});
    for (const auto& effect_value : value.at("world_effect_candidates")) {
        state.world_effect_candidates.push_back({required<std::string>(effect_value, "parameter"),
                                                 required<int>(effect_value, "delta"),
                                                 required<std::string>(effect_value, "reason")});
    }
    state.priority = required<int>(value, "priority");
    state.is_terminal = value.value("is_terminal", false);
    return state;
}

json stage_e_runtime_to_json(const AiNpcState& npc) {
    json evaluations = json::array();
    for (const auto& evaluation : npc.last_rule_evaluations) {
        evaluations.push_back({{"rule_id", evaluation.rule_id}, {"selected", evaluation.selected},
                               {"reason", evaluation.reason}});
    }
    return {
        {"state_entered_at", npc.state_entered_at},
        {"timed_transition_at", npc.timed_transition_at ? json(*npc.timed_transition_at) : json(nullptr)},
        {"legacy_state_pending_stage_e_entry", npc.legacy_state_pending_stage_e_entry},
        {"evidence_evaluation", {{"source_event_id", npc.evidence_evaluation.source_event_id},
                                  {"credibility", npc.evidence_evaluation.credibility},
                                  {"evidence_level", npc.evidence_evaluation.evidence_level},
                                  {"perception", npc.evidence_evaluation.perception}}},
        {"last_transition_reason", npc.last_transition_reason},
        {"last_rule_evaluations", evaluations}
    };
}

void load_stage_e_runtime(AiNpcState& npc, const json& value, std::int64_t world_minutes) {
    npc.state_entered_at = value.value("state_entered_at", world_minutes);
    if (value.contains("timed_transition_at") && !value.at("timed_transition_at").is_null()) {
        npc.timed_transition_at = value.at("timed_transition_at").get<std::int64_t>();
    } else {
        npc.timed_transition_at.reset();
    }
    npc.legacy_state_pending_stage_e_entry = value.value(
        "legacy_state_pending_stage_e_entry", npc.current_state_id >= 1 && npc.current_state_id <= 5);
    const auto& evidence = value.contains("evidence_evaluation")
        ? value.at("evidence_evaluation") : json::object();
    npc.evidence_evaluation.source_event_id = evidence.value("source_event_id", "");
    npc.evidence_evaluation.credibility = evidence.value("credibility", 0.0);
    npc.evidence_evaluation.evidence_level = evidence.value("evidence_level", 0.0);
    npc.evidence_evaluation.perception = evidence.value("perception", "NONE");
    npc.last_transition_reason = value.value("last_transition_reason", "MIGRATED_FROM_STAGE_D");
    npc.last_rule_evaluations.clear();
    for (const auto& evaluation : value.value("last_rule_evaluations", json::array())) {
        npc.last_rule_evaluations.push_back({evaluation.value("rule_id", ""),
                                             evaluation.value("selected", false),
                                             evaluation.value("reason", "")});
    }
}

} // namespace

Simulation Simulation::from_fixture(const std::filesystem::path& fixture_path) {
    std::ifstream input(fixture_path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open fixture: " + fixture_path.string());
    }
    json root;
    input >> root;

    Simulation simulation;
    simulation.fixture_path_ = fixture_path;
    simulation.fixture_id_ = required<std::string>(root, "fixture_id");
    if (!root.value("fixture_only", false)) {
        throw std::runtime_error("Stage A requires fixture_only=true");
    }
    simulation.schema_version_ = required<std::string>(root, "schema_version");
    simulation.simulation_version_ = required<std::string>(root, "simulation_version");
    simulation.fixture_schema_version_ = simulation.schema_version_;
    simulation.fixture_simulation_version_ = simulation.simulation_version_;

    const auto& world = root.at("world");
    simulation.world_seed_ = required<std::uint64_t>(world, "world_seed");
    simulation.current_world_time_minutes_ = world.value("current_world_time_minutes", std::int64_t{});
    simulation.last_simulated_at_ = world.value("last_simulated_at", std::int64_t{});

    const auto& settings = root.at("settings");
    simulation.real_seconds_per_game_hour_ = required<int>(settings, "real_seconds_per_game_hour");
    simulation.offline_limit_seconds_ = required<std::int64_t>(settings, "offline_limit_seconds");
    simulation.chain_limit_ = required<int>(settings, "chain_limit");
    simulation.witness_threshold_ = required<double>(settings, "witness_threshold");
    if (simulation.real_seconds_per_game_hour_ <= 0 || simulation.offline_limit_seconds_ < 0 || simulation.chain_limit_ <= 0) {
        throw std::runtime_error("invalid Stage A timing or chain settings");
    }

    simulation.allowed_actions_ = required<std::vector<std::string>>(root, "allowed_player_actions");
    for (const auto& location : root.at("locations")) {
        const std::string id = required<std::string>(location, "location_id");
        const double visibility = required<double>(location, "visibility");
        const double obstruction = location.value("obstruction", 0.0);
        simulation.location_visibility_[id] = std::clamp(visibility * (1.0 - obstruction), 0.0, 1.0);
    }
    if (simulation.location_visibility_.size() != 5) {
        throw std::runtime_error("Stage A fixture must contain exactly five locations");
    }

    load_country_runtime(simulation.country_, root.at("country"));
    load_player_runtime(simulation.player_, root.at("player"));

    std::unordered_set<std::string> npc_ids;
    for (const auto& value : root.at("ai_npcs")) {
        AiNpcState npc;
        npc.npc_id = required<std::string>(value, "npc_id");
        if (!npc_ids.insert(npc.npc_id).second) {
            throw std::runtime_error("duplicate AI NPC id: " + npc.npc_id);
        }
        npc.name = required<std::string>(value, "name");
        npc.role = required<std::string>(value, "role");
        npc.faction_id = required<std::string>(value, "faction_id");
        npc.country_id = required<std::string>(value, "country_id");
        npc.current_location_id = required<std::string>(value, "current_location_id");
        npc.current_state_id = required<int>(value, "initial_state_id");
        npc.attention = required<double>(value, "attention");
        npc.hearing_trust_threshold = required<double>(value, "hearing_trust_threshold");
        npc.personality_traits = value.value("personality_traits", std::map<std::string, int>{});
        npc.player_evaluation = value.value("player_evaluation", 0);
        npc.relationships = value.value("relationships", std::map<std::string, std::map<std::string, int>>{});
        npc.current_goal = value.value("current_goal", "");
        npc.memory_summary = value.value("memory_summary", "");
        npc.known_events = value.value("known_events", std::vector<std::string>{});
        npc.active_action = value.value("active_action", "");
        npc.status = value.value("status", "ACTIVE");
        npc.created_at = value.value("created_at", std::int64_t{});
        npc.updated_at = value.value("updated_at", std::int64_t{});

        const int state_slot_count = required<int>(value, "state_slot_count");
        if (state_slot_count != 255 || value.at("states").size() != 255) {
            throw std::runtime_error(npc.npc_id + " must define exactly 255 state slots");
        }
        npc.states.reserve(255);
        int expected_state_id = 1;
        for (const auto& state_value : value.at("states")) {
            StateDefinition state;
            state.state_id = required<int>(state_value, "state_id");
            if (state.state_id != expected_state_id++) {
                throw std::runtime_error(npc.npc_id + " state ids must be ordered 1..255");
            }
            state.state_name = required<std::string>(state_value, "state_name");
            state.state_description = required<std::string>(state_value, "state_description");
            state.undefined = required<bool>(state_value, "undefined");
            if (state.undefined && state.state_name != "UNDEFINED") {
                throw std::runtime_error(npc.npc_id + " undefined state must be explicit UNDEFINED");
            }
            for (const auto& dialogue_value : state_value.at("dialogue_candidates")) {
                DialogueCandidate dialogue;
                dialogue.dialogue_id = required<std::string>(dialogue_value, "dialogue_id");
                dialogue.text = required<std::string>(dialogue_value, "text");
                dialogue.tone = required<std::string>(dialogue_value, "tone");
                dialogue.priority = required<int>(dialogue_value, "priority");
                dialogue.once_only = dialogue_value.value("once_only", false);
                dialogue.cooldown = dialogue_value.value("cooldown", std::int64_t{});
                state.dialogue_candidates.push_back(std::move(dialogue));
            }
            for (const auto& action_value : state_value.at("action_candidates")) {
                ActionCandidate action;
                action.action_type = required<std::string>(action_value, "type");
                action.target_id = action_value.value("target_id", "");
                action.priority = required<int>(action_value, "priority");
                action.credibility_factor = action_value.value("credibility_factor", 1.0);
                for (const auto& effect_value : action_value.at("country_effects")) {
                    action.country_effects.push_back({
                        required<std::string>(effect_value, "parameter"),
                        required<int>(effect_value, "delta"),
                        required<std::string>(effect_value, "reason")
                    });
                }
                state.action_candidates.push_back(std::move(action));
            }
            for (const auto& rule_value : state_value.at("transition_rules")) {
                TransitionRule rule;
                rule.rule_id = required<std::string>(rule_value, "rule_id");
                rule.trigger_event_type = required<std::string>(rule_value, "trigger_event_type");
                rule.subject_event_type = required<std::string>(rule_value, "subject_event_type");
                rule.perception = required<std::string>(rule_value, "perception");
                rule.min_credibility = rule_value.value("min_credibility", 0.0);
                rule.max_credibility = rule_value.value("max_credibility", 1.0);
                rule.target_state_id = required<int>(rule_value, "target_state_id");
                rule.priority = required<int>(rule_value, "priority");
                rule.cooldown = rule_value.value("cooldown", std::int64_t{});
                rule.once_only = rule_value.value("once_only", false);
                rule.player_evaluation_delta = rule_value.value("player_evaluation_delta", 0);
                rule.relationship_metric = rule_value.value("relationship_metric", "");
                rule.relationship_delta = rule_value.value("relationship_delta", 0);
                rule.relationship_target = rule_value.value("relationship_target", "");
                state.transition_rules.push_back(std::move(rule));
            }
            state.goal_modifier = state_value.value("goal_modifier", "");
            state.player_evaluation_modifier = state_value.value("player_evaluation_modifier", 0);
            state.priority = state_value.value("priority", 0);
            state.is_terminal = state_value.value("is_terminal", false);
            npc.states.push_back(std::move(state));
        }
        if (npc.current_state_id < 1 || npc.current_state_id > 255 ||
            npc.states[static_cast<std::size_t>(npc.current_state_id - 1)].undefined) {
            throw std::runtime_error(npc.npc_id + " has invalid initial state");
        }
        simulation.ai_npcs_.push_back(std::move(npc));
    }
    if (simulation.ai_npcs_.size() != 20) {
        throw std::runtime_error("Stage A fixture must contain exactly 20 AI NPCs");
    }

    std::unordered_set<std::string> non_ai_ids;
    for (const auto& value : root.at("non_ai_npcs")) {
        NonAiNpcState npc;
        npc.npc_id = required<std::string>(value, "non_ai_npc_id");
        if (!non_ai_ids.insert(npc.npc_id).second) {
            throw std::runtime_error("duplicate NON AI NPC id: " + npc.npc_id);
        }
        npc.seed = required<std::uint64_t>(value, "seed");
        npc.country_id = required<std::string>(value, "country_id");
        npc.current_location_id = required<std::string>(value, "current_location_id");
        npc.category = required<std::string>(value, "category");
        npc.occupation = required<std::string>(value, "occupation");
        npc.basic_profile = required<std::string>(value, "basic_profile");
        npc.current_activity = required<std::string>(value, "current_activity");
        npc.temporary_memory = value.value("temporary_memory", std::vector<std::string>{});
        npc.spawned_at = value.value("spawned_at", std::int64_t{});
        npc.despawn_policy = required<std::string>(value, "despawn_policy");
        simulation.non_ai_npcs_.push_back(std::move(npc));
    }
    if (simulation.non_ai_npcs_.size() != 20) {
        throw std::runtime_error("Stage A fixture must contain exactly 20 NON AI NPCs");
    }
    return simulation;
}

void Simulation::ensure_stage_e_defaults(
    AiNpcState& npc, const std::map<std::string, std::map<std::string, int>>& defaults) {
    for (const auto& [target, metrics] : defaults) {
        for (const auto& [metric, value] : metrics) {
            npc.relationships[target].try_emplace(metric, value);
        }
    }
}

void Simulation::apply_stage_e_overlay(const std::filesystem::path& overlay_path,
                                       const std::string& definition_sha256,
                                       bool initialize_stage_e_states) {
    std::ifstream input(overlay_path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open Stage E overlay: " + overlay_path.string());
    json root;
    input >> root;
    if (required<std::string>(root, "base_fixture_id") != fixture_id_) {
        throw std::runtime_error("Stage E overlay base fixture mismatch");
    }
    if (required<std::vector<int>>(root, "state_id_range") != std::vector<int>{6, 25}) {
        throw std::runtime_error("Stage E overlay state range must be 6..25");
    }
    stage_e_active_ = true;
    stage_e_overlay_id_ = required<std::string>(root, "overlay_id");
    stage_e_overlay_schema_version_ = required<std::string>(root, "overlay_schema_version");
    stage_e_definition_sha256_ = definition_sha256;
    if (stage_e_definition_sha256_.empty()) {
        std::ostringstream hash;
        hash << std::hex << deterministic_hash(root.dump());
        stage_e_definition_sha256_ = hash.str();
    }
    schema_version_ = "stage_e_save_schema_v1";
    simulation_version_ = required<std::string>(root, "simulation_version");
    if (simulation_version_ != "stage-e-0.1.0") {
        throw std::runtime_error("Stage E overlay simulation_version mismatch");
    }

    std::unordered_set<std::string> loaded;
    for (const auto& npc_value : root.at("npcs")) {
        const std::string npc_id = required<std::string>(npc_value, "npc_id");
        auto& npc = mutable_ai_npc(npc_id);
        if (npc.role != required<std::string>(npc_value, "expected_role")) {
            throw std::runtime_error("Stage E overlay role mismatch for " + npc_id);
        }
        const auto defaults = npc_value.value(
            "relationship_defaults", std::map<std::string, std::map<std::string, int>>{});
        ensure_stage_e_defaults(npc, defaults);
        std::unordered_set<int> ids;
        for (const auto& state_value : npc_value.at("states")) {
            StateDefinition state = stage_e_state_from_json(state_value);
            if (state.state_id < 6 || state.state_id > 25 || !ids.insert(state.state_id).second) {
                throw std::runtime_error("invalid or duplicate Stage E state for " + npc_id);
            }
            for (const auto& time_rule : state.time_based_rules) {
                TransitionRule converted;
                converted.rule_id = time_rule.rule_id;
                converted.trigger_event_type = "AI_NPC_OBSERVED_EVENT";
                converted.subject_event_type = "TIME_ELAPSED";
                converted.perception = "TIME";
                converted.target_state_id = time_rule.target_state_id;
                converted.priority = time_rule.priority;
                converted.once_only = time_rule.once_only;
                converted.min_state_minutes = time_rule.after_game_minutes;
                state.transition_rules.push_back(std::move(converted));
            }
            npc.states[static_cast<std::size_t>(state.state_id - 1)] = std::move(state);
        }
        if (ids.size() != 20) throw std::runtime_error(npc_id + " must define 20 Stage E states");
        std::unordered_set<int> legacy_sources;
        for (const auto& entry_value : npc_value.at("legacy_entry_rules")) {
            const int source = required<int>(entry_value, "source_state_id");
            if (source < 1 || source > 5 || !legacy_sources.insert(source).second) {
                throw std::runtime_error("invalid Stage E legacy entry for " + npc_id);
            }
            npc.states[static_cast<std::size_t>(source - 1)].transition_rules.push_back(
                stage_e_rule_from_json(entry_value));
        }
        if (legacy_sources.size() != 5) throw std::runtime_error(npc_id + " must define five legacy entries");
        if (initialize_stage_e_states) {
            npc.current_state_id = required<int>(npc_value, "initial_state_id");
            npc.current_goal = state_definition(npc, npc.current_state_id).current_goal;
            npc.state_entered_at = current_world_time_minutes_;
            npc.legacy_state_pending_stage_e_entry = false;
            const auto& state = state_definition(npc, npc.current_state_id);
            if (!state.time_based_rules.empty()) {
                npc.timed_transition_at = current_world_time_minutes_ + state.time_based_rules.front().after_game_minutes;
            }
        } else {
            npc.legacy_state_pending_stage_e_entry = npc.current_state_id >= 1 && npc.current_state_id <= 5;
            npc.state_entered_at = current_world_time_minutes_;
            npc.last_transition_reason = "MIGRATED_FROM_STAGE_D";
        }
        loaded.insert(npc_id);
    }
    if (loaded != std::unordered_set<std::string>{"ai_npc_001", "ai_npc_012", "ai_npc_002"}) {
        throw std::runtime_error("Stage E overlay target NPC set mismatch");
    }
    for (const auto& rule_value : root.value("cross_npc_rules", json::array())) {
        auto& npc = mutable_ai_npc(required<std::string>(rule_value, "npc_id"));
        const int source = required<int>(rule_value, "source_state_id");
        if (source < 1 || source > 255) throw std::runtime_error("invalid cross-NPC source state");
        npc.states[static_cast<std::size_t>(source - 1)].transition_rules.push_back(
            stage_e_rule_from_json(rule_value));
    }
}

Simulation Simulation::from_fixture_with_overlay(const std::filesystem::path& fixture_path,
                                                 const std::filesystem::path& overlay_path,
                                                 const std::string& definition_sha256) {
    Simulation simulation = from_fixture(fixture_path);
    simulation.apply_stage_e_overlay(overlay_path, definition_sha256, true);
    return simulation;
}

Simulation Simulation::load_save_with_overlay(const std::filesystem::path& fixture_path,
                                              const std::filesystem::path& overlay_path,
                                              const std::filesystem::path& save_path,
                                              const std::string& definition_sha256) {
    std::ifstream input(save_path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open save: " + save_path.string());
    json root;
    input >> root;
    const std::string save_schema = required<std::string>(root, "schema_version");
    if (save_schema != "stage_e_save_schema_v1") {
        Simulation legacy = load_save(fixture_path, save_path);
        legacy.stage_e_source_schema_version_ = legacy.schema_version_;
        legacy.stage_e_source_simulation_version_ = legacy.simulation_version_;
        legacy.apply_stage_e_overlay(overlay_path, definition_sha256, false);
        return legacy;
    }

    Simulation simulation = from_fixture_with_overlay(fixture_path, overlay_path, definition_sha256);
    if (required<std::string>(root, "fixture_id") != simulation.fixture_id_ ||
        required<std::string>(root, "simulation_version") != "stage-e-0.1.0" ||
        required<std::string>(root, "fixture_schema_version") != simulation.fixture_schema_version_) {
        throw std::runtime_error("Stage E save version or fixture mismatch");
    }
    const auto& overlay = root.at("stage_e_overlay");
    if (required<std::string>(overlay, "overlay_id") != simulation.stage_e_overlay_id_ ||
        required<std::string>(overlay, "overlay_schema_version") != simulation.stage_e_overlay_schema_version_ ||
        required<std::string>(overlay, "definition_sha256") != simulation.stage_e_definition_sha256_) {
        throw std::runtime_error("Stage E save overlay mismatch");
    }
    const auto& world = root.at("world");
    simulation.world_seed_ = required<std::uint64_t>(world, "world_seed");
    simulation.current_world_time_minutes_ = required<std::int64_t>(world, "current_world_time_minutes");
    simulation.last_simulated_at_ = required<std::int64_t>(world, "last_simulated_at");
    simulation.simulation_tick_ = required<std::int64_t>(world, "simulation_tick");
    simulation.next_event_sequence_ = required<std::uint64_t>(world, "next_event_sequence");
    simulation.chain_limit_ = required<int>(world, "chain_limit");
    load_country_runtime(simulation.country_, root.at("country"));
    load_player_runtime(simulation.player_, root.at("player"));
    for (const auto& value : root.at("ai_runtime")) {
        auto& npc = simulation.mutable_ai_npc(required<std::string>(value, "npc_id"));
        const auto defaults = npc.relationships;
        npc.current_location_id = required<std::string>(value, "current_location_id");
        npc.current_state_id = required<int>(value, "current_state_id");
        if (simulation.state_definition(npc, npc.current_state_id).undefined) {
            throw std::runtime_error("Stage E save attempted to load an UNDEFINED AI state");
        }
        npc.player_evaluation = value.value("player_evaluation", 0);
        npc.relationships = value.value("relationships", std::map<std::string, std::map<std::string, int>>{});
        simulation.ensure_stage_e_defaults(npc, defaults);
        npc.current_goal = value.value("current_goal", "");
        npc.known_events = value.value("known_events", std::vector<std::string>{});
        npc.active_action = value.value("active_action", "");
        npc.status = value.value("status", "ACTIVE");
        npc.updated_at = value.value("updated_at", npc.updated_at);
        npc.used_rules = value.value("used_rules", std::set<std::string>{});
        npc.rule_last_used_tick = value.value("rule_last_used_tick", std::map<std::string, std::int64_t>{});
        npc.used_dialogues = value.value("used_dialogues", std::set<std::string>{});
        npc.dialogue_last_used_tick = value.value("dialogue_last_used_tick", std::map<std::string, std::int64_t>{});
        if (value.contains("stage_e_runtime")) {
            load_stage_e_runtime(npc, value.at("stage_e_runtime"), simulation.current_world_time_minutes_);
        } else {
            load_stage_e_runtime(npc, json::object(), simulation.current_world_time_minutes_);
        }
    }
    for (const auto& value : root.at("non_ai_runtime")) {
        auto& npc = simulation.mutable_non_ai_npc(required<std::string>(value, "npc_id"));
        npc.current_location_id = required<std::string>(value, "current_location_id");
        npc.current_activity = value.value("current_activity", npc.current_activity);
        npc.temporary_memory = value.value("temporary_memory", std::vector<std::string>{});
        npc.spawned_at = value.value("spawned_at", npc.spawned_at);
        npc.despawn_policy = value.value("despawn_policy", npc.despawn_policy);
    }
    for (const auto& value : root.at("event_history")) simulation.event_history_.push_back(event_from_json(value));
    for (const auto& value : root.at("audit_log")) simulation.audit_log_.push_back(audit_from_json(value));
    for (const auto& value : root.at("pending_events")) simulation.pending_events_.push_back(event_from_json(value));
    simulation.root_processed_counts_ = root.value("root_processed_counts", std::map<std::string, std::size_t>{});
    if (root.contains("migration")) {
        const auto& migration = root.at("migration");
        simulation.stage_e_source_schema_version_ = migration.value("source_schema_version", "");
        simulation.stage_e_source_simulation_version_ = migration.value("source_simulation_version", "");
        simulation.stage_e_source_save_sha256_ = migration.value("source_save_sha256", "");
        simulation.stage_e_source_metadata_sha256_ = migration.value("source_metadata_sha256", "");
        simulation.stage_e_source_backup_path_ = migration.value("source_backup_path", "");
        simulation.stage_e_metadata_backup_path_ = migration.value("metadata_backup_path", "");
    }
    return simulation;
}

Simulation Simulation::load_save(const std::filesystem::path& fixture_path,
                                 const std::filesystem::path& save_path) {
    Simulation simulation = from_fixture(fixture_path);
    std::ifstream input(save_path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open save: " + save_path.string());
    }
    json root;
    input >> root;
    if (required<std::string>(root, "fixture_id") != simulation.fixture_id_ ||
        required<std::string>(root, "schema_version") != simulation.schema_version_ ||
        required<std::string>(root, "simulation_version") != simulation.simulation_version_) {
        throw std::runtime_error("save version or fixture mismatch");
    }
    const auto& world = root.at("world");
    simulation.world_seed_ = required<std::uint64_t>(world, "world_seed");
    simulation.current_world_time_minutes_ = required<std::int64_t>(world, "current_world_time_minutes");
    simulation.last_simulated_at_ = required<std::int64_t>(world, "last_simulated_at");
    simulation.simulation_tick_ = required<std::int64_t>(world, "simulation_tick");
    simulation.next_event_sequence_ = required<std::uint64_t>(world, "next_event_sequence");
    simulation.chain_limit_ = required<int>(world, "chain_limit");
    load_country_runtime(simulation.country_, root.at("country"));
    load_player_runtime(simulation.player_, root.at("player"));

    for (const auto& value : root.at("ai_runtime")) {
        auto& npc = simulation.mutable_ai_npc(required<std::string>(value, "npc_id"));
        npc.current_location_id = required<std::string>(value, "current_location_id");
        npc.current_state_id = required<int>(value, "current_state_id");
        if (simulation.state_definition(npc, npc.current_state_id).undefined) {
            throw std::runtime_error("save attempted to load an UNDEFINED AI state");
        }
        npc.player_evaluation = value.value("player_evaluation", 0);
        npc.relationships = value.value("relationships", std::map<std::string, std::map<std::string, int>>{});
        npc.current_goal = value.value("current_goal", "");
        npc.known_events = value.value("known_events", std::vector<std::string>{});
        npc.active_action = value.value("active_action", "");
        npc.status = value.value("status", "ACTIVE");
        npc.updated_at = value.value("updated_at", npc.updated_at);
        npc.used_rules = value.value("used_rules", std::set<std::string>{});
        npc.rule_last_used_tick = value.value("rule_last_used_tick", std::map<std::string, std::int64_t>{});
        npc.used_dialogues = value.value("used_dialogues", std::set<std::string>{});
        npc.dialogue_last_used_tick = value.value("dialogue_last_used_tick", std::map<std::string, std::int64_t>{});
    }
    for (const auto& value : root.at("non_ai_runtime")) {
        auto& npc = simulation.mutable_non_ai_npc(required<std::string>(value, "npc_id"));
        npc.current_location_id = required<std::string>(value, "current_location_id");
        npc.current_activity = value.value("current_activity", npc.current_activity);
        npc.temporary_memory = value.value("temporary_memory", std::vector<std::string>{});
        npc.spawned_at = value.value("spawned_at", npc.spawned_at);
        npc.despawn_policy = value.value("despawn_policy", npc.despawn_policy);
    }
    for (const auto& value : root.at("event_history")) simulation.event_history_.push_back(event_from_json(value));
    for (const auto& value : root.at("audit_log")) simulation.audit_log_.push_back(audit_from_json(value));
    for (const auto& value : root.at("pending_events")) simulation.pending_events_.push_back(event_from_json(value));
    simulation.root_processed_counts_ = root.value("root_processed_counts", std::map<std::string, std::size_t>{});
    return simulation;
}

std::string Simulation::next_event_id() {
    std::ostringstream out;
    out << "evt_";
    out.width(10);
    out.fill('0');
    out << next_event_sequence_++;
    return out.str();
}

Event Simulation::make_event(std::string type, std::string actor_id, std::string actor_type,
                             std::string target_id, std::string target_type,
                             std::string location_id) {
    Event event;
    event.event_id = next_event_id();
    event.event_type = std::move(type);
    event.actor_id = std::move(actor_id);
    event.actor_type = std::move(actor_type);
    event.target_id = std::move(target_id);
    event.target_type = std::move(target_type);
    event.location_id = std::move(location_id);
    event.country_id = country_.country_id;
    event.occurred_at = current_world_time_minutes_;
    event.participants = {event.actor_id};
    if (!event.target_id.empty()) event.participants.push_back(event.target_id);
    return event;
}

bool Simulation::is_allowed_action(const std::string& action) const {
    return contains_string(allowed_actions_, action);
}

std::string Simulation::enqueue_player_action(const std::string& action,
                                              const std::string& target_id,
                                              const std::string& destination_id) {
    if (!is_allowed_action(action)) {
        throw std::invalid_argument("action is not enabled by fixture: " + action);
    }
    bool target_is_ai = false;
    std::string target_type;
    if (action == "HELP" || action == "HARM" || action == "TALK" || action == "TRADE" || action == "STEAL") {
        if (target_id.empty()) throw std::invalid_argument(action + " requires a target id");
        const auto ai_it = std::find_if(ai_npcs_.begin(), ai_npcs_.end(), [&](const auto& npc) { return npc.npc_id == target_id; });
        const auto non_ai_it = std::find_if(non_ai_npcs_.begin(), non_ai_npcs_.end(), [&](const auto& npc) { return npc.npc_id == target_id; });
        if (ai_it == ai_npcs_.end() && non_ai_it == non_ai_npcs_.end()) {
            throw std::invalid_argument("unknown player action target: " + target_id);
        }
        target_is_ai = ai_it != ai_npcs_.end();
        target_type = target_is_ai ? "AI_NPC" : "NON_AI_NPC";
        const std::string& target_location = target_is_ai ? ai_it->current_location_id : non_ai_it->current_location_id;
        if (target_location != player_.current_location_id) {
            throw std::invalid_argument("player and target must be in the same location");
        }
    }
    if (action == "MOVE") {
        if (!location_visibility_.contains(destination_id)) {
            throw std::invalid_argument("unknown MOVE destination: " + destination_id);
        }
        if (destination_id == player_.current_location_id) {
            throw std::invalid_argument("MOVE destination must differ from current location");
        }
    }

    Event event = make_event(action_event_type(action, target_is_ai), player_.player_id, "PLAYER",
                             target_id, target_type, player_.current_location_id);
    event.payload["player_action"] = action;
    event.payload["salience"] = std::to_string(event_salience(event.event_type));
    if (action == "MOVE") {
        event.payload["previous_location_id"] = player_.current_location_id;
        event.payload["destination_id"] = destination_id;
        player_.current_location_id = destination_id;
    } else if (action == "WAIT") {
        event.payload["game_minutes"] = "60";
        event.payload["offline"] = "false";
    }
    event.root_event_id = event.event_id;
    player_.action_history.push_back(action + ":" + (target_id.empty() ? destination_id : target_id));
    player_.updated_at = current_world_time_minutes_;
    const std::string id = event.event_id;
    pending_events_.push_back(std::move(event));
    return id;
}

std::string Simulation::player_action(const std::string& action,
                                      const std::string& target_id,
                                      const std::string& destination_id) {
    const std::string id = enqueue_player_action(action, target_id, destination_id);
    process_pending();
    return id;
}

std::string Simulation::enqueue_rumor(const std::string& non_ai_npc_id,
                                      const std::string& target_ai_npc_id,
                                      const std::string& original_event_id,
                                      double credibility) {
    if (credibility < 0.0 || credibility > 1.0) {
        throw std::invalid_argument("rumor credibility must be in [0,1]");
    }
    auto& source = mutable_non_ai_npc(non_ai_npc_id);
    const auto& target = ai_npc(target_ai_npc_id);
    const auto& original = event(original_event_id);
    if (!contains_string(source.temporary_memory, original_event_id)) {
        throw std::invalid_argument("NON AI NPC does not remember the source event");
    }
    Event rumor = make_event("RUMOR_CREATED", non_ai_npc_id, "NON_AI_NPC",
                             target_ai_npc_id, "AI_NPC", source.current_location_id);
    rumor.credibility = credibility;
    rumor.evidence_level = original.evidence_level * credibility;
    rumor.source_event_id = original.event_id;
    rumor.root_event_id = original.root_event_id;
    rumor.payload["original_event_id"] = original.event_id;
    rumor.payload["subject_event_type"] = original.event_type;
    rumor.payload["original_actor"] = original.actor_id;
    rumor.payload["original_target"] = original.target_id;
    rumor.payload["original_location"] = original.location_id;
    rumor.payload["distortion_level"] = std::to_string(1.0 - credibility);
    rumor.payload["hop_count"] = "0";
    rumor.payload["target_location"] = target.current_location_id;
    const std::string id = rumor.event_id;
    pending_events_.push_back(std::move(rumor));
    return id;
}

std::string Simulation::share_rumor(const std::string& non_ai_npc_id,
                                    const std::string& target_ai_npc_id,
                                    const std::string& original_event_id,
                                    double credibility) {
    const std::string id = enqueue_rumor(non_ai_npc_id, target_ai_npc_id, original_event_id, credibility);
    process_pending();
    return id;
}

std::size_t Simulation::process_pending(std::size_t max_events) {
    std::size_t processed = 0;
    std::string last_actor;
    while (!pending_events_.empty() && processed < max_events) {
        Event current = std::move(pending_events_.front());
        pending_events_.pop_front();
        const std::string root = current.root_event_id.empty() ? current.event_id : current.root_event_id;
        auto& root_count = root_processed_counts_[root];
        if (root_count >= static_cast<std::size_t>(chain_limit_)) {
            std::size_t removed = 1;
            std::erase_if(pending_events_, [&](const Event& event) {
                if (event.root_event_id == root) {
                    ++removed;
                    return true;
                }
                return false;
            });
            append_chain_limit_event(root, removed, last_actor.empty() ? current.actor_id : last_actor);
            ++processed;
            continue;
        }
        ++root_count;
        ++processed;
        last_actor = current.actor_id;
        auto children = derive_events(current);
        for (auto& child : children) {
            if (child.source_event_id.empty()) child.source_event_id = current.event_id;
            if (child.root_event_id.empty()) child.root_event_id = root;
            current.resulting_events.push_back(child.event_id);
        }
        append_history(std::move(current));
        for (auto& child : children) pending_events_.push_back(std::move(child));
    }
    return processed;
}

std::vector<Event> Simulation::derive_events(Event& event) {
    if (event.event_type == "RUMOR_CREATED" || event.event_type == "RUMOR_TRANSFERRED") {
        return derive_rumor_event(event);
    }
    if (event.event_type == "AI_NPC_OBSERVED_EVENT" || event.event_type == "AI_NPC_HEARD_EVENT") {
        return derive_perception_event(event);
    }
    if (event.actor_type == "PLAYER" || event.event_type == "TIME_ELAPSED") {
        return derive_player_event(event);
    }
    return {};
}

std::vector<Event> Simulation::derive_player_event(Event& event) {
    std::vector<Event> generated;
    if (event.event_type == "PLAYER_LEFT_LOCATION") {
        Event entered = make_event("PLAYER_ENTERED_LOCATION", player_.player_id, "PLAYER", "", "",
                                   event.payload.at("destination_id"));
        entered.payload["previous_location_id"] = event.payload.at("previous_location_id");
        generated.push_back(std::move(entered));
        return generated;
    }
    if (event.event_type == "TIME_ELAPSED") {
        const auto minutes = std::stoll(event.payload.at("game_minutes"));
        current_world_time_minutes_ += minutes;
        if (event.payload.at("offline") == "true") {
            Event completed = make_event("OFFLINE_SIMULATION_COMPLETED", "world_001", "WORLD", "", "",
                                         player_.current_location_id);
            completed.payload = event.payload;
            completed.payload["elapsed_game_minutes"] = std::to_string(minutes);
            generated.push_back(std::move(completed));
        }
        if (stage_e_active_) {
            for (const auto& npc : ai_npcs_) {
                if (npc.npc_id != "ai_npc_001" && npc.npc_id != "ai_npc_012" && npc.npc_id != "ai_npc_002") continue;
                Event elapsed = make_event("AI_NPC_OBSERVED_EVENT", "world_001", "WORLD",
                                           npc.npc_id, "AI_NPC", npc.current_location_id);
                elapsed.credibility = 1.0;
                elapsed.evidence_level = 0.0;
                elapsed.payload["subject_event_id"] = event.event_id;
                elapsed.payload["subject_event_type"] = "TIME_ELAPSED";
                elapsed.payload["perception"] = "TIME";
                elapsed.payload["game_minutes"] = std::to_string(minutes);
                generated.push_back(std::move(elapsed));
            }
        }
        return generated;
    }

    if (event.target_type == "NON_AI_NPC" &&
        (event.event_type == "PLAYER_HELPED_NON_AI_NPC" || event.event_type == "PLAYER_HARMED_NON_AI_NPC" ||
         event.event_type == "PLAYER_TRADED" || event.event_type == "PLAYER_STOLE" || event.event_type == "PLAYER_TALKED")) {
        auto& target = mutable_non_ai_npc(event.target_id);
        target.temporary_memory.push_back(event.event_id);
    }
    if (event.event_type == "PLAYER_HELPED_NON_AI_NPC") {
        player_.reputation += 1;
        player_.achievement_tags.push_back("HELPED_CIVILIAN");
    } else if (event.event_type == "PLAYER_HARMED_NON_AI_NPC") {
        player_.achievement_tags.push_back("ASSAULTED_CIVILIAN");
    } else if (event.event_type == "PLAYER_STOLE") {
        player_.achievement_tags.push_back("STOLE_FROM_CIVILIAN");
    } else if (event.event_type == "PLAYER_TRADED") {
        player_.achievement_tags.push_back("SUCCESSFUL_TRADE");
    }

    if (event.target_type == "AI_NPC" &&
        (event.event_type == "PLAYER_HELPED_AI_NPC" || event.event_type == "PLAYER_HARMED_AI_NPC")) {
        event.observed_by.push_back(event.target_id);
        Event direct = make_event("AI_NPC_OBSERVED_EVENT", event.target_id, "AI_NPC",
                                  event.target_id, "AI_NPC", event.location_id);
        direct.credibility = 1.0;
        direct.evidence_level = 1.0;
        direct.payload["subject_event_id"] = event.event_id;
        direct.payload["subject_event_type"] = event.event_type;
        direct.payload["perception"] = "OBSERVED";
        direct.payload["witness_score"] = "1.0";
        generated.push_back(std::move(direct));
    }

    const auto salience_it = event.payload.find("salience");
    const double salience = std::stod(salience_it == event.payload.end() ? "0.4" : salience_it->second);
    const double visibility = location_visibility_.contains(event.location_id)
        ? location_visibility_.at(event.location_id) : 0.0;
    for (const auto& npc : ai_npcs_) {
        if (npc.current_location_id != event.location_id || npc.npc_id == event.target_id) continue;
        const double witness_score = npc.attention * visibility * salience;
        if (witness_score < witness_threshold_) continue;
        event.observed_by.push_back(npc.npc_id);
        Event observed = make_event("AI_NPC_OBSERVED_EVENT", npc.npc_id, "AI_NPC",
                                    npc.npc_id, "AI_NPC", event.location_id);
        observed.credibility = 1.0;
        observed.evidence_level = 1.0;
        observed.payload["subject_event_id"] = event.event_id;
        observed.payload["subject_event_type"] = event.event_type;
        observed.payload["perception"] = "OBSERVED";
        observed.payload["witness_score"] = std::to_string(witness_score);
        generated.push_back(std::move(observed));
    }
    return generated;
}

std::vector<Event> Simulation::derive_rumor_event(Event& event) {
    std::vector<Event> generated;
    if (event.event_type == "RUMOR_CREATED") {
        Event transferred = make_event("RUMOR_TRANSFERRED", event.actor_id, "NON_AI_NPC",
                                       event.target_id, "AI_NPC", event.location_id);
        transferred.credibility = event.credibility;
        transferred.evidence_level = event.evidence_level;
        transferred.payload = event.payload;
        transferred.payload["hop_count"] = "1";
        generated.push_back(std::move(transferred));
    } else {
        event.heard_by.push_back(event.target_id);
        Event heard = make_event("AI_NPC_HEARD_EVENT", event.actor_id, event.actor_type,
                                 event.target_id, "AI_NPC", ai_npc(event.target_id).current_location_id);
        heard.credibility = event.credibility;
        heard.evidence_level = event.evidence_level;
        heard.payload = event.payload;
        heard.payload["perception"] = "HEARD";
        generated.push_back(std::move(heard));
    }
    return generated;
}

std::string Simulation::rule_rejection_reason(const AiNpcState& npc, const TransitionRule& rule,
                                              const Event& perception) const {
    const std::string subject = perception.payload.contains("subject_event_type")
        ? perception.payload.at("subject_event_type") : "";
    const std::string perception_kind = perception.payload.contains("perception")
        ? perception.payload.at("perception") : "";
    if (rule.trigger_event_type != perception.event_type) return "trigger_event_type mismatch";
    if (rule.subject_event_type != subject) return "subject_event_type mismatch";
    if (rule.perception != perception_kind) return "perception mismatch";
    if (!rule.required_actor_id.empty() && rule.required_actor_id != perception.actor_id) return "event actor mismatch";
    if (perception.credibility < rule.min_credibility || perception.credibility > rule.max_credibility) return "credibility out of range";
    if (perception.evidence_level < rule.min_evidence || perception.evidence_level > rule.max_evidence) return "evidence out of range";
    if (npc.player_evaluation < rule.min_player_evaluation || npc.player_evaluation > rule.max_player_evaluation) return "player evaluation out of range";
    if (!rule.relationship_metric.empty()) {
        const auto target = npc.relationships.find(rule.relationship_target);
        const auto metric = target == npc.relationships.end()
            ? std::map<std::string, int>::const_iterator{}
            : target->second.find(rule.relationship_metric);
        const int value = target == npc.relationships.end() || metric == target->second.end() ? 0 : metric->second;
        if (value < rule.min_relationship || value > rule.max_relationship) return "relationship metric out of range";
    }
    if (!rule.country_parameter.empty()) {
        int value = 0;
        if (rule.country_parameter == "stability") value = country_.stability;
        else if (rule.country_parameter == "security") value = country_.security;
        else if (rule.country_parameter == "economy") value = country_.economy;
        else if (rule.country_parameter == "food") value = country_.food;
        else if (rule.country_parameter == "military") value = country_.military;
        else if (rule.country_parameter == "public_support") value = country_.public_support;
        else if (rule.country_parameter == "authority") value = country_.authority;
        else if (rule.country_parameter == "crime_level") value = country_.crime_level;
        else return "unknown country parameter";
        if (value < rule.min_country || value > rule.max_country) return "country parameter out of range";
    }
    if (current_world_time_minutes_ - npc.state_entered_at < rule.min_state_minutes) return "state duration not reached";
    if (rule.once_only && npc.used_rules.contains(rule.rule_id)) return "once_only consumed";
    const auto last = npc.rule_last_used_tick.find(rule.rule_id);
    if (last != npc.rule_last_used_tick.end() && simulation_tick_ - last->second < rule.cooldown) return "cooldown active";
    return {};
}

std::vector<const TransitionRule*> Simulation::matching_rules(AiNpcState& npc,
                                                              const Event& perception) {
    const auto& state = state_definition(npc, npc.current_state_id);
    if (state.undefined) {
        throw std::runtime_error("attempted to use UNDEFINED state for " + npc.npc_id);
    }
    std::vector<const TransitionRule*> matches;
    npc.last_rule_evaluations.clear();
    for (const auto& rule : state.transition_rules) {
        const std::string reason = rule_rejection_reason(npc, rule, perception);
        npc.last_rule_evaluations.push_back({rule.rule_id, false, reason.empty() ? "matched" : reason});
        if (!reason.empty()) continue;
        matches.push_back(&rule);
    }
    return matches;
}

std::vector<Event> Simulation::derive_perception_event(Event& event) {
    auto& npc = mutable_ai_npc(event.target_id);
    event.processed_by.push_back(npc.npc_id);
    npc.known_events.push_back(event.event_id);
    AuditEntry audit;
    audit.simulation_tick = simulation_tick_ + 1;
    audit.timestamp = current_world_time_minutes_;
    audit.event_id = event.event_id;
    audit.root_event_id = event.root_event_id;
    audit.event_actor_id = event.actor_id;
    audit.decision_npc_id = npc.npc_id;
    audit.target_id = event.target_id;
    audit.current_state = npc.current_state_id;
    auto generated = execute_ai_reaction(npc, event, audit);
    audit_log_.push_back(std::move(audit));
    return generated;
}

std::vector<Event> Simulation::execute_ai_reaction(AiNpcState& npc, const Event& perception,
                                                   AuditEntry& audit) {
    const auto matches = matching_rules(npc, perception);
    for (const auto* rule : matches) audit.matched_rules.push_back(rule->rule_id);
    for (const auto& evaluation : npc.last_rule_evaluations) {
        if (!evaluation.reason.empty() && evaluation.reason != "matched") {
            audit.rejected_rules.push_back(evaluation.rule_id + ":" + evaluation.reason);
        }
    }
    if (matches.empty()) {
        audit.next_state = npc.current_state_id;
        return {};
    }
    const int highest_priority = (*std::max_element(matches.begin(), matches.end(),
        [](const auto* lhs, const auto* rhs) { return lhs->priority < rhs->priority; }))->priority;
    std::vector<const TransitionRule*> finalists;
    for (const auto* rule : matches) if (rule->priority == highest_priority) finalists.push_back(rule);
    const std::string seed_material = std::to_string(world_seed_) + "|" + npc.npc_id + "|" +
        perception.event_id + "|" + std::to_string(npc.current_state_id) + "|" +
        std::to_string(simulation_tick_ + 1);
    audit.random_seed = deterministic_hash(seed_material);
    const auto* selected = finalists[static_cast<std::size_t>(audit.random_seed % finalists.size())];
    audit.selected_rule = selected->rule_id;
    npc.used_rules.insert(selected->rule_id);
    npc.rule_last_used_tick[selected->rule_id] = simulation_tick_ + 1;
    for (auto& evaluation : npc.last_rule_evaluations) {
        if (evaluation.rule_id == selected->rule_id) {
            evaluation.selected = true;
            evaluation.reason = "selected highest priority";
        } else if (evaluation.reason == "matched") {
            evaluation.reason = "not selected: lower priority";
            audit.rejected_rules.push_back(evaluation.rule_id + ":lower priority");
        }
    }

    const int old_state = npc.current_state_id;
    const auto& next_state = state_definition(npc, selected->target_state_id);
    if (next_state.undefined) {
        throw std::runtime_error("transition selected UNDEFINED state for " + npc.npc_id);
    }
    npc.current_state_id = selected->target_state_id;
    npc.player_evaluation += selected->player_evaluation_delta + next_state.player_evaluation_modifier;
    if (!next_state.goal_modifier.empty()) npc.current_goal = next_state.goal_modifier;
    npc.state_entered_at = current_world_time_minutes_;
    npc.timed_transition_at.reset();
    if (!next_state.time_based_rules.empty()) {
        const auto earliest = std::min_element(next_state.time_based_rules.begin(), next_state.time_based_rules.end(),
            [](const auto& lhs, const auto& rhs) { return lhs.after_game_minutes < rhs.after_game_minutes; });
        npc.timed_transition_at = current_world_time_minutes_ + earliest->after_game_minutes;
    }
    npc.legacy_state_pending_stage_e_entry = false;
    npc.evidence_evaluation.source_event_id = perception.payload.contains("subject_event_id")
        ? perception.payload.at("subject_event_id") : perception.event_id;
    npc.evidence_evaluation.credibility = perception.credibility;
    npc.evidence_evaluation.evidence_level = perception.evidence_level;
    npc.evidence_evaluation.perception = perception.payload.contains("perception")
        ? perception.payload.at("perception") : "NONE";
    npc.last_transition_reason = selected->rule_id;
    npc.updated_at = current_world_time_minutes_;
    audit.next_state = npc.current_state_id;

    const DialogueCandidate* dialogue = nullptr;
    for (const auto& candidate : next_state.dialogue_candidates) {
        if (candidate.once_only && npc.used_dialogues.contains(candidate.dialogue_id)) continue;
        const auto last = npc.dialogue_last_used_tick.find(candidate.dialogue_id);
        if (last != npc.dialogue_last_used_tick.end() &&
            simulation_tick_ + 1 - last->second < candidate.cooldown) continue;
        if (dialogue == nullptr || candidate.priority > dialogue->priority) dialogue = &candidate;
    }
    if (dialogue != nullptr) {
        audit.selected_dialogue = dialogue->dialogue_id;
        npc.used_dialogues.insert(dialogue->dialogue_id);
        npc.dialogue_last_used_tick[dialogue->dialogue_id] = simulation_tick_ + 1;
    }
    const ActionCandidate* action = nullptr;
    for (const auto& candidate : next_state.action_candidates) {
        if (action == nullptr || candidate.priority > action->priority) action = &candidate;
    }
    if (action != nullptr) audit.selected_action = action->action_type;

    std::vector<Event> generated;
    if (selected->relationship_delta != 0 && !selected->relationship_metric.empty()) {
        std::string relationship_target;
        if (selected->relationship_target == "EVENT_ACTOR") relationship_target = perception.actor_id;
        else relationship_target = selected->relationship_target;
        if (!relationship_target.empty() && relationship_target != npc.npc_id) {
            auto& value = npc.relationships[relationship_target][selected->relationship_metric];
            const int old_value = value;
            value += selected->relationship_delta;
            Event relationship = make_event("AI_NPC_CHANGED_RELATIONSHIP", npc.npc_id, "AI_NPC",
                                            relationship_target, "NPC", npc.current_location_id);
            relationship.source_event_id = perception.event_id;
            relationship.root_event_id = perception.root_event_id;
            relationship.payload["metric"] = selected->relationship_metric;
            relationship.payload["old_value"] = std::to_string(old_value);
            relationship.payload["new_value"] = std::to_string(value);
            relationship.payload["reason"] = "transition_rule:" + selected->rule_id;
            generated.push_back(std::move(relationship));
        }
    }
    Event changed = make_event("AI_NPC_CHANGED_STATE", npc.npc_id, "AI_NPC", npc.npc_id, "AI_NPC",
                               npc.current_location_id);
    changed.source_event_id = perception.event_id;
    changed.root_event_id = perception.root_event_id;
    changed.payload["old_state"] = std::to_string(old_state);
    changed.payload["new_state"] = std::to_string(npc.current_state_id);
    changed.payload["selected_rule"] = selected->rule_id;
    generated.push_back(std::move(changed));

    if (action == nullptr) {
        for (const auto& child : generated) audit.generated_events.push_back(child.event_id);
        return generated;
    }
    npc.active_action = action->action_type;
    Event started = make_event("AI_NPC_STARTED_ACTION", npc.npc_id, "AI_NPC", action->target_id, "ACTION_TARGET",
                               npc.current_location_id);
    started.source_event_id = perception.event_id;
    started.root_event_id = perception.root_event_id;
    started.payload["action_type"] = action->action_type;
    const std::string started_id = started.event_id;
    generated.push_back(std::move(started));

    Event completed = make_event("AI_NPC_COMPLETED_ACTION", npc.npc_id, "AI_NPC", action->target_id, "ACTION_TARGET",
                                 npc.current_location_id);
    completed.source_event_id = started_id;
    completed.root_event_id = perception.root_event_id;
    completed.payload["action_type"] = action->action_type;
    const std::string completed_id = completed.event_id;
    generated.push_back(std::move(completed));
    npc.active_action.clear();

    if (action->action_type == "REPORT" && !action->target_id.empty()) {
        Event heard = make_event("AI_NPC_HEARD_EVENT", npc.npc_id, "AI_NPC", action->target_id, "AI_NPC",
                                 ai_npc(action->target_id).current_location_id);
        heard.source_event_id = completed_id;
        heard.root_event_id = perception.root_event_id;
        heard.credibility = std::clamp(perception.credibility * action->credibility_factor, 0.0, 1.0);
        heard.evidence_level = perception.evidence_level;
        const auto subject_id = perception.payload.find("subject_event_id");
        const auto original_id = perception.payload.find("original_event_id");
        heard.payload["subject_event_id"] = subject_id != perception.payload.end()
            ? subject_id->second
            : (original_id != perception.payload.end() ? original_id->second : perception.source_event_id);
        const auto subject_type = perception.payload.find("subject_event_type");
        heard.payload["subject_event_type"] = subject_type != perception.payload.end()
            ? subject_type->second : perception.event_type;
        heard.payload["perception"] = "HEARD";
        heard.payload["reported_by"] = npc.npc_id;
        generated.push_back(std::move(heard));
    }

    if (action->action_type == "ARREST" || action->action_type == "WARN" ||
        action->action_type == "PROTECT" || action->action_type == "HELP") {
        const auto subject_it = perception.payload.find("subject_event_id");
        Event intervened = make_event("AI_NPC_INTERVENED", npc.npc_id, "AI_NPC",
                                      subject_it == perception.payload.end() ? "" : subject_it->second, "EVENT",
                                      npc.current_location_id);
        intervened.source_event_id = completed_id;
        intervened.root_event_id = perception.root_event_id;
        intervened.payload["action_type"] = action->action_type;
        generated.push_back(std::move(intervened));
        if (action->action_type == "ARREST") player_.crime_record += 1;
    }

    for (const auto& effect : action->country_effects) {
        int* parameter = mutable_country_parameter(effect.parameter);
        const int old_value = *parameter;
        *parameter += effect.delta;
        country_.updated_at = current_world_time_minutes_;
        Event country_event = make_event("COUNTRY_STATE_CHANGED", npc.npc_id, "AI_NPC",
                                         country_.country_id, "COUNTRY", npc.current_location_id);
        country_event.source_event_id = completed_id;
        country_event.root_event_id = perception.root_event_id;
        country_event.payload["parameter"] = effect.parameter;
        country_event.payload["old_value"] = std::to_string(old_value);
        country_event.payload["new_value"] = std::to_string(*parameter);
        country_event.payload["reason"] = effect.reason;
        audit.country_state_changes.push_back(effect.parameter + ":" + std::to_string(old_value) + "->" + std::to_string(*parameter));
        generated.push_back(std::move(country_event));
    }
    for (const auto& child : generated) audit.generated_events.push_back(child.event_id);
    return generated;
}

void Simulation::append_history(Event event) {
    event.simulation_tick = ++simulation_tick_;
    event_history_.push_back(std::move(event));
}

void Simulation::append_chain_limit_event(const std::string& root_event_id,
                                          std::size_t remaining,
                                          const std::string& last_actor) {
    Event limit = make_event("CHAIN_LIMIT_REACHED", "simulation", "SYSTEM", "", "",
                             player_.current_location_id);
    limit.root_event_id = root_event_id;
    limit.payload["root_event_id"] = root_event_id;
    limit.payload["processed_event_count"] = std::to_string(root_processed_counts_[root_event_id]);
    limit.payload["remaining_event_count"] = std::to_string(remaining);
    limit.payload["last_processed_actor"] = last_actor;
    append_history(std::move(limit));
}

OfflineResult Simulation::advance_offline(std::int64_t elapsed_real_seconds) {
    if (elapsed_real_seconds < 0) throw std::invalid_argument("offline elapsed time cannot be negative");
    OfflineResult result;
    result.requested_real_seconds = elapsed_real_seconds;
    result.applied_real_seconds = std::min(elapsed_real_seconds, offline_limit_seconds_);
    result.elapsed_game_minutes = result.applied_real_seconds * 60 / real_seconds_per_game_hour_;
    std::map<std::string, int> states_before;
    for (const auto& npc : ai_npcs_) states_before[npc.npc_id] = npc.current_state_id;
    const auto before_history = event_history_.size();
    Event elapsed = make_event("TIME_ELAPSED", "world_001", "WORLD", "", "",
                               player_.current_location_id);
    elapsed.root_event_id = elapsed.event_id;
    elapsed.payload["game_minutes"] = std::to_string(result.elapsed_game_minutes);
    elapsed.payload["real_seconds"] = std::to_string(result.applied_real_seconds);
    elapsed.payload["requested_real_seconds"] = std::to_string(result.requested_real_seconds);
    elapsed.payload["offline"] = "true";
    last_simulated_at_ += result.applied_real_seconds;
    pending_events_.push_back(std::move(elapsed));
    process_pending();
    for (std::size_t i = before_history; i < event_history_.size(); ++i) {
        result.important_events.push_back(event_history_[i].event_id + ":" + event_history_[i].event_type);
        if (event_history_[i].event_type == "COUNTRY_STATE_CHANGED") {
            result.country_changes.push_back(event_history_[i].event_id);
        }
    }
    for (const auto& npc : ai_npcs_) {
        if (states_before.at(npc.npc_id) != npc.current_state_id) result.changed_ai_npcs.push_back(npc.npc_id);
    }
    for (const auto& pending : pending_events_) result.unresolved_events.push_back(pending.event_id);
    return result;
}

void Simulation::save(const std::filesystem::path& save_path) const {
    json root;
    root["fixture_id"] = fixture_id_;
    root["schema_version"] = schema_version_;
    root["simulation_version"] = simulation_version_;
    if (stage_e_active_) {
        root["fixture_schema_version"] = fixture_schema_version_;
        root["stage_e_overlay"] = {
            {"overlay_id", stage_e_overlay_id_},
            {"overlay_schema_version", stage_e_overlay_schema_version_},
            {"base_fixture_id", fixture_id_},
            {"definition_sha256", stage_e_definition_sha256_}
        };
        root["migration"] = {
            {"migration_id", "stage_d_to_stage_e_v1"},
            {"source_schema_version", stage_e_source_schema_version_.empty() ? fixture_schema_version_ : stage_e_source_schema_version_},
            {"source_simulation_version", stage_e_source_simulation_version_.empty() ? fixture_simulation_version_ : stage_e_source_simulation_version_},
            {"source_save_sha256", stage_e_source_save_sha256_},
            {"source_metadata_sha256", stage_e_source_metadata_sha256_},
            {"source_backup_path", stage_e_source_backup_path_},
            {"metadata_backup_path", stage_e_metadata_backup_path_}
        };
    }
    root["world"] = {
        {"world_seed", world_seed_}, {"current_world_time_minutes", current_world_time_minutes_},
        {"last_simulated_at", last_simulated_at_}, {"simulation_tick", simulation_tick_},
        {"next_event_sequence", next_event_sequence_}, {"chain_limit", chain_limit_}
    };
    root["country"] = country_to_json(country_);
    root["player"] = player_to_json(player_);
    root["ai_runtime"] = json::array();
    for (const auto& npc : ai_npcs_) {
        json runtime = ai_runtime_to_json(npc);
        if (stage_e_active_ && (npc.npc_id == "ai_npc_001" || npc.npc_id == "ai_npc_012" || npc.npc_id == "ai_npc_002")) {
            runtime["stage_e_runtime"] = stage_e_runtime_to_json(npc);
        }
        root["ai_runtime"].push_back(std::move(runtime));
    }
    root["non_ai_runtime"] = json::array();
    for (const auto& npc : non_ai_npcs_) root["non_ai_runtime"].push_back(non_ai_runtime_to_json(npc));
    root["event_history"] = json::array();
    for (const auto& value : event_history_) root["event_history"].push_back(event_to_json(value));
    root["audit_log"] = json::array();
    for (const auto& value : audit_log_) root["audit_log"].push_back(audit_to_json(value));
    root["pending_events"] = json::array();
    for (const auto& value : pending_events_) root["pending_events"].push_back(event_to_json(value));
    root["root_processed_counts"] = root_processed_counts_;

    if (!save_path.parent_path().empty()) std::filesystem::create_directories(save_path.parent_path());
    const auto temporary = save_path.string() + ".tmp";
    const auto backup = save_path.string() + ".bak";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("cannot create temporary save: " + temporary);
        output << root.dump(2) << '\n';
        output.flush();
        if (!output) throw std::runtime_error("failed while writing temporary save: " + temporary);
    }
    std::error_code error;
    std::filesystem::remove(backup, error);
    error.clear();
    const bool had_previous = std::filesystem::exists(save_path);
    if (had_previous) {
        std::filesystem::rename(save_path, backup, error);
        if (error) {
            std::filesystem::remove(temporary);
            throw std::runtime_error("cannot preserve previous save: " + error.message());
        }
    }
    error.clear();
    std::filesystem::rename(temporary, save_path, error);
    if (error) {
        if (had_previous) {
            std::error_code restore_error;
            std::filesystem::rename(backup, save_path, restore_error);
        }
        std::filesystem::remove(temporary);
        throw std::runtime_error("cannot commit save atomically: " + error.message());
    }
    std::filesystem::remove(backup, error);
}

std::vector<std::string> Simulation::causal_path(const std::string& event_id) const {
    std::vector<std::string> path;
    std::unordered_set<std::string> visited;
    std::string current = event_id;
    while (!current.empty()) {
        if (!visited.insert(current).second) throw std::runtime_error("causal path cycle detected at " + current);
        const auto it = std::find_if(event_history_.begin(), event_history_.end(),
            [&](const Event& value) { return value.event_id == current; });
        if (it == event_history_.end()) throw std::out_of_range("event missing from causal path: " + current);
        path.push_back(it->event_id + ":" + it->event_type);
        current = it->source_event_id;
    }
    std::reverse(path.begin(), path.end());
    return path;
}

std::string Simulation::canonical_snapshot() const {
    json root;
    root["stage_e_active"] = stage_e_active_;
    if (stage_e_active_) {
        root["stage_e_overlay"] = {{"overlay_id", stage_e_overlay_id_},
                                    {"overlay_schema_version", stage_e_overlay_schema_version_},
                                    {"definition_sha256", stage_e_definition_sha256_}};
    }
    root["world_seed"] = world_seed_;
    root["current_world_time_minutes"] = current_world_time_minutes_;
    root["last_simulated_at"] = last_simulated_at_;
    root["simulation_tick"] = simulation_tick_;
    root["next_event_sequence"] = next_event_sequence_;
    root["country"] = country_to_json(country_);
    root["player"] = player_to_json(player_);
    root["ai_runtime"] = json::array();
    for (const auto& npc : ai_npcs_) {
        json runtime = ai_runtime_to_json(npc);
        if (stage_e_active_ && (npc.npc_id == "ai_npc_001" || npc.npc_id == "ai_npc_012" || npc.npc_id == "ai_npc_002")) {
            runtime["stage_e_runtime"] = stage_e_runtime_to_json(npc);
        }
        root["ai_runtime"].push_back(std::move(runtime));
    }
    root["non_ai_runtime"] = json::array();
    for (const auto& npc : non_ai_npcs_) root["non_ai_runtime"].push_back(non_ai_runtime_to_json(npc));
    root["event_history"] = json::array();
    for (const auto& value : event_history_) root["event_history"].push_back(event_to_json(value));
    root["audit_log"] = json::array();
    for (const auto& value : audit_log_) root["audit_log"].push_back(audit_to_json(value));
    root["pending_events"] = json::array();
    for (const auto& value : pending_events_) root["pending_events"].push_back(event_to_json(value));
    root["root_processed_counts"] = root_processed_counts_;
    return root.dump();
}

std::string Simulation::audit_log_json_lines() const {
    std::ostringstream output;
    for (const auto& audit : audit_log_) output << audit_to_json(audit).dump() << '\n';
    return output.str();
}

std::string Simulation::causal_log_json_lines() const {
    std::ostringstream output;
    for (const auto& event_value : event_history_) {
        output << json{{"event_id", event_value.event_id}, {"event_type", event_value.event_type},
                       {"source_event_id", event_value.source_event_id},
                       {"root_event_id", event_value.root_event_id},
                       {"actor_id", event_value.actor_id}, {"target_id", event_value.target_id}}.dump() << '\n';
    }
    return output.str();
}

const AiNpcState& Simulation::ai_npc(const std::string& npc_id) const {
    const auto it = std::find_if(ai_npcs_.begin(), ai_npcs_.end(),
        [&](const auto& npc) { return npc.npc_id == npc_id; });
    if (it == ai_npcs_.end()) throw std::out_of_range("unknown AI NPC: " + npc_id);
    return *it;
}

AiNpcState& Simulation::mutable_ai_npc(const std::string& npc_id) {
    const auto it = std::find_if(ai_npcs_.begin(), ai_npcs_.end(),
        [&](const auto& npc) { return npc.npc_id == npc_id; });
    if (it == ai_npcs_.end()) throw std::out_of_range("unknown AI NPC: " + npc_id);
    return *it;
}

NonAiNpcState& Simulation::mutable_non_ai_npc(const std::string& npc_id) {
    const auto it = std::find_if(non_ai_npcs_.begin(), non_ai_npcs_.end(),
        [&](const auto& npc) { return npc.npc_id == npc_id; });
    if (it == non_ai_npcs_.end()) throw std::out_of_range("unknown NON AI NPC: " + npc_id);
    return *it;
}

const Event& Simulation::event(const std::string& event_id) const {
    const auto it = std::find_if(event_history_.begin(), event_history_.end(),
        [&](const Event& value) { return value.event_id == event_id; });
    if (it == event_history_.end()) throw std::out_of_range("unknown processed event: " + event_id);
    return *it;
}

const StateDefinition& Simulation::state_definition(const AiNpcState& npc, int state_id) const {
    if (state_id < 1 || state_id > static_cast<int>(npc.states.size())) {
        throw std::out_of_range("invalid state id for " + npc.npc_id + ": " + std::to_string(state_id));
    }
    return npc.states[static_cast<std::size_t>(state_id - 1)];
}

int* Simulation::mutable_country_parameter(const std::string& parameter) {
    if (parameter == "stability") return &country_.stability;
    if (parameter == "security") return &country_.security;
    if (parameter == "economy") return &country_.economy;
    if (parameter == "food") return &country_.food;
    if (parameter == "military") return &country_.military;
    if (parameter == "public_support") return &country_.public_support;
    if (parameter == "authority") return &country_.authority;
    if (parameter == "crime_level") return &country_.crime_level;
    throw std::invalid_argument("country effect uses unsupported parameter: " + parameter);
}

std::uint64_t Simulation::deterministic_hash(const std::string& value) const {
    std::uint64_t hash = 14695981039346656037ull;
    for (const unsigned char byte : value) {
        hash ^= byte;
        hash *= 1099511628211ull;
    }
    return hash;
}

void Simulation::set_chain_limit_for_test(int limit) {
    if (limit <= 0) throw std::invalid_argument("chain limit must be positive");
    chain_limit_ = limit;
}

void Simulation::set_relationship_metric(const std::string& npc_id, const std::string& target_id,
                                         const std::string& metric, int value) {
    mutable_ai_npc(npc_id).relationships[target_id][metric] = value;
}

void Simulation::set_country_parameter(const std::string& parameter, int value) {
    *mutable_country_parameter(parameter) = value;
}

void Simulation::configure_stage_e_migration(std::string source_schema_version,
                                             std::string source_simulation_version,
                                             std::string source_save_sha256,
                                             std::string source_metadata_sha256,
                                             std::string source_backup_path,
                                             std::string metadata_backup_path) {
    if (!stage_e_active_) throw std::logic_error("Stage E migration metadata requires an active overlay");
    stage_e_source_schema_version_ = std::move(source_schema_version);
    stage_e_source_simulation_version_ = std::move(source_simulation_version);
    stage_e_source_save_sha256_ = std::move(source_save_sha256);
    stage_e_source_metadata_sha256_ = std::move(source_metadata_sha256);
    stage_e_source_backup_path_ = std::move(source_backup_path);
    stage_e_metadata_backup_path_ = std::move(metadata_backup_path);
}

} // namespace nation_sim
