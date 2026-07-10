#include "nation_sim/simulation.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using nation_sim::Simulation;

std::vector<std::string> tokens(const std::string& line) {
    std::istringstream input(line);
    std::vector<std::string> values;
    for (std::string value; input >> value;) values.push_back(std::move(value));
    return values;
}

void print_help() {
    std::cout
        << "Stage A commands:\n"
        << "  status                              world, player, and country state\n"
        << "  npcs                                list AI NPC current states\n"
        << "  npc <ai_npc_id>                    inspect one AI NPC\n"
        << "  relationships                      show non-zero AI relationships\n"
        << "  action <HELP|HARM|TALK|TRADE|STEAL> <target_id>\n"
        << "  move <location_id>                 MOVE player action\n"
        << "  wait                                WAIT for 60 game minutes\n"
        << "  rumor <non_ai_id> <ai_id> <event_id> <credibility>\n"
        << "  offline <real_seconds>              simulate capped offline elapsed time\n"
        << "  events [count]                      show recent event history\n"
        << "  trace <event_id>                    show causal path\n"
        << "  save <path>                         atomic save\n"
        << "  load <path>                         load and validate a save\n"
        << "  export-logs <directory>             write audit.jsonl and causal.jsonl\n"
        << "  help\n"
        << "  quit\n";
}

void print_status(const Simulation& simulation) {
    const auto& country = simulation.country();
    const auto& player = simulation.player();
    std::cout << "world_seed=" << simulation.world_seed()
              << " game_minutes=" << simulation.current_world_time_minutes()
              << " tick=" << simulation.simulation_tick() << '\n'
              << "player=" << player.player_id << " location=" << player.current_location_id
              << " reputation=" << player.reputation << " crime_record=" << player.crime_record << '\n'
              << "country=" << country.country_id << " name=\"" << country.name << "\""
              << " stability=" << country.stability << " security=" << country.security
              << " economy=" << country.economy << " food=" << country.food
              << " military=" << country.military << " public_support=" << country.public_support
              << " authority=" << country.authority << " crime_level=" << country.crime_level << '\n'
              << "events=" << simulation.event_history().size()
              << " pending=" << simulation.pending_events().size()
              << " audits=" << simulation.audit_log().size() << '\n';
}

