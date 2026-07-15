#include "StageG1B/StageG1BMeshyAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1B/StageG1BSettings.h"

void UStageG1BMeshyAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
    const UStageG1BSettings* Settings = GetDefault<UStageG1BSettings>();
    WalkSpeed = Settings->WalkSpeed;
    RunSpeed = Settings->RunSpeed;
    DashSpeed = Settings->DashSpeed;
    HorizontalSpeed = 0.0f;
    bIsMoving = false;
    MoveMode = EStageG1BMoveMode::Walk;
    PlayRate = 1.0f;
    State = EStageG1BMeshyAnimState::Idle;
    StateName = TEXT("IDLE");
    ApplyAnimationAsset(IdleSequence, 1.0f);
}

void UStageG1BMeshyAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    const AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(TryGetPawnOwner());
    if (!Player)
    {
        HorizontalSpeed = 0.0f;
        bIsMoving = false;
        PlayRate = 1.0f;
        State = EStageG1BMeshyAnimState::Idle;
        StateName = TEXT("IDLE");
        ApplyAnimationAsset(IdleSequence, 1.0f);
        return;
    }

    const FVector Velocity = Player->GetVelocity();
    HorizontalSpeed = FVector(Velocity.X, Velocity.Y, 0.0f).Size();
    bIsMoving = HorizontalSpeed >= 8.0f;

    if (!bIsMoving)
    {
        State = EStageG1BMeshyAnimState::Idle;
        StateName = TEXT("IDLE");
        PlayRate = 1.0f;
        ApplyAnimationAsset(IdleSequence, PlayRate);
        return;
    }

    const UStageG1BSettings* Settings = GetDefault<UStageG1BSettings>();
    if (MoveMode == EStageG1BMoveMode::Dash)
    {
        State = EStageG1BMeshyAnimState::Dash;
        StateName = TEXT("DASH");
        PlayRate = FMath::Clamp(HorizontalSpeed / DashSpeed,
            Settings->AnimationMinPlayRate, Settings->AnimationMaxPlayRate);
        ApplyAnimationAsset(DashSequence, PlayRate);
    }
    else if (MoveMode == EStageG1BMoveMode::Run)
    {
        State = EStageG1BMeshyAnimState::Run;
        StateName = TEXT("RUN");
        PlayRate = FMath::Clamp(HorizontalSpeed / RunSpeed,
            Settings->AnimationMinPlayRate, Settings->AnimationMaxPlayRate);
        ApplyAnimationAsset(RunSequence, PlayRate);
    }
    else
    {
        State = EStageG1BMeshyAnimState::Walk;
        StateName = TEXT("WALK");
        PlayRate = FMath::Clamp(HorizontalSpeed / WalkSpeed,
            Settings->AnimationMinPlayRate, Settings->AnimationMaxPlayRate);
        ApplyAnimationAsset(WalkSequence, PlayRate);
    }
}

void UStageG1BMeshyAnimInstance::SetLocomotionSequences(
    UAnimSequence* InIdleSequence, UAnimSequence* InWalkSequence,
    UAnimSequence* InRunSequence, UAnimSequence* InDashSequence)
{
    IdleSequence = InIdleSequence;
    WalkSequence = InWalkSequence;
    RunSequence = InRunSequence;
    DashSequence = InDashSequence;
    ApplyAnimationAsset(IdleSequence, 1.0f);
}

void UStageG1BMeshyAnimInstance::SetMoveMode(EStageG1BMoveMode InMoveMode)
{
    MoveMode = InMoveMode;
}

FString UStageG1BMeshyAnimInstance::GetAnimationSourceName() const
{
    switch (State)
    {
    case EStageG1BMeshyAnimState::Idle: return TEXT("Idle_11");
    case EStageG1BMeshyAnimState::Walk: return TEXT("Walking");
    case EStageG1BMeshyAnimState::Run: return TEXT("Running");
    case EStageG1BMeshyAnimState::Dash: return TEXT("Run_02");
    default: return TEXT("NONE");
    }
}

float UStageG1BMeshyAnimInstance::GetConfiguredSpeed() const
{
    switch (MoveMode)
    {
    case EStageG1BMoveMode::Walk: return WalkSpeed;
    case EStageG1BMoveMode::Run: return RunSpeed;
    case EStageG1BMoveMode::Dash: return DashSpeed;
    default: return 0.0f;
    }
}

void UStageG1BMeshyAnimInstance::ApplyAnimationAsset(UAnimationAsset* NewAsset, float NewPlayRate)
{
    if (GetAnimationAsset() != NewAsset)
    {
        SetAnimationAsset(NewAsset, true, NewPlayRate);
    }
    SetLooping(true);
    SetPlaying(NewAsset != nullptr);
    SetPlayRate(NewPlayRate);
}
