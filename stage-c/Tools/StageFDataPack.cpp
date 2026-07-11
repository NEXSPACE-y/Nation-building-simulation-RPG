#include "StageFDataPack.hpp"

#include "nation_sim/stage_f_runtime.hpp"

#include <algorithm>
#include <fstream>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include <nlohmann/json.hpp>

namespace nation_sim_stage_f {
namespace {
using json = nlohmann::json;

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot read file: " + path.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

json read_json(const std::filesystem::path& path) { return json::parse(read_text(path)); }

void write_text(const std::filesystem::path& path, const std::string& value) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("cannot write file: " + path.string());
    output << value;
    output.flush();
    if (!output) throw std::runtime_error("cannot flush file: " + path.string());
}

void write_json(const std::filesystem::path& path, const json& value) { write_text(path, value.dump(2) + '\n'); }

std::string padded(std::uint64_t value, int width) {
    std::ostringstream output;
    output.width(width); output.fill('0'); output << value;
    return output.str();
}

std::string hash_file(const std::filesystem::path& path) {
    return nation_sim::StageFProductionRuntime::sha256_file(path);
}

json generated_state(const std::string& npc_id, int state_id) {
    const int target = state_id == 255 ? 1 : state_id + 1;
    const std::string suffix = padded(static_cast<std::uint64_t>(state_id), 3);
    return {
        {"state_id",state_id},
        {"state_name",npc_id+"_stage_f_fixture_state_"+suffix},
        {"state_description","Stage F fixture-only deterministic state "+suffix+" for "+npc_id},
        {"current_goal","stage_f_fixture_goal_"+npc_id+"_"+suffix},
        {"dialogue_candidates",json::array({{{"dialogue_id","dlg_stage_f_"+npc_id+"_"+suffix},
                                              {"text","Stage F fixture-only line "+suffix+" ["+npc_id+"]"},
                                              {"tone","fixture"},{"priority",100},{"once_only",false},{"cooldown",0}}})},
        {"action_candidates",json::array({{{"type","WAIT"},{"target_id",""},{"priority",100},
                                            {"credibility_factor",1.0},{"country_effects",json::array()}}})},
        {"transition_rules",json::array({{{"rule_id","rule_stage_f_"+npc_id+"_"+suffix},
                                           {"trigger_event_type","STAGE_F_FIXTURE_SIGNAL"},
                                           {"subject_event_type","TIME_ELAPSED"},{"perception","SCHEDULED"},
                                           {"min_credibility",0.0},{"max_credibility",1.0},
                                           {"target_state_id",target},{"priority",100},{"cooldown",0},{"once_only",false}}})},
        {"player_evaluation_modifier",0},{"relationship_modifiers",json::object()},
        {"world_effect_candidates",json::array()},{"time_based_rules",json::array()},
        {"priority",100},{"is_terminal",false}
    };
}

void issue(std::vector<ValidationIssue>& out, std::string severity, std::string country,
           std::string npc, std::optional<int> state, std::string rule, std::string code,
           std::string message, const std::filesystem::path& source) {
    out.push_back({std::move(severity),std::move(country),std::move(npc),state,std::move(rule),
                   std::move(code),std::move(message),source.generic_string()});
}

std::set<std::string> fixture_event_types(const json& fixture, const json& overlay) {
    std::set<std::string> values(fixture.at("event_types").begin(),fixture.at("event_types").end());
    values.insert("STAGE_F_FIXTURE_SIGNAL"); values.insert("STAGE_F_FIXTURE_CROSS_COUNTRY");
    (void)overlay;
    return values;
}

std::vector<ValidationIssue> validate_state_document(
    const std::string& npc_id, const std::string& expected_country_id, const json& shard,
    const json& fixture, const json& overlay, const std::filesystem::path& source) {
    std::vector<ValidationIssue> out;
    const auto events=fixture_event_types(fixture,overlay);
    const std::set<std::string> actions(overlay.at("allowed_action_types").begin(),overlay.at("allowed_action_types").end());
    const std::set<std::string> metrics(overlay.at("relationship_metrics").begin(),overlay.at("relationship_metrics").end());
    if (shard.value("schema_version","")!="stage_f_state_shard_v1" || !shard.value("fixture_only",false) || shard.value("production_content",true))
        issue(out,"ERROR",expected_country_id,npc_id,std::nullopt,"","PACK_SCHEMA_MISMATCH","state shard schema/boundary mismatch",source);
    if (shard.value("npc_id","")!=npc_id || shard.value("country_id","")!=expected_country_id)
        issue(out,"ERROR",expected_country_id,npc_id,std::nullopt,"","CROSS_COUNTRY_REFERENCE_INVALID","state shard identity/country mismatch",source);
    const auto& states=shard.at("states");
    if (states.size()!=255) issue(out,"ERROR",expected_country_id,npc_id,std::nullopt,"","STATE_SLOT_COUNT_MISMATCH","state shard must contain 255 slots",source);
    std::set<int> ids;
    std::map<int,std::set<int>> graph;
    for (const auto& state : states) {
        const int id=state.value("state_id",0);
        if (!ids.insert(id).second) issue(out,"ERROR",expected_country_id,npc_id,id,"","STATE_ID_DUPLICATE","duplicate state id",source);
        if (id<1 || id>255) issue(out,"ERROR",expected_country_id,npc_id,id,"","STATE_ID_OUT_OF_RANGE","state id outside 1..255",source);
        std::map<std::tuple<std::string,std::string,std::string,int>,std::string> priorities;
        for (const auto& action : state.value("action_candidates",json::array()))
            if (!actions.contains(action.value("type",""))) issue(out,"ERROR",expected_country_id,npc_id,id,"","UNKNOWN_ACTION_TYPE","unknown action type",source);
        for (const auto& [metric,value] : state.value("relationship_modifiers",json::object()).items()) {
            (void)value;
            if (!metrics.contains(metric)) issue(out,"ERROR",expected_country_id,npc_id,id,"","UNKNOWN_RELATIONSHIP_METRIC","unknown relationship modifier",source);
        }
        for (const auto& rule : state.value("transition_rules",json::array())) {
            const std::string rule_id=rule.value("rule_id","");
            const std::string trigger=rule.value("trigger_event_type","");
            const std::string subject=rule.value("subject_event_type","");
            if (!events.contains(trigger) || !events.contains(subject))
                issue(out,"ERROR",expected_country_id,npc_id,id,rule_id,"UNKNOWN_EVENT_TYPE","unknown event type",source);
            const std::string metric=rule.value("relationship_metric","");
            if (!metric.empty() && !metrics.contains(metric))
                issue(out,"ERROR",expected_country_id,npc_id,id,rule_id,"UNKNOWN_RELATIONSHIP_METRIC","unknown rule metric",source);
            const int target=rule.value("target_state_id",0);
            graph[id].insert(target);
            if (trigger=="AI_NPC_CHANGED_STATE" && target==id)
                issue(out,"ERROR",expected_country_id,npc_id,id,rule_id,"IMMEDIATE_LOOP","immediate state-change self loop",source);
            const auto signature=std::make_tuple(trigger,subject,rule.value("perception",""),rule.value("priority",0));
            if (priorities.contains(signature))
                issue(out,"ERROR",expected_country_id,npc_id,id,rule_id,"PRIORITY_CONFLICT","same input and priority",source);
            priorities[signature]=rule_id;
        }
        for (const auto& rule : state.value("time_based_rules",json::array())) {
            const int target=rule.value("target_state_id",0);
            graph[id].insert(target);
            if (rule.value("after_game_minutes",0)==0 && target==id)
                issue(out,"ERROR",expected_country_id,npc_id,id,rule.value("rule_id",""),"IMMEDIATE_LOOP","zero-delay self loop",source);
        }
    }
    std::map<std::tuple<int,std::string,std::string,std::string,int>,std::string> external_priorities;
    for (const auto& rule : shard.value("external_transition_rules",json::array())) {
        const std::string rule_id=rule.value("rule_id","");
        const int source_state_id=rule.value("source_state_id",0);
        const int target_state_id=rule.value("target_state_id",0);
        const std::string trigger=rule.value("trigger_event_type","");
        const std::string subject=rule.value("subject_event_type","");
        if (!events.contains(trigger) || !events.contains(subject))
            issue(out,"ERROR",expected_country_id,npc_id,source_state_id,rule_id,"UNKNOWN_EVENT_TYPE","unknown external transition event type",source);
        if (source_state_id<1 || source_state_id>255 || target_state_id<1 || target_state_id>255)
            issue(out,"ERROR",expected_country_id,npc_id,source_state_id,rule_id,"STATE_ID_OUT_OF_RANGE","external transition state id outside 1..255",source);
        graph[source_state_id].insert(target_state_id);
        const auto signature=std::make_tuple(source_state_id,trigger,subject,rule.value("perception",""),rule.value("priority",0));
        if (external_priorities.contains(signature))
            issue(out,"ERROR",expected_country_id,npc_id,source_state_id,rule_id,"PRIORITY_CONFLICT","same external input and priority",source);
        external_priorities[signature]=rule_id;
    }
    std::set<int> reachable{1}; std::queue<int> pending; pending.push(1);
    while (!pending.empty()) { const int current=pending.front(); pending.pop(); for (const int target:graph[current]) if (ids.contains(target)&&reachable.insert(target).second) pending.push(target); }
    for (const int id:ids) if (!reachable.contains(id)) issue(out,"ERROR",expected_country_id,npc_id,id,"","UNREACHABLE_REQUIRED_STATE","state is unreachable from state 1",source);
    return out;
}
}

