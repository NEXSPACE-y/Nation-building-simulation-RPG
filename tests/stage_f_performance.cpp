#include "nation_sim/simulation.hpp"
#include "nation_sim/stage_f_runtime.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#endif

using json=nlohmann::json;
using Clock=std::chrono::steady_clock;
using nation_sim::StageFProductionRuntime;

namespace {
const std::filesystem::path scale_root=NATION_SIM_STAGE_F_DATA_ROOT;
const std::filesystem::path fixture_path=NATION_SIM_TEST_FIXTURE;
const std::filesystem::path overlay_path=NATION_SIM_STAGE_E_OVERLAY;

void require(bool value,const std::string& message) { if (!value) throw std::runtime_error(message); }

std::string ai_id(int number) {
    const std::string digits=std::to_string(number);
    return "ai_npc_"+std::string(digits.size()<3?3-digits.size():0,'0')+digits;
}

std::string read_text(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    if (!input) throw std::runtime_error("cannot read "+path.string());
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}

void write_json(const std::filesystem::path& path,const json& value) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path,std::ios::binary|std::ios::trunc);
    if (!output) throw std::runtime_error("cannot write "+path.string());
    output<<value.dump(2)<<'\n';
}

std::uint64_t working_set_bytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb=sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters)))
        return static_cast<std::uint64_t>(counters.WorkingSetSize);
#endif
    return 0;
}

std::uint64_t peak_working_set_bytes() {
#if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb=sizeof(counters);
    if (GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),sizeof(counters)))
        return static_cast<std::uint64_t>(counters.PeakWorkingSetSize);
#endif
    return 0;
}

std::string processor_identifier() {
#if defined(_WIN32)
    char* value=nullptr;
    std::size_t size=0;
    if (_dupenv_s(&value,&size,"PROCESSOR_IDENTIFIER")==0 && value) {
        const std::string result(value);
        std::free(value);
        return result;
    }
    std::free(value);
#endif
    return "unknown";
}

json environment(const StageFProductionRuntime& runtime) {
    std::uint64_t ram=0; int cores=1;
#if defined(_WIN32)
    MEMORYSTATUSEX memory{}; memory.dwLength=sizeof(memory); GlobalMemoryStatusEx(&memory); ram=memory.ullTotalPhys;
    SYSTEM_INFO system{}; GetSystemInfo(&system); cores=static_cast<int>(system.dwNumberOfProcessors);
#endif
    return {{"cpu",processor_identifier()},{"logical_core_count",cores},{"ram_bytes",ram},{"os","Windows"},
#if defined(_MSC_FULL_VER)
            {"compiler","MSVC " + std::to_string(_MSC_FULL_VER)},
#else
            {"compiler","unknown"},
#endif
            {"build_configuration","Release"},{"dataset_hash",runtime.scale_data_sha256()},{"worker_count",1}};
}

std::string visible_core(const std::filesystem::path& path) {
    auto simulation=nation_sim::Simulation::from_fixture_with_overlay(fixture_path,overlay_path);
    simulation.save(path);
    return read_text(path);
}

