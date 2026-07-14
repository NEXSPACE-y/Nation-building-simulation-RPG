#include "StageG1A/StageG1APlayerCharacter.h"

#include "Animation/AnimInstance.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "StageG1A/StageG1AGameMode.h"
#include "StageG1A/StageG1ASettings.h"
#include "UObject/ConstructorHelpers.h"

AStageG1APlayerCharacter::AStageG1APlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, Settings->RotationRateYaw, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = Settings->WalkSpeed;
    GetCharacterMovement()->MaxStepHeight = 55.0f;
    GetCharacterMovement()->SetWalkableFloorAngle(48.0f);
    GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;

    static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyMesh(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    if (MannyMesh.Succeeded()) GetMesh()->SetSkeletalMeshAsset(MannyMesh.Object);
    static ConstructorHelpers::FClassFinder<UAnimInstance> StandardAnim(
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
    if (StandardAnim.Succeeded()) GetMesh()->SetAnimInstanceClass(StandardAnim.Class);
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, Settings->MeshOffsetZ));
    GetMesh()->SetRelativeRotation(FRotator(0.0f, Settings->MeshYaw, 0.0f));
    GetMesh()->SetRelativeScale3D(FVector(Settings->CharacterScale));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCastShadow(true);
    GetMesh()->bCastDynamicShadow = true;
    GetMesh()->bCastContactShadow = true;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("StageG1ACameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = Settings->CameraArmLength;
    CameraBoom->SetRelativeRotation(FRotator(Settings->CameraPitch, Settings->CameraYaw, 0.0f));
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = false;
    CameraBoom->bInheritRoll = false;
    CameraBoom->bDoCollisionTest = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("StageG1AFollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;
    FollowCamera->SetFieldOfView(Settings->CameraFov);

    DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageG1ADestinationMarker"));
    DestinationMarker->SetupAttachment(GetRootComponent());
    DestinationMarker->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.02f));
    DestinationMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DestinationMarker->SetCastShadow(false);
    DestinationMarker->SetVisibility(false, true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> MarkerMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (MarkerMesh.Succeeded()) DestinationMarker->SetStaticMesh(MarkerMesh.Object);
    if (BaseMaterial.Succeeded())
    {
        if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(
            BaseMaterial.Object, this, TEXT("StageG1ADestinationMarkerMaterial")))
        {
            Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.02f, 0.85f, 1.0f, 1.0f));
            DestinationMarker->SetMaterial(0, Material);
        }
    }
}

void AStageG1APlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    SetMovementMode(EStageG1AMovementMode::Walk);
    DestinationMarker->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    LastSafeLocation = GetActorLocation();
    ProgressSampleLocation = LastSafeLocation;
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        PlayerController->bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        PlayerController->SetInputMode(InputMode);
    }
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1A_PLAYER_READY mesh=%s anim=%s capsule_radius=%.1f capsule_half_height=%.1f scale=%.2f"),
        GetMesh()->GetSkeletalMeshAsset() ? *GetMesh()->GetSkeletalMeshAsset()->GetPathName() : TEXT("NONE"),
        GetMesh()->GetAnimClass() ? *GetMesh()->GetAnimClass()->GetPathName() : TEXT("NONE"),
        GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight(),
        GetDefault<UStageG1ASettings>()->CharacterScale);
}

void AStageG1APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Stage G-1A intentionally has no MoveForward/MoveRight bindings. Normal and
    // automation movement both enter through the same NavMesh point-move path.
    PlayerInputComponent->BindAxis(TEXT("StageGCameraZoom"), this, &AStageG1APlayerCharacter::ZoomCamera);
    PlayerInputComponent->BindAction(TEXT("StageGPointDrag"), IE_Pressed, this, &AStageG1APlayerCharacter::BeginPointMove);
    PlayerInputComponent->BindAction(TEXT("StageGPointDrag"), IE_Released, this, &AStageG1APlayerCharacter::EndPointMove);
    PlayerInputComponent->BindAction(TEXT("ToggleDebug"), IE_Pressed, this, &AStageG1APlayerCharacter::ToggleDebug);
}

void AStageG1APlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (MovementView.bLeftHeld)
    {
        HoldAccumulator += DeltaSeconds;
        if (HoldAccumulator >= GetDefault<UStageG1ASettings>()->HoldUpdateSeconds)
        {
            HoldAccumulator = 0.0f;
            TryIssueMoveFromCursor(true);
        }
    }
    TickMove(DeltaSeconds);
    UpdateMovementPresentation();
    RecoverFromFallIfNeeded();
}

EStageG1AMovementBand AStageG1APlayerCharacter::SelectMovementBand(
    float Speed, float WalkThreshold, float RunThreshold)
{
    if (Speed < WalkThreshold) return EStageG1AMovementBand::Idle;
    return Speed < RunThreshold ? EStageG1AMovementBand::Walk : EStageG1AMovementBand::Run;
}

