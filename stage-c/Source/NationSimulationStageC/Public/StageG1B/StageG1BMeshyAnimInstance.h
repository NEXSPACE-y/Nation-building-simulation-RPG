#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "StageG1BMeshyAnimInstance.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class EStageG1BMeshyAnimState : uint8
{
    Idle,
    Walk,
    Run,
    Dash
};

UENUM(BlueprintType)
enum class EStageG1BMoveMode : uint8
{
    Walk,
    Run,
    Dash
};

/**
 * Meshy-skeleton locomotion driver.  The generated Stage G-1B Animation
 * Blueprint derives from this class, while its native single-sequence proxy
 * keeps Manny's animation graph completely out of the runtime path.
 */
UCLASS(Blueprintable, BlueprintType)
class NATIONSIMULATIONSTAGEC_API UStageG1BMeshyAnimInstance : public UAnimSingleNodeInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    void SetLocomotionSequences(UAnimSequence* InIdleSequence, UAnimSequence* InWalkSequence,
        UAnimSequence* InRunSequence, UAnimSequence* InDashSequence);
    void SetMoveMode(EStageG1BMoveMode InMoveMode);

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    float HorizontalSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    bool bIsMoving = false;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    EStageG1BMoveMode MoveMode = EStageG1BMoveMode::Walk;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    float WalkSpeed = 250.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    float RunSpeed = 500.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    float DashSpeed = 750.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    float PlayRate = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    EStageG1BMeshyAnimState State = EStageG1BMeshyAnimState::Idle;

    UPROPERTY(BlueprintReadOnly, Category = "Stage G-1B|Locomotion")
    FString StateName = TEXT("IDLE");

    UAnimSequence* GetIdleSequence() const { return IdleSequence; }
    UAnimSequence* GetWalkSequence() const { return WalkSequence; }
    UAnimSequence* GetRunSequence() const { return RunSequence; }
    UAnimSequence* GetDashSequence() const { return DashSequence; }
    UAnimationAsset* GetActiveAnimationAsset() const { return GetAnimationAsset(); }
    FString GetAnimationSourceName() const;
    float GetConfiguredSpeed() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> IdleSequence;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> WalkSequence;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> RunSequence;

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequence> DashSequence;

    void ApplyAnimationAsset(UAnimationAsset* NewAsset, float NewPlayRate);
};