json run_standard(const std::filesystem::path& root) {
    std::filesystem::create_directories(root);
    const auto load_start=Clock::now();
    auto loaded=StageFProductionRuntime::load(scale_root,root/"event_logs_a");
    const auto load_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-load_start).count();
    const auto load_memory=working_set_bytes();
    require(loaded.counters().loaded_country_count==5&&loaded.counters().ai_npc_total_count==2500,"full scale load count");

    auto replay=StageFProductionRuntime::load(scale_root,root/"event_logs_b");
    const auto event_start=Clock::now();
    for (int index=0;index<10000;++index) {
        const int country_number=index%5+1;
        const std::string country_id="country_00"+std::to_string(country_number);
        const std::string actor_id=ai_id((country_number-1)*500+(index/5)%500+1);
        loaded.enqueue_country_event(country_id,actor_id,"load-event-"+std::to_string(index));
        replay.enqueue_country_event(country_id,actor_id,"load-event-"+std::to_string(index));
    }
    const auto processed_a=loaded.process_pending();
    const auto processed_b=replay.process_pending();
    const auto event_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-event_start).count();
    require(processed_a==10000&&processed_b==10000,"10,000 events processed");
    require(loaded.counters().pending_event_count==0&&replay.counters().pending_event_count==0,"event queues drained");
    require(loaded.canonical_sha256()==replay.canonical_sha256(),"10,000 event deterministic replay");

    auto offline_a=StageFProductionRuntime::load(scale_root);
    auto offline_b=StageFProductionRuntime::load(scale_root);
    const auto offline_start=Clock::now();
    const auto offline_result_a=offline_a.advance_offline(604800);
    const auto offline_result_b=offline_b.advance_offline(604800);
    const auto offline_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-offline_start).count();
    require(offline_result_a.applied_real_seconds==604800,"offline seven-day applied");
    require(offline_result_a.due_actions_processed>0&&offline_result_a.country_intervals_aggregated>0&&offline_result_a.population_intervals_aggregated>0,"offline aggregate work");
    require(offline_a.canonical_sha256()==offline_b.canonical_sha256(),"offline deterministic hash");

    const auto visible_path=root/"visible_stage_e.json";
    const std::string visible=visible_core(visible_path);
    auto cycling=StageFProductionRuntime::load(scale_root,root/"save_logs");
    const auto save_path=root/"save_cycle"/"stage_d_save.json";
    int save_load_mismatch=0;
    const auto cycle_start=Clock::now();
    for (int iteration=1;iteration<=100;++iteration) {
        const auto saved=cycling.save_generation(save_path,visible,1700000000+iteration);
        require(saved.success,"save cycle "+std::to_string(iteration)+": "+saved.error);
        const std::string expected=cycling.canonical_sha256();
        std::filesystem::path loaded_visible;
        nation_sim::StageFSaveResult load_result;
        auto restored=StageFProductionRuntime::load_generation(scale_root,save_path,loaded_visible,load_result,root/"save_logs");
        if (!load_result.success||restored.canonical_sha256()!=expected) ++save_load_mismatch;
        cycling=std::move(restored);
    }
    const auto cycle_ms=std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now()-cycle_start).count();
    require(save_load_mismatch==0,"100 save/load canonical mismatch");
    require(cycling.counters().current_save_generation==100,"100 save generations");

    const std::vector<std::string> injection_points={"BEFORE_SHARD_WRITE","DURING_SHARD_WRITE","BEFORE_MANIFEST_WRITE",
        "DURING_MANIFEST_WRITE","BEFORE_CURRENT_SWITCH","AFTER_SWITCH_READBACK_FAILURE"};
    json injection_results=json::array();
    for (const auto& point:injection_points) {
        const auto injection_root=root/"failure_injection"/point;
        auto runtime=StageFProductionRuntime::load(scale_root);
        const auto manifest=injection_root/"stage_d_save.json";
        const auto baseline=runtime.save_generation(manifest,visible,1700001000);
        require(baseline.success,"failure injection baseline");
        const std::string before=read_text(manifest);
        const auto failed=runtime.save_generation(manifest,visible,1700001001,point);
        require(!failed.success,"failure injection did not fail: "+point);
        require(read_text(manifest)==before,"failure injection changed current manifest: "+point);
        std::filesystem::path loaded_visible;
        nation_sim::StageFSaveResult recovered_result;
        auto recovered=StageFProductionRuntime::load_generation(scale_root,manifest,loaded_visible,recovered_result);
        require(recovered_result.success&&recovered.counters().current_save_generation==1,"failure injection recovery: "+point);
        injection_results.push_back({{"point",point},{"pass",true},{"audit_message",failed.audit_message},{"error",failed.error}});
    }

    return {{"environment",environment(loaded)},
        {"full_scale_load",{{"pass",true},{"duration_ms",load_ms},{"working_set_bytes",load_memory},
            {"country_count",5},{"ai_npc_count",2500},{"state_slot_count",637500},{"non_ai_population_slots",350000000}}},
        {"event_load",{{"pass",true},{"events",10000},{"processed",processed_a},{"duration_ms",event_ms},
            {"pending_after",loaded.counters().pending_event_count},{"canonical_sha256",loaded.canonical_sha256()}}},
        {"offline_seven_days",{{"pass",true},{"duration_ms",offline_ms},{"due_actions",offline_result_a.due_actions_processed},
            {"country_intervals",offline_result_a.country_intervals_aggregated},{"population_intervals",offline_result_a.population_intervals_aggregated},
            {"canonical_sha256",offline_a.canonical_sha256()}}},
        {"save_load_cycles",{{"pass",true},{"cycles",100},{"duration_ms",cycle_ms},{"hash_mismatch_count",save_load_mismatch},
            {"final_generation",cycling.counters().current_save_generation}}},
        {"failure_injection",{{"pass",true},{"results",injection_results}}},
        {"peak_working_set_bytes",peak_working_set_bytes()},{"pass",true}};
}