EStageG1AMovementBand AStageG1APlayerCharacter::SelectMovementBandForMode(
    float Speed, float MovementThreshold, EStageG1AMovementMode Mode)
{
    if (Speed < MovementThreshold) return EStageG1AMovementBand::Idle;
    return Mode == EStageG1AMovementMode::Walk
        ? EStageG1AMovementBand::Walk : EStageG1AMovementBand::Run;
}

float AStageG1APlayerCharacter::ResolveMovementSpeed(
    EStageG1AMovementMode Mode, float WalkSpeed, float RunSpeed)
{
    return Mode == EStageG1AMovementMode::Walk ? WalkSpeed : RunSpeed;
}

float AStageG1APlayerCharacter::CalculateAnimationPlayRate(
    float Speed, float NominalSpeed, float MinRate, float MaxRate)
{
    if (Speed < KINDA_SMALL_NUMBER || NominalSpeed <= KINDA_SMALL_NUMBER) return 1.0f;
    return FMath::Clamp(Speed / NominalSpeed, MinRate, MaxRate);
}

float AStageG1APlayerCharacter::CalculateModeAnimationPlayRate(
    float Speed, EStageG1AMovementMode Mode, float WalkSpeed, float RunSpeed, float MinRate, float MaxRate)
{
    return CalculateAnimationPlayRate(
        Speed, ResolveMovementSpeed(Mode, WalkSpeed, RunSpeed), MinRate, MaxRate);
}

void AStageG1APlayerCharacter::SetMovementMode(EStageG1AMovementMode NewMode)
{
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
    if (!MovementComponent) return;

    MovementView.Mode = NewMode;
    MovementComponent->MaxWalkSpeed = ResolveMovementSpeed(NewMode, Settings->WalkSpeed, Settings->RunSpeed);
    if (NewMode == EStageG1AMovementMode::Walk)
    {
        FVector HorizontalVelocity(MovementComponent->Velocity.X, MovementComponent->Velocity.Y, 0.0f);
        if (HorizontalVelocity.SizeSquared() > FMath::Square(Settings->WalkSpeed))
        {
            HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * Settings->WalkSpeed;
            MovementComponent->Velocity.X = HorizontalVelocity.X;
            MovementComponent->Velocity.Y = HorizontalVelocity.Y;
        }
    }
    UpdateMovementPresentation();
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1A_MOVEMENT_MODE mode=%s max_speed=%.1f destination=%s path_points=%d move_active=%d"),
        NewMode == EStageG1AMovementMode::Walk ? TEXT("WALK") : TEXT("RUN"),
        MovementComponent->MaxWalkSpeed, *MovementView.Destination.ToCompactString(),
        ActivePathPoints.Num(), bMoveCommandActive ? 1 : 0);
}

bool AStageG1APlayerCharacter::ShouldUpdateDestination(
    const FVector& Previous, const FVector& Next, float MinimumDistance)
{
    return FVector::DistSquared2D(Previous, Next) >= FMath::Square(MinimumDistance);
}

void AStageG1APlayerCharacter::BeginPointMove()
{
    MovementView.bLeftHeld = true;
    if (IsPointerOverUi())
    {
        MovementView.Status = TEXT("UI入力をゲーム移動へ送信しません");
        MovementView.bLeftHeld = false;
        return;
    }
    if (TrySelectTargetUnderCursor())
    {
        MovementView.bLeftHeld = false;
        return;
    }
    HoldAccumulator = 0.0f;
    TryIssueMoveFromCursor(false);
}

void AStageG1APlayerCharacter::EndPointMove()
{
    MovementView.bLeftHeld = false;
}

void AStageG1APlayerCharacter::ZoomCamera(float Value)
{
    if (Value == 0.0f || !CameraBoom) return;
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    CameraBoom->TargetArmLength = FMath::Clamp(
        CameraBoom->TargetArmLength - Value * Settings->CameraZoomStep,
        Settings->CameraMinArmLength, Settings->CameraMaxArmLength);
}

void AStageG1APlayerCharacter::ToggleDebug()
{
    if (AStageG1AGameMode* GameMode = GetWorld()->GetAuthGameMode<AStageG1AGameMode>())
        GameMode->ToggleDebugHud();
}

