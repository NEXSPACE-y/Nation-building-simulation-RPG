#include "nation_sim/simulation.hpp"
#include "nation_sim/stage_f_runtime.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;
using nation_sim::StageFProductionRuntime;

namespace {
const std::filesystem::path scale_root=NATION_SIM_STAGE_F_DATA_ROOT;
const std::filesystem::path fixture_path=NATION_SIM_TEST_FIXTURE;
const std::filesystem::path overlay_path=NATION_SIM_STAGE_E_OVERLAY;
const std::filesystem::path temp_root=NATION_SIM_STAGE_F_TEST_TEMP_DIR;
const std::filesystem::path evidence_root=NATION_SIM_STAGE_F_EVIDENCE_DIR;
json evidence=json::object();

std::string ai_id(int number) {
    const std::string digits=std::to_string(number);
    return "ai_npc_"+std::string(digits.size()<3?3-digits.size():0,'0')+digits;
}

void require(bool condition,const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    if (!input) throw std::runtime_error("cannot read "+path.string());
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}

void write_text(const std::filesystem::path& path,const std::string& value) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path,std::ios::binary|std::ios::trunc);
    if (!output) throw std::runtime_error("cannot write "+path.string());
    output.write(value.data(),static_cast<std::streamsize>(value.size()));
}

json canonical(const StageFProductionRuntime& runtime) {
    return json::parse(runtime.canonical_snapshot());
}

std::string create_stage_e_save(const std::filesystem::path& path) {
    auto simulation=nation_sim::Simulation::from_fixture_with_overlay(fixture_path,overlay_path);
    simulation.enqueue_player_action("HARM","non_ai_npc_004");
    simulation.process_pending(3);
    simulation.save(path);
    return simulation.canonical_snapshot();
}

const json& country(const json& snapshot,const std::string& id) {
    for (const auto& value:snapshot.at("countries")) if (value.at("country_id")==id) return value;
    throw std::runtime_error("country missing: "+id);
}

void scenario_f1() {
    auto runtime=StageFProductionRuntime::load(scale_root);
    const auto& counters=runtime.counters();
    require(counters.loaded_country_count==5,"country count");
    require(counters.ai_npc_total_count==2500,"AI count");
    require(counters.active_count+counters.background_count+counters.dormant_count==2500,"tier count");
    const json world=json::parse(read_text(scale_root/"world_manifest.json"));
    require(world.at("counts").at("state_slots")==637500,"state slot count");
    require(world.at("counts").at("resident_population_slots")==250000000,"resident population count");
    require(world.at("counts").at("mobile_population_slots")==100000000,"mobile population count");
    for (const std::string id:{"ai_npc_001","ai_npc_500","ai_npc_501","ai_npc_1001","ai_npc_2500"}) {
        require(json::parse(runtime.state_definition_json(id,1)).at("state_id")==1,"state 1 lookup "+id);
        require(json::parse(runtime.state_definition_json(id,255)).at("state_id")==255,"state 255 lookup "+id);
    }
    require(runtime.counters().state_cache_size<=32,"bounded state cache");
    evidence["scale_contract"]={{"countries",5},{"ai_npcs_per_country",500},{"ai_npcs",2500},
        {"state_slots_per_ai_npc",255},{"state_slots",637500},{"resident_population_slots",250000000},
        {"mobile_population_slots",100000000},{"dataset_sha256",runtime.scale_data_sha256()},{"pass",true}};
}

