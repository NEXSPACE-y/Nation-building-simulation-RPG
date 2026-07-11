#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nation_sim_stage_e {

struct ValidationIssue {
    std::string severity;
    std::string npc_id;
    std::optional<int> state_id;
    std::string rule_id;
    std::string error_code;
    std::string message;
    std::string source_file;
};

class StateDefinitionValidator final {
public:
    static std::vector<ValidationIssue> validate(
        const std::filesystem::path& overlay_path,
        const std::filesystem::path& fixture_path);
    static std::string json_lines(const std::vector<ValidationIssue>& issues);
    static int error_count(const std::vector<ValidationIssue>& issues);
};

} // namespace nation_sim_stage_e
