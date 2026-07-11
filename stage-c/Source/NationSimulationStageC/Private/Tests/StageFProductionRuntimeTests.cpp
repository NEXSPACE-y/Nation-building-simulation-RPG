#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDPlayerCharacter.h"
#include "nation_sim/simulation.hpp"
#include "nation_sim/stage_f_runtime.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using json=nlohmann::json;

void RequireStageF(bool Condition,const std::string& Message)
{
    if (!Condition) throw std::runtime_error(Message);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStageFProductionRuntimeTest,
    "NationSimulation.StageF.ProductionRuntime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageFProductionRuntimeTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FString RepositoryRoot=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(),TEXT("..")));
    const std::filesystem::path Fixture(*FPaths::Combine(RepositoryRoot,TEXT("data/stage_a_fixture.json")));
    const std::filesystem::path Overlay(*FPaths::Combine(FPaths::ProjectDir(),TEXT("Data/StageE/stage_e_state_definitions.json")));
    const std::filesystem::path ScaleRoot(*FPaths::Combine(FPaths::ProjectDir(),TEXT("Data/StageF/Generated")));
    const std::filesystem::path Output(*FPaths::Combine(RepositoryRoot,TEXT("out/stage-f/test-output")));
    std::filesystem::create_directories(Output);
    int Passed=0;
    int Failed=0;
    std::vector<std::string> Results;
    json Evidence;
    const auto Run=[&](const char* Name,const std::function<void()>& Body)
    {
        try
        {
            Body(); ++Passed; Results.push_back(std::string("PASS | ")+Name);
            AddInfo(FString::Printf(TEXT("PASS | %s"),UTF8_TO_TCHAR(Name)));
        }
        catch (const std::exception& Exception)
        {
            ++Failed; Results.push_back(std::string("FAIL | ")+Name+" | "+Exception.what());
            AddError(FString::Printf(TEXT("FAIL | %s | %s"),UTF8_TO_TCHAR(Name),UTF8_TO_TCHAR(Exception.what())));
        }
    };

    Run("full-scale core loads five countries and 2500 AI without Actors",[&]
    {
        auto Runtime=nation_sim::StageFProductionRuntime::load(ScaleRoot);
        const auto& Counters=Runtime.counters();
        RequireStageF(Counters.loaded_country_count==5,"country count mismatch");
        RequireStageF(Counters.ai_npc_total_count==2500,"AI count mismatch");
        RequireStageF(Counters.active_count+Counters.background_count+Counters.dormant_count==2500,"tier count mismatch");
        Evidence["runtime_counts"]={{"countries",Counters.loaded_country_count},{"ai_npcs",Counters.ai_npc_total_count},
            {"active",Counters.active_count},{"background",Counters.background_count},{"dormant",Counters.dormant_count}};
    });

    Run("state shard loading is on demand and cache bounded",[&]
    {
        auto Runtime=nation_sim::StageFProductionRuntime::load(ScaleRoot);
        RequireStageF(Runtime.counters().loaded_state_shard_count==0,"state shard eagerly loaded");
        for (int Index=1;Index<=40;++Index)
            Runtime.state_definition_json("ai_npc_"+std::string(Index<10?"00":Index<100?"0":"")+std::to_string(Index),255);
        RequireStageF(Runtime.counters().loaded_state_shard_count==40,"on-demand load count mismatch");
        RequireStageF(Runtime.counters().state_cache_size<=32,"state cache exceeds bound");
    });

    Run("existing 20 AI and 20 NON AI remain the only presentation fixture",[&]
    {
        const auto Simulation=nation_sim::Simulation::from_fixture_with_overlay(Fixture,Overlay);
        RequireStageF(Simulation.ai_npcs().size()==20&&Simulation.non_ai_npcs().size()==20,"presentation fixture count changed");
        for (const std::string Id:{"ai_npc_001","ai_npc_002","ai_npc_012"})
            RequireStageF(!Simulation.ai_npc(Id).states.at(5).undefined,"Stage E target state missing: "+Id);
        Evidence["presentation_actor_contract"]={{"ai",20},{"non_ai",20},{"total",40}};
    });

    Run("Unreal presentation boundary remains subsystem player and NPC actor",[&]
    {
        RequireStageF(UNationSimulationGameInstanceSubsystem::StaticClass()->IsChildOf(UGameInstanceSubsystem::StaticClass()),"subsystem missing");
        RequireStageF(AStageDPlayerCharacter::StaticClass()->IsChildOf(ACharacter::StaticClass()),"player character missing");
        RequireStageF(AStageDNpcActor::StaticClass()->IsChildOf(AActor::StaticClass()),"NPC actor missing");
        RequireStageF(FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(),TEXT("Maps/StageD_Capital.umap"))),"StageD_Capital missing");
    });

    Run("Actor and HUD do not contain Stage F decision logic or bulk spawn",[&]
    {
        const TArray<FString> Sources={TEXT("StageDNpcActor.cpp"),TEXT("StageDPlayerCharacter.cpp"),TEXT("StageDHudWidget.cpp")};
        for (const FString& File:Sources)
        {
            FString Source;
            const FString Path=FPaths::Combine(FPaths::ProjectDir(),TEXT("Source/NationSimulationStageC/Private/StageD"),File);
            RequireStageF(FFileHelper::LoadFileToString(Source,*Path),"cannot inspect "+std::string(TCHAR_TO_UTF8(*File)));
            RequireStageF(!Source.Contains(TEXT("2500"))&&!Source.Contains(TEXT("StageFProductionRuntime")),
                "bulk runtime logic leaked into presentation: "+std::string(TCHAR_TO_UTF8(*File)));
        }
    });

    Evidence["summary"]={{"passed",Passed},{"failed",Failed},{"expected",5}};
    std::ofstream EvidenceFile(Output/"stage_f_unreal_evidence.json",std::ios::trunc);
    EvidenceFile<<Evidence.dump(2)<<'\n';
    std::ofstream ResultFile(Output/"stage_f_unreal_results.txt",std::ios::trunc);
    for (const auto& Line:Results) ResultFile<<Line<<'\n';
    ResultFile<<"SUMMARY | "<<Passed<<"/5 tests passed\n";
    return Failed==0;
}

#endif
