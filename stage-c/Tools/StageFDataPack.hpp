#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace nation_sim_stage_f {

struct GenerationResult {
    std::string world_manifest_sha256;
    std::string definition_hash_manifest_sha256;
    std::string state_index_sha256;
    std::size_t country_count{};
    std::size_t ai_npc_count{};
    std::size_t state_slot_count{};
};

struct ValidationIssue {
    std::string severity;
    std::string country_id;
    std::string npc_id;
    std::optional<int> state_id;
    std::string rule_id;
    std::string error_code;
    std::string message;
    std::string source_file;
};

class StageFScaleDataGenerator final {
public:
    static GenerationResult generate(const std::filesystem::path& config_path,
                                     const std::filesystem::path& fixture_path,
                                     const std::filesystem::path& stage_e_overlay_path,
                                     const std::filesystem::path& output_root);
};

class StageFDataPackValidator final {
public:
    static std::vector<ValidationIssue> validate(const std::filesystem::path& data_root,
                                                 const std::filesystem::path& fixture_path,
                                                 const std::filesystem::path& stage_e_overlay_path,
                                                 bool scan_all_state_shards = true);
    static std::vector<ValidationIssue> validate_state_shard_document(
        const std::string& npc_id,
        const std::string& expected_country_id,
        const std::string& shard_json,
        const std::filesystem::path& fixture_path,
        const std::filesystem::path& stage_e_overlay_path,
        const std::string& source_file = "negative_state_shard.json");
    static std::string json_lines(const std::vector<ValidationIssue>& issues);
    static int error_count(const std::vector<ValidationIssue>& issues);
};

} // namespace nation_sim_stage_f
