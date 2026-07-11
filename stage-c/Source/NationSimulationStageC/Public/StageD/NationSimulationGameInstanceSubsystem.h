#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "StageD/StageDTypes.h"
#include "NationSimulationGameInstanceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStageDCoreStateChanged);

UCLASS(Config=Game, DefaultConfig)
class NATIONSIMULATIONSTAGEC_API UNationSimulationGameInstanceSubsystem final : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category="Stage D|Causal Core")
    bool SubmitPlayerAction(const FString& Action, const FString& TargetNpcId = TEXT(""), const FString& DestinationId = TEXT(""));

    UFUNCTION(BlueprintCallable, Category="Stage D|Causal Core")
    bool EnterLocation(const FString& LocationId);

    UFUNCTION(BlueprintCallable, Category="Stage D|Causal Core")
    bool SaveMidChain();

    UFUNCTION(BlueprintCallable, Category="Stage D|Causal Core")
    bool ReloadSave();

    UFUNCTION(BlueprintCallable, Category="Stage D|Causal Core")
    void SetTargetNpc(const FString& NpcId);

    UFUNCTION(BlueprintCallable, Category="Stage D|Causal Core")
    void ClearTargetNpc(const FString& Reason = TEXT(""));

    UFUNCTION(BlueprintPure, Category="Stage D|Causal Core")
    FStageDWorldView GetWorldView() const;

    UFUNCTION(BlueprintPure, Category="Stage D|Causal Core")
    FStageDNpcView GetNpcView(const FString& NpcId) const;

    UFUNCTION(BlueprintPure, Category="Stage D|Causal Core")
    TArray<FStageDNpcView> GetAllNpcViews() const;

    UFUNCTION(BlueprintPure, Category="Stage D|Causal Core")
    bool HasPendingEvents() const;

    UFUNCTION(BlueprintPure, Category="Stage D|Causal Core")
    float GetInteractionRange() const { return InteractionRange; }

    void AdvancePresentation(float DeltaSeconds);

    UPROPERTY(BlueprintAssignable, Category="Stage D|Causal Core")
    FStageDCoreStateChanged OnCoreStateChanged;

private:
    struct FCoreStorage;
    FCoreStorage* Core = nullptr;
    FString TargetNpcId;
    FString LastError;
    FString RecentEvent;
    FString LastDialogue;
    FString LastActionRejection;
    float EventAccumulator = 0.0f;
    int64 DeferredOfflineSeconds = 0;
    int64 OfflineSecondsApplied = 0;

    UPROPERTY(Config)
    float InteractionRange = 350.0f;

    FString ResolveFixturePath() const;
    FString SavePath() const;
    FString SaveMetadataPath() const;
    bool LoadCore(bool bApplyOfflineElapsed);
    bool ValidateInteractionTarget(const FString& NpcId, FString& OutReason) const;
    void RejectAction(const FString& Action, const FString& NpcId, const FString& Reason);
    void RefreshTargetValidity();
    void RefreshProjection(int32 PreviousEventCount, int32 PreviousAuditCount);
    void RecordError(const FString& Context, const std::exception& Exception);
    void RecordErrorMessage(const FString& Context, const FString& Message);
};
