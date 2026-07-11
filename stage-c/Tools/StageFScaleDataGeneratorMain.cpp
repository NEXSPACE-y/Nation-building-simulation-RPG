#include "StageFDataPack.hpp"

#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: stage_f_scale_data_generator <config> <fixture> <stage-e-overlay> <output-root>\n";
        return 2;
    }
    try {
        const auto result=nation_sim_stage_f::StageFScaleDataGenerator::generate(argv[1],argv[2],argv[3],argv[4]);
        std::cout << "STAGE_F_SCALE_DATA_GENERATED | countries=" << result.country_count
                  << " ai_npcs=" << result.ai_npc_count << " state_slots=" << result.state_slot_count
                  << " world_sha256=" << result.world_manifest_sha256 << '\n';
        return 0;
    } catch (const std::exception& exception) {
        std::cerr << "STAGE_F_SCALE_DATA_GENERATION_FAILED | " << exception.what() << '\n';
        return 1;
    }
}
