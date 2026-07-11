cmake_minimum_required(VERSION 3.25)

project(NationSimulationStageA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)
set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(nlohmann_json)

add_library(nation_sim
    src/simulation.cpp
    src/stage_f_runtime.cpp
)
target_include_directories(nation_sim PUBLIC include)
target_link_libraries(nation_sim PUBLIC nlohmann_json::nlohmann_json)

if(MSVC)
    target_compile_options(nation_sim PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(nation_sim PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(nation_cli src/main.cpp)
target_link_libraries(nation_cli PRIVATE nation_sim)
target_compile_definitions(nation_cli PRIVATE
    NATION_SIM_DEFAULT_FIXTURE="${CMAKE_SOURCE_DIR}/data/stage_a_fixture.json"
)

add_executable(stage_a_fixture_generator tools/generate_stage_a_fixture.cpp)

add_executable(stage_e_state_validator
    stage-c/Tools/StageEStateValidatorMain.cpp
    stage-c/Tools/StageEStateValidator.cpp
)
target_include_directories(stage_e_state_validator PRIVATE stage-c/Tools)
target_link_libraries(stage_e_state_validator PRIVATE nlohmann_json::nlohmann_json)
target_compile_features(stage_e_state_validator PRIVATE cxx_std_23)

add_executable(stage_f_scale_data_generator
    stage-c/Tools/StageFScaleDataGeneratorMain.cpp
    stage-c/Tools/StageFDataPack.cpp
)
target_include_directories(stage_f_scale_data_generator PRIVATE stage-c/Tools)
target_link_libraries(stage_f_scale_data_generator PRIVATE nation_sim nlohmann_json::nlohmann_json)
target_compile_features(stage_f_scale_data_generator PRIVATE cxx_std_23)

add_executable(stage_f_validator
    stage-c/Tools/StageFValidatorMain.cpp
    stage-c/Tools/StageFDataPack.cpp
)
target_include_directories(stage_f_validator PRIVATE stage-c/Tools)
target_link_libraries(stage_f_validator PRIVATE nation_sim nlohmann_json::nlohmann_json)
target_compile_features(stage_f_validator PRIVATE cxx_std_23)

if(MSVC)
    target_compile_options(stage_a_fixture_generator PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(stage_f_scale_data_generator PRIVATE /W4 /permissive- /utf-8)
    target_compile_options(stage_f_validator PRIVATE /W4 /permissive- /utf-8)
endif()

enable_testing()
add_executable(nation_tests tests/test_main.cpp)
target_link_libraries(nation_tests PRIVATE nation_sim)
target_compile_definitions(nation_tests PRIVATE
    NATION_SIM_TEST_FIXTURE="${CMAKE_SOURCE_DIR}/data/stage_a_fixture.json"
    NATION_SIM_PARITY_CONTRACT="${CMAKE_SOURCE_DIR}/data/stage_parity_contract.json"
    NATION_SIM_TEST_TEMP_DIR="${CMAKE_BINARY_DIR}/test-output"
)
add_test(NAME stage_a_acceptance COMMAND nation_tests)

add_executable(stage_f_acceptance tests/stage_f_acceptance.cpp)
target_link_libraries(stage_f_acceptance PRIVATE nation_sim nlohmann_json::nlohmann_json)
target_compile_definitions(stage_f_acceptance PRIVATE
    NATION_SIM_TEST_FIXTURE="${CMAKE_SOURCE_DIR}/data/stage_a_fixture.json"
    NATION_SIM_STAGE_E_OVERLAY="${CMAKE_SOURCE_DIR}/stage-c/Data/StageE/stage_e_state_definitions.json"
    NATION_SIM_STAGE_F_DATA_ROOT="${CMAKE_SOURCE_DIR}/stage-c/Data/StageF/Generated"
    NATION_SIM_STAGE_F_TEST_TEMP_DIR="${CMAKE_BINARY_DIR}/stage-f-test-output"
    NATION_SIM_STAGE_F_EVIDENCE_DIR="${CMAKE_SOURCE_DIR}/out/stage-f/test-output"
)
target_compile_features(stage_f_acceptance PRIVATE cxx_std_23)
if(MSVC)
    target_compile_options(stage_f_acceptance PRIVATE /W4 /permissive- /utf-8)
endif()
add_test(NAME stage_f_core_acceptance COMMAND stage_f_acceptance)

add_executable(stage_f_validator_negative
    tests/stage_f_validator_negative.cpp
    stage-c/Tools/StageFDataPack.cpp
)
target_include_directories(stage_f_validator_negative PRIVATE stage-c/Tools)
target_link_libraries(stage_f_validator_negative PRIVATE nation_sim nlohmann_json::nlohmann_json)
target_compile_definitions(stage_f_validator_negative PRIVATE
    NATION_SIM_TEST_FIXTURE="${CMAKE_SOURCE_DIR}/data/stage_a_fixture.json"
    NATION_SIM_STAGE_E_OVERLAY="${CMAKE_SOURCE_DIR}/stage-c/Data/StageE/stage_e_state_definitions.json"
    NATION_SIM_STAGE_F_DATA_ROOT="${CMAKE_SOURCE_DIR}/stage-c/Data/StageF/Generated"
    NATION_SIM_STAGE_F_NEGATIVE_OUTPUT_DIR="${CMAKE_SOURCE_DIR}/out/stage-f/validation-negative"
)
target_compile_features(stage_f_validator_negative PRIVATE cxx_std_23)
if(MSVC)
    target_compile_options(stage_f_validator_negative PRIVATE /W4 /permissive- /utf-8)
endif()
add_test(NAME stage_f_validator_negative_acceptance COMMAND stage_f_validator_negative)

add_executable(stage_f_performance tests/stage_f_performance.cpp)
target_link_libraries(stage_f_performance PRIVATE nation_sim nlohmann_json::nlohmann_json)
if(WIN32)
    target_link_libraries(stage_f_performance PRIVATE Psapi)
endif()
target_compile_definitions(stage_f_performance PRIVATE
    NATION_SIM_TEST_FIXTURE="${CMAKE_SOURCE_DIR}/data/stage_a_fixture.json"
    NATION_SIM_STAGE_E_OVERLAY="${CMAKE_SOURCE_DIR}/stage-c/Data/StageE/stage_e_state_definitions.json"
    NATION_SIM_STAGE_F_DATA_ROOT="${CMAKE_SOURCE_DIR}/stage-c/Data/StageF/Generated"
)
target_compile_features(stage_f_performance PRIVATE cxx_std_23)
if(MSVC)
    target_compile_options(stage_f_performance PRIVATE /W4 /permissive- /utf-8)
endif()