void scenario_f2() {
    const auto root=temp_root/"scenario_f2";
    std::filesystem::create_directories(root);
    const auto save=root/"stage_d_save.json";
    const auto metadata=root/"stage_d_save.json.utc";
    const std::string before_snapshot=create_stage_e_save(save);
    write_text(metadata,"{\"saved_at_utc_epoch\":1700000000}\n");
    const std::string source_save=read_text(save);
    const std::string source_metadata=read_text(metadata);
    const auto migrated=StageFProductionRuntime::migrate_stage_e_save(scale_root,save,metadata,1700000000,root/"logs");
    require(migrated.success&&migrated.migrated,"Stage E migration");
    require(std::filesystem::exists(migrated.save_backup_path),"save backup exists");
    require(std::filesystem::exists(migrated.metadata_backup_path),"metadata backup exists");
    require(read_text(migrated.save_backup_path)==source_save,"save backup immutable bytes");
    require(read_text(migrated.metadata_backup_path)==source_metadata,"metadata backup immutable bytes");
    std::filesystem::path visible;
    nation_sim::StageFSaveResult load_result;
    auto runtime=StageFProductionRuntime::load_generation(scale_root,save,visible,load_result,root/"logs");
    (void)runtime;
    require(load_result.success&&!load_result.recovered_previous_generation,"migrated generation load");
    const auto reloaded=nation_sim::Simulation::load_save_with_overlay(fixture_path,overlay_path,visible);
    require(reloaded.canonical_snapshot()==before_snapshot,"visible Stage E causal state preserved");
    const auto second=StageFProductionRuntime::migrate_stage_e_save(scale_root,save,metadata,1700000000,root/"logs");
    require(second.success&&!second.migrated,"double migration suppressed");
    require(read_text(migrated.save_backup_path)==source_save,"existing save backup not overwritten");
    const auto failure_root=root/"failure";
    const auto failure_save=failure_root/"stage_d_save.json";
    const auto failure_metadata=failure_root/"stage_d_save.json.utc";
    create_stage_e_save(failure_save);
    write_text(failure_metadata,"{\"saved_at_utc_epoch\":1700000000}\n");
    const std::string failure_save_before=read_text(failure_save);
    const std::string failure_metadata_before=read_text(failure_metadata);
    const auto failed=StageFProductionRuntime::migrate_stage_e_save(
        root/"missing-scale-data",failure_save,failure_metadata,1700000000,failure_root/"logs");
    require(!failed.success,"migration failure injection unexpectedly succeeded");
    require(read_text(failure_save)==failure_save_before&&read_text(failure_metadata)==failure_metadata_before,
        "failed migration changed source save or metadata");
    evidence["migration"]={{"source_save_sha256",migrated.source_save_sha256},
        {"source_metadata_sha256",migrated.source_metadata_sha256},{"save_backup_path",migrated.save_backup_path.generic_string()},
        {"metadata_backup_path",migrated.metadata_backup_path.generic_string()},{"visible_core_canonical_preserved",true},
        {"double_migration_suppressed",true},{"failure_preserved_source_bytes",true}};
}

void scenario_f3() {
    const auto root=temp_root/"scenario_f3";
    std::filesystem::create_directories(root);
    auto runtime=StageFProductionRuntime::load(scale_root,root/"logs");
    const std::string first_root=runtime.enqueue_country_event("country_001","ai_npc_001","local-only-0");
    for (int index=1;index<=1000;++index)
        runtime.enqueue_country_event("country_001","ai_npc_001","local-only-"+std::to_string(index));
    require(runtime.process_pending()==1001,"local events processed");
    const json snapshot=canonical(runtime);
    require(country(snapshot,"country_001").at("local_events")==1001,"country 1 receives local events");
    for (int index=2;index<=5;++index)
        require(country(snapshot,"country_00"+std::to_string(index)).at("local_events")==0,"local event isolated");
    require(runtime.archived_causal_path(first_root).size()==1,"segmented causal path lookup");
    const auto visible_path=root/"visible_stage_e.json";
    create_stage_e_save(visible_path);
    const auto saved=runtime.save_generation(root/"stage_d_save.json",read_text(visible_path),1700000050);
    require(saved.success,"segmented log save");
    const std::string expected_hash=runtime.canonical_sha256();
    std::filesystem::path visible;
    nation_sim::StageFSaveResult load_result;
    auto reloaded=StageFProductionRuntime::load_generation(scale_root,root/"stage_d_save.json",visible,load_result,root/"logs");
    require(reloaded.canonical_sha256()==expected_hash,"segmented log canonical restore");
    require(reloaded.archived_causal_path(first_root).size()==1,"segmented causal path restore");
    bool mismatched_actor_rejected=false;
    try { reloaded.enqueue_country_event("country_002","ai_npc_001","invalid-partition"); }
    catch (const std::exception&) { mismatched_actor_rejected=true; }
    require(mismatched_actor_rejected,"mismatched AI actor partition accepted");
    evidence["country_isolation"]={{"source_country","country_001"},{"local_events",1001},
        {"other_country_changes",0},{"segmented_causal_path_restored",true},
        {"mismatched_actor_rejected",true},{"pass",true}};
}

void scenario_f4() {
    auto runtime=StageFProductionRuntime::load(scale_root);
    runtime.enqueue_cross_country_event("country_001","country_004","ai_npc_001","explicit-cross");
    require(runtime.process_pending()==1,"cross-country event processed");
    const json snapshot=canonical(runtime);
    for (int index=1;index<=5;++index) {
        const std::string id="country_00"+std::to_string(index);
        require(country(snapshot,id).at("cross_country_events_received")==(index==4?1:0),"cross event exact target");
    }
    evidence["cross_country"]={{"source","country_001"},{"target","country_004"},{"received_only_by_target",true}};
}

