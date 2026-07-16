#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG3ACapitalEvidenceActor.generated.h"

class AStageG1APlayerCharacter;
class AStageG1BPlayerVisualAdapter;
class AStageG2ACameraModeAdapter;
class AStageG2BGuardMActor;
class AStageG3AStandardCameraAutoFollowActor;

/** G3A-only verifier and screenshot driver. It owns no gameplay or save state. */
UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG3ACapitalEvidenceActor final : public AActor
{
    GENERATED_BODY()

public:
    AStageG3ACapitalEvidenceActor();
    virtual void BeginPlay() override;

private:
    UPROPERTY() TWeakObjectPtr<AStageG1APlayerCharacter> AcceptedPlayer;
    UPROPERTY() TWeakObjectPtr<AStageG1BPlayerVisualAdapter> PlayerVisualAdapter;
    UPROPERTY() TWeakObjectPtr<AStageG2ACameraModeAdapter> CameraAdapter;
    UPROPERTY() TWeakObjectPtr<AStageG2BGuardMActor> GuardActor;
    UPROPERTY() TWeakObjectPtr<AStageG3AStandardCameraAutoFollowActor> AutoFollowActor;

    FVector OriginalPlayerLocation = FVector::ZeroVector;
    FRotator OriginalPlayerRotation = FRotator::ZeroRotator;
    FVector MovementEvidenceStart = FVector::ZeroVector;
    int32 ClickTraceSampleCount = 0;
    int32 ClickTracePassedCount = 0;
    bool bMovementCommandIssued = false;
    FString MovementCommandFailure;
    int32 ScreenshotStep = 0;
    FString PendingScreenshotFilename;
    FTimerHandle ScreenshotSequenceTimer;

    void ResolveAcceptedActors();
    void BeginRuntimeEvidence();
    void BeginManualDragEvidence();
    void EndManualDragEvidence();
    void EnterTacticalEvidence();
    void ReturnStandardEvidence();
    void WriteRuntimeEvidence();
    void BeginScreenshotSequence();
    void AdvanceScreenshotSequence();
    void CapturePendingScreenshot();
    void FinishScreenshotSequence();
    void RequestEvidenceScreenshot(const FString& Filename);
    void SetPlayerForShot(const FVector& Location, float Yaw = 90.0f);
    void EnsureStandardCamera();
    void EnsureTacticalCamera(bool bZoomOut);
    void RestorePlayerTransform();
};
