#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

struct Profile {
    const char* name;
    const char* role;
    const char* location;
    double attention;
    double trust;
    const char* observed_action;
    const char* heard_action;
    const char* action_target;
};

constexpr std::array<Profile, 20> profiles{{
    {"Test Guard Rhea", "GUARD", "market", 0.95, 0.55, "REPORT", "REPORT", "ai_npc_002"},
    {"Test Captain Cael", "CAPTAIN", "capital", 0.92, 0.60, "ARREST", "CHANGE_COUNTRY_STATE", ""},
    {"Test Merchant Mira", "MERCHANT", "capital", 0.72, 0.45, "REFUSE_TRADE", "REFUSE_TRADE", ""},
    {"Test Investigator Orrin", "INVESTIGATOR", "gate", 0.88, 0.75, "INVESTIGATE", "INVESTIGATE", ""},
    {"Test Healer Sela", "HEALER", "residential", 0.82, 0.58, "PROTECT", "HELP", ""},
    {"Test Clerk Dain", "CLERK", "capital", 0.68, 0.62, "WARN", "REPORT", "ai_npc_002"},
    {"Test Smith Brann", "SMITH", "market", 0.78, 0.57, "WARN", "REFUSE_TRADE", ""},
    {"Test Innkeeper Nia", "INNKEEPER", "tavern", 0.76, 0.50, "SPREAD_RUMOR", "SPREAD_RUMOR", ""},
    {"Test Courier Toma", "COURIER", "gate", 0.84, 0.48, "REPORT", "REPORT", "ai_npc_001"},
    {"Test Farmer Edda", "FARMER", "residential", 0.66, 0.52, "FLEE", "WARN", ""},
    {"Test Priest Iven", "PRIEST", "capital", 0.81, 0.64, "PROTECT", "HELP", ""},
    {"Test Broker Vela", "BROKER", "market", 0.74, 0.42, "REFUSE_TRADE", "SPREAD_RUMOR", ""},
    {"Test Scout Fen", "SCOUT", "gate", 0.93, 0.56, "INVESTIGATE", "REPORT", "ai_npc_001"},
    {"Test Artisan Lio", "ARTISAN", "residential", 0.69, 0.61, "WARN", "REFUSE_TRADE", ""},
    {"Test Bard Asha", "BARD", "tavern", 0.79, 0.47, "SPREAD_RUMOR", "SPREAD_RUMOR", ""},
    {"Test Magistrate Korr", "MAGISTRATE", "capital", 0.86, 0.70, "REPORT", "INVESTIGATE", "ai_npc_002"},
    {"Test Porter Juna", "PORTER", "market", 0.64, 0.54, "FLEE", "WARN", ""},
    {"Test Herbalist Pera", "HERBALIST", "residential", 0.71, 0.59, "HELP", "INVESTIGATE", ""},
    {"Test Gambler Soren", "GAMBLER", "tavern", 0.73, 0.39, "FLEE", "SPREAD_RUMOR", ""},
    {"Test Gatekeeper Yara", "GATEKEEPER", "gate", 0.90, 0.66, "WARN", "REPORT", "ai_npc_001"},
}};

std::string id_for(std::string_view prefix, int value) {
    std::string result(prefix);
    if (value < 10) {
        result += "00";
    } else if (value < 100) {
        result += "0";
    }
    result += std::to_string(value);
    return result;
}