void scenario_f5() {
    auto tiered=StageFProductionRuntime::load(scale_root);
    auto reference=StageFProductionRuntime::load(scale_root);
    reference.set_all_active_reference_mode(true);
    for (int index=0;index<100;++index) {
        const std::string country_id="country_00"+std::to_string(index%5+1);
        const std::string actor_id=ai_id((index%5)*500+1);
        tiered.enqueue_country_event(country_id,actor_id,std::to_string(index));
        reference.enqueue_country_event(country_id,actor_id,std::to_string(index));
    }
    tiered.advance_offline(86400);
    reference.advance_offline(86400);
    require(tiered.canonical_sha256()==reference.canonical_sha256(),"tiered/reference canonical hash");
    evidence["tier_determinism"]={{"tiered_sha256",tiered.canonical_sha256()},
        {"all_active_reference_sha256",reference.canonical_sha256()},{"match",true}};
}

void scenario_f6() {
    auto runtime=StageFProductionRuntime::load(scale_root);
    const auto first=runtime.materialize_non_ai("country_003","RESIDENT",424242);
    runtime.dematerialize_non_ai(first.npc_id);
    const auto regenerated=runtime.materialize_non_ai("country_003","RESIDENT",424242);
    const auto other=runtime.materialize_non_ai("country_003","RESIDENT",424243);
    require(first.npc_id==regenerated.npc_id&&first.deterministic_seed==regenerated.deterministic_seed,"NON AI deterministic regeneration");
    require(first.npc_id!=other.npc_id&&first.deterministic_seed!=other.deterministic_seed,"NON AI index separation");
    require(runtime.counters().materialized_non_ai_count<1000,"NON AI population not fully materialized");
    evidence["non_ai_regeneration"]={{"npc_id",first.npc_id},{"seed",first.deterministic_seed},
        {"same_index_match",true},{"different_index_differs",true},{"materialized_count",runtime.counters().materialized_non_ai_count}};
}

void scenario_f7() {
    const auto root=temp_root/"scenario_f7";
    std::filesystem::create_directories(root);
    const auto visible_path=root/"visible_stage_e.json";
    create_stage_e_save(visible_path);
    auto runtime=StageFProductionRuntime::load(scale_root,root/"logs");
    const auto npc=runtime.materialize_non_ai("country_002","RESIDENT",77);
    runtime.promote_non_ai(npc.npc_id,"important_evt_001");
    runtime.dematerialize_non_ai(npc.npc_id);
    const auto saved=runtime.save_generation(root/"stage_d_save.json",read_text(visible_path),1700000100);
    require(saved.success,"promoted save");
    std::filesystem::path visible;
    nation_sim::StageFSaveResult loaded_result;
    auto loaded=StageFProductionRuntime::load_generation(scale_root,root/"stage_d_save.json",visible,loaded_result,root/"logs");
    const auto restored=loaded.non_ai(npc.npc_id);
    require(restored&&restored->promoted,"promoted identity restored");
    require(restored->important_event_ids==std::vector<std::string>{"important_evt_001"},"promoted memory restored");
    evidence["non_ai_promotion"]={{"npc_id",npc.npc_id},{"event_id","important_evt_001"},{"save_load_preserved",true}};
}

void scenario_f8() {
    const auto root=temp_root/"scenario_f8";
    std::filesystem::create_directories(root);
    const auto visible_path=root/"visible_stage_e.json";
    create_stage_e_save(visible_path);
    auto initial=StageFProductionRuntime::load(scale_root);
    require(initial.save_generation(root/"stage_d_save.json",read_text(visible_path),1700000200).success,"offline baseline save");
    std::filesystem::path visible_a,visible_b;
    nation_sim::StageFSaveResult result_a,result_b;
    auto continuous=StageFProductionRuntime::load_generation(scale_root,root/"stage_d_save.json",visible_a,result_a);
    auto resumed=StageFProductionRuntime::load_generation(scale_root,root/"stage_d_save.json",visible_b,result_b);
    const auto offline_a=continuous.advance_offline(604800+9999);
    const auto offline_b=resumed.advance_offline(604800+9999);
    require(offline_a.applied_real_seconds==604800&&offline_b.applied_real_seconds==604800,"seven-day cap");
    require(continuous.canonical_sha256()==resumed.canonical_sha256(),"offline continuous/resumed hash");
    evidence["offline"]={{"requested_real_seconds",614799},{"applied_real_seconds",offline_a.applied_real_seconds},
        {"continuous_sha256",continuous.canonical_sha256()},{"resumed_sha256",resumed.canonical_sha256()},{"match",true}};
}