void export_logs(const Simulation& simulation, const std::filesystem::path& directory) {
    std::filesystem::create_directories(directory);
    {
        std::ofstream output(directory / "audit.jsonl", std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("cannot write audit log");
        output << simulation.audit_log_json_lines();
    }
    {
        std::ofstream output(directory / "causal.jsonl", std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("cannot write causal log");
        output << simulation.causal_log_json_lines();
    }
    std::cout << "logs exported to " << directory.string() << '\n';
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::filesystem::path fixture = NATION_SIM_DEFAULT_FIXTURE;
        for (int i = 1; i < argc; ++i) {
            const std::string argument = argv[i];
            if (argument == "--fixture" && i + 1 < argc) fixture = argv[++i];
            else if (argument == "--help") {
                std::cout << "Usage: nation_cli [--fixture <stage_a_fixture.json>]\n";
                return 0;
            } else {
                throw std::invalid_argument("unknown command-line argument: " + argument);
            }
        }
        Simulation simulation = Simulation::from_fixture(fixture);
        std::cout << "Nation Simulation Stage A CLI\n"
                  << "fixture=" << fixture.string() << "\n"
                  << "Type 'help' for commands. Fixture content is not production content.\n";
        print_status(simulation);

        for (std::string line; std::cout << "> " && std::getline(std::cin, line);) {
            const auto args = tokens(line);
            if (args.empty()) continue;
            try {
                if (args[0] == "quit" || args[0] == "exit") break;
                if (args[0] == "help") print_help();
                else if (args[0] == "status") print_status(simulation);
                else if (args[0] == "npcs") {
                    for (const auto& npc : simulation.ai_npcs()) {
                        const auto& state = npc.states.at(static_cast<std::size_t>(npc.current_state_id - 1));
                        std::cout << npc.npc_id << " role=" << npc.role << " location=" << npc.current_location_id
                                  << " state=" << npc.current_state_id << ':' << state.state_name
                                  << " player_evaluation=" << npc.player_evaluation << '\n';
                    }
                } else if (args[0] == "npc" && args.size() == 2) {
                    const auto& npc = simulation.ai_npc(args[1]);
                    const auto& state = npc.states.at(static_cast<std::size_t>(npc.current_state_id - 1));
                    std::cout << npc.npc_id << " name=\"" << npc.name << "\" role=" << npc.role
                              << " location=" << npc.current_location_id << " state=" << npc.current_state_id
                              << ':' << state.state_name << " goal=" << npc.current_goal
                              << " player_evaluation=" << npc.player_evaluation
                              << " known_events=" << npc.known_events.size() << '\n';
                } else if (args[0] == "relationships") {
                    for (const auto& npc : simulation.ai_npcs()) {
                        for (const auto& [other, metrics] : npc.relationships) {
                            for (const auto& [metric, value] : metrics) {
                                std::cout << npc.npc_id << " -> " << other << ' ' << metric << '=' << value << '\n';
                            }
                        }
                    }
                } else if (args[0] == "action" && args.size() == 3) {
                    std::cout << "root_event=" << simulation.player_action(args[1], args[2]) << '\n';
                } else if (args[0] == "move" && args.size() == 2) {
                    std::cout << "root_event=" << simulation.player_action("MOVE", "", args[1]) << '\n';
                } else if (args[0] == "wait" && args.size() == 1) {
                    std::cout << "root_event=" << simulation.player_action("WAIT") << '\n';
                } else if (args[0] == "rumor" && args.size() == 5) {
                    std::cout << "rumor_event=" << simulation.share_rumor(args[1], args[2], args[3], std::stod(args[4])) << '\n';
                } else if (args[0] == "offline" && args.size() == 2) {
                    const auto result = simulation.advance_offline(std::stoll(args[1]));
                    std::cout << "requested_real_seconds=" << result.requested_real_seconds
                              << " applied_real_seconds=" << result.applied_real_seconds
                              << " elapsed_game_minutes=" << result.elapsed_game_minutes
                              << " important_events=" << result.important_events.size()
                              << " changed_ai_npcs=" << result.changed_ai_npcs.size()
                              << " country_changes=" << result.country_changes.size()
                              << " unresolved_events=" << result.unresolved_events.size() << '\n';
                } else if (args[0] == "events") {
                    const auto& events = simulation.event_history();
                    const std::size_t count = args.size() == 2 ? static_cast<std::size_t>(std::stoull(args[1])) : 20;
                    const std::size_t begin = events.size() > count ? events.size() - count : 0;
                    for (std::size_t i = begin; i < events.size(); ++i) {
                        const auto& event = events[i];
                        std::cout << event.event_id << " tick=" << event.simulation_tick
                                  << " type=" << event.event_type << " actor=" << event.actor_id
                                  << " target=" << event.target_id << " source=" << event.source_event_id
                                  << " root=" << event.root_event_id << '\n';
                    }
                } else if (args[0] == "trace" && args.size() == 2) {
                    for (const auto& entry : simulation.causal_path(args[1])) std::cout << entry << '\n';
                } else if (args[0] == "save" && args.size() == 2) {
                    simulation.save(args[1]);
                    std::cout << "saved=" << args[1] << '\n';
                } else if (args[0] == "load" && args.size() == 2) {
                    simulation = Simulation::load_save(fixture, args[1]);
                    std::cout << "loaded=" << args[1] << '\n';
                } else if (args[0] == "export-logs" && args.size() == 2) {
                    export_logs(simulation, args[1]);
                } else {
                    std::cout << "Unknown or malformed command. Type 'help'.\n";
                }
            } catch (const std::exception& ex) {
                std::cout << "ERROR: " << ex.what() << '\n';
            }
        }
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Fatal Stage A error: " << ex.what() << '\n';
        return 1;
    }
}
