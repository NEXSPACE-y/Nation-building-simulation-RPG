#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StageD/StageDSaveMetadata.h"
#include "StageE/StageESaveMigration.h"
#include "nation_sim/simulation.hpp"

#include <filesystem>
#include <algorithm>
#include <fstream>
#include <nlohmann/json.hpp>

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
using json = nlohmann::json;

void SaveRequire(bool Condition, const FString& Message)
{
    if (!Condition) throw std::runtime_error(TCHAR_TO_UTF8(*Message));
}

struct FSaveContext
{
    FString RepositoryRoot;
    FString Fixture;
    FString Overlay;
    FString Output;

    FSaveContext()
    {
        RepositoryRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
        Fixture = FPaths::Combine(RepositoryRoot, TEXT("data/stage_a_fixture.json"));
        Overlay = FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/StageE/stage_e_state_definitions.json"));
        Output = FPaths::Combine(RepositoryRoot, TEXT("out/stage-e/migration-output"));
        IFileManager::Get().DeleteDirectory(*Output, false, true);
        IFileManager::Get().MakeDirectory(*Output, true);
    }
};

void MakeStageDSave(const FSaveContext& Context, const FString& Save, const FString& Metadata)
{
    auto Simulation = nation_sim::Simulation::from_fixture(std::filesystem::path(*Context.Fixture));
    const auto Target = std::find_if(Simulation.non_ai_npcs().begin(), Simulation.non_ai_npcs().end(),
        [&](const nation_sim::NonAiNpcState& Npc) { return Npc.current_location_id == Simulation.player().current_location_id; });
    if (Target == Simulation.non_ai_npcs().end()) throw std::runtime_error("Stage D migration target missing");
    Simulation.enqueue_player_action("HARM", Target->npc_id);
    Simulation.process_pending(1);
    Simulation.save(std::filesystem::path(*Save));
    FString Error;
    SaveRequire(FStageDSaveMetadata::WriteVerified(Save, Metadata, 1700000000, Error), Error);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStageESaveMigrationTest,
    "NationSimulation.StageE.SaveMigration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageESaveMigrationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FSaveContext Context;
    json Evidence;
    int Passed = 0;
    int Failed = 0;
    auto Run = [&](const char* Name, auto&& Body)
    {
        try { Body(); AddInfo(FString::Printf(TEXT("PASS | %s"), UTF8_TO_TCHAR(Name))); ++Passed; }
        catch (const std::exception& Exception)
        {
            AddError(FString::Printf(TEXT("FAIL | %s | %s"), UTF8_TO_TCHAR(Name), UTF8_TO_TCHAR(Exception.what())));
            ++Failed;
        }
    };

    const FString Save = FPaths::Combine(Context.Output, TEXT("stage_d_save.json"));
    const FString Metadata = Save + TEXT(".utc");
    FString OriginalSaveSha, OriginalMetadataSha, Error;
    FStageESaveMigrationResult Migration;

    Run("Stage D save migrates in place with immutable backups", [&]
    {
        MakeStageDSave(Context, Save, Metadata);
        SaveRequire(FStageESaveMigration::Sha256File(Save, OriginalSaveSha, Error), Error);
        SaveRequire(FStageESaveMigration::Sha256File(Metadata, OriginalMetadataSha, Error), Error);
        Migration = FStageESaveMigration::MigrateIfNeeded(Save, Metadata, Context.Fixture, Context.Overlay);
        SaveRequire(Migration.bSuccess && Migration.bMigrated, Migration.Error);
        SaveRequire(FPaths::FileExists(Migration.SaveBackupPath), TEXT("save backup missing"));
        SaveRequire(FPaths::FileExists(Migration.MetadataBackupPath), TEXT("metadata backup missing"));
        FString BackupSaveSha, BackupMetadataSha;
        SaveRequire(FStageESaveMigration::Sha256File(Migration.SaveBackupPath, BackupSaveSha, Error), Error);
        SaveRequire(FStageESaveMigration::Sha256File(Migration.MetadataBackupPath, BackupMetadataSha, Error), Error);
        SaveRequire(BackupSaveSha == OriginalSaveSha, TEXT("save backup SHA mismatch"));
        SaveRequire(BackupMetadataSha == OriginalMetadataSha, TEXT("metadata backup SHA mismatch"));
        FString MigratedText;
        SaveRequire(FFileHelper::LoadFileToString(MigratedText, *Save), TEXT("cannot read migrated save"));
        const json Root = json::parse(TCHAR_TO_UTF8(*MigratedText));
        SaveRequire(Root.at("schema_version") == "stage_e_save_schema_v1", TEXT("Stage E schema missing"));
        SaveRequire(Root.at("simulation_version") == "stage-e-0.1.0", TEXT("Stage E simulation version missing"));
        SaveRequire(Root.at("migration").at("source_schema_version") == "1.0.0", TEXT("source schema missing"));
        SaveRequire(Root.at("migration").at("source_simulation_version") == "stage-a-0.1.0", TEXT("source simulation version missing"));
        Evidence["migration"] = {{"source_save_sha256", TCHAR_TO_UTF8(*OriginalSaveSha)},
                                 {"source_metadata_sha256", TCHAR_TO_UTF8(*OriginalMetadataSha)},
                                 {"save_backup", TCHAR_TO_UTF8(*Migration.SaveBackupPath)},
                                 {"metadata_backup", TCHAR_TO_UTF8(*Migration.MetadataBackupPath)}};
    });

    Run("Stage E resave and reload are identical", [&]
    {
        auto Loaded = nation_sim::Simulation::load_save_with_overlay(
            std::filesystem::path(*Context.Fixture), std::filesystem::path(*Context.Overlay),
            std::filesystem::path(*Save), TCHAR_TO_UTF8(*Migration.DefinitionSha256));
        Loaded.process_pending();
        Loaded.save(std::filesystem::path(*Save));
        SaveRequire(FStageDSaveMetadata::WriteVerified(Save, Metadata, 1700000100, Error), Error);
        auto Reloaded = nation_sim::Simulation::load_save_with_overlay(
            std::filesystem::path(*Context.Fixture), std::filesystem::path(*Context.Overlay),
            std::filesystem::path(*Save), TCHAR_TO_UTF8(*Migration.DefinitionSha256));
        SaveRequire(Loaded.canonical_snapshot() == Reloaded.canonical_snapshot(), TEXT("Stage E resave mismatch"));
        Evidence["resave_snapshot_equal"] = true;
    });

    Run("Existing immutable backup is never overwritten", [&]
    {
        FString BeforeSha, AfterSha;
        SaveRequire(FStageESaveMigration::Sha256File(Migration.SaveBackupPath, BeforeSha, Error), Error);
        const auto Second = FStageESaveMigration::MigrateIfNeeded(Save, Metadata, Context.Fixture, Context.Overlay);
        SaveRequire(Second.bSuccess && !Second.bMigrated, Second.Error);
        SaveRequire(FStageESaveMigration::Sha256File(Migration.SaveBackupPath, AfterSha, Error), Error);
        SaveRequire(BeforeSha == AfterSha, TEXT("immutable backup changed"));
        Evidence["backup_reused_without_overwrite"] = true;
    });

    Run("Conflicting backup aborts without changing source", [&]
    {
        const FString FailureSave = FPaths::Combine(Context.Output, TEXT("failure_stage_d_save.json"));
        const FString FailureMetadata = FailureSave + TEXT(".utc");
        MakeStageDSave(Context, FailureSave, FailureMetadata);
        FString SaveShaBefore, MetadataShaBefore;
        SaveRequire(FStageESaveMigration::Sha256File(FailureSave, SaveShaBefore, Error), Error);
        SaveRequire(FStageESaveMigration::Sha256File(FailureMetadata, MetadataShaBefore, Error), Error);
        const FString ConflictBackup = FailureSave + TEXT(".stage-d-") + SaveShaBefore + TEXT(".bak");
        SaveRequire(FFileHelper::SaveStringToFile(TEXT("conflicting backup"), *ConflictBackup), TEXT("cannot write conflict backup"));
        const auto FailedMigration = FStageESaveMigration::MigrateIfNeeded(
            FailureSave, FailureMetadata, Context.Fixture, Context.Overlay);
        SaveRequire(!FailedMigration.bSuccess, TEXT("conflicting backup was accepted"));
        FString SaveShaAfter, MetadataShaAfter;
        SaveRequire(FStageESaveMigration::Sha256File(FailureSave, SaveShaAfter, Error), Error);
        SaveRequire(FStageESaveMigration::Sha256File(FailureMetadata, MetadataShaAfter, Error), Error);
        SaveRequire(SaveShaBefore == SaveShaAfter && MetadataShaBefore == MetadataShaAfter,
            TEXT("source changed after failed migration"));
        Evidence["failure_preserved_source"] = {{"save_sha256", TCHAR_TO_UTF8(*SaveShaAfter)},
                                                 {"metadata_sha256", TCHAR_TO_UTF8(*MetadataShaAfter)}};
    });

    Evidence["summary"] = {{"passed", Passed}, {"failed", Failed}};
    const FString EvidencePath = FPaths::Combine(Context.Output, TEXT("stage_e_migration_evidence.json"));
    std::ofstream Out{std::filesystem::path(*EvidencePath)};
    Out << Evidence.dump(2) << '\n';
    AddInfo(FString::Printf(TEXT("STAGE_E_SAVE_MIGRATION | %d/4 tests passed"), Passed));
    return Failed == 0;
}

#endif
