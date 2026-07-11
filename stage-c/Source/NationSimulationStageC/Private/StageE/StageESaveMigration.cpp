#include "StageE/StageESaveMigration.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "StageD/StageDSaveMetadata.h"
#include "nation_sim/simulation.hpp"

#include <filesystem>
#include <array>
#include <nlohmann/json.hpp>

namespace
{
uint32 RotateRight(uint32 Value, uint32 Count)
{
    return (Value >> Count) | (Value << (32u - Count));
}

void Sha256Bytes(const TArray<uint8>& Input, uint8 OutDigest[32])
{
    static constexpr std::array<uint32, 64> K{
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
    TArray<uint8> Message = Input;
    const uint64 BitLength = static_cast<uint64>(Message.Num()) * 8u;
    Message.Add(0x80u);
    while ((Message.Num() % 64) != 56) Message.Add(0u);
    for (int Shift = 56; Shift >= 0; Shift -= 8) Message.Add(static_cast<uint8>((BitLength >> Shift) & 0xffu));

    std::array<uint32, 8> Hash{0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
                               0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    for (int32 Offset = 0; Offset < Message.Num(); Offset += 64)
    {
        std::array<uint32, 64> W{};
        for (int Index = 0; Index < 16; ++Index)
        {
            const int32 Base = Offset + Index * 4;
            W[Index] = (static_cast<uint32>(Message[Base]) << 24) |
                       (static_cast<uint32>(Message[Base + 1]) << 16) |
                       (static_cast<uint32>(Message[Base + 2]) << 8) |
                       static_cast<uint32>(Message[Base + 3]);
        }
        for (int Index = 16; Index < 64; ++Index)
        {
            const uint32 S0 = RotateRight(W[Index - 15], 7) ^ RotateRight(W[Index - 15], 18) ^ (W[Index - 15] >> 3);
            const uint32 S1 = RotateRight(W[Index - 2], 17) ^ RotateRight(W[Index - 2], 19) ^ (W[Index - 2] >> 10);
            W[Index] = W[Index - 16] + S0 + W[Index - 7] + S1;
        }
        uint32 A=Hash[0], B=Hash[1], C=Hash[2], D=Hash[3], E=Hash[4], F=Hash[5], G=Hash[6], H=Hash[7];
        for (int Index = 0; Index < 64; ++Index)
        {
            const uint32 S1 = RotateRight(E, 6) ^ RotateRight(E, 11) ^ RotateRight(E, 25);
            const uint32 Choice = (E & F) ^ ((~E) & G);
            const uint32 T1 = H + S1 + Choice + K[Index] + W[Index];
            const uint32 S0 = RotateRight(A, 2) ^ RotateRight(A, 13) ^ RotateRight(A, 22);
            const uint32 Majority = (A & B) ^ (A & C) ^ (B & C);
            const uint32 T2 = S0 + Majority;
            H=G; G=F; F=E; E=D+T1; D=C; C=B; B=A; A=T1+T2;
        }
        Hash[0]+=A; Hash[1]+=B; Hash[2]+=C; Hash[3]+=D;
        Hash[4]+=E; Hash[5]+=F; Hash[6]+=G; Hash[7]+=H;
    }
    for (int Index = 0; Index < 8; ++Index)
    {
        OutDigest[Index * 4] = static_cast<uint8>(Hash[Index] >> 24);
        OutDigest[Index * 4 + 1] = static_cast<uint8>(Hash[Index] >> 16);
        OutDigest[Index * 4 + 2] = static_cast<uint8>(Hash[Index] >> 8);
        OutDigest[Index * 4 + 3] = static_cast<uint8>(Hash[Index]);
    }
}

std::filesystem::path NativePath(const FString& Path)
{
    return std::filesystem::path(*Path);
}

bool CopyExact(const FString& Source, const FString& Destination, FString& OutError)
{
    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    if (!Files.CopyFile(*Destination, *Source))
    {
        OutError = FString::Printf(TEXT("cannot copy %s to %s"), *Source, *Destination);
        return false;
    }
    return true;
}

bool RestorePair(const FString& SaveBackup, const FString& MetadataBackup,
    const FString& SavePath, const FString& MetadataPath, FString& OutError)
{
    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    Files.DeleteFile(*SavePath);
    Files.DeleteFile(*MetadataPath);
    if (!CopyExact(SaveBackup, SavePath, OutError)) return false;
    if (!CopyExact(MetadataBackup, MetadataPath, OutError)) return false;
    return true;
}
}

bool FStageESaveMigration::Sha256File(const FString& Path, FString& OutSha256, FString& OutError)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *Path))
    {
        OutError = FString::Printf(TEXT("cannot read file for SHA-256: %s"), *Path);
        return false;
    }
    uint8 Digest[32]{};
    Sha256Bytes(Bytes, Digest);
    OutSha256 = BytesToHex(Digest, UE_ARRAY_COUNT(Digest)).ToLower();
    return true;
}

