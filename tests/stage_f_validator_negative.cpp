#include "StageFDataPack.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

using json=nlohmann::json;
using nation_sim_stage_f::StageFDataPackValidator;
using nation_sim_stage_f::ValidationIssue;

namespace {
const std::filesystem::path scale_root=NATION_SIM_STAGE_F_DATA_ROOT;
const std::filesystem::path fixture_path=NATION_SIM_TEST_FIXTURE;
const std::filesystem::path overlay_path=NATION_SIM_STAGE_E_OVERLAY;
const std::filesystem::path output_root=NATION_SIM_STAGE_F_NEGATIVE_OUTPUT_DIR;

json read_json(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    if (!input) throw std::runtime_error("cannot read "+path.string());
    return json::parse(input);
}

void write_json(const std::filesystem::path& path,const json& value) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path,std::ios::binary|std::ios::trunc);
    if (!output) throw std::runtime_error("cannot write "+path.string());
    output<<value.dump(2)<<'\n';
}

std::vector<ValidationIssue> state_negative_issues() {
    json shard=read_json(scale_root/"state_shards"/"ai_npc_003.json");
    shard["schema_version"]="invalid";
    shard["npc_id"]="wrong_npc";
    shard["states"].erase(shard["states"].end()-1);
    shard["states"][1]["state_id"]=1;
    shard["states"][2]["state_id"]=999;
    shard["states"][3]["action_candidates"][0]["type"]="UNKNOWN_ACTION";
    shard["states"][4]["relationship_modifiers"]["unknown_metric"]=1;
    shard["states"][5]["transition_rules"][0]["trigger_event_type"]="UNKNOWN_EVENT";
    shard["states"][6]["transition_rules"][0]["trigger_event_type"]="AI_NPC_CHANGED_STATE";
    shard["states"][6]["transition_rules"][0]["target_state_id"]=7;
    shard["states"][7]["transition_rules"].push_back(shard["states"][7]["transition_rules"][0]);
    shard["states"][9]["transition_rules"]=json::array();
    return StageFDataPackValidator::validate_state_shard_document(
        "ai_npc_003","country_001",shard.dump(),fixture_path,overlay_path,"negative_state_shard.json");
}

std::vector<ValidationIssue> pack_negative_issues() {
    const auto root=output_root/"pack_fixture";
    std::filesystem::create_directories(root/"ai");
    std::filesystem::create_directories(root/"states");
    json shared=read_json(scale_root/"state_shards"/"ai_npc_003.json");
    json state_one=shared; state_one["npc_id"]="ai_npc_001"; state_one["country_id"]="country_001";
    json state_two=shared; state_two["npc_id"]="ai_npc_002"; state_two["country_id"]="country_001";
    write_json(root/"states"/"one.json",state_one);
    write_json(root/"states"/"two.json",state_two);
    const json ai_one={{"country_id","country_001"},{"state_slot_count",254},{"state_shard","states/one.json"},
                       {"state_shard_sha256","invalid"},{"runtime",{{"country_id","country_001"}}}};
    const json ai_two={{"country_id","country_001"},{"state_slot_count",255},{"state_shard","states/two.json"},
                       {"state_shard_sha256","invalid"},{"runtime",{{"country_id","country_001"}}}};
    write_json(root/"ai"/"one.json",ai_one);
    write_json(root/"ai"/"two.json",ai_two);
    const json ai_refs=json::array({
        {{"npc_id","ai_npc_001"},{"path","ai/one.json"},{"sha256","invalid"}},
        {{"npc_id","ai_npc_001"},{"path","ai/missing.json"},{"sha256","invalid"}},
        {{"npc_id","ai_npc_9999"},{"path","ai/two.json"},{"sha256","invalid"}},
        {{"npc_id","ai_npc_002"},{"path","ai/two.json"},{"sha256","invalid"}}});
    write_json(root/"country.json",{{"country_id","wrong_country"},{"ai_npc_count",4},{"ai_manifests",ai_refs}});
    write_json(root/"world_manifest.json",{
        {"fixture_only",false},{"production_content",true},{"schema_version","invalid"},
        {"base_fixture_id",read_json(fixture_path).at("fixture_id")},
        {"stage_e_overlay_id",read_json(overlay_path).at("overlay_id")},
        {"counts",{{"countries",1},{"ai_npcs",4},{"state_slots",1019}}},
        {"state_index","missing_index.json"},{"state_index_sha256","invalid"},
        {"definition_hash_manifest","missing_hashes.json"},{"definition_hash_manifest_sha256","invalid"},
        {"country_manifests",json::array({{{"country_id","country_001"},{"path","country.json"},{"sha256","invalid"}}})}});
    return StageFDataPackValidator::validate(root,fixture_path,overlay_path,true);
}
}

int main() {
    try {
        std::filesystem::create_directories(output_root);
        auto issues=state_negative_issues();
        auto pack_issues=pack_negative_issues();
        issues.insert(issues.end(),pack_issues.begin(),pack_issues.end());
        const std::set<std::string> required={
            "COUNTRY_COUNT_MISMATCH","AI_NPC_COUNT_MISMATCH","AI_NPC_COUNTRY_DISTRIBUTION_MISMATCH",
            "STATE_SLOT_COUNT_MISMATCH","STATE_ID_DUPLICATE","STATE_ID_OUT_OF_RANGE","NPC_ID_DUPLICATE",
            "NPC_TABLE_IDENTICAL","MANIFEST_HASH_MISMATCH","SHARD_HASH_MISMATCH","INDEX_TARGET_MISSING",
            "STAGE_E_DEFINITION_OVERWRITTEN","LEGACY_NPC_MISSING","CROSS_COUNTRY_REFERENCE_INVALID",
            "UNKNOWN_EVENT_TYPE","UNKNOWN_ACTION_TYPE","UNKNOWN_RELATIONSHIP_METRIC",
            "UNREACHABLE_REQUIRED_STATE","IMMEDIATE_LOOP","PRIORITY_CONFLICT","PACK_SCHEMA_MISMATCH"};
        std::set<std::string> observed;
        for (const auto& value:issues) if (value.severity=="ERROR") observed.insert(value.error_code);
        write_json(output_root/"stage_f_validator_negative_evidence.json",{
            {"required_error_count",required.size()},{"observed_error_count",observed.size()},
            {"required",required},{"observed",observed},{"pass",std::includes(observed.begin(),observed.end(),required.begin(),required.end())}});
        std::ofstream jsonl(output_root/"stage_f_negative_validator.jsonl",std::ios::binary|std::ios::trunc);
        jsonl<<StageFDataPackValidator::json_lines(issues);
        bool pass=true;
        for (const auto& code:required) {
            const bool found=observed.contains(code);
            std::cout<<(found?"[PASS] ":"[FAIL] ")<<code<<'\n';
            pass=pass&&found;
        }
        std::cout<<"Stage F negative validator: "<<(pass?required.size():observed.size())<<'/'<<required.size()<<" required errors detected\n";
        return pass?0:1;
    } catch (const std::exception& exception) {
        std::cerr<<"[FATAL] "<<exception.what()<<'\n';
        return 1;
    }
}
