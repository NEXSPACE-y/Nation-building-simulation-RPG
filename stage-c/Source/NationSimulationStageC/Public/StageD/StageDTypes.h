#pragma once

#include "CoreMinimal.h"
#include "StageDTypes.generated.h"

USTRUCT(BlueprintType)
struct FStageDNpcView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString NpcId;
    UPROPERTY(BlueprintReadOnly) FString Role;
    UPROPERTY(BlueprintReadOnly) FString LocationId;
    UPROPERTY(BlueprintReadOnly) bool bIsAi = false;
    UPROPERTY(BlueprintReadOnly) int32 CurrentStateId = 0;
    UPROPERTY(BlueprintReadOnly) int32 PlayerEvaluation = 0;
    UPROPERTY(BlueprintReadOnly) FString SelectedRule;
    UPROPERTY(BlueprintReadOnly) FString SelectedAction;
    UPROPERTY(BlueprintReadOnly) FString SelectedTargetId;
    UPROPERTY(BlueprintReadOnly) FString RootEventId;
    UPROPERTY(BlueprintReadOnly) FString Dialogue;
};

USTRUCT(BlueprintType)
struct FStageDWorldView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString CurrentLocationId;
    UPROPERTY(BlueprintReadOnly) FString TargetNpcId;
    UPROPERTY(BlueprintReadOnly) FString TargetRole;
    UPROPERTY(BlueprintReadOnly) FString Dialogue;
    UPROPERTY(BlueprintReadOnly) FString RecentEvent;
    UPROPERTY(BlueprintReadOnly) int32 Security = 0;
    UPROPERTY(BlueprintReadOnly) int32 CrimeLevel = 0;
    UPROPERTY(BlueprintReadOnly) int64 SimulationTick = 0;
    UPROPERTY(BlueprintReadOnly) int32 PendingEventCount = 0;
    UPROPERTY(BlueprintReadOnly) int64 OfflineRealSecondsApplied = 0;
    UPROPERTY(BlueprintReadOnly) FString ActionBlockedReason;
};