json run_soak(const std::filesystem::path& root,int duration_seconds) {
    std::filesystem::create_directories(root);
    const std::string visible=visible_core(root/"visible_stage_e.json");
    auto runtime=StageFProductionRuntime::load(scale_root,root/"logs");
    const auto start=Clock::now();
    auto next_save=start+std::chrono::seconds(60);
    auto next_report=start+std::chrono::seconds(60);
    std::uint64_t warmup_memory=0;
    const int warmup_seconds=std::max(60,duration_seconds/2);
    std::uint64_t maximum_memory=working_set_bytes();
    std::uint64_t events=0;
    std::uint64_t saves=0;
    const auto manifest=root/"stage_d_save.json";
    while (std::chrono::duration_cast<std::chrono::seconds>(Clock::now()-start).count()<duration_seconds) {
        const std::string country_id="country_00"+std::to_string(events%5+1);
        runtime.enqueue_country_event(country_id,ai_id(static_cast<int>((events%5)*500+1)),"soak-"+std::to_string(events));
        runtime.process_pending();
        ++events;
        const auto now=Clock::now();
        if (now>=next_save) {
            const auto saved=runtime.save_generation(manifest,visible,1700010000+static_cast<std::int64_t>(events));
            require(saved.success,"soak save failed: "+saved.error);
            std::filesystem::path loaded_visible;
            nation_sim::StageFSaveResult load_result;
            runtime=StageFProductionRuntime::load_generation(scale_root,manifest,loaded_visible,load_result,root/"logs");
            require(load_result.success,"soak load failed");
            ++saves;
            next_save+=std::chrono::seconds(60);
        }
        const auto memory=working_set_bytes();
        maximum_memory=std::max(maximum_memory,memory);
        if (!warmup_memory&&std::chrono::duration_cast<std::chrono::seconds>(now-start).count()>=warmup_seconds) warmup_memory=memory;
        if (now>=next_report) {
            std::cout<<"SOAK_PROGRESS seconds="<<std::chrono::duration_cast<std::chrono::seconds>(now-start).count()
                     <<" events="<<events<<" saves="<<saves<<" memory="<<memory<<std::endl;
            next_report+=std::chrono::seconds(60);
        }
        require(runtime.counters().pending_event_count==0,"soak queue growth");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    const auto end_memory=working_set_bytes();
    if (!warmup_memory) warmup_memory=end_memory;
    const double growth=warmup_memory?static_cast<double>(end_memory)/static_cast<double>(warmup_memory)-1.0:0.0;
    require(growth<=0.10,"soak memory growth exceeds 10 percent");
    return {{"duration_seconds",duration_seconds},{"warmup_seconds",warmup_seconds},{"events_processed",events},{"save_load_cycles",saves},
        {"warmup_working_set_bytes",warmup_memory},{"end_working_set_bytes",end_memory},{"maximum_working_set_bytes",maximum_memory},
        {"memory_growth_ratio",growth},{"pending_event_count",runtime.counters().pending_event_count},
        {"canonical_sha256",runtime.canonical_sha256()},{"crash",false},{"deadlock",false},{"pass",true}};
}
}

int main(int argc,char** argv) {
    try {
        if (argc<3) throw std::runtime_error("usage: stage_f_performance <standard|soak> <output-json> [soak-seconds]");
        const std::string mode=argv[1];
        const std::filesystem::path output=argv[2];
        const auto root=output.parent_path()/(mode+"-runtime");
        json result;
        if (mode=="standard") result=run_standard(root);
        else if (mode=="soak") {
            const int seconds=argc>=4?std::stoi(argv[3]):1800;
            result=run_soak(root,seconds);
        } else throw std::runtime_error("unknown mode: "+mode);
        write_json(output,result);
        std::cout<<"STAGE_F_PERFORMANCE_"<<(mode=="standard"?"STANDARD":"SOAK")<<" | PASS | "<<output.string()<<'\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr<<"STAGE_F_PERFORMANCE | FAIL | "<<exception.what()<<'\n';
        return 1;
    }
}