std::string escaped(std::string_view value) {
    std::string out;
    for (const char c : value) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

void write_country_effects(std::ostream& out, std::string_view action) {
    if (action == "CHANGE_COUNTRY_STATE") {
        out << "[{\"parameter\":\"security\",\"delta\":-2,\"reason\":\"AI command response to a reported incident\"},"
               "{\"parameter\":\"crime_level\",\"delta\":2,\"reason\":\"Recorded incident escalated by AI authority\"}]";
    } else {
        out << "[]";
    }
}

void write_action(std::ostream& out, std::string_view type, std::string_view target, int priority) {
    out << "{\"type\":\"" << type << "\",\"target_id\":\"" << target
        << "\",\"priority\":" << priority << ",\"credibility_factor\":0.9,\"country_effects\":";
    write_country_effects(out, type);
    out << "}";
}

void write_dialogue(std::ostream& out, const std::string& npc_id, int state_id,
                    std::string_view text, std::string_view tone) {
    out << "{\"dialogue_id\":\"dlg_" << npc_id << "_" << state_id
        << "\",\"text\":\"" << escaped(text) << "\",\"tone\":\"" << tone
        << "\",\"priority\":100,\"once_only\":false,\"cooldown\":0}";
}

void write_rule(std::ostream& out, const std::string& npc_id, std::string_view suffix,
                std::string_view subject, std::string_view perception,
                double min_credibility, double max_credibility,
                int target_state, int priority, int evaluation_delta) {
    out << "{\"rule_id\":\"rule_" << npc_id << "_" << suffix
        << "\",\"trigger_event_type\":\"";
    if (perception == "OBSERVED") {
        out << "AI_NPC_OBSERVED_EVENT";
    } else {
        out << "AI_NPC_HEARD_EVENT";
    }
    out << "\",\"subject_event_type\":\"" << subject
        << "\",\"perception\":\"" << perception
        << "\",\"min_credibility\":" << min_credibility
        << ",\"max_credibility\":" << max_credibility
        << ",\"target_state_id\":" << target_state
        << ",\"priority\":" << priority
        << ",\"cooldown\":0,\"once_only\":false,\"player_evaluation_delta\":"
        << evaluation_delta;
    if (perception == "HEARD") {
        const int relationship_delta = subject == "PLAYER_HELPED_NON_AI_NPC" ? 2 :
            (suffix == "harm_heard_doubt" ? -1 : 1);
        out << ",\"relationship_metric\":\"trust\",\"relationship_delta\":"
            << relationship_delta << ",\"relationship_target\":\"EVENT_ACTOR\"";
    } else {
        out << ",\"relationship_metric\":\"\",\"relationship_delta\":0,\"relationship_target\":\"\"";
    }
    out << "}";
}

void write_defined_state(std::ostream& out, const Profile& profile, const std::string& npc_id,
                         int npc_number, int state_id) {
    const std::string unique = npc_id + "_s" + std::to_string(state_id);
    out << "{\"state_id\":" << state_id << ",\"state_name\":\"";
    switch (state_id) {
    case 1: out << unique << "_baseline"; break;
    case 2: out << unique << "_favorable_response"; break;
    case 3: out << unique << "_direct_incident_response"; break;
    case 4: out << unique << "_skeptical_inquiry"; break;
    case 5: out << unique << "_trusted_report_response"; break;
    default: throw std::runtime_error("unexpected defined state");
    }
    out << "\",\"state_description\":\"NPC-specific fixture state " << state_id
        << " for " << npc_id << " (" << profile.role << ")\",\"undefined\":false,";

    out << "\"dialogue_candidates\":[";
    switch (state_id) {
    case 1: write_dialogue(out, npc_id, state_id, "Fixture baseline response for " + npc_id, "neutral"); break;
    case 2: write_dialogue(out, npc_id, state_id, "I recorded the help attributed to you. [" + npc_id + "]", "positive"); break;
    case 3: write_dialogue(out, npc_id, state_id, "I directly observed the incident. [" + npc_id + "]", "urgent"); break;
    case 4: write_dialogue(out, npc_id, state_id, "The report is uncertain; I will investigate. [" + npc_id + "]", "skeptical"); break;
    case 5: write_dialogue(out, npc_id, state_id, "I accept the report and will act. [" + npc_id + "]", "decisive"); break;
    }
    out << "],\"action_candidates\":[";
    switch (state_id) {
    case 1: write_action(out, "WAIT", "", 100); break;
    case 2: write_action(out, "TALK", "player_001", 100); break;
    case 3: write_action(out, profile.observed_action, profile.action_target, 100); break;
    case 4: write_action(out, "INVESTIGATE", "player_001", 100); break;
    case 5: write_action(out, profile.heard_action, profile.action_target, 100); break;
    }
    out << "],\"transition_rules\":[";
    if (state_id == 1) {
        const int positive = 4 + (npc_number % 4);
        const int negative = -(8 + (npc_number % 5));
        write_rule(out, npc_id, "help_observed", "PLAYER_HELPED_NON_AI_NPC", "OBSERVED", 0.0, 1.0, 2, 110, positive);
        out << ',';
        write_rule(out, npc_id, "help_heard", "PLAYER_HELPED_NON_AI_NPC", "HEARD", profile.trust, 1.0, 2, 100, positive);
        out << ',';
        write_rule(out, npc_id, "harm_observed", "PLAYER_HARMED_NON_AI_NPC", "OBSERVED", 0.0, 1.0, 3, 120, negative);
        out << ',';
        write_rule(out, npc_id, "harm_heard_doubt", "PLAYER_HARMED_NON_AI_NPC", "HEARD", 0.0, profile.trust - 0.001, 4, 115, -1);
        out << ',';
        write_rule(out, npc_id, "harm_heard_trusted", "PLAYER_HARMED_NON_AI_NPC", "HEARD", profile.trust, 1.0, 5, 105, negative / 2);
        out << ',';
        write_rule(out, npc_id, "steal_observed", "PLAYER_STOLE", "OBSERVED", 0.0, 1.0, 3, 118, negative);
        out << ',';
        write_rule(out, npc_id, "trade_observed", "PLAYER_TRADED", "OBSERVED", 0.0, 1.0, 2, 80, 2);
        out << ',';
        write_rule(out, npc_id, "help_ai_direct", "PLAYER_HELPED_AI_NPC", "OBSERVED", 0.0, 1.0, 2, 125, positive);
        out << ',';
        write_rule(out, npc_id, "harm_ai_direct", "PLAYER_HARMED_AI_NPC", "OBSERVED", 0.0, 1.0, 3, 130, negative);
    }
    out << "],\"goal_modifier\":\"goal_" << unique
        << "\",\"player_evaluation_modifier\":0,\"relationship_modifiers\":{},"
           "\"world_effect_candidates\":[],\"time_based_rules\":[],\"priority\":100,\"is_terminal\":false}";
}

void write_undefined_state(std::ostream& out, const std::string& npc_id, int state_id) {
    out << "{\"state_id\":" << state_id
        << ",\"state_name\":\"UNDEFINED\",\"state_description\":\"Explicitly undefined fixture slot "
        << state_id << " for " << npc_id
        << "\",\"undefined\":true,\"dialogue_candidates\":[],\"action_candidates\":[],"
           "\"transition_rules\":[],\"goal_modifier\":\"\",\"player_evaluation_modifier\":0,"
           "\"relationship_modifiers\":{},\"world_effect_candidates\":[],\"time_based_rules\":[],"
           "\"priority\":0,\"is_terminal\":false}";
}

void generate(const std::filesystem::path& output_path) {
    if (!output_path.parent_path().empty()) {
        std::filesystem::create_directories(output_path.parent_path());
    }
    std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("cannot open fixture output: " + output_path.string());
    }
    out << std::fixed << std::setprecision(3);
    out << "{\n"
           "  \"fixture_id\": \"stage_a_fixture_v1\",\n"
           "  \"fixture_only\": true,\n"
           "  \"schema_version\": \"1.0.0\",\n"
           "  \"simulation_version\": \"stage-a-0.1.0\",\n"
           "  \"world\": {\"world_id\":\"world_001\",\"world_seed\":424242,\"current_world_time_minutes\":0,\"last_simulated_at\":0},\n"
           "  \"settings\": {\"real_seconds_per_game_hour\":60,\"offline_limit_seconds\":604800,\"chain_limit\":128,\"witness_threshold\":0.35},\n"
           "  \"allowed_player_actions\": [\"HELP\",\"HARM\",\"TALK\",\"TRADE\",\"STEAL\",\"MOVE\",\"WAIT\"],\n"
           "  \"event_types\": [\"PLAYER_HELPED_NON_AI_NPC\",\"PLAYER_HARMED_NON_AI_NPC\",\"PLAYER_HELPED_AI_NPC\",\"PLAYER_HARMED_AI_NPC\",\"PLAYER_TALKED\",\"PLAYER_TRADED\",\"PLAYER_STOLE\",\"PLAYER_ENTERED_LOCATION\",\"PLAYER_LEFT_LOCATION\",\"AI_NPC_OBSERVED_EVENT\",\"AI_NPC_HEARD_EVENT\",\"AI_NPC_INTERVENED\",\"AI_NPC_CHANGED_STATE\",\"AI_NPC_CHANGED_RELATIONSHIP\",\"AI_NPC_STARTED_ACTION\",\"AI_NPC_COMPLETED_ACTION\",\"RUMOR_CREATED\",\"RUMOR_TRANSFERRED\",\"COUNTRY_STATE_CHANGED\",\"TIME_ELAPSED\",\"OFFLINE_SIMULATION_COMPLETED\",\"CHAIN_LIMIT_REACHED\"],\n"
           "  \"locations\": [\n"
           "    {\"location_id\":\"capital\",\"visibility\":0.85,\"obstruction\":0.10},\n"
           "    {\"location_id\":\"market\",\"visibility\":0.90,\"obstruction\":0.10},\n"
           "    {\"location_id\":\"tavern\",\"visibility\":0.60,\"obstruction\":0.30},\n"
           "    {\"location_id\":\"residential\",\"visibility\":0.75,\"obstruction\":0.20},\n"
           "    {\"location_id\":\"gate\",\"visibility\":0.95,\"obstruction\":0.05}\n"
           "  ],\n"
           "  \"country\": {\"country_id\":\"country_001\",\"name\":\"MVP Test Nation\",\"stability\":50,\"security\":50,\"economy\":50,\"food\":50,\"military\":50,\"public_support\":50,\"authority\":50,\"crime_level\":10,\"active_issues\":[],\"updated_at\":0},\n"
           "  \"player\": {\"player_id\":\"player_001\",\"country_id\":\"country_001\",\"current_location_id\":\"capital\",\"status\":\"ACTIVE\",\"inventory\":[],\"funds\":100,\"reputation\":0,\"crime_record\":0,\"achievement_tags\":[],\"action_history\":[],\"last_login_at\":0,\"created_at\":0,\"updated_at\":0},\n"
           "  \"ai_npcs\": [\n";

    for (int index = 0; index < static_cast<int>(profiles.size()); ++index) {
        const auto& profile = profiles[static_cast<std::size_t>(index)];
        const int number = index + 1;
        const std::string npc_id = id_for("ai_npc_", number);
        out << "    {\"npc_id\":\"" << npc_id << "\",\"name\":\"" << profile.name
            << "\",\"role\":\"" << profile.role << "\",\"faction_id\":\"fixture_faction_"
            << ((number - 1) % 4 + 1) << "\",\"country_id\":\"country_001\","
               "\"current_location_id\":\"" << profile.location
            << "\",\"initial_state_id\":1,\"state_slot_count\":255,\"attention\":" << profile.attention
            << ",\"hearing_trust_threshold\":" << profile.trust
            << ",\"personality_traits\":{\"caution\":" << (40 + number)
            << ",\"empathy\":" << (70 - number) << "}"
            << ",\"player_evaluation\":0,\"relationships\":{},\"current_goal\":\"fixture_goal_"
            << npc_id << "\",\"memory_summary\":\"Temporary memory summary for " << npc_id
            << "\",\"known_events\":[],\"active_action\":\"\",\"status\":\"ACTIVE\",\"created_at\":0,\"updated_at\":0,\"states\":[";
        for (int state_id = 1; state_id <= 255; ++state_id) {
            if (state_id > 1) {
                out << ',';
            }
            if (state_id <= 5) {
                write_defined_state(out, profile, npc_id, number, state_id);
            } else {
                write_undefined_state(out, npc_id, state_id);
            }
        }
        out << "]}";
        if (number != static_cast<int>(profiles.size())) {
            out << ',';
        }
        out << '\n';
    }

    constexpr std::array<const char*, 5> locations{{"tavern", "market", "residential", "capital", "gate"}};
    out << "  ],\n  \"non_ai_npcs\": [\n";
    for (int number = 1; number <= 20; ++number) {
        const std::string npc_id = id_for("non_ai_npc_", number);
        out << "    {\"non_ai_npc_id\":\"" << npc_id << "\",\"seed\":" << (9000 + number)
            << ",\"country_id\":\"country_001\",\"current_location_id\":\""
            << locations[static_cast<std::size_t>((number - 1) % 5)]
            << "\",\"category\":\"FIXTURE_CIVILIAN\",\"occupation\":\"fixture_occupation_"
            << number << "\",\"basic_profile\":\"Temporary Stage A profile " << number
            << "\",\"current_activity\":\"WAIT\",\"temporary_memory\":[],\"spawned_at\":0,\"despawn_policy\":\"FIXTURE_PERSIST\"}";
        if (number != 20) {
            out << ',';
        }
        out << '\n';
    }
    out << "  ]\n}\n";
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::filesystem::path output = argc >= 2 ? argv[1] : "data/stage_a_fixture.json";
        generate(output);
        std::cout << "Generated Stage A fixture: " << output.string() << '\n';
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fixture generation failed: " << ex.what() << '\n';
        return 1;
    }
}
