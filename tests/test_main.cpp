#include "nation_sim/simulation.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace {

using nation_sim::Event;
using nation_sim::Simulation;

struct TestFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

#define CHECK(condition) \
    do { if (!(condition)) throw TestFailure(std::string("CHECK failed: ") + #condition + \
        " at " + __FILE__ + ":" + std::to_string(__LINE__)); } while (false)

const std::filesystem::path fixture = NATION_SIM_TEST_FIXTURE;
const std::filesystem::path parity_contract_path = NATION_SIM_PARITY_CONTRACT;
const std::filesystem::path output_dir = NATION_SIM_TEST_TEMP_DIR;

const nlohmann::json& parity_contract() {
    static const nlohmann::json value = [] {
        std::ifstream input(parity_contract_path, std::ios::binary);
        if (!input) throw TestFailure("cannot open parity contract");
        return nlohmann::json::parse(input);
    }();
    return value;
}

std::string sha256(std::string_view input) {
    constexpr std::array<std::uint32_t, 64> constants{
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    std::vector<std::uint8_t> bytes(input.begin(), input.end());
    const std::uint64_t bit_length = static_cast<std::uint64_t>(bytes.size()) * 8;
    bytes.push_back(0x80);
    while (bytes.size() % 64 != 56) bytes.push_back(0);
    for (int shift = 56; shift >= 0; shift -= 8) bytes.push_back(static_cast<std::uint8_t>(bit_length >> shift));
    std::array<std::uint32_t, 8> hash{
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    const auto rotate_right = [](std::uint32_t value, unsigned count) {
        return (value >> count) | (value << (32 - count));
    };
    for (std::size_t offset = 0; offset < bytes.size(); offset += 64) {
        std::array<std::uint32_t, 64> words{};
        for (std::size_t i = 0; i < 16; ++i) {
            const auto base = offset + i * 4;
            words[i] = (static_cast<std::uint32_t>(bytes[base]) << 24) |
                       (static_cast<std::uint32_t>(bytes[base + 1]) << 16) |
                       (static_cast<std::uint32_t>(bytes[base + 2]) << 8) |
                       static_cast<std::uint32_t>(bytes[base + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const auto s0 = rotate_right(words[i - 15], 7) ^ rotate_right(words[i - 15], 18) ^ (words[i - 15] >> 3);
            const auto s1 = rotate_right(words[i - 2], 17) ^ rotate_right(words[i - 2], 19) ^ (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        auto a=hash[0], b=hash[1], c=hash[2], d=hash[3], e=hash[4], f=hash[5], g=hash[6], h=hash[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const auto sum1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
            const auto choice = (e & f) ^ (~e & g);
            const auto temp1 = h + sum1 + choice + constants[i] + words[i];
            const auto sum0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
            const auto majority = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = sum0 + majority;
            h=g; g=f; f=e; e=d+temp1; d=c; c=b; b=a; a=temp1+temp2;
        }
        hash[0]+=a; hash[1]+=b; hash[2]+=c; hash[3]+=d;
        hash[4]+=e; hash[5]+=f; hash[6]+=g; hash[7]+=h;
    }
    std::ostringstream output;
    output << std::hex << std::setfill('0');
    for (const auto value : hash) output << std::setw(8) << value;
    return output.str();
}

const Event& first_event(const Simulation& simulation, const std::string& type,
                         const std::string& actor = {}) {
    const auto& values = simulation.event_history();
    const auto it = std::find_if(values.begin(), values.end(), [&](const Event& event) {
        return event.event_type == type && (actor.empty() || event.actor_id == actor);
    });
    if (it == values.end()) throw TestFailure("event not found: " + type + " actor=" + actor);
    return *it;
}

const nation_sim::AuditEntry& audit_for(const Simulation& simulation, const std::string& npc_id,
                                        const std::string& selected_action = {}) {
    const auto& values = simulation.audit_log();
    const auto it = std::find_if(values.begin(), values.end(), [&](const auto& audit) {
        return audit.decision_npc_id == npc_id && (selected_action.empty() || audit.selected_action == selected_action);
    });
    if (it == values.end()) throw TestFailure("audit not found for " + npc_id + " action=" + selected_action);
    return *it;
}

bool path_contains(const std::vector<std::string>& path, const std::string& type) {
    return std::any_of(path.begin(), path.end(), [&](const std::string& value) {
        return value.ends_with(":" + type);
    });
}

Simulation run_scenario_a() {
    const auto& contract = parity_contract().at("scenario_a");
    auto simulation = Simulation::from_fixture(fixture);
    simulation.player_action("MOVE", "", "tavern");
    const std::string help = simulation.player_action("HELP", "non_ai_npc_001");
    const auto& help_event = simulation.event(help);
    CHECK(std::find(help_event.observed_by.begin(), help_event.observed_by.end(), "ai_npc_003") ==
          help_event.observed_by.end());
    const int before = simulation.ai_npc("ai_npc_003").player_evaluation;
    simulation.share_rumor("non_ai_npc_001", "ai_npc_003", help, 0.90);
    const auto& merchant = simulation.ai_npc("ai_npc_003");
    CHECK(merchant.current_state_id == contract.at("expected_state_id").get<int>());
    CHECK(merchant.player_evaluation > before);
    const auto& audit = audit_for(simulation, "ai_npc_003", contract.at("expected_action").get<std::string>());
    CHECK(!audit.selected_dialogue.empty());
    CHECK(audit.selected_rule.find("help_heard") != std::string::npos);
    CHECK(first_event(simulation, "RUMOR_CREATED").source_event_id == help);
    CHECK(first_event(simulation, "RUMOR_TRANSFERRED").credibility == 0.90);
    return simulation;
}

Simulation run_scenario_b_and_d() {
    const auto& root_contract = parity_contract();
    const auto& scenario_b = root_contract.at("scenario_b");
    const auto& scenario_d = root_contract.at("scenario_d");
    auto simulation = Simulation::from_fixture(fixture);
    simulation.player_action("MOVE", "", "market");
    const int initial_security = simulation.country().security;
    const int initial_crime = simulation.country().crime_level;
    const std::string harm = simulation.player_action("HARM", "non_ai_npc_002");
    const auto& guard = simulation.ai_npc("ai_npc_001");
    CHECK(guard.current_state_id == scenario_b.at("expected_state_id").get<int>());
    const auto& guard_audit = audit_for(simulation, "ai_npc_001", scenario_b.at("expected_action").get<std::string>());
    CHECK(guard_audit.selected_rule.find("harm_observed") != std::string::npos);
    const auto& captain = simulation.ai_npc("ai_npc_002");
    CHECK(captain.current_state_id == scenario_d.at("expected_state_id").get<int>());
    CHECK(captain.relationships.at("ai_npc_001").at("trust") == 1);
    CHECK(first_event(simulation, "AI_NPC_CHANGED_RELATIONSHIP", "ai_npc_002").target_id == "ai_npc_001");
    CHECK(audit_for(simulation, "ai_npc_002", "CHANGE_COUNTRY_STATE").selected_rule.find("harm_heard_trusted") != std::string::npos);
    CHECK(simulation.country().security == initial_security + scenario_d.at("security_delta").get<int>());
    CHECK(simulation.country().crime_level == initial_crime + scenario_d.at("crime_level_delta").get<int>());
    const auto& country_event = first_event(simulation, "COUNTRY_STATE_CHANGED", "ai_npc_002");
    const auto path = simulation.causal_path(country_event.event_id);
    CHECK(path.front().starts_with(harm + ":PLAYER_HARMED_NON_AI_NPC"));
    CHECK(path_contains(path, "AI_NPC_OBSERVED_EVENT"));
    CHECK(path_contains(path, "AI_NPC_HEARD_EVENT"));
    CHECK(path_contains(path, "COUNTRY_STATE_CHANGED"));
    return simulation;
}

void test_fixture_contract() {
    const auto& contract = parity_contract();
    const auto simulation = Simulation::from_fixture(fixture);
    CHECK(simulation.country().country_id == contract.at("country_id").get<std::string>());
    CHECK(simulation.country().name == "MVP Test Nation");
    CHECK(simulation.ai_npcs().size() == contract.at("ai_npc_count").get<std::size_t>());
    CHECK(simulation.non_ai_npcs().size() == contract.at("non_ai_npc_count").get<std::size_t>());
    CHECK(simulation.world_seed() == contract.at("world_seed").get<std::uint64_t>());
    CHECK(simulation.chain_limit() == 128);
    CHECK(simulation.player().status == "ACTIVE");
    std::set<std::string> ids;
    std::set<std::string> state_signatures;
    for (const auto& npc : simulation.ai_npcs()) {
        CHECK(ids.insert(npc.npc_id).second);
        CHECK(npc.states.size() == contract.at("state_slots_per_ai_npc").get<std::size_t>());
        CHECK(!npc.faction_id.empty());
        CHECK(npc.personality_traits.contains("caution"));
        CHECK(npc.current_state_id >= 1 && npc.current_state_id <= 255);
        CHECK(!npc.states[0].undefined);
        CHECK(!npc.states[1].undefined);
        for (std::size_t index = 5; index < npc.states.size(); ++index) {
            CHECK(npc.states[index].undefined);
            CHECK(npc.states[index].state_name == "UNDEFINED");
        }
        std::string signature;
        for (std::size_t index = 0; index < 5; ++index) {
            signature += npc.states[index].state_name + "|" + npc.states[index].state_description + "|";
        }
        CHECK(state_signatures.insert(signature).second);
    }
}

void test_scenario_a_non_ai_good_deed_rumor() {
    const auto simulation = run_scenario_a();
    CHECK(!simulation.audit_log().empty());
}

void test_scenario_b_observed_violence() {
    const auto simulation = run_scenario_b_and_d();
    CHECK(first_event(simulation, "AI_NPC_OBSERVED_EVENT", "ai_npc_001").credibility == 1.0);
}

void test_scenario_c_doubted_rumor() {
    const auto& contract = parity_contract().at("scenario_c");
    auto simulation = Simulation::from_fixture(fixture);
    simulation.player_action("MOVE", "", "residential");
    const std::string harm = simulation.player_action("HARM", "non_ai_npc_003");
    const int initial_security = simulation.country().security;
    simulation.share_rumor("non_ai_npc_003", "ai_npc_004", harm, 0.50);
    const auto& investigator = simulation.ai_npc("ai_npc_004");
    CHECK(investigator.current_state_id == contract.at("expected_state_id").get<int>());
    const auto& audit = audit_for(simulation, "ai_npc_004", contract.at("expected_action").get<std::string>());
    CHECK(audit.selected_rule.find("harm_heard_doubt") != std::string::npos);
    CHECK(simulation.country().security == initial_security);
    const bool investigator_changed_country = std::any_of(simulation.event_history().begin(), simulation.event_history().end(),
        [](const Event& event) { return event.event_type == "COUNTRY_STATE_CHANGED" && event.actor_id == "ai_npc_004"; });
    CHECK(!investigator_changed_country);
}

void test_scenario_d_ai_to_ai_to_country_chain() {
    const auto simulation = run_scenario_b_and_d();
    const auto& country_event = first_event(simulation, "COUNTRY_STATE_CHANGED", "ai_npc_002");
    const auto path = simulation.causal_path(country_event.event_id);
    const auto expected_types = parity_contract().at("scenario_d").at("path_event_types").get<std::vector<std::string>>();
    std::vector<std::string> actual_types;
    for (const auto& entry : path) actual_types.push_back(entry.substr(entry.find(':') + 1));
    CHECK(actual_types == expected_types);
    std::filesystem::create_directories(output_dir);
    std::ofstream output(output_dir / "scenario_d_causal_path.txt", std::ios::trunc);
    for (const auto& value : path) output << value << '\n';
}

void test_scenario_e_offline_elapsed_time() {
    const auto& contract = parity_contract().at("scenario_e");
    auto simulation = Simulation::from_fixture(fixture);
    const auto real_seconds = contract.at("real_seconds").get<std::int64_t>();
    const auto game_minutes = contract.at("expected_game_minutes").get<std::int64_t>();
    const auto limit = contract.at("offline_limit_seconds").get<std::int64_t>();
    const auto one_minute = simulation.advance_offline(real_seconds);
    CHECK(one_minute.applied_real_seconds == real_seconds);
    CHECK(one_minute.elapsed_game_minutes == game_minutes);
    CHECK(simulation.current_world_time_minutes() == game_minutes);
    CHECK(first_event(simulation, "OFFLINE_SIMULATION_COMPLETED").payload.at("elapsed_game_minutes") == std::to_string(game_minutes));

    auto capped = Simulation::from_fixture(fixture);
    const auto result = capped.advance_offline(limit + 12345);
    CHECK(result.applied_real_seconds == limit);
    CHECK(result.elapsed_game_minutes == limit);
}

void test_scenario_f_reproducibility() {
    const auto first = run_scenario_a();
    const auto second = run_scenario_a();
    CHECK(first.canonical_snapshot() == second.canonical_snapshot());
    CHECK(first.audit_log_json_lines() == second.audit_log_json_lines());
    CHECK(first.causal_log_json_lines() == second.causal_log_json_lines());
}

void test_scenario_g_mid_chain_save_load() {
    auto continuous = Simulation::from_fixture(fixture);
    continuous.player_action("MOVE", "", "market");
    continuous.enqueue_player_action("HARM", "non_ai_npc_002");
    continuous.process_pending();

    auto interrupted = Simulation::from_fixture(fixture);
    interrupted.player_action("MOVE", "", "market");
    interrupted.enqueue_player_action("HARM", "non_ai_npc_002");
    interrupted.process_pending(2);
    CHECK(!interrupted.pending_events().empty());
    std::filesystem::create_directories(output_dir);
    const auto pending_save_path = output_dir / "mid_chain_pending_save.json";
    const auto resumed_save_path = output_dir / "mid_chain_resumed_save.json";
    interrupted.save(pending_save_path);
    nlohmann::json pending_save;
    {
        std::ifstream input(pending_save_path, std::ios::binary);
        input >> pending_save;
    }
    CHECK(!pending_save.at("pending_events").empty());
    auto resumed = Simulation::load_save(fixture, pending_save_path);
    CHECK(!resumed.pending_events().empty());
    resumed.process_pending();
    const std::string continuous_snapshot = continuous.canonical_snapshot();
    const std::string resumed_snapshot = resumed.canonical_snapshot();
    const std::string continuous_hash = sha256(continuous_snapshot);
    const std::string resumed_hash = sha256(resumed_snapshot);
    CHECK(continuous_hash == resumed_hash);

    {
        std::ofstream output(output_dir / "scenario_g_continuous_snapshot.json", std::ios::trunc);
        output << continuous_snapshot << '\n';
    }
    {
        std::ofstream output(output_dir / "scenario_g_resumed_snapshot.json", std::ios::trunc);
        output << resumed_snapshot << '\n';
    }
    const nlohmann::json evidence{
        {"pending_save_file", pending_save_path.filename().string()},
        {"pending_simulation_tick", pending_save.at("world").at("simulation_tick")},
        {"pending_next_event_sequence", pending_save.at("world").at("next_event_sequence")},
        {"pending_event_count", pending_save.at("pending_events").size()},
        {"pending_events", pending_save.at("pending_events")},
        {"hash_algorithm", "SHA-256"},
        {"continuous_snapshot_sha256", continuous_hash},
        {"resumed_snapshot_sha256", resumed_hash},
        {"hashes_match", continuous_hash == resumed_hash},
        {"continuous_event_count", continuous.event_history().size()},
        {"resumed_event_count", resumed.event_history().size()}
    };
    {
        std::ofstream output(output_dir / "scenario_g_evidence.json", std::ios::trunc);
        output << evidence.dump(2) << '\n';
    }

    resumed.save(resumed_save_path);
    const auto loaded_again = Simulation::load_save(fixture, resumed_save_path);
    CHECK(loaded_again.canonical_snapshot() == resumed.canonical_snapshot());
}

void test_chain_limit_is_logged() {
    auto simulation = Simulation::from_fixture(fixture);
    simulation.set_chain_limit_for_test(1);
    simulation.player_action("MOVE", "", "market");
    const auto& limit = first_event(simulation, "CHAIN_LIMIT_REACHED");
    CHECK(limit.payload.at("processed_event_count") == "1");
    CHECK(limit.payload.contains("remaining_event_count"));
    CHECK(simulation.pending_events().empty());
}

void test_player_action_surface_is_wired() {
    auto simulation = Simulation::from_fixture(fixture);
    simulation.player_action("MOVE", "", "market");
    simulation.player_action("TALK", "non_ai_npc_002");
    simulation.player_action("TRADE", "non_ai_npc_002");
    simulation.player_action("STEAL", "non_ai_npc_002");
    simulation.player_action("WAIT");
    CHECK(first_event(simulation, "PLAYER_TALKED").target_id == "non_ai_npc_002");
    CHECK(first_event(simulation, "PLAYER_TRADED").target_id == "non_ai_npc_002");
    CHECK(first_event(simulation, "PLAYER_STOLE").target_id == "non_ai_npc_002");
    CHECK(first_event(simulation, "TIME_ELAPSED").payload.at("offline") == "false");

    auto help_ai = Simulation::from_fixture(fixture);
    help_ai.player_action("MOVE", "", "gate");
    help_ai.player_action("HELP", "ai_npc_004");
    CHECK(help_ai.ai_npc("ai_npc_004").current_state_id == 2);

    auto harm_ai = Simulation::from_fixture(fixture);
    harm_ai.player_action("MOVE", "", "gate");
    harm_ai.player_action("HARM", "ai_npc_004");
    CHECK(harm_ai.ai_npc("ai_npc_004").current_state_id == 3);
}

void write_evidence_logs() {
    const auto simulation = run_scenario_b_and_d();
    std::filesystem::create_directories(output_dir);
    {
        std::ofstream output(output_dir / "stage_a_audit.jsonl", std::ios::trunc);
        output << simulation.audit_log_json_lines();
    }
    {
        std::ofstream output(output_dir / "stage_a_causal.jsonl", std::ios::trunc);
        output << simulation.causal_log_json_lines();
    }
    {
        std::ofstream output(output_dir / "stage_a_reproducible_snapshot.json", std::ios::trunc);
        output << simulation.canonical_snapshot() << '\n';
    }
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"Fixture contract: 1 country, 20 AI, 20 NON AI, 255 independent slots", test_fixture_contract},
        {"Scenario A: NON AI good deed reaches AI by rumor", test_scenario_a_non_ai_good_deed_rumor},
        {"Scenario B: guard observes violence and acts", test_scenario_b_observed_violence},
        {"Scenario C: skeptical AI investigates doubtful rumor", test_scenario_c_doubted_rumor},
        {"Scenario D: AI-to-AI report changes country state", test_scenario_d_ai_to_ai_to_country_chain},
        {"Scenario E: offline elapsed time and seven-day cap", test_scenario_e_offline_elapsed_time},
        {"Scenario F: deterministic replay", test_scenario_f_reproducibility},
        {"Scenario G: mid-chain save/load equivalence", test_scenario_g_mid_chain_save_load},
        {"Chain limit emits an auditable error event", test_chain_limit_is_logged},
        {"Configured player action surface is wired", test_player_action_surface_is_wired},
    };

    std::ostringstream report;
    std::size_t passed = 0;
    for (const auto& [name, test] : tests) {
        try {
            test();
            ++passed;
            std::cout << "[PASS] " << name << '\n';
            report << "PASS | " << name << '\n';
        } catch (const std::exception& ex) {
            std::cerr << "[FAIL] " << name << " | " << ex.what() << '\n';
            report << "FAIL | " << name << " | " << ex.what() << '\n';
        }
    }
    try {
        write_evidence_logs();
    } catch (const std::exception& ex) {
        std::cerr << "[FAIL] Evidence log generation | " << ex.what() << '\n';
        report << "FAIL | Evidence log generation | " << ex.what() << '\n';
    }
    std::filesystem::create_directories(output_dir);
    report << "SUMMARY | " << passed << '/' << tests.size() << " tests passed\n";
    std::ofstream report_file(output_dir / "acceptance_results.txt", std::ios::trunc);
    report_file << report.str();
    std::cout << "Stage A acceptance: " << passed << '/' << tests.size() << " tests passed\n";
    return passed == tests.size() ? 0 : 1;
}
