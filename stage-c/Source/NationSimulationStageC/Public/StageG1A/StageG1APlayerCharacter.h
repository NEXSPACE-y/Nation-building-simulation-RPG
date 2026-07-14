#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StageG0/StageG0Targetable.h"
#include "StageG1APlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EStageG1AMovementBand : uint8
{
    Idle,
    Walk,
    Run
};

UENUM(BlueprintType)
enum class EStageG1AMovementMode : uint8
{
    Walk,
    Run
};

USTRUCT(BlueprintType)
struct NATIONSIMULATIONSTAGEC_API FStageG1AMovementView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) EStageG1AMovementMode Mode = EStageG1AMovementMode::Walk;
    UPROPERTY(BlueprintReadOnly) EStageG1AMovementBand Band = EStageG1AMovementBand::Idle;
    UPROPERTY(BlueprintReadOnly) float Speed = 0.0f;
    UPROPERTY(BlueprintReadOnly) float PlayRate = 1.0f;
    UPROPERTY(BlueprintReadOnly) FVector Destination = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) bool bMoving = false;
    UPROPERTY(BlueprintReadOnly) bool bLeftHeld = false;
    UPROPERTY(BlueprintReadOnly) int32 DestinationUpdateCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 FallRecoveryCount = 0;
    UPROPERTY(BlueprintReadOnly) FString Status = TEXT("待機");
    UPROPERTY(BlueprintReadOnly) FString SelectedTargetId;
};

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG1APlayerCharacter final : public ACharacter
{
    GENERATED_BODY()

public:
    AStageG1APlayerCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    const FStageG1AMovementView& GetMovementView() const { return MovementView; }
    USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
    UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    EStageG1AMovementMode GetMovementMode() const { return MovementView.Mode; }
    int32 GetActivePathPointCount() const { return ActivePathPoints.Num(); }
    bool IsMoveCommandActive() const { return bMoveCommandActive; }
    void SetMovementMode(EStageG1AMovementMode NewMode);
    static EStageG1AMovementBand SelectMovementBand(float Speed, float WalkThreshold, float RunThreshold);
    static EStageG1AMovementBand SelectMovementBandForMode(
        float Speed, float MovementThreshold, EStageG1AMovementMode Mode);
    static float ResolveMovementSpeed(EStageG1AMovementMode Mode, float WalkSpeed, float RunSpeed);
    static float CalculateAnimationPlayRate(float Speed, float NominalSpeed, float MinRate, float MaxRate);
    static float CalculateModeAnimationPlayRate(float Speed, EStageG1AMovementMode Mode,
        float WalkSpeed, float RunSpeed, float MinRate, float MaxRate);
    static bool ShouldUpdateDestination(const FVector& Previous, const FVector& Next, float MinimumDistance);
    bool IssueMoveToLocation(const FVector& WorldLocation, bool bFromHold, FString& OutReason);

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FollowCamera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> DestinationMarker;
    UPROPERTY() TWeakObjectPtr<AActor> SelectedTarget;
    FStageG1AMovementView MovementView;
    FVector LastSafeLocation = FVector::ZeroVector;
    FVector ProgressSampleLocation = FVector::ZeroVector;
    TArray<FVector> ActivePathPoints;
    int32 ActivePathPointIndex = INDEX_NONE;
    float HoldAccumulator = 0.0f;
    float StuckAccumulator = 0.0f;
    bool bMoveCommandActive = false;
    bool bReplanAttempted = false;

    void BeginPointMove();
    void EndPointMove();
    void ZoomCamera(float Value);
    void ToggleDebug();
    bool IsPointerOverUi() const;
    bool TrySelectTargetUnderCursor();
    bool TryIssueMoveFromCursor(bool bFromHold);
    bool BuildActiveNavRoute(const FVector& WorldLocation, FVector& OutDestination, FString& OutReason);
    void StopMove(const FString& Status, bool bHideMarker);
    void TickMove(float DeltaSeconds);
    void UpdateMovementPresentation();
    void RecoverFromFallIfNeeded();
};