bool FStageESaveMigration::EnsureImmutableBackup(
    const FString& SourcePath, const FString& BackupPath,
    const FString& ExpectedSha256, FString& OutError)
{
    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    if (Files.FileExists(*BackupPath))
    {
        FString ExistingSha;
        if (!Sha256File(BackupPath, ExistingSha, OutError)) return false;
        if (!ExistingSha.Equals(ExpectedSha256, ESearchCase::IgnoreCase))
        {
            OutError = FString::Printf(TEXT("immutable backup exists with different content: %s"), *BackupPath);
            return false;
        }
        return true;
    }
    if (!CopyExact(SourcePath, BackupPath, OutError)) return false;
    FString WrittenSha;
    if (!Sha256File(BackupPath, WrittenSha, OutError) ||
        !WrittenSha.Equals(ExpectedSha256, ESearchCase::IgnoreCase))
    {
        Files.DeleteFile(*BackupPath);
        OutError = FString::Printf(TEXT("immutable backup verification failed: %s"), *BackupPath);
        return false;
    }
    return true;
}

FStageESaveMigrationResult FStageESaveMigration::MigrateIfNeeded(
    const FString& SavePath, const FString& MetadataPath,
    const FString& FixturePath, const FString& OverlayPath)
{
    FStageESaveMigrationResult Result;
    FString OverlaySha;
    if (!Sha256File(OverlayPath, OverlaySha, Result.Error)) return Result;
    Result.DefinitionSha256 = OverlaySha;

    FString SaveText;
    if (!FFileHelper::LoadFileToString(SaveText, *SavePath))
    {
        Result.Error = FString::Printf(TEXT("cannot read save for Stage E migration: %s"), *SavePath);
        return Result;
    }
    nlohmann::json Root;
    try
    {
        Root = nlohmann::json::parse(TCHAR_TO_UTF8(*SaveText));
    }
    catch (const std::exception& Exception)
    {
        Result.Error = FString::Printf(TEXT("invalid save JSON: %s"), UTF8_TO_TCHAR(Exception.what()));
        return Result;
    }
    const std::string Schema = Root.value("schema_version", "");
    if (Schema == "stage_e_save_schema_v1")
    {
        try
        {
            nation_sim::Simulation::load_save_with_overlay(
                NativePath(FixturePath), NativePath(OverlayPath), NativePath(SavePath), TCHAR_TO_UTF8(*OverlaySha));
            Result.bSuccess = true;
            return Result;
        }
        catch (const std::exception& Exception)
        {
            Result.Error = UTF8_TO_TCHAR(Exception.what());
            return Result;
        }
    }

    const FStageDSaveMetadataReadResult Metadata = FStageDSaveMetadata::ReadValidated(SavePath, MetadataPath);
    if (!Metadata.bValid)
    {
        Result.Error = Metadata.Error;
        return Result;
    }
    if (!Sha256File(SavePath, Result.SourceSaveSha256, Result.Error) ||
        !Sha256File(MetadataPath, Result.SourceMetadataSha256, Result.Error)) return Result;

    Result.SaveBackupPath = SavePath + TEXT(".stage-d-") + Result.SourceSaveSha256 + TEXT(".bak");
    Result.MetadataBackupPath = MetadataPath + TEXT(".stage-d-") + Result.SourceSaveSha256 + TEXT(".bak");
    if (!EnsureImmutableBackup(SavePath, Result.SaveBackupPath, Result.SourceSaveSha256, Result.Error) ||
        !EnsureImmutableBackup(MetadataPath, Result.MetadataBackupPath, Result.SourceMetadataSha256, Result.Error)) return Result;

    const FString TemporarySave = SavePath + TEXT(".stage-e-migration.tmp");
    const FString TemporaryMetadata = MetadataPath + TEXT(".stage-e-migration.tmp");
    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    Files.DeleteFile(*TemporarySave);
    Files.DeleteFile(*TemporaryMetadata);
    try
    {
        auto Simulation = nation_sim::Simulation::load_save_with_overlay(
            NativePath(FixturePath), NativePath(OverlayPath), NativePath(SavePath), TCHAR_TO_UTF8(*OverlaySha));
        Simulation.configure_stage_e_migration(
            Root.value("schema_version", ""), Root.value("simulation_version", ""),
            TCHAR_TO_UTF8(*Result.SourceSaveSha256), TCHAR_TO_UTF8(*Result.SourceMetadataSha256),
            TCHAR_TO_UTF8(*Result.SaveBackupPath), TCHAR_TO_UTF8(*Result.MetadataBackupPath));
        Simulation.save(NativePath(TemporarySave));
        FString MetadataError;
        if (!FStageDSaveMetadata::WriteVerified(
            TemporarySave, TemporaryMetadata, Metadata.SavedAtUtcEpoch, MetadataError))
        {
            throw std::runtime_error(TCHAR_TO_UTF8(*MetadataError));
        }
        nation_sim::Simulation::load_save_with_overlay(
            NativePath(FixturePath), NativePath(OverlayPath), NativePath(TemporarySave), TCHAR_TO_UTF8(*OverlaySha));
        const auto TemporaryValidation = FStageDSaveMetadata::ReadValidated(TemporarySave, TemporaryMetadata);
        if (!TemporaryValidation.bValid) throw std::runtime_error(TCHAR_TO_UTF8(*TemporaryValidation.Error));
    }
    catch (const std::exception& Exception)
    {
        Files.DeleteFile(*TemporarySave);
        Files.DeleteFile(*TemporaryMetadata);
        Result.Error = FString::Printf(TEXT("Stage E migration staging failed: %s"), UTF8_TO_TCHAR(Exception.what()));
        return Result;
    }

    const FString RollbackSave = SavePath + TEXT(".stage-e-rollback.tmp");
    const FString RollbackMetadata = MetadataPath + TEXT(".stage-e-rollback.tmp");
    Files.DeleteFile(*RollbackSave);
    Files.DeleteFile(*RollbackMetadata);
    if (!Files.MoveFile(*RollbackSave, *SavePath) || !Files.MoveFile(*RollbackMetadata, *MetadataPath) ||
        !Files.MoveFile(*SavePath, *TemporarySave) || !Files.MoveFile(*MetadataPath, *TemporaryMetadata))
    {
        FString RestoreError;
        RestorePair(Result.SaveBackupPath, Result.MetadataBackupPath, SavePath, MetadataPath, RestoreError);
        Files.DeleteFile(*TemporarySave);
        Files.DeleteFile(*TemporaryMetadata);
        Files.DeleteFile(*RollbackSave);
        Files.DeleteFile(*RollbackMetadata);
        Result.Error = TEXT("Stage E migration commit failed");
        if (!RestoreError.IsEmpty()) Result.Error += TEXT("; restore failed: ") + RestoreError;
        return Result;
    }

    Files.DeleteFile(*RollbackSave);
    Files.DeleteFile(*RollbackMetadata);
    try
    {
        nation_sim::Simulation::load_save_with_overlay(
            NativePath(FixturePath), NativePath(OverlayPath), NativePath(SavePath), TCHAR_TO_UTF8(*OverlaySha));
        const auto FinalMetadata = FStageDSaveMetadata::ReadValidated(SavePath, MetadataPath);
        if (!FinalMetadata.bValid) throw std::runtime_error(TCHAR_TO_UTF8(*FinalMetadata.Error));
    }
    catch (const std::exception& Exception)
    {
        FString RestoreError;
        RestorePair(Result.SaveBackupPath, Result.MetadataBackupPath, SavePath, MetadataPath, RestoreError);
        Result.Error = FString::Printf(TEXT("Stage E migration verification failed: %s"), UTF8_TO_TCHAR(Exception.what()));
        if (!RestoreError.IsEmpty()) Result.Error += TEXT("; restore failed: ") + RestoreError;
        return Result;
    }
    Result.bMigrated = true;
    Result.bSuccess = true;
    return Result;
}
