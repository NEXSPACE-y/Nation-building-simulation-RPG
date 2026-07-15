#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG2A/StageG2ACameraModeSettings.h"
#include "StageG2ACameraModeAdapter.generated.h"

class AStageG1APlayerCharacter;
class AStageG1BPlayerVisualAdapter;
class AStaticMeshActor;
class UCameraComponent;
class USpringArmComponent;
class UStageG2ACameraStatusWidget;
class UStageG2ACameraTechnicalWidget;

/**
 * G2A-only presentation adapter. It controls the accepted Player camera
 * components and never owns movement, target selection, save, or simulation.
 */
UCLASS(Blueprintable)
class NATIONSIMULATIONSTAGEC_API AStageG2ACameraModeAdapter final : public AActor
{
    GENERATED_BODY()

public:
    AStageG2ACameraModeAdapter();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    EStageG2ACameraMode GetCameraMode() const { return ActiveMode; }
    bool IsStandardMode() const { return ActiveMode == EStageG2ACameraMode::StandardCharacterCamera; }
    FString GetModeDisplayNameJa() const;
    AStageG1APlayerCharacter* GetAcceptedPlayer() const { return AcceptedPlayer.Get(); }
    float GetCameraYaw() const;
    float GetCameraPitch() const { return CurrentPitch; }
    float GetTargetArmLength() const { return TargetArmLength; }
    float GetActualCameraArmLength() const;
    float GetCurrentFov() const;
    float GetActiveMinDistance() const;
    float GetActiveMaxDistance() const;
    float GetActiveMinPitch() const;
    float GetActiveMaxPitch() const;
    float GetActiveCollisionMinDistance() const;
    bool IsRightDragActive() const { return bRightDragActive; }
    bool IsCameraCollisionShortened() const;
    bool WasLastCameraInputBlockedByUi() const { return bLastCameraInputBlockedByUi; }
    void ToggleCameraMode();

    static float NormalizeYaw(float Yaw);
    static float AccumulateYaw(float CurrentYaw, float DeltaYaw);
    static float YawDegreesFromMouseDelta(float DeltaPixels, float Sensitivity);
    static float PitchDegreesFromMouseDelta(float DeltaPixels, float Sensitivity);
    static float ClampPitch(float Pitch, float MinPitch, float MaxPitch);
    static float ClampArmLength(float ArmLength, float MinArmLength, float MaxArmLength);

private:
    struct FModeState
    {
        float Yaw = 0.0f;
        float Pitch = 0.0f;
        float ArmLength = 0.0f;
    };

    UPROPERTY() TWeakObjectPtr<AStageG1APlayerCharacter> AcceptedPlayer;
    UPROPERTY() TWeakObjectPtr<AStageG1BPlayerVisualAdapter> G1BVisualAdapter;
    UPROPERTY() TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY() TObjectPtr<UCameraComponent> FollowCamera;
    UPROPERTY() TObjectPtr<UStageG2ACameraStatusWidget> StatusWidget;
    UPROPERTY() TObjectPtr<UStageG2ACameraTechnicalWidget> TechnicalWidget;
    UPROPERTY() TWeakObjectPtr<AActor> EvidenceCollisionBlocker;

    EStageG2ACameraMode ActiveMode = EStageG2ACameraMode::StandardCharacterCamera;
    FModeState StandardState;
    FModeState TacticalState;
    float CurrentYaw = 0.0f;
    float TargetYaw = 0.0f;
    float CurrentPitch = -15.0f;
    float TargetPitch = -15.0f;
    float CurrentArmLength = 520.0f;
    float TargetArmLength = 520.0f;
    FVector2D CursorBeforeOrbit = FVector2D::ZeroVector;
    bool bRightDragActive = false;
    bool bLastCameraInputBlockedByUi = false;
    bool bCameraReady = false;

    FVector EvidencePlayerLocation = FVector::ZeroVector;
    FRotator EvidencePlayerRotation = FRotator::ZeroRotator;
    FVector EvidenceDestination = FVector::ZeroVector;
    FString EvidenceSelectedTarget;
    int32 EvidenceDestinationUpdateCount = 0;
    int32 EvidencePathPointCount = 0;
    uint8 EvidenceMoveMode = 0;
    float EvidenceStandardActualArm = 0.0f;
    float EvidenceCollisionArm = 0.0f;
    float EvidenceRecoveredArm = 0.0f;
    bool bEvidenceTacticalReached = false;
    bool bEvidenceReturnedStandard = false;
    bool bEvidenceDestinationMaintained = false;
    bool bEvidencePathMaintained = false;
    bool bEvidenceMovementModeMaintained = false;
    bool bEvidenceSelectedTargetMaintained = false;
    bool bEvidenceCollisionShortened = false;
    bool bEvidenceCollisionRecovered = false;
    bool bEvidenceCollisionTraceHit = false;
    bool bEvidenceCollisionTraceCleared = false;

    int32 ScreenshotStep = 0;
    FString PendingScreenshotFilename;
    FTimerHandle ScreenshotSequenceTimer;

    void ConfigureCamera();
    void ConfigureInput();
    void ConfigureWidgets();
    void BeginRightDrag();
    void EndRightDrag();
    void RestorePointerAfterOrbit();
    void HandleZoomAxis(float Value);
    void ToggleTechnicalDisplay();
    bool IsPointerOverUi() const;
    void SaveActiveModeState();
    void ApplyMode(EStageG2ACameraMode NewMode, bool bSnap);
    void ApplyModeParameters();
    void SetCameraTargets(float Yaw, float Pitch, float ArmLength, bool bSnap);
    void ApplyOrbitDeltaDegrees(float DeltaYaw, float DeltaPitch);
    void ApplyZoomSteps(float Steps);

    void BeginRuntimeEvidence();
    void StartEvidenceMove();
    void ExerciseModeSwitchEvidence();
    void SpawnEvidenceCollisionBlocker();
    void CaptureCollisionShortened();
    void RemoveEvidenceCollisionBlocker();
    void CaptureCollisionRecoveryAndWriteEvidence();
    void WriteRuntimeEvidence();

    void BeginScreenshotSequence();
    void AdvanceScreenshotSequence();
    void CapturePendingScreenshot();
    void FinishScreenshotSequence();
    void RequestEvidenceScreenshot(const FString& Filename);
};