void scenario_f9() {
    const auto root=temp_root/"scenario_f9";
    std::filesystem::create_directories(root);
    const auto visible_path=root/"visible_stage_e.json";
    create_stage_e_save(visible_path);
    auto runtime=StageFProductionRuntime::load(scale_root);
    require(runtime.save_generation(root/"stage_d_save.json",read_text(visible_path),1700000300).success,"generation 1 save");
    runtime.enqueue_country_event("country_001","ai_npc_001","generation-2");
    runtime.process_pending();
    const auto second=runtime.save_generation(root/"stage_d_save.json",read_text(visible_path),1700000400);
    require(second.success,"generation 2 save");
    write_text(root/"stage_f_generations"/second.generation_id/"countries"/"country_001.json","{corrupt\n");
    std::filesystem::path visible;
    nation_sim::StageFSaveResult recovered_result;
    auto recovered=StageFProductionRuntime::load_generation(scale_root,root/"stage_d_save.json",visible,recovered_result);
    require(recovered_result.success&&recovered_result.recovered_previous_generation,"previous generation recovery");
    require(recovered_result.generation_id!=second.generation_id,"corrupt current generation rejected");
    require(recovered_result.audit_message.find("STAGE_F_RECOVERED_PREVIOUS_GENERATION")!=std::string::npos,"recovery audit marker");
    require(canonical(recovered).at("save_generation")==1,"generation 1 restored");
    const auto retry_root=root/"after_switch_retry";
    auto retry_runtime=StageFProductionRuntime::load(scale_root);
    const auto retry_manifest=retry_root/"stage_d_save.json";
    require(retry_runtime.save_generation(retry_manifest,read_text(visible_path),1700000500).success,"retry baseline save");
    const auto injected=retry_runtime.save_generation(
        retry_manifest,read_text(visible_path),1700000501,"AFTER_SWITCH_READBACK_FAILURE");
    require(!injected.success,"after-switch failure injection succeeded");
    const auto retry=retry_runtime.save_generation(retry_manifest,read_text(visible_path),1700000502);
    require(retry.success,"save could not continue after after-switch rollback");
    evidence["save_recovery"]={{"corrupt_generation",second.generation_id},
        {"recovered_generation",recovered_result.generation_id},{"audit_message",recovered_result.audit_message},
        {"after_switch_retry_generation",retry.generation_id},{"pass",true}};
}
}

int main() {
    try {
        const auto absolute_temp=std::filesystem::weakly_canonical(temp_root.parent_path())/temp_root.filename();
        require(absolute_temp.string().find("stage-f")!=std::string::npos,"unsafe Stage F test temp path");
        std::filesystem::remove_all(absolute_temp);
        std::filesystem::create_directories(absolute_temp);
        const std::vector<std::pair<std::string,std::function<void()>>> tests={
            {"Scenario F-1 scale contract",scenario_f1},{"Scenario F-2 Stage E preservation",scenario_f2},
            {"Scenario F-3 country isolation",scenario_f3},{"Scenario F-4 explicit cross-country routing",scenario_f4},
            {"Scenario F-5 activity tier equality",scenario_f5},{"Scenario F-6 NON AI regeneration",scenario_f6},
            {"Scenario F-7 NON AI promotion persistence",scenario_f7},{"Scenario F-8 seven-day offline determinism",scenario_f8},
            {"Scenario F-9 corrupt shard recovery",scenario_f9}};
        int passed=0;
        for (const auto& [name,test]:tests) {
            try { test(); ++passed; std::cout<<"[PASS] "<<name<<'\n'; }
            catch (const std::exception& exception) { std::cerr<<"[FAIL] "<<name<<": "<<exception.what()<<'\n'; }
        }
        std::filesystem::create_directories(evidence_root);
        evidence["summary"]={{"passed",passed},{"failed",static_cast<int>(tests.size())-passed},{"expected",tests.size()}};
        write_text(evidence_root/"stage_f_scenario_evidence.json",evidence.dump(2)+"\n");
        write_text(evidence_root/"stage_f_scale_contract.json",evidence.at("scale_contract").dump(2)+"\n");
        write_text(evidence_root/"stage_f_determinism_evidence.json",json{{"tier",evidence.at("tier_determinism")},
            {"offline",evidence.at("offline")}}.dump(2)+"\n");
        write_text(evidence_root/"stage_f_offline_evidence.json",evidence.at("offline").dump(2)+"\n");
        write_text(evidence_root/"stage_f_save_recovery_evidence.json",json{{"migration",evidence.at("migration")},
            {"recovery",evidence.at("save_recovery")}}.dump(2)+"\n");
        write_text(evidence_root/"stage_f_non_ai_population_evidence.json",json{{"regeneration",evidence.at("non_ai_regeneration")},
            {"promotion",evidence.at("non_ai_promotion")}}.dump(2)+"\n");
        std::cout<<"Stage F core scenarios: "<<passed<<'/'<<tests.size()<<" passed\n";
        return passed==static_cast<int>(tests.size())?0:1;
    } catch (const std::exception& exception) {
        std::cerr<<"[FATAL] "<<exception.what()<<'\n';
        return 1;
    }
}
