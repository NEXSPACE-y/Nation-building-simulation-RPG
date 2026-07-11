#include "StageFDataPack.hpp"

#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: stage_f_validator <data-root> <fixture> <stage-e-overlay> <output-jsonl>\n";
        return 2;
    }
    try {
        const auto issues=nation_sim_stage_f::StageFDataPackValidator::validate(argv[1],argv[2],argv[3],true);
        std::ofstream output(argv[4],std::ios::binary|std::ios::trunc);
        output << nation_sim_stage_f::StageFDataPackValidator::json_lines(issues);
        const int errors=nation_sim_stage_f::StageFDataPackValidator::error_count(issues);
        std::cout << "STAGE_F_VALIDATION | errors=" << errors << " issues=" << issues.size() << '\n';
        return errors==0?0:1;
    } catch (const std::exception& exception) {
        std::cerr << "STAGE_F_VALIDATION_FAILED | " << exception.what() << '\n';
        return 1;
    }
}
