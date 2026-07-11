#include "StageD/StageDSaveMetadata.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

namespace
{
constexpr TCHAR MetadataSchema[] = TEXT("stage_d_save_metadata_v2");

TMap<FString, FString> ParseMetadata(const FString& Content)
{
    TMap<FString, FString> Values;
    TArray<FString> Lines;
    Content.ParseIntoArrayLines(Lines, true);
    for (const FString& Line : Lines)
    {
        FString Key;
        FString Value;
        if (Line.Split(TEXT("="), &Key, &Value)) Values.Add(Key.TrimStartAndEnd(), Value.TrimStartAndEnd());
    }
    return Values;
}
}

bool FStageDSaveMetadata::HashSave(const FString& SavePath, FString& OutSha1, FString& OutError)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *SavePath))
    {
        OutError = FString::Printf(TEXT("cannot read save for metadata integrity: %s"), *SavePath);
        return false;
    }
    uint8 Digest[FSHA1::DigestSize]{};
    FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest);
    OutSha1 = BytesToHex(Digest, UE_ARRAY_COUNT(Digest)).ToLower();
    return true;
}

bool FStageDSaveMetadata::WriteVerified(const FString& SavePath, const FString& MetadataPath,
    int64 SavedAtUtcEpoch, FString& OutError)
{
    OutError.Reset();
    if (SavedAtUtcEpoch <= 0)
    {
        OutError = TEXT("saved_at_utc_epoch must be positive");
        return false;
    }

    FString SaveSha1;
    if (!HashSave(SavePath, SaveSha1, OutError)) return false;

    IPlatformFile& Files = FPlatformFileManager::Get().GetPlatformFile();
    const FString Parent = FPaths::GetPath(MetadataPath);
    if (!Parent.IsEmpty() && !Files.DirectoryExists(*Parent) && !Files.CreateDirectoryTree(*Parent))
    {
        OutError = FString::Printf(TEXT("cannot create save metadata directory: %s"), *Parent);
        return false;
    }

    const FString TemporaryPath = MetadataPath + TEXT(".tmp");
    const FString Content = FString::Printf(
        TEXT("schema=%s\nsaved_at_utc_epoch=%lld\nsave_sha1=%s\n"), MetadataSchema, SavedAtUtcEpoch, *SaveSha1);
    if (!FFileHelper::SaveStringToFile(Content, *TemporaryPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        OutError = FString::Printf(TEXT("failed to write save timestamp metadata: %s"), *TemporaryPath);
        return false;
    }

    FString WrittenContent;
    if (!FFileHelper::LoadFileToString(WrittenContent, *TemporaryPath) || WrittenContent != Content)
    {
        Files.DeleteFile(*TemporaryPath);
        OutError = FString::Printf(TEXT("save timestamp metadata verification failed: %s"), *TemporaryPath);
        return false;
    }

    if (Files.FileExists(*MetadataPath) && !Files.DeleteFile(*MetadataPath))
    {
        Files.DeleteFile(*TemporaryPath);
        OutError = FString::Printf(TEXT("cannot replace existing save timestamp metadata: %s"), *MetadataPath);
        return false;
    }
    if (!Files.MoveFile(*MetadataPath, *TemporaryPath))
    {
        Files.DeleteFile(*TemporaryPath);
        OutError = FString::Printf(TEXT("cannot commit save timestamp metadata: %s"), *MetadataPath);
        return false;
    }

    const FStageDSaveMetadataReadResult Validation = ReadValidated(SavePath, MetadataPath);
    if (!Validation.bValid || Validation.SavedAtUtcEpoch != SavedAtUtcEpoch)
    {
        OutError = Validation.Error.IsEmpty() ? TEXT("committed save timestamp metadata did not validate") : Validation.Error;
        return false;
    }
    return true;
}

FStageDSaveMetadataReadResult FStageDSaveMetadata::ReadValidated(
    const FString& SavePath, const FString& MetadataPath)
{
    FStageDSaveMetadataReadResult Result;
    FString Content;
    if (!FFileHelper::LoadFileToString(Content, *MetadataPath))
    {
        Result.Error = FString::Printf(TEXT("save timestamp metadata missing or unreadable: %s"), *MetadataPath);
        return Result;
    }
    const TMap<FString, FString> Values = ParseMetadata(Content);
    const FString* Schema = Values.Find(TEXT("schema"));
    const FString* Epoch = Values.Find(TEXT("saved_at_utc_epoch"));
    const FString* ExpectedSha1 = Values.Find(TEXT("save_sha1"));
    if (!Schema || *Schema != MetadataSchema || !Epoch || !ExpectedSha1)
    {
        Result.Error = TEXT("save timestamp metadata schema mismatch; legacy/stale time was not used");
        return Result;
    }
    if (!Epoch->IsNumeric())
    {
        Result.Error = TEXT("save timestamp metadata contains a non-numeric epoch");
        return Result;
    }
    Result.SavedAtUtcEpoch = FCString::Atoi64(**Epoch);
    if (Result.SavedAtUtcEpoch <= 0)
    {
        Result.Error = TEXT("save timestamp metadata contains an invalid epoch");
        return Result;
    }
    FString ActualSha1;
    if (!HashSave(SavePath, ActualSha1, Result.Error)) return Result;
    if (!ActualSha1.Equals(*ExpectedSha1, ESearchCase::IgnoreCase))
    {
        Result.Error = TEXT("save JSON and timestamp metadata integrity mismatch; offline time was not applied");
        return Result;
    }
    Result.SaveSha1 = ActualSha1;
    Result.bValid = true;
    return Result;
}