GenerationResult StageFScaleDataGenerator::generate(const std::filesystem::path& config_path,
                                                    const std::filesystem::path& fixture_path,
                                                    const std::filesystem::path& stage_e_overlay_path,
                                                    const std::filesystem::path& output_root) {
    const json config=read_json(config_path);
    const json fixture=read_json(fixture_path);
    const json overlay=read_json(stage_e_overlay_path);
    if (!config.value("fixture_only",false) || config.value("production_content",true))
        throw std::runtime_error("Stage F generator config must be fixture-only and non-production");
    if (config.value("country_count",0)!=5 || config.value("ai_npc_per_country",0)!=500 ||
        config.value("ai_npc_total",0)!=2500 || config.value("state_slots_per_ai_npc",0)!=255 ||
        config.value("state_slot_total",0)!=637500 ||
        config.value("resident_population_slots_per_country",0)!=50000000 ||
        config.value("resident_population_slots_total",0)!=250000000 ||
        config.value("mobile_population_slots",0)!=100000000)
        throw std::runtime_error("Stage F generator scale contract mismatch");
    if (fixture.at("ai_npcs").size()!=20 || overlay.at("npcs").size()!=3)
        throw std::runtime_error("Stage F generator base fixture/overlay count mismatch");
    std::filesystem::create_directories(output_root/"countries");
    std::filesystem::create_directories(output_root/"ai_manifests");
    std::filesystem::create_directories(output_root/"state_shards");

    std::map<std::string,json> fixture_ai;
    for (const auto& npc:fixture.at("ai_npcs")) fixture_ai[npc.at("npc_id")]=npc;
    std::map<std::string,json> stage_e_ai;
    for (const auto& npc:overlay.at("npcs")) stage_e_ai[npc.at("npc_id")]=npc;
    std::vector<std::string> role_cycle;
    for (const auto& [id,npc]:fixture_ai) { (void)id; role_cycle.push_back(npc.at("role")); }
    std::sort(role_cycle.begin(),role_cycle.end()); role_cycle.erase(std::unique(role_cycle.begin(),role_cycle.end()),role_cycle.end());

    json state_index=json::array();
    json hash_entries=json::array();
    std::array<json,5> country_ai_refs;
    for (int npc_number=1;npc_number<=2500;++npc_number) {
        const std::string npc_id="ai_npc_"+padded(npc_number,3);
        const int country_number=(npc_number-1)/500+1;
        const std::string country_id="country_"+padded(country_number,3);
        json states=json::array();
        const auto fixture_found=fixture_ai.find(npc_id);
        const auto overlay_found=stage_e_ai.find(npc_id);
        for (int state_id=1;state_id<=255;++state_id) {
            if (fixture_found!=fixture_ai.end() && state_id<=5) states.push_back(fixture_found->second.at("states").at(static_cast<std::size_t>(state_id-1)));
            else if (overlay_found!=stage_e_ai.end() && state_id>=6 && state_id<=25)
                states.push_back(overlay_found->second.at("states").at(static_cast<std::size_t>(state_id-6)));
            else states.push_back(generated_state(npc_id,state_id));
        }
        const auto state_relative=std::filesystem::path("state_shards")/(npc_id+".json");
        const auto state_path=output_root/state_relative;
        json external_transition_rules=json::array();
        if (fixture_found!=fixture_ai.end()) {
            if (overlay_found!=stage_e_ai.end()) {
                external_transition_rules=overlay_found->second.at("legacy_entry_rules");
                external_transition_rules.push_back({{"rule_id","rule_stage_f_"+npc_id+"_025_to_026"},
                    {"source_state_id",25},{"trigger_event_type","STAGE_F_FIXTURE_SIGNAL"},
                    {"subject_event_type","TIME_ELAPSED"},{"perception","SCHEDULED"},
                    {"target_state_id",26},{"priority",100}});
            } else {
                for (int legacy_state_id=1;legacy_state_id<=5;++legacy_state_id) {
                    external_transition_rules.push_back({
                        {"rule_id","rule_stage_f_"+npc_id+"_legacy_"+std::to_string(legacy_state_id)},
                        {"source_state_id",legacy_state_id},{"trigger_event_type","STAGE_F_FIXTURE_SIGNAL"},
                        {"subject_event_type","TIME_ELAPSED"},{"perception","SCHEDULED"},
                        {"target_state_id",6},{"priority",100}});
                }
            }
        }
        write_json(state_path,{{"fixture_only",true},{"production_content",false},
                               {"schema_version","stage_f_state_shard_v1"},{"pack_version","stage-f-scale-0.1.0"},
                               {"npc_id",npc_id},{"country_id",country_id},
                               {"external_transition_rules",external_transition_rules},{"states",states}});
        const std::string state_hash=hash_file(state_path);

        json runtime;
        if (fixture_found!=fixture_ai.end()) {
            const auto& source=fixture_found->second;
            runtime={{"npc_id",npc_id},{"country_id",country_id},{"current_location_id",source.at("current_location_id")},
                     {"role",source.at("role")},{"faction_id",source.at("faction_id")},{"current_state_id",source.at("initial_state_id")},
                     {"player_evaluation",source.value("player_evaluation",0)},{"relationships_summary",source.value("relationships",json::object())},
                     {"current_goal",source.value("current_goal","")},{"known_event_summary",source.value("known_events",json::array())},
                     {"active_action",source.value("active_action","")}};
        } else {
            runtime={{"npc_id",npc_id},{"country_id",country_id},
                     {"current_location_id",country_id+"_scale_location_"+padded(static_cast<std::uint64_t>((npc_number-1)%20+1),2)},
                     {"role",role_cycle.at(static_cast<std::size_t>((npc_number-1)%role_cycle.size()))},
                     {"faction_id","stage_f_fixture_faction_"+country_id},{"current_state_id",1},
                     {"player_evaluation",0},{"relationships_summary",json::object()},
                     {"current_goal","stage_f_fixture_goal_"+npc_id},{"known_event_summary",json::array()},{"active_action",""}};
        }
        const bool active=npc_number<=20;
        const bool background=!active && ((npc_number-21)%5==0);
        runtime["activity_tier"]=active?"ACTIVE":(background?"BACKGROUND":"DORMANT");
        runtime["next_due_time"]=background?json((npc_number%360)+1):json(nullptr);
        runtime["status"]="ACTIVE"; runtime["version"]=1; runtime["due_action_count"]=0;
        const auto ai_relative=std::filesystem::path("ai_manifests")/(npc_id+".json");
        const auto ai_path=output_root/ai_relative;
        write_json(ai_path,{{"fixture_only",true},{"production_content",false},{"schema_version","stage_f_ai_manifest_v1"},
                            {"npc_id",npc_id},{"country_id",country_id},{"state_slot_count",255},
                            {"state_shard",state_relative.generic_string()},{"state_shard_sha256",state_hash},{"runtime",runtime}});
        const std::string ai_hash=hash_file(ai_path);
        country_ai_refs.at(static_cast<std::size_t>(country_number-1)).push_back({{"npc_id",npc_id},{"path",ai_relative.generic_string()},{"sha256",ai_hash}});
        state_index.push_back({{"npc_id",npc_id},{"country_id",country_id},{"state_shard",state_relative.generic_string()},
                               {"sha256",state_hash},{"state_slot_count",255}});
        hash_entries.push_back({{"path",state_relative.generic_string()},{"sha256",state_hash}});
        hash_entries.push_back({{"path",ai_relative.generic_string()},{"sha256",ai_hash}});
    }

    const auto index_path=output_root/"state_index.json";
    write_json(index_path,{{"fixture_only",true},{"production_content",false},{"schema_version","stage_f_state_index_v1"},
                           {"entry_count",2500},{"entries",state_index}});
    const std::string index_hash=hash_file(index_path);
    hash_entries.push_back({{"path","state_index.json"},{"sha256",index_hash}});

    write_json(output_root/"population_spaces.json",{{"fixture_only",true},{"production_content",false},
        {"schema_version","stage_f_population_spaces_v1"},{"world_seed",config.at("world_seed")},
        {"resident_population_spaces",json::array({
            {{"country_id","country_001"},{"population_class","RESIDENT"},{"slot_count",50000000}},
            {{"country_id","country_002"},{"population_class","RESIDENT"},{"slot_count",50000000}},
            {{"country_id","country_003"},{"population_class","RESIDENT"},{"slot_count",50000000}},
            {{"country_id","country_004"},{"population_class","RESIDENT"},{"slot_count",50000000}},
            {{"country_id","country_005"},{"population_class","RESIDENT"},{"slot_count",50000000}}})},
        {"mobile_population_space",{{"population_class","MOBILE"},{"slot_count",100000000}}}});
    const std::string population_hash=hash_file(output_root/"population_spaces.json");
    hash_entries.push_back({{"path","population_spaces.json"},{"sha256",population_hash}});

    json country_refs=json::array();
    for (int country_number=1;country_number<=5;++country_number) {
        const std::string country_id="country_"+padded(country_number,3);
        const auto relative=std::filesystem::path("countries")/(country_id+".json");
        const auto path=output_root/relative;
        write_json(path,{{"fixture_only",true},{"production_content",false},{"schema_version","stage_f_country_manifest_v1"},
                         {"country_id",country_id},{"ai_npc_count",500},
                         {"resident_population_slot_count",50000000},{"ai_manifests",country_ai_refs.at(static_cast<std::size_t>(country_number-1))}});
        const std::string hash=hash_file(path);
        country_refs.push_back({{"country_id",country_id},{"path",relative.generic_string()},{"sha256",hash}});
        hash_entries.push_back({{"path",relative.generic_string()},{"sha256",hash}});
    }
    std::sort(hash_entries.begin(),hash_entries.end(),[](const json& left,const json& right){return left.at("path")<right.at("path");});
    const auto hashes_path=output_root/"definition_hash_manifest.json";
    write_json(hashes_path,{{"fixture_only",true},{"production_content",false},{"schema_version","stage_f_definition_hash_manifest_v1"},
                            {"hash_algorithm","SHA-256"},{"entry_count",hash_entries.size()},{"entries",hash_entries}});
    const std::string hashes_hash=hash_file(hashes_path);

    const auto world_path=output_root/"world_manifest.json";
    const json counts{{"countries",5},{"ai_npcs_per_country",500},{"ai_npcs",2500},
                      {"state_slots_per_ai_npc",255},{"state_slots",637500},
                      {"resident_population_slots",250000000},{"mobile_population_slots",100000000}};
    write_json(world_path,{{"fixture_only",true},{"production_content",false},{"schema_version","stage_f_world_manifest_v1"},
                           {"pack_version","stage-f-scale-0.1.0"},{"simulation_version","stage-f-0.1.0"},
                           {"base_fixture_id",fixture.at("fixture_id")},{"stage_e_overlay_id",overlay.at("overlay_id")},
                           {"world_seed",config.at("world_seed")},{"counts",counts},{"country_manifests",country_refs},
                           {"state_index","state_index.json"},{"state_index_sha256",index_hash},
                           {"definition_hash_manifest","definition_hash_manifest.json"},{"definition_hash_manifest_sha256",hashes_hash},
                           {"population_spaces","population_spaces.json"},{"population_spaces_sha256",population_hash},
                           {"state_cache_capacity",config.at("state_cache_capacity")},
                           {"log_segment_entry_limit",config.at("log_segment_entry_limit")},
                           {"background_interval_game_minutes",config.at("background_interval_game_minutes")},
                           {"offline_limit_real_seconds",config.at("offline_limit_real_seconds")}});
    const std::string world_hash=hash_file(world_path);
    write_json(output_root/"expected_count_hash_contract.json",{{"fixture_only",true},{"production_content",false},
        {"contract_id","stage_f_generated_count_hash_contract_v1"},{"counts",counts},
        {"world_manifest_sha256",world_hash},{"definition_hash_manifest_sha256",hashes_hash},
        {"state_index_sha256",index_hash},{"population_spaces_sha256",population_hash}});
    return {world_hash,hashes_hash,index_hash,5,2500,637500};
}

