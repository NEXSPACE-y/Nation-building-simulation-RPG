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
    UPROPERTY(BlueprintReadOnly) FString CurrentStateName;
    UPROPERTY(BlueprintReadOnly) FString CurrentGoal;
    UPROPERTY(BlueprintReadOnly) int32 PlayerEvaluation = 0;
    UPROPERTY(BlueprintReadOnly) FString MajorRelationships;
    UPROPERTY(BlueprintReadOnly) FString EvidenceEvaluation;
    UPROPERTY(BlueprintReadOnly) FString CandidateRules;
    UPROPERTY(BlueprintReadOnly) FString SelectedRule;
    UPROPERTY(BlueprintReadOnly) FString RejectedReasons;
    UPROPERTY(BlueprintReadOnly) FString SelectedAction;
    UPROPERTY(BlueprintReadOnly) FString SelectedTargetId;
    UPROPERTY(BlueprintReadOnly) FString RootEventId;
    UPROPERTY(BlueprintReadOnly) FString LastTransitionReason;
    UPROPERTY(BlueprintReadOnly) int64 StateResidenceMinutes = 0;
    UPROPERTY(BlueprintReadOnly) int64 NextTimedTransitionAt = -1;
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

USTRUCT(BlueprintType)
struct FStageFRuntimeView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 LoadedCountryCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 AiNpcTotalCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 ActiveCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 BackgroundCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 DormantCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaterializedNonAiCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 PromotedNonAiCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 PendingEventCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 NextDueCount = 0;
    UPROPERTY(BlueprintReadOnly) int64 CurrentSaveGeneration = 0;
    UPROPERTY(BlueprintReadOnly) int32 LoadedStateShardCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 StateCacheSize = 0;
    UPROPERTY(BlueprintReadOnly) int64 LastOfflineDurationSeconds = 0;
    UPROPERTY(BlueprintReadOnly) int64 LastSaveDurationMilliseconds = 0;
    UPROPERTY(BlueprintReadOnly) int64 LastLoadDurationMilliseconds = 0;
    UPROPERTY(BlueprintReadOnly) FString DatasetSha256;
    UPROPERTY(BlueprintReadOnly) bool bCoreReady = false;
};
