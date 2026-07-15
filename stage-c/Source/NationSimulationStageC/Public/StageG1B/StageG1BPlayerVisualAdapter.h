#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG1B/StageG1BMeshyAnimInstance.h"
#include "StageG1BPlayerVisualAdapter.generated.h"

class AStageG1APlayerCharacter;
class UAnimSequence;
class UStageG1BMovementWidget;
class UStageG1BTechnicalWidget;

/** Dedicated-map presentation adapter; never owns movement or collision. */
UCLASS(Blueprintable)
class NATIONSIMULATIONSTAGEC_API AStageG1BPlayerVisualAdapter : public AActor
{
    GENERATED_BODY()

public:
    AStageG1BPlayerVisualAdapter();
    virtual void BeginPlay() override;

    bool IsMeshyVisualActive() const { return bMeshyVisualActive; }
    bool IsMannyFallback() const { return bMannyFallback; }
    AStageG1APlayerCharacter* GetAcceptedPlayer() const { return AcceptedPlayer.Get(); }
    EStageG1BMoveMode GetMoveMode() const { return MoveMode; }
    void SetMoveMode(EStageG1BMoveMode NewMode);
    void CycleMovementMode();
    FString GetMoveModeDisplayNameJa() const;
    FString GetNextMoveModeDisplayNameJa() const;
    float GetConfiguredSpeed() const;

private:
    UPROPERTY()
    TWeakObjectPtr<AStageG1APlayerCharacter> AcceptedPlayer;

    UPROPERTY()
    TObjectPtr<UStageG1BTechnicalWidget> TechnicalWidget;

    UPROPERTY()
    TObjectPtr<UStageG1BMovementWidget> MovementWidget;

    UPROPERTY()
    TObjectPtr<UAnimSequence> IdleSequence;

    UPROPERTY()
    TObjectPtr<UAnimSequence> WalkSequence;

    UPROPERTY()
    TObjectPtr<UAnimSequence> RunSequence;

    UPROPERTY()
    TObjectPtr<UAnimSequence> DashSequence;

    bool bMeshyVisualActive = false;
    bool bMannyFallback = false;
    EStageG1BMoveMode MoveMode = EStageG1BMoveMode::Walk;
    FVector EvidenceStart = FVector::ZeroVector;
    FVector IdleEvidenceLocation = FVector::ZeroVector;
    FRotator IdleEvidenceRotation = FRotator::ZeroRotator;
    FVector EvidenceDestination = FVector::ZeroVector;
    FString EvidenceSelectedTarget;
    int32 EvidenceDestinationUpdateCount = 0;
    int32 EvidencePathPointCount = 0;
    float IdleStartTime = 0.0f;
    float IdleEndTime = 0.0f;
    float IdlePlayLength = 0.0f;
    float WalkSampleSpeed = 0.0f;
    float RunSampleSpeed = 0.0f;
    float DashSampleSpeed = 0.0f;
    float WalkSamplePlayRate = 1.0f;
    float RunSamplePlayRate = 1.0f;
    float DashSamplePlayRate = 1.0f;
    bool bIdleSequenceObserved = false;
    bool bIdleTimeProgressed = false;
    bool bIdleLoopMaintained = false;
    bool bIdleActorTransformStable = false;
    bool bIdleReturnObserved = false;
    bool bWalkSequenceObserved = false;
    bool bRunSequenceObserved = false;
    bool bDashSequenceObserved = false;
    bool bRunDestinationMaintained = false;
    bool bRunPathMaintained = false;
    bool bRunMoveMaintained = false;
    bool bDashDestinationMaintained = false;
    bool bDashPathMaintained = false;
    bool bDashMoveMaintained = false;
    bool bWalkReturnDestinationMaintained = false;
    bool bWalkReturnPathMaintained = false;
    bool bWalkReturnMoveMaintained = false;
    bool bUiDestinationCountMaintained = false;
    bool bUiTargetMaintained = false;

    void ToggleTechnicalDisplay();
    void ConfigureMeshyVisual();
    void CaptureIdleStart();
    void CaptureIdleAfterLoops();
    void BeginEvidenceMove();
    void CaptureWalkAndSwitchToRun();
    void CaptureRunAndSwitchToDash();
    void CaptureDashAndReturnToWalk();
    void CaptureIdleReturn();
    void WriteEvidence();
    void CaptureIdleScreenshot();
    void CaptureWalkScreenshot();
    void CaptureRunScreenshot();
    void CaptureDashScreenshot();
    void CaptureTechnicalScreenshot();
    void FinishScreenshotCapture();
    void RequestEvidenceScreenshot(const FString& Filename);
};