std::vector<ValidationIssue> StageFDataPackValidator::validate(
    const std::filesystem::path& data_root, const std::filesystem::path& fixture_path,
    const std::filesystem::path& stage_e_overlay_path, bool scan_all_state_shards) {
    const json fixture=read_json(fixture_path); const json overlay=read_json(stage_e_overlay_path);
    std::vector<ValidationIssue> out;
    const auto world_path=data_root/"world_manifest.json";
    json world;
    try { world=read_json(world_path); }
    catch (const std::exception& exception) { issue(out,"ERROR","","",std::nullopt,"","PACK_SCHEMA_MISMATCH",exception.what(),world_path); return out; }
    if (world.value("schema_version","")!="stage_f_world_manifest_v1" || !world.value("fixture_only",false) || world.value("production_content",true))
        issue(out,"ERROR","","",std::nullopt,"","PACK_SCHEMA_MISMATCH","world manifest schema/boundary mismatch",world_path);
    const auto counts=world.value("counts",json::object());
    if (counts.value("countries",0)!=5) issue(out,"ERROR","","",std::nullopt,"","COUNTRY_COUNT_MISMATCH","country count must be 5",world_path);
    if (counts.value("ai_npcs",0)!=2500) issue(out,"ERROR","","",std::nullopt,"","AI_NPC_COUNT_MISMATCH","AI count must be 2500",world_path);
    if (counts.value("state_slots",0)!=637500) issue(out,"ERROR","","",std::nullopt,"","STATE_SLOT_COUNT_MISMATCH","state total must be 637500",world_path);
    if (world.value("base_fixture_id","")!=fixture.value("fixture_id","") || world.value("stage_e_overlay_id","")!=overlay.value("overlay_id",""))
        issue(out,"ERROR","","",std::nullopt,"","PACK_SCHEMA_MISMATCH","base fixture or Stage E overlay mismatch",world_path);
    const auto country_refs=world.value("country_manifests",json::array());
    if (country_refs.size()!=5) issue(out,"ERROR","","",std::nullopt,"","COUNTRY_COUNT_MISMATCH","country manifest reference count mismatch",world_path);

    std::map<std::string,json> fixture_ai; for (const auto& npc:fixture.at("ai_npcs")) fixture_ai[npc.at("npc_id")]=npc;
    std::map<std::string,json> stage_e_ai; for (const auto& npc:overlay.at("npcs")) stage_e_ai[npc.at("npc_id")]=npc;
    std::set<std::string> npc_ids; std::set<std::string> table_hashes;
    std::map<std::string,std::string> index_hashes;
    const auto index_path=data_root/world.value("state_index","state_index.json");
    try {
        if (hash_file(index_path)!=world.value("state_index_sha256","")) issue(out,"ERROR","","",std::nullopt,"","MANIFEST_HASH_MISMATCH","state index hash mismatch",index_path);
        const json index=read_json(index_path);
        for (const auto& entry:index.at("entries")) index_hashes[entry.at("npc_id")]=entry.at("sha256");
        if (index_hashes.size()!=2500) issue(out,"ERROR","","",std::nullopt,"","AI_NPC_COUNT_MISMATCH","state index entry count mismatch",index_path);
    } catch (const std::exception& exception) { issue(out,"ERROR","","",std::nullopt,"","INDEX_TARGET_MISSING",exception.what(),index_path); }

    for (std::size_t country_offset=0;country_offset<country_refs.size();++country_offset) {
        const auto& ref=country_refs[country_offset];
        const std::string expected_country="country_"+padded(country_offset+1,3);
        const auto country_path=data_root/ref.value("path","");
        if (!std::filesystem::exists(country_path)) { issue(out,"ERROR",expected_country,"",std::nullopt,"","INDEX_TARGET_MISSING","country manifest missing",country_path); continue; }
        if (hash_file(country_path)!=ref.value("sha256","")) issue(out,"ERROR",expected_country,"",std::nullopt,"","MANIFEST_HASH_MISMATCH","country manifest hash mismatch",country_path);
        const json country=read_json(country_path);
        if (country.value("country_id","")!=expected_country) issue(out,"ERROR",expected_country,"",std::nullopt,"","CROSS_COUNTRY_REFERENCE_INVALID","country manifest partition mismatch",country_path);
        if (country.value("ai_npc_count",0)!=500 || country.at("ai_manifests").size()!=500)
            issue(out,"ERROR",expected_country,"",std::nullopt,"","AI_NPC_COUNTRY_DISTRIBUTION_MISMATCH","country must contain 500 AI",country_path);
        for (const auto& ai_ref:country.value("ai_manifests",json::array())) {
            const std::string npc_id=ai_ref.value("npc_id","");
            if (!npc_ids.insert(npc_id).second) issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","NPC_ID_DUPLICATE","duplicate AI id",country_path);
            int numeric=0;
            try {
                if (npc_id.starts_with("ai_npc_") && npc_id.size()>7) numeric=std::stoi(npc_id.substr(7));
            } catch (const std::exception&) { numeric=0; }
            const int expected_number=static_cast<int>(country_offset)*500+(static_cast<int>(npc_ids.size())-static_cast<int>(country_offset)*500);
            (void)expected_number;
            if (numeric<(static_cast<int>(country_offset)*500+1) || numeric>(static_cast<int>(country_offset)+1)*500)
                issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","AI_NPC_COUNTRY_DISTRIBUTION_MISMATCH","AI id outside country allocation",country_path);
            const auto ai_path=data_root/ai_ref.value("path","");
            if (!std::filesystem::exists(ai_path)) { issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","LEGACY_NPC_MISSING","AI manifest missing",ai_path); continue; }
            if (hash_file(ai_path)!=ai_ref.value("sha256","")) issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","MANIFEST_HASH_MISMATCH","AI manifest hash mismatch",ai_path);
            const json ai_manifest=read_json(ai_path);
            if (ai_manifest.value("country_id","")!=expected_country || ai_manifest.at("runtime").value("country_id","")!=expected_country)
                issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","CROSS_COUNTRY_REFERENCE_INVALID","AI runtime country mismatch",ai_path);
            if (ai_manifest.value("state_slot_count",0)!=255) issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","STATE_SLOT_COUNT_MISMATCH","AI state slot count mismatch",ai_path);
            if (numeric>=1 && numeric<=20 && !fixture_ai.contains(npc_id)) issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","LEGACY_NPC_MISSING","legacy AI missing",ai_path);
            const auto state_path=data_root/ai_manifest.value("state_shard","");
            if (!std::filesystem::exists(state_path)) { issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","INDEX_TARGET_MISSING","state shard missing",state_path); continue; }
            const std::string actual_state_hash=hash_file(state_path);
            if (actual_state_hash!=ai_manifest.value("state_shard_sha256","") || !index_hashes.contains(npc_id) || actual_state_hash!=index_hashes[npc_id])
                issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","SHARD_HASH_MISMATCH","state shard hash mismatch",state_path);
            if (!scan_all_state_shards) continue;
            const json shard=read_json(state_path);
            auto shard_issues=validate_state_document(npc_id,expected_country,shard,fixture,overlay,state_path);
            out.insert(out.end(),shard_issues.begin(),shard_issues.end());
            const std::string table_hash=nation_sim::StageFProductionRuntime::sha256_bytes(shard.at("states").dump());
            if (!table_hashes.insert(table_hash).second) issue(out,"ERROR",expected_country,npc_id,std::nullopt,"","NPC_TABLE_IDENTICAL","state table duplicates another NPC",state_path);
            if (numeric>=1 && numeric<=20 && fixture_ai.contains(npc_id)) for (int state_id=1;state_id<=5;++state_id)
                if (shard.at("states").at(static_cast<std::size_t>(state_id-1))!=fixture_ai[npc_id].at("states").at(static_cast<std::size_t>(state_id-1)))
                    issue(out,"ERROR",expected_country,npc_id,state_id,"","LEGACY_NPC_MISSING","legacy state was not preserved",state_path);
            if (stage_e_ai.contains(npc_id)) for (int state_id=6;state_id<=25;++state_id)
                if (shard.at("states").at(static_cast<std::size_t>(state_id-1))!=stage_e_ai[npc_id].at("states").at(static_cast<std::size_t>(state_id-6)))
                    issue(out,"ERROR",expected_country,npc_id,state_id,"","STAGE_E_DEFINITION_OVERWRITTEN","Stage E state changed",state_path);
        }
    }
    if (npc_ids.size()!=2500) issue(out,"ERROR","","",std::nullopt,"","AI_NPC_COUNT_MISMATCH","unique AI count mismatch",world_path);
    const auto hashes_path=data_root/world.value("definition_hash_manifest","definition_hash_manifest.json");
    if (!std::filesystem::exists(hashes_path) || hash_file(hashes_path)!=world.value("definition_hash_manifest_sha256","")) {
        issue(out,"ERROR","","",std::nullopt,"","MANIFEST_HASH_MISMATCH","definition hash manifest mismatch",hashes_path);
    } else {
        try {
            const json hash_manifest=read_json(hashes_path);
            if (hash_manifest.value("entry_count",0)!=hash_manifest.at("entries").size())
                issue(out,"ERROR","","",std::nullopt,"","MANIFEST_HASH_MISMATCH","definition hash entry count mismatch",hashes_path);
            for (const auto& entry:hash_manifest.at("entries")) {
                const auto target=data_root/entry.value("path","");
                if (!std::filesystem::exists(target))
                    issue(out,"ERROR","","",std::nullopt,"","INDEX_TARGET_MISSING","definition hash target missing",target);
                else if (hash_file(target)!=entry.value("sha256",""))
                    issue(out,"ERROR","","",std::nullopt,"","MANIFEST_HASH_MISMATCH","definition hash target mismatch",target);
            }
        } catch (const std::exception& exception) {
            issue(out,"ERROR","","",std::nullopt,"","MANIFEST_HASH_MISMATCH",exception.what(),hashes_path);
        }
    }
    const auto population_path=data_root/world.value("population_spaces","population_spaces.json");
    try {
        if (hash_file(population_path)!=world.value("population_spaces_sha256",""))
            issue(out,"ERROR","","",std::nullopt,"","MANIFEST_HASH_MISMATCH","population-space hash mismatch",population_path);
        const json population=read_json(population_path);
        bool population_valid=population.value("schema_version","")=="stage_f_population_spaces_v1" &&
            population.value("resident_population_spaces",json::array()).size()==5 &&
            population.value("mobile_population_space",json::object()).value("slot_count",0ull)==100000000ull;
        for (const auto& resident:population.value("resident_population_spaces",json::array()))
            population_valid=population_valid&&resident.value("slot_count",0ull)==50000000ull;
        if (!population_valid) issue(out,"ERROR","","",std::nullopt,"","PACK_SCHEMA_MISMATCH","population-space count/schema mismatch",population_path);
    } catch (const std::exception& exception) {
        issue(out,"ERROR","","",std::nullopt,"","INDEX_TARGET_MISSING",exception.what(),population_path);
    }
    return out;
}

std::vector<ValidationIssue> StageFDataPackValidator::validate_state_shard_document(
    const std::string& npc_id,const std::string& expected_country_id,const std::string& shard_json,
    const std::filesystem::path& fixture_path,const std::filesystem::path& stage_e_overlay_path,
    const std::string& source_file) {
    return validate_state_document(npc_id,expected_country_id,json::parse(shard_json),
                                   read_json(fixture_path),read_json(stage_e_overlay_path),source_file);
}

std::string StageFDataPackValidator::json_lines(const std::vector<ValidationIssue>& issues) {
    std::ostringstream output;
    for (const auto& value:issues) output << json{{"severity",value.severity},{"country_id",value.country_id},
        {"npc_id",value.npc_id},{"state_id",value.state_id?json(*value.state_id):json(nullptr)},
        {"rule_id",value.rule_id.empty()?json(nullptr):json(value.rule_id)},{"error_code",value.error_code},
        {"message",value.message},{"source_file",value.source_file}}.dump() << '\n';
    return output.str();
}

int StageFDataPackValidator::error_count(const std::vector<ValidationIssue>& issues) {
    return static_cast<int>(std::count_if(issues.begin(),issues.end(),[](const ValidationIssue& value){return value.severity=="ERROR";}));
}

} // namespace nation_sim_stage_f