bool AStageG1APlayerCharacter::IsPointerOverUi() const
{
    if (!FSlateApplication::IsInitialized()) return false;
    FSlateApplication& Slate = FSlateApplication::Get();
    const FWidgetPath Path = Slate.LocateWindowUnderMouse(
        Slate.GetCursorPos(), Slate.GetInteractiveTopLevelWindows(), true);
    if (!Path.IsValid()) return false;
    for (int32 Index = 0; Index < Path.Widgets.Num(); ++Index)
    {
        const FString Type = Path.Widgets[Index].Widget->GetTypeAsString();
        if (Type == TEXT("SObjectWidget") || Type == TEXT("SButton") ||
            Type == TEXT("SCheckBox") || Type == TEXT("SComboButton")) return true;
    }
    return false;
}

bool AStageG1APlayerCharacter::TrySelectTargetUnderCursor()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !GetWorld()) return false;
    FVector RayOrigin;
    FVector RayDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection)) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG1ATarget), false, this);
    if (!GetWorld()->LineTraceSingleByChannel(
        Hit, RayOrigin, RayOrigin + RayDirection * 100000.0f, ECC_GameTraceChannel2, Params)) return false;
    AActor* Actor = Hit.GetActor();
    IStageG0Targetable* Targetable = Cast<IStageG0Targetable>(Actor);
    if (!Actor || !Targetable) return false;
    if (AActor* Previous = SelectedTarget.Get())
        if (IStageG0Targetable* PreviousTargetable = Cast<IStageG0Targetable>(Previous))
            PreviousTargetable->SetStageG0TargetSelected(false);
    const FStageG0TargetInfo Info = Targetable->GetStageG0TargetInfo();
    if (!Info.bIsActive || !Info.bIsTargetable) return false;
    SelectedTarget = Actor;
    Targetable->SetStageG0TargetSelected(true);
    MovementView.SelectedTargetId = Info.TargetId;
    MovementView.Status = FString::Printf(TEXT("対象選択: %s"), *Info.DisplayNameJa);
    UE_LOG(LogTemp, Display, TEXT("STAGE_G1A_TARGET_SELECTED id=%s type=%s move_issued=0"),
        *Info.TargetId, *Info.TargetType);
    return true;
}

bool AStageG1APlayerCharacter::TryIssueMoveFromCursor(bool bFromHold)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !GetWorld()) return false;
    FVector RayOrigin;
    FVector RayDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection)) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG1AGround), false, this);
    if (!GetWorld()->LineTraceSingleByChannel(
        Hit, RayOrigin, RayOrigin + RayDirection * 100000.0f, ECC_GameTraceChannel1, Params))
    {
        MovementView.Status = TEXT("移動不可: 通行可能地面ではありません");
        return false;
    }
    FString Reason;
    return IssueMoveToLocation(Hit.ImpactPoint, bFromHold, Reason);
}

bool AStageG1APlayerCharacter::IssueMoveToLocation(
    const FVector& WorldLocation, bool bFromHold, FString& OutReason)
{
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    if (FMath::Abs(WorldLocation.X) > Settings->WorldBounds ||
        FMath::Abs(WorldLocation.Y) > Settings->WorldBounds || WorldLocation.Z < Settings->FallRecoveryZ)
    {
        OutReason = TEXT("移動不可: G-1A検証範囲外です");
        MovementView.Status = OutReason;
        return false;
    }
    if (bFromHold && bMoveCommandActive && !ShouldUpdateDestination(
        MovementView.Destination, WorldLocation, Settings->DestinationMinUpdateDistance)) return false;
    FVector Destination;
    if (!BuildActiveNavRoute(WorldLocation, Destination, OutReason))
    {
        MovementView.Status = OutReason;
        return false;
    }
    MovementView.Destination = Destination;
    MovementView.bMoving = true;
    MovementView.Status = FString::Printf(TEXT("NavMesh経路追従中 (%d points)"), ActivePathPoints.Num());
    ++MovementView.DestinationUpdateCount;
    bMoveCommandActive = true;
    bReplanAttempted = false;
    ProgressSampleLocation = GetActorLocation();
    StuckAccumulator = 0.0f;
    DestinationMarker->SetWorldLocation(Destination + FVector(0.0f, 0.0f, 3.0f));
    DestinationMarker->SetWorldRotation(FRotator::ZeroRotator);
    DestinationMarker->SetVisibility(true, true);
    OutReason.Reset();
    UE_LOG(LogTemp, Display, TEXT("STAGE_G1A_CLICK_MOVE destination=%s path_points=%d hold_update=%d driver=CharacterMovement"),
        *Destination.ToCompactString(), ActivePathPoints.Num(), bFromHold ? 1 : 0);
    return true;
}

