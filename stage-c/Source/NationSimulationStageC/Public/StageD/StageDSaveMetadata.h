#pragma once

#include "CoreMinimal.h"

struct FStageDSaveMetadataReadResult
{
    bool bValid = false;
    int64 SavedAtUtcEpoch = 0;
    FString SaveSha1;
    FString Error;
};
class NATIONSIMULATIONSTAGEC_API FStageDSaveMetadata final
{
public:
    static bool WriteVerified(const FString& SavePath, const FString& MetadataPath,
        int64 SavedAtUtcEpoch, FString& OutError);
    static FStageDSaveMetadataReadResult ReadValidated(const FString& SavePath, const FString& MetadataPath);

private:
    static bool HashSave(const FString& SavePath, FString& OutSha1, FString& OutError);
};
