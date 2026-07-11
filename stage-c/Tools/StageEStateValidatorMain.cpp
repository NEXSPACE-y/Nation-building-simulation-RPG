#include "StageEStateValidator.hpp"

#include <fstream>
#include <algorithm>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: stage_e_state_validator <overlay.json> <fixture.json> <output.jsonl>\n";
        return 2;
    }
    try {
        const auto issues = nation_sim_stage_e::StateDefinitionValidator::validate(argv[1], argv[2]);
        std::ofstream output(argv[3], std::ios::binary | std::ios::trunc);
        if (!output) throw std::runtime_error("cannot create validator output");
        output << nation_sim_stage_e::StateDefinitionValidator::json_lines(issues);
        const int errors = nation_sim_stage_e::StateDefinitionValidator::error_count(issues);
        const int warnings = static_cast<int>(std::count_if(issues.begin(), issues.end(), [](const auto& issue){return issue.severity=="WARNING";}));
        const int info = static_cast<int>(issues.size()) - errors - warnings;
        std::cout << "STAGE_E_STATE_VALIDATION | errors=" << errors << " warnings=" << warnings << " info=" << info << '\n';
        return errors == 0 ? 0 : 1;
    } catch (const std::exception& exception) {
        std::cerr << "STAGE_E_STATE_VALIDATION | fatal=" << exception.what() << '\n';
        return 2;
    }
}