bool AStageG1APlayerCharacter::BuildActiveNavRoute(
    const FVector& WorldLocation, FVector& OutDestination, FString& OutReason)
{
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation Start;
    FNavLocation Destination;
    if (!Navigation || !Navigation->ProjectPointToNavigation(
            GetActorLocation(), Start, FVector(100.0f, 100.0f, 300.0f)) ||
        !Navigation->ProjectPointToNavigation(
            WorldLocation, Destination, FVector(100.0f, 100.0f, 300.0f)))
    {
        OutReason = TEXT("移動不可: NavMesh外です");
        return false;
    }
    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        GetWorld(), Start.Location, Destination.Location, this);
    if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num() < 2)
    {
        OutReason = TEXT("移動不可: 完全経路がありません");
        return false;
    }
    ActivePathPoints = Path->PathPoints;
    ActivePathPointIndex = ActivePathPoints.Num() > 1 ? 1 : INDEX_NONE;
    OutDestination = Destination.Location;
    OutReason.Reset();
    return true;
}

void AStageG1APlayerCharacter::StopMove(const FString& Status, bool bHideMarker)
{
    if (UPathFollowingComponent* PathFollowing = GetController()
        ? GetController()->FindComponentByClass<UPathFollowingComponent>() : nullptr)
        PathFollowing->AbortMove(*this, FPathFollowingResultFlags::ForcedScript);
    if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
    bMoveCommandActive = false;
    ActivePathPoints.Reset();
    ActivePathPointIndex = INDEX_NONE;
    MovementView.bMoving = false;
    MovementView.Status = Status;
    if (bHideMarker) DestinationMarker->SetVisibility(false, true);
}

void AStageG1APlayerCharacter::TickMove(float DeltaSeconds)
{
    if (!bMoveCommandActive) return;
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    if (FVector::Dist2D(GetActorLocation(), MovementView.Destination) <= Settings->ArrivalTolerance)
    {
        StopMove(TEXT("目的地へ到着"), true);
        return;
    }
    while (ActivePathPoints.IsValidIndex(ActivePathPointIndex) &&
        FVector::Dist2D(GetActorLocation(), ActivePathPoints[ActivePathPointIndex]) <= Settings->ArrivalTolerance)
    {
        ++ActivePathPointIndex;
    }
    if (!ActivePathPoints.IsValidIndex(ActivePathPointIndex))
    {
        StopMove(TEXT("目的地へ到着"), true);
        return;
    }
    FVector MoveDirection = ActivePathPoints[ActivePathPointIndex] - GetActorLocation();
    MoveDirection.Z = 0.0f;
    if (!MoveDirection.IsNearlyZero()) AddMovementInput(MoveDirection.GetSafeNormal(), 1.0f);
    if (FVector::Dist2D(GetActorLocation(), ProgressSampleLocation) >= 14.0f)
    {
        ProgressSampleLocation = GetActorLocation();
        StuckAccumulator = 0.0f;
    }
    else if ((StuckAccumulator += DeltaSeconds) >= Settings->StuckSeconds)
    {
        StuckAccumulator = 0.0f;
        if (!bReplanAttempted)
        {
            bReplanAttempted = true;
            FVector ReplannedDestination;
            FString ReplanReason;
            if (BuildActiveNavRoute(MovementView.Destination, ReplannedDestination, ReplanReason))
            {
                MovementView.Destination = ReplannedDestination;
                MovementView.Status = TEXT("停止検知: NavMesh経路を1回再計算");
            }
            else StopMove(ReplanReason, true);
        }
        else StopMove(TEXT("移動不能: 再計算後も進めないため停止"), true);
    }
}

void AStageG1APlayerCharacter::UpdateMovementPresentation()
{
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    MovementView.Speed = GetVelocity().Size2D();
    MovementView.Band = SelectMovementBandForMode(
        MovementView.Speed, Settings->WalkStateThreshold, MovementView.Mode);
    MovementView.PlayRate = MovementView.Band == EStageG1AMovementBand::Idle ? 1.0f :
        CalculateModeAnimationPlayRate(MovementView.Speed, MovementView.Mode,
            Settings->WalkSpeed, Settings->RunSpeed,
            Settings->AnimationMinPlayRate, Settings->AnimationMaxPlayRate);
    GetMesh()->GlobalAnimRateScale = MovementView.PlayRate;
}

void AStageG1APlayerCharacter::RecoverFromFallIfNeeded()
{
    if (GetCharacterMovement()->IsMovingOnGround() && GetActorLocation().Z > 0.0f)
        LastSafeLocation = GetActorLocation();
    if (GetActorLocation().Z >= GetDefault<UStageG1ASettings>()->FallRecoveryZ) return;
    SetActorLocation(LastSafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
    GetCharacterMovement()->StopMovementImmediately();
    ++MovementView.FallRecoveryCount;
    StopMove(TEXT("落下を検知し最終安全地点へ復帰"), true);
    UE_LOG(LogTemp, Warning, TEXT("STAGE_G1A_FALL_RECOVERY count=%d location=%s"),
        MovementView.FallRecoveryCount, *LastSafeLocation.ToCompactString());
}
