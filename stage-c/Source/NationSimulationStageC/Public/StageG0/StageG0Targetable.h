#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "StageG0Targetable.generated.h"

USTRUCT(BlueprintType)
struct NATIONSIMULATIONSTAGEC_API FStageG0TargetInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString TargetId;
    UPROPERTY(BlueprintReadOnly) FString TargetType;
    UPROPERTY(BlueprintReadOnly) FString DisplayNameJa;
    UPROPERTY(BlueprintReadOnly) FString CountryId;
    UPROPERTY(BlueprintReadOnly) FString LocationId;
    UPROPERTY(BlueprintReadOnly) bool bIsActive = true;
    UPROPERTY(BlueprintReadOnly) bool bIsTargetable = true;
    UPROPERTY(BlueprintReadOnly) FVector InteractionOrigin = FVector::ZeroVector;
};

UINTERFACE(MinimalAPI)
class UStageG0Targetable : public UInterface
{
    GENERATED_BODY()
};

class NATIONSIMULATIONSTAGEC_API IStageG0Targetable
{
    GENERATED_BODY()

public:
    virtual FStageG0TargetInfo GetStageG0TargetInfo() const = 0;
    virtual void SetStageG0TargetSelected(bool bSelected) = 0;
};
