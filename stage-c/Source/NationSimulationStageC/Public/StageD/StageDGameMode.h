#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StageDGameMode.generated.h"

class UStageDHudWidget;
class UStageG0VerificationPanel;
class AStageG0FangRatActor;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageDGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    AStageDGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    static FVector GetLocationCenter(const FString& LocationId);
    UStageDHudWidget* GetStageDHud() const { return StageDHud; }
    bool IsStageG0VisualPoC() const { return bStageG0VisualPoC; }
    static TArray<FString> GetStageG0TestPointIds();
    static FVector GetStageG0TestPoint(const FString& PointId);
    void TeleportPlayerToStageG0TestPoint(const FString& PointId);
    bool ShouldIgnoreStageG0LocationEntry() const;
    void SetStageG0ShadowsEnabled(bool bEnabled);
    void NotifyStageG0FallRecovery(int32 Count, const FVector& SafeLocation);
    void NotifyStageG0MovementRecovery(const FVector& SafeLocation);
    void SelectStageG0VisualTarget(const FString& TargetKey);
    const FString& GetStageG0RecoveryMessage() const { return StageG0RecoveryMessage; }
    int32 GetStageG0RecoveryCount() const { return StageG0RecoveryCount; }
    AStageG0FangRatActor* GetStageG0FangRat() const { return StageG0FangRat; }
    const FString& GetStageG0SelectedVisualTarget() const { return StageG0SelectedVisualTarget; }

private:
    UPROPERTY() TObjectPtr<UStageDHudWidget> StageDHud;
    UPROPERTY() TObjectPtr<UStageG0VerificationPanel> StageG0VerificationPanel;
    UPROPERTY() TObjectPtr<AStageG0FangRatActor> StageG0FangRat;
    bool bStageG0VisualPoC = false;
    bool bStageG0PerformanceWritten = false;
    float StageG0Elapsed = 0.0f;
    TArray<float> StageG0FrameTimes;
    FString StageG0RecoveryMessage;
    FString StageG0SelectedVisualTarget;
    int32 StageG0RecoveryCount = 0;
    double StageG0IgnoreLocationEntriesUntil = 0.0;
    void BuildCapitalBlock();
    void BuildStageG0VisualPoC();
    void SpawnNpcPresentation();
    void SpawnStageG0FangRat();
    void SpawnStageG0TestSign(const FString& Label, const FVector& Location);
    void SpawnStageG0BlockingVolume(const FString& Name, const FVector& Location, const FVector& Extent);
    void WriteStageG0PerformanceEvidence();
    void SpawnBlock(const FVector& Location, const FVector& Scale,
        const FRotator& Rotation = FRotator::ZeroRotator, bool bWalkableSurface = false);
};
