#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG3BModularCastleEvidenceActor.generated.h"

class AStageG1APlayerCharacter;
class AStageG1BPlayerVisualAdapter;
class AStageG2ACameraModeAdapter;
class AStageG2BGuardMActor;
class AStageG3AStandardCameraAutoFollowActor;

/** G-3B-R-only verifier and screenshot driver. It never owns gameplay or save state. */
UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG3BModularCastleEvidenceActor final : public AActor
{
    GENERATED_BODY()

public:
    AStageG3BModularCastleEvidenceActor();
    virtual void BeginPlay() override;

private:
    UPROPERTY() TWeakObjectPtr<AStageG1APlayerCharacter> AcceptedPlayer;
    UPROPERTY() TWeakObjectPtr<AStageG1BPlayerVisualAdapter> PlayerVisualAdapter;
    UPROPERTY() TWeakObjectPtr<AStageG2ACameraModeAdapter> CameraAdapter;
    UPROPERTY() TWeakObjectPtr<AStageG2BGuardMActor> GuardActor;
    UPROPERTY() TWeakObjectPtr<AStageG3AStandardCameraAutoFollowActor> AutoFollowActor;

    FVector OriginalPlayerLocation = FVector::ZeroVector;
    FRotator OriginalPlayerRotation = FRotator::ZeroRotator;
    FVector MovementStart = FVector::ZeroVector;
    bool bMovementCommandIssued = false;
    FString MovementFailure;
    int32 ScreenshotStep = 0;
    FString PendingScreenshotFilename;
    FTimerHandle ScreenshotTimer;

    void ResolveAcceptedActors();
    void BeginRuntimeEvidence();
    void WriteRuntimeEvidence();
    void BeginScreenshotSequence();
    void AdvanceScreenshotSequence();
    void CapturePendingScreenshot();
    void FinishScreenshotSequence();
    void RequestScreenshot(const FString& Filename);
    void SetPlayerForShot(const FVector& Location, float Yaw = 90.0f);
    void SetEvidenceCamera(bool bTactical, float Pitch, float Yaw, float ArmLength, float Fov,
        bool bCollision);
    void RestoreAcceptedCameraAndPlayer();
};
