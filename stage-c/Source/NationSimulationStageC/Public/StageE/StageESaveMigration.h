#pragma once

#include "CoreMinimal.h"

struct FStageESaveMigrationResult
{
    bool bSuccess = false;
    bool bMigrated = false;
    FString DefinitionSha256;
    FString SourceSaveSha256;
    FString SourceMetadataSha256;
    FString SaveBackupPath;
    FString MetadataBackupPath;
    FString Error;
};

class NATIONSIMULATIONSTAGEC_API FStageESaveMigration final
{
public:
    static FStageESaveMigrationResult MigrateIfNeeded(
        const FString& SavePath,
        const FString& MetadataPath,
        const FString& FixturePath,
        const FString& OverlayPath);

    static bool Sha256File(const FString& Path, FString& OutSha256, FString& OutError);

private:
    static bool EnsureImmutableBackup(
        const FString& SourcePath,
        const FString& BackupPath,
        const FString& ExpectedSha256,
        FString& OutError);
};
