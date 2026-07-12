#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StageG0/StageG0Targetable.h"
#include "StageDPlayerCharacter.generated.h"

class UCameraComponent;
class UStageGDirectionalFlipbookComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UNavigationPath;

USTRUCT(BlueprintType)
struct NATIONSIMULATIONSTAGEC_API FStageG0ClickDebugView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FVector ClickLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FString GroundResult = TEXT("未判定");
    UPROPERTY(BlueprintReadOnly) FVector NavigationLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector CurrentDestination = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FString PathStatus = TEXT("停止中");
    UPROPERTY(BlueprintReadOnly) int32 PathPointCount = 0;
    UPROPERTY(BlueprintReadOnly) bool bMoving = false;
    UPROPERTY(BlueprintReadOnly) bool bLeftHeld = false;
    UPROPERTY(BlueprintReadOnly) int32 DestinationUpdateCount = 0;
    UPROPERTY(BlueprintReadOnly) FString InvalidReason;
    UPROPERTY(BlueprintReadOnly) FStageG0TargetInfo Target;
    UPROPERTY(BlueprintReadOnly) bool bTargetActionPossible = false;
    UPROPERTY(BlueprintReadOnly) float TargetDistance = 0.0f;
    UPROPERTY(BlueprintReadOnly) bool bSameLocation = false;
    UPROPERTY(BlueprintReadOnly) FString TargetBlockedReason;
};

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageDPlayerCharacter final : public ACharacter
{
    GENERATED_BODY()

public:
    AStageDPlayerCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void ExecuteCoreAction(const FString& Action);
    void MoveToNextLocation();
    void CycleTarget();
    UStageGDirectionalFlipbookComponent* GetStageGVisual() const { return StageGVisual; }
    void ResetStageGCamera();
    void SetStageGCamera(float Yaw, float Pitch, float ArmLength);
    float GetStageGCameraPitch() const;
    float GetStageGCameraYaw() const;
    float GetStageGCameraArmLength() const;
    void SetBlobShadowVisible(bool bVisible);
    static float ClampStageGCameraPitch(float Pitch);
    static float ClampStageGCameraZoom(float ArmLength);
    static bool ShouldRecoverFromFall(float Z, float Threshold);
    static bool IsStageGPointTargetWalkable(const FVector& ImpactNormal, float WalkableFloorZ);
    bool IsStageGPointMoving() const { return ClickDebug.bMoving; }
    const FStageG0ClickDebugView& GetStageG0ClickDebugView() const { return ClickDebug; }
    static bool ShouldUpdateClickDestination(const FVector& Previous, const FVector& Next, float MinimumDistance);
    static int32 CompareTargetPriority(float DepthA, float CursorDistanceA, const FString& IdA,
        float DepthB, float CursorDistanceB, const FString& IdB);

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FollowCamera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PlaceholderBody;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStageGDirectionalFlipbookComponent> StageGVisual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BlobShadow;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PointMoveMarker;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UWidgetComponent> PointMoveMarkerLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> PlayerLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UWidgetComponent> StageGWorldLabel;
    float TargetScanAccumulator = 0.0f;
    float TargetLostSeconds = 0.0f;
    bool bManualTargetSelection = false;
    FString StageGPlayerAction;
    float StageGPlayerActionRemaining = 0.0f;
    bool bStageGPointDragActive = false;
    bool bStageGPointTargetValid = false;
    FVector StageGPointTarget = FVector::ZeroVector;
    FVector StageGPointSampleLocation = FVector::ZeroVector;
    float StageGPointStuckSeconds = 0.0f;
    float StageGPointHoldAccumulator = 0.0f;
    bool bStageGMoveCommandActive = false;
    bool bStageGReplanAttempted = false;
    TWeakObjectPtr<AActor> StageG0SelectedTarget;
    FStageG0TargetInfo StageG0SelectedTargetInfo;
    FStageG0ClickDebugView ClickDebug;
    FVector LastSafeLocation = FVector::ZeroVector;
    int32 FallRecoveryCount = 0;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void ZoomCamera(float Value);
    void BeginStageGPointDrag();
    void EndStageGPointDrag();
    bool IsPointerOverStageG0Ui() const;
    bool TrySelectStageG0TargetUnderCursor();
    bool TryIssueStageG0MoveFromCursor(bool bFromHold);
    bool ResolveStageG0MoveDestination(const FVector& RayOrigin, const FVector& RayDirection,
        FVector& OutDestination, int32& OutPathPointCount, FString& OutReason);
    void SetStageG0SelectedTarget(AActor* TargetActor, const FStageG0TargetInfo& TargetInfo);
    void StopStageG0Path(const FString& JapaneseReason, bool bHidePointer);
    void TickStageGPointMove(float DeltaSeconds);
    void Talk();
    void Help();
    void Harm();
    void Trade();
    void Steal();
    void WaitAction();
    void SaveSimulation();
    void LoadSimulation();
    void ToggleDebug();
    void ScanTarget();
    void UpdateBlobShadow();
    bool IsStageG0Map() const;
};
