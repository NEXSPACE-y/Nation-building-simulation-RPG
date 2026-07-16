#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG3AStandardCameraAutoFollowActor.generated.h"

class AStageG1APlayerCharacter;
class AStageG2ACameraModeAdapter;
class USceneComponent;

/**
 * G3A-only presentation adapter that restores movement-direction Yaw follow for
 * StandardCharacterCamera. TacticalOverlookCamera is deliberately excluded.
 */
UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG3AStandardCameraAutoFollowActor final : public AActor
{
    GENERATED_BODY()

public:
    AStageG3AStandardCameraAutoFollowActor();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    bool IsAutoFollowActive() const { return bAutoFollowActive; }
    bool IsManualOverrideActive() const { return bManualDragObservedNow || ManualOverrideRemaining > 0.0f; }
    float GetManualOverrideRemaining() const { return ManualOverrideRemaining; }
    float GetLastDesiredYaw() const { return LastDesiredYaw; }
    float GetManagedYaw() const { return ManagedYaw; }
    float GetLastVisualTargetYaw() const { return LastVisualTargetYaw; }
    int32 GetAutoFollowApplyCount() const { return AutoFollowApplyCount; }
    bool WasManualDragObserved() const { return bEvidenceManualDragObserved; }
    bool WasManualHoldObserved() const { return bEvidenceManualHoldObserved; }
    bool WasAutoFollowObserved() const { return bEvidenceAutoFollowObserved; }
    bool WasAutoFollowResumedAfterManual() const { return bEvidenceResumedAfterManual; }
    bool WasTacticalNoAutoFollowObserved() const { return bEvidenceTacticalNoFollowObserved; }
    bool WasReturnToStandardAutoFollowObserved() const { return bEvidenceReturnedStandardFollow; }
    float GetTacticalMaxUncommandedYawDelta() const { return TacticalMaxUncommandedYawDelta; }
    void SetManagedYawForEvidence(float Yaw);
    void WriteEvidenceReport() const;

    static float InterpolateYaw(float CurrentYaw, float DesiredYaw, float DeltaSeconds, float InterpSpeed);
    static bool ShouldBlockForUiInput(bool bPointerOverUi, bool bMouseInputActive)
    {
        return bPointerOverUi && bMouseInputActive;
    }

private:
    UPROPERTY() TWeakObjectPtr<AStageG1APlayerCharacter> AcceptedPlayer;
    UPROPERTY() TWeakObjectPtr<AStageG2ACameraModeAdapter> CameraAdapter;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> AutoFollowPivot;

    float ManualOverrideRemaining = 0.0f;
    float LastDesiredYaw = 0.0f;
    float ManagedYaw = 0.0f;
    float LastVisualTargetYaw = 0.0f;
    float TacticalEntryYaw = 0.0f;
    float TacticalMaxUncommandedYawDelta = 0.0f;
    int32 AutoFollowApplyCount = 0;
    int32 TacticalObservedFrames = 0;
    bool bWasRightDragActive = false;
    bool bWasTactical = false;
    bool bManagedYawInitialized = false;
    bool bStandardFollowSeenBeforeTactical = false;
    bool bManualDragObservedNow = false;
    bool bAutoFollowActive = false;
    bool bEvidenceManualDragObserved = false;
    bool bEvidenceManualHoldObserved = false;
    bool bEvidenceAutoFollowObserved = false;
    bool bEvidenceResumedAfterManual = false;
    bool bEvidenceTacticalNoFollowObserved = false;
    bool bEvidenceReturnedStandardFollow = false;

    void ResolveAcceptedActors();
    bool ResolveDesiredYaw(float& OutYaw) const;
    bool IsPointerOverUi() const;
    bool IsUiInputActive() const;
    void ApplyManagedYaw() const;
};
