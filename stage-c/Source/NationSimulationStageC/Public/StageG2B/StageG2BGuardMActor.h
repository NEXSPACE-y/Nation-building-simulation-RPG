#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG2BGuardMActor.generated.h"

class AStageG1APlayerCharacter;
class AStageG1BPlayerVisualAdapter;
class AStageG2ACameraModeAdapter;
class UAnimSequence;
class UCapsuleComponent;
class USkeletalMeshComponent;

/** G2B-only visual Guard. It never owns AI, causal state, save data, or Player assets. */
UCLASS(Blueprintable)
class NATIONSIMULATIONSTAGEC_API AStageG2BGuardMActor final : public AActor
{
    GENERATED_BODY()

public:
    AStageG2BGuardMActor();
    virtual void BeginPlay() override;

    USkeletalMeshComponent* GetGuardMeshComponent() const { return GuardMesh; }
    bool IsGuardVisualActive() const { return bGuardVisualActive; }
    bool IsUsingReferencePose() const { return bUsingReferencePose; }
    bool IsPlayerSkeletonShared() const { return bPlayerSkeletonShared; }
    UAnimSequence* GetIdleSequence() const { return IdleSequence; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> GuardCapsule;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> GuardMesh;
    UPROPERTY() TObjectPtr<UAnimSequence> IdleSequence;
    UPROPERTY() TWeakObjectPtr<AStageG1APlayerCharacter> AcceptedPlayer;
    UPROPERTY() TWeakObjectPtr<AStageG1BPlayerVisualAdapter> PlayerVisualAdapter;
    UPROPERTY() TWeakObjectPtr<AStageG2ACameraModeAdapter> CameraAdapter;

    bool bGuardVisualActive = false;
    bool bUsingReferencePose = false;
    bool bPlayerSkeletonShared = false;
    FVector OriginalGuardLocation = FVector::ZeroVector;
    FRotator OriginalGuardRotation = FRotator::ZeroRotator;
    FVector OriginalPlayerLocation = FVector::ZeroVector;
    FRotator OriginalPlayerRotation = FRotator::ZeroRotator;
    int32 ScreenshotStep = 0;
    FString PendingScreenshotFilename;
    FTimerHandle ScreenshotSequenceTimer;

    void ConfigureGuardVisual();
    void ResolveAcceptedAdapters();
    void WriteRuntimeEvidence();
    void BeginScreenshotSequence();
    void AdvanceScreenshotSequence();
    void CapturePendingScreenshot();
    void FinishScreenshotSequence();
    void RequestEvidenceScreenshot(const FString& Filename);
    void RestoreScreenshotTransforms();
};
