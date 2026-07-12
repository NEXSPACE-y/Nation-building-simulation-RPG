#include "StageD/StageDPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDGameMode.h"
#include "StageD/StageDHudWidget.h"
#include "StageD/StageDNpcActor.h"
#include "StageG0/StageG0Settings.h"
#include "StageG0/StageG0FangRatActor.h"
#include "StageG0/StageGDirectionalFlipbookComponent.h"
#include "StageG0/StageG0WorldLabelWidget.h"
#include "StageG0/StageG0Targetable.h"
#include "UObject/ConstructorHelpers.h"

AStageDPlayerCharacter::AStageDPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetCapsuleComponent()->SetCanEverAffectNavigation(false);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 600.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    const UStageG0Settings* VisualSettings = GetDefault<UStageG0Settings>();
    CameraBoom->TargetArmLength = VisualSettings->CameraArmLength;
    CameraBoom->SetRelativeRotation(FRotator(VisualSettings->CameraPitch, VisualSettings->CameraYaw, 0.0f));
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = false;
    CameraBoom->bInheritRoll = false;
    CameraBoom->bDoCollisionTest = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(GetRootComponent());
    PlaceholderBody->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
    PlaceholderBody->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.95f));
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CapsuleMesh.Succeeded()) PlaceholderBody->SetStaticMesh(CapsuleMesh.Object);
    PlaceholderBody->SetHiddenInGame(true);

    StageGVisual = CreateDefaultSubobject<UStageGDirectionalFlipbookComponent>(TEXT("StageGDirectionalVisual"));
    StageGVisual->SetupAttachment(GetRootComponent());
    StageGVisual->SetRelativeLocation(FVector::ZeroVector);
    StageGVisual->ConfigureVisual(TEXT("player"), TEXT("PLAYER"), false);

    BlobShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageGBlobShadow"));
    BlobShadow->SetupAttachment(GetRootComponent());
    BlobShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -94.0f));
    BlobShadow->SetRelativeScale3D(FVector(0.55f, 0.36f, 0.018f));
    BlobShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlobShadow->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ShadowCylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShadowBaseMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (ShadowCylinder.Succeeded()) BlobShadow->SetStaticMesh(ShadowCylinder.Object);
    BlobShadow->ComponentTags.Add(TEXT("StageG0BlobShadow"));
    if (ShadowBaseMaterial.Succeeded())
    {
        UMaterialInstanceDynamic* ShadowMaterial = UMaterialInstanceDynamic::Create(
            ShadowBaseMaterial.Object, this, TEXT("StageGPlayerShadowMaterial"));
        if (ShadowMaterial)
        {
            ShadowMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.015f, 0.015f, 0.02f, 1.0f));
            BlobShadow->SetMaterial(0, ShadowMaterial);
        }
    }

    PointMoveMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageG0PointMoveMarker"));
    PointMoveMarker->SetupAttachment(GetRootComponent());
    PointMoveMarker->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.018f));
    PointMoveMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PointMoveMarker->SetCastShadow(false);
    PointMoveMarker->SetVisibility(false, true);
    if (ShadowCylinder.Succeeded()) PointMoveMarker->SetStaticMesh(ShadowCylinder.Object);
    if (ShadowBaseMaterial.Succeeded())
    {
        UMaterialInstanceDynamic* MarkerMaterial = UMaterialInstanceDynamic::Create(
            ShadowBaseMaterial.Object, this, TEXT("StageG0PointMoveMarkerMaterial"));
        if (MarkerMaterial)
        {
            MarkerMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 0.85f, 1.0f, 1.0f));
            PointMoveMarker->SetMaterial(0, MarkerMaterial);
        }
    }

    PointMoveMarkerLabel = CreateDefaultSubobject<UWidgetComponent>(TEXT("StageG0PointMoveMarkerLabel"));
    PointMoveMarkerLabel->SetupAttachment(PointMoveMarker);
    PointMoveMarkerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 4200.0f));
    PointMoveMarkerLabel->SetAbsolute(false, false, true);
    PointMoveMarkerLabel->SetWidgetSpace(EWidgetSpace::Screen);
    PointMoveMarkerLabel->SetDrawSize(FVector2D(180.0f, 42.0f));
    PointMoveMarkerLabel->SetPivot(FVector2D(0.5f, 0.5f));
    PointMoveMarkerLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PointMoveMarkerLabel->SetWidgetClass(UStageG0WorldLabelWidget::StaticClass());

    PlayerLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StageGPlayerLabel"));
    PlayerLabel->SetupAttachment(GetRootComponent());
    PlayerLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 135.0f));
    PlayerLabel->SetHorizontalAlignment(EHTA_Center);
    PlayerLabel->SetWorldSize(18.0f);
    PlayerLabel->SetTextRenderColor(FColor(255, 225, 80));
    PlayerLabel->SetText(FText::FromString(TEXT("プレイヤー")));

    StageGWorldLabel = CreateDefaultSubobject<UWidgetComponent>(TEXT("StageG0JapaneseWorldLabel"));
    StageGWorldLabel->SetupAttachment(GetRootComponent());
    StageGWorldLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 150.0f));
    StageGWorldLabel->SetWidgetSpace(EWidgetSpace::Screen);
    StageGWorldLabel->SetDrawSize(FVector2D(320.0f, 52.0f));
    StageGWorldLabel->SetPivot(FVector2D(0.5f, 0.5f));
    StageGWorldLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StageGWorldLabel->SetWidgetClass(UStageG0WorldLabelWidget::StaticClass());
}

void AStageDPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    const bool bStageG0Map = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("StageG0_VisualPoC"));
    StageGWorldLabel->SetVisibility(bStageG0Map);
    PlayerLabel->SetHiddenInGame(bStageG0Map);
    if (bStageG0Map)
    {
        CameraBoom->bDoCollisionTest = false;
        GetCharacterMovement()->MaxStepHeight = 55.0f;
        GetCharacterMovement()->SetWalkableFloorAngle(50.0f);
        GetCharacterMovement()->bUseFlatBaseForFloorChecks = true;
        GetCapsuleComponent()->SetCanEverAffectNavigation(false);
        PointMoveMarker->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        PointMoveMarkerLabel->InitWidget();
        if (auto* MarkerLabel = Cast<UStageG0WorldLabelWidget>(PointMoveMarkerLabel->GetUserWidgetObject()))
            MarkerLabel->SetWorldLabel(TEXT("移動先"), FLinearColor(0.0f, 0.9f, 1.0f), 18);
        StageGWorldLabel->InitWidget();
        if (auto* Label = Cast<UStageG0WorldLabelWidget>(StageGWorldLabel->GetUserWidgetObject()))
            Label->SetWorldLabel(TEXT("プレイヤー player"), FLinearColor(1.0f, 0.85f, 0.2f), 20);
    }
    LastSafeLocation = GetActorLocation();
    if (PlayerLabel) PlayerLabel->SetHiddenInGame(IsStageG0Map());
}

void AStageDPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AStageDPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AStageDPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("StageGCameraZoom"), this, &AStageDPlayerCharacter::ZoomCamera);
    PlayerInputComponent->BindAction(TEXT("StageGPointDrag"), IE_Pressed, this, &AStageDPlayerCharacter::BeginStageGPointDrag);
    PlayerInputComponent->BindAction(TEXT("StageGPointDrag"), IE_Released, this, &AStageDPlayerCharacter::EndStageGPointDrag);
    PlayerInputComponent->BindAction(TEXT("Talk"), IE_Pressed, this, &AStageDPlayerCharacter::Talk);
    PlayerInputComponent->BindAction(TEXT("Help"), IE_Pressed, this, &AStageDPlayerCharacter::Help);
    PlayerInputComponent->BindAction(TEXT("Harm"), IE_Pressed, this, &AStageDPlayerCharacter::Harm);
    PlayerInputComponent->BindAction(TEXT("Trade"), IE_Pressed, this, &AStageDPlayerCharacter::Trade);
    PlayerInputComponent->BindAction(TEXT("Steal"), IE_Pressed, this, &AStageDPlayerCharacter::Steal);
    PlayerInputComponent->BindAction(TEXT("Wait"), IE_Pressed, this, &AStageDPlayerCharacter::WaitAction);
    PlayerInputComponent->BindAction(TEXT("MoveLocation"), IE_Pressed, this, &AStageDPlayerCharacter::MoveToNextLocation);
    PlayerInputComponent->BindAction(TEXT("CycleTarget"), IE_Pressed, this, &AStageDPlayerCharacter::CycleTarget);
    PlayerInputComponent->BindAction(TEXT("SaveSimulation"), IE_Pressed, this, &AStageDPlayerCharacter::SaveSimulation);
    PlayerInputComponent->BindAction(TEXT("LoadSimulation"), IE_Pressed, this, &AStageDPlayerCharacter::LoadSimulation);
    PlayerInputComponent->BindAction(TEXT("ToggleDebug"), IE_Pressed, this, &AStageDPlayerCharacter::ToggleDebug);
}

void AStageDPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsStageG0Map())
    {
        if (PlayerLabel)
        {
            if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
                PlayerLabel->SetWorldRotation(FRotator(0.0f,
                    (Camera->GetCameraLocation() - PlayerLabel->GetComponentLocation()).Rotation().Yaw, 0.0f));
        }
        UpdateBlobShadow();
        if (bStageGPointDragActive)
        {
            StageGPointHoldAccumulator += DeltaSeconds;
            if (StageGPointHoldAccumulator >= GetDefault<UStageG0Settings>()->ClickHoldUpdateSeconds)
            {
                StageGPointHoldAccumulator = 0.0f;
                TryIssueStageG0MoveFromCursor(true);
            }
        }
        TickStageGPointMove(DeltaSeconds);
        if (AActor* Selected = StageG0SelectedTarget.Get())
        {
            if (IStageG0Targetable* Targetable = Cast<IStageG0Targetable>(Selected))
            {
                StageG0SelectedTargetInfo = Targetable->GetStageG0TargetInfo();
                ClickDebug.Target = StageG0SelectedTargetInfo;
                ClickDebug.TargetDistance = FVector::Distance(
                    GetActorLocation(), StageG0SelectedTargetInfo.InteractionOrigin);
                if (auto* Subsystem = GetGameInstance()
                    ? GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>() : nullptr)
                {
                    ClickDebug.bSameLocation = Subsystem->GetWorldView().CurrentLocationId ==
                        StageG0SelectedTargetInfo.LocationId;
                    ClickDebug.bTargetActionPossible = StageG0SelectedTargetInfo.TargetType != TEXT("FANG_RAT") &&
                        StageG0SelectedTargetInfo.bIsActive && StageG0SelectedTargetInfo.bIsTargetable &&
                        ClickDebug.bSameLocation && ClickDebug.TargetDistance <= Subsystem->GetInteractionRange();
                    ClickDebug.TargetBlockedReason = StageG0SelectedTargetInfo.TargetType == TEXT("FANG_RAT")
                        ? TEXT("牙鼠はStage G-0表示fixture専用です")
                        : !StageG0SelectedTargetInfo.bIsActive ? TEXT("対象は無効です")
                        : !ClickDebug.bSameLocation ? TEXT("同一地点ではありません")
                        : ClickDebug.TargetDistance > Subsystem->GetInteractionRange() ? TEXT("行動範囲外です")
                        : TEXT("");
                }
            }
        }
        if (GetCharacterMovement()->IsMovingOnGround() &&
            !ShouldRecoverFromFall(GetActorLocation().Z, GetDefault<UStageG0Settings>()->FallRecoveryZ))
            LastSafeLocation = GetActorLocation();
        if (ShouldRecoverFromFall(GetActorLocation().Z, GetDefault<UStageG0Settings>()->FallRecoveryZ))
        {
            SetActorLocation(LastSafeLocation, false, nullptr, ETeleportType::TeleportPhysics);
            GetCharacterMovement()->StopMovementImmediately();
            ++FallRecoveryCount;
            if (AStageDGameMode* GameMode = GetWorld()->GetAuthGameMode<AStageDGameMode>())
                GameMode->NotifyStageG0FallRecovery(FallRecoveryCount, LastSafeLocation);
        }
    }
    if (StageGVisual)
    {
        if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
        {
            StageGPlayerActionRemaining = FMath::Max(0.0f, StageGPlayerActionRemaining - DeltaSeconds);
            const FString VisualAction = StageGPlayerActionRemaining > 0.0f ? StageGPlayerAction : TEXT("MOVE");
            StageGVisual->UpdatePresentation(GetVelocity(), VisualAction,
                Camera->GetCameraLocation(), Camera->GetCameraRotation(), DeltaSeconds);
        }
    }
    TargetScanAccumulator += DeltaSeconds;
    if (TargetScanAccumulator >= 0.15f)
    {
        TargetScanAccumulator = 0.0f;
        ScanTarget();
    }
}

void AStageDPlayerCharacter::MoveForward(float Value)
{
    if (IsStageG0Map()) return;
    if (FollowCamera && Value != 0.0f)
    {
        FVector Direction = FollowCamera->GetForwardVector();
        Direction.Z = 0.0f;
        AddMovementInput(Direction.GetSafeNormal(), Value);
    }
}

void AStageDPlayerCharacter::MoveRight(float Value)
{
    if (IsStageG0Map()) return;
    if (FollowCamera && Value != 0.0f)
    {
        FVector Direction = FollowCamera->GetRightVector();
        Direction.Z = 0.0f;
        AddMovementInput(Direction.GetSafeNormal(), Value);
    }
}

void AStageDPlayerCharacter::ZoomCamera(float Value)
{
    if (!IsStageG0Map() || !CameraBoom || Value == 0.0f) return;
    SetStageGCamera(CameraBoom->GetRelativeRotation().Yaw, CameraBoom->GetRelativeRotation().Pitch,
        CameraBoom->TargetArmLength - Value * GetDefault<UStageG0Settings>()->CameraZoomStep);
}

void AStageDPlayerCharacter::BeginStageGPointDrag()
{
    if (!IsStageG0Map()) return;
    ClickDebug.bLeftHeld = true;
    if (IsPointerOverStageG0Ui())
    {
        ClickDebug.GroundResult = TEXT("UI入力");
        ClickDebug.InvalidReason = TEXT("UI上の操作はゲーム内へ送信されません");
        return;
    }
    if (TrySelectStageG0TargetUnderCursor())
    {
        bStageGPointDragActive = false;
        return;
    }
    bStageGPointDragActive = true;
    StageGPointHoldAccumulator = 0.0f;
    StageGPointSampleLocation = GetActorLocation();
    StageGPointStuckSeconds = 0.0f;
    TryIssueStageG0MoveFromCursor(false);
}

void AStageDPlayerCharacter::EndStageGPointDrag()
{
    if (!IsStageG0Map()) return;
    bStageGPointDragActive = false;
    ClickDebug.bLeftHeld = false;
    if (!GetDefault<UStageG0Settings>()->bContinueToDestinationOnRelease)
        StopStageG0Path(TEXT("左クリック解放により停止しました"), true);
}

bool AStageDPlayerCharacter::IsStageGPointTargetWalkable(
    const FVector& ImpactNormal, float WalkableFloorZ)
{
    return ImpactNormal.GetSafeNormal().Z >= WalkableFloorZ;
}

bool AStageDPlayerCharacter::ShouldUpdateClickDestination(
    const FVector& Previous, const FVector& Next, float MinimumDistance)
{
    return FVector::DistSquared(Previous, Next) >= FMath::Square(MinimumDistance);
}

int32 AStageDPlayerCharacter::CompareTargetPriority(
    float DepthA, float CursorDistanceA, const FString& IdA,
    float DepthB, float CursorDistanceB, const FString& IdB)
{
    if (!FMath::IsNearlyEqual(DepthA, DepthB, 0.1f)) return DepthA < DepthB ? -1 : 1;
    if (!FMath::IsNearlyEqual(CursorDistanceA, CursorDistanceB, 0.1f)) return CursorDistanceA < CursorDistanceB ? -1 : 1;
    return IdA.Compare(IdB, ESearchCase::CaseSensitive);
}

bool AStageDPlayerCharacter::IsPointerOverStageG0Ui() const
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
            Type == TEXT("SCheckBox") || Type == TEXT("SComboButton"))
            return true;
    }
    return false;
}

bool AStageDPlayerCharacter::TrySelectStageG0TargetUnderCursor()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !GetWorld()) return false;
    FVector RayOrigin;
    FVector RayDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection)) return false;
    float MouseX = 0.0f;
    float MouseY = 0.0f;
    PlayerController->GetMousePosition(MouseX, MouseY);
    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG0ClickTargetable), false, this);
    GetWorld()->LineTraceMultiByChannel(Hits, RayOrigin, RayOrigin + RayDirection * 100000.0f,
        ECC_GameTraceChannel2, Params);

    struct FCandidate
    {
        TWeakObjectPtr<AActor> Actor;
        FStageG0TargetInfo Info;
        float Depth = 0.0f;
        float CursorDistance = 0.0f;
    };
    TArray<FCandidate> Candidates;
    TSet<TWeakObjectPtr<AActor>> Seen;
    for (const FHitResult& Hit : Hits)
    {
        AActor* Actor = Hit.GetActor();
        if (!Actor || Seen.Contains(Actor)) continue;
        IStageG0Targetable* Targetable = Cast<IStageG0Targetable>(Actor);
        if (!Targetable) continue;
        const FStageG0TargetInfo Info = Targetable->GetStageG0TargetInfo();
        if (!Info.bIsActive || !Info.bIsTargetable) continue;
        Seen.Add(Actor);
        FVector2D Screen;
        PlayerController->ProjectWorldLocationToScreen(Info.InteractionOrigin, Screen, true);
        FCandidate Candidate;
        Candidate.Actor = Actor;
        Candidate.Info = Info;
        Candidate.Depth = Hit.Distance;
        Candidate.CursorDistance = FVector2D::Distance(Screen, FVector2D(MouseX, MouseY));
        Candidates.Add(MoveTemp(Candidate));
    }
    Candidates.Sort([](const FCandidate& A, const FCandidate& B)
    {
        return AStageDPlayerCharacter::CompareTargetPriority(
            A.Depth, A.CursorDistance, A.Info.TargetId,
            B.Depth, B.CursorDistance, B.Info.TargetId) < 0;
    });
    if (Candidates.IsEmpty()) return false;
    SetStageG0SelectedTarget(Candidates[0].Actor.Get(), Candidates[0].Info);
    ClickDebug.GroundResult = TEXT("対象選択");
    ClickDebug.InvalidReason.Reset();
    return true;
}

void AStageDPlayerCharacter::SetStageG0SelectedTarget(
    AActor* TargetActor, const FStageG0TargetInfo& TargetInfo)
{
    if (AActor* Previous = StageG0SelectedTarget.Get())
        if (IStageG0Targetable* PreviousTargetable = Cast<IStageG0Targetable>(Previous))
            PreviousTargetable->SetStageG0TargetSelected(false);
    StageG0SelectedTarget = TargetActor;
    StageG0SelectedTargetInfo = TargetInfo;
    ClickDebug.Target = TargetInfo;
    if (IStageG0Targetable* Targetable = Cast<IStageG0Targetable>(TargetActor))
        Targetable->SetStageG0TargetSelected(true);

    bManualTargetSelection = true;
    ClickDebug.TargetDistance = TargetActor ? FVector::Distance(GetActorLocation(), TargetInfo.InteractionOrigin) : 0.0f;
    ClickDebug.bSameLocation = false;
    ClickDebug.bTargetActionPossible = false;
    ClickDebug.TargetBlockedReason.Reset();
    if (GetGameInstance())
    {
        if (auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>())
        {
            ClickDebug.bSameLocation = Subsystem->GetWorldView().CurrentLocationId == TargetInfo.LocationId;
            if (TargetInfo.TargetType == TEXT("FANG_RAT"))
                ClickDebug.TargetBlockedReason = TEXT("牙鼠はStage G-0表示fixture専用です");
            else
            {
                Subsystem->SetTargetNpc(TargetInfo.TargetId);
                ClickDebug.bTargetActionPossible = ClickDebug.bSameLocation &&
                    ClickDebug.TargetDistance <= Subsystem->GetInteractionRange();
                if (!ClickDebug.bSameLocation) ClickDebug.TargetBlockedReason = TEXT("同一地点ではありません");
                else if (!ClickDebug.bTargetActionPossible) ClickDebug.TargetBlockedReason = TEXT("行動範囲外です");
            }
        }
    }
    if (AStageDGameMode* GameMode = GetWorld()->GetAuthGameMode<AStageDGameMode>())
        GameMode->SelectStageG0VisualTarget(TargetInfo.TargetType == TEXT("FANG_RAT")
            ? TEXT("FANG_RAT") : TargetInfo.TargetId);
    UE_LOG(LogTemp, Display, TEXT("STAGE_G0_CLICK_TARGET_SELECTED id=%s type=%s display=%s move_issued=0"),
        *TargetInfo.TargetId, *TargetInfo.TargetType, *TargetInfo.DisplayNameJa);
}

bool AStageDPlayerCharacter::ResolveStageG0MoveDestination(
    const FVector& RayOrigin, const FVector& RayDirection, FVector& OutDestination,
    int32& OutPathPointCount, FString& OutReason)
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG0ClickMoveSurface), false, this);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, RayOrigin, RayOrigin + RayDirection * 100000.0f,
        ECC_GameTraceChannel1, Params))
    {
        OutReason = TEXT("移動不可：通行可能な地面ではありません");
        return false;
    }
    ClickDebug.ClickLocation = Hit.ImpactPoint;
    if (FMath::Abs(Hit.ImpactPoint.X) > 4200.0f || FMath::Abs(Hit.ImpactPoint.Y) > 4200.0f ||
        Hit.ImpactPoint.Z < GetDefault<UStageG0Settings>()->FallRecoveryZ)
    {
        OutReason = TEXT("移動不可：検証範囲外です");
        return false;
    }
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    FNavLocation ProjectedStart;
    FNavLocation Projected;
    if (!Navigation || !Navigation->ProjectPointToNavigation(
        GetActorLocation(), ProjectedStart, FVector(90.0f, 90.0f, 300.0f)) ||
        !Navigation->ProjectPointToNavigation(
        Hit.ImpactPoint, Projected, FVector(90.0f, 90.0f, 300.0f)))
    {
        OutReason = TEXT("移動不可：NavMesh上の地点ではありません");
        return false;
    }
    ClickDebug.NavigationLocation = Projected.Location;
    UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
        GetWorld(), ProjectedStart.Location, Projected.Location, this);
    if (!Path || !Path->IsValid() || Path->IsPartial() || Path->PathPoints.Num() < 2)
    {
        OutReason = TEXT("移動不可：経路がありません");
        return false;
    }
    OutDestination = Projected.Location;
    OutPathPointCount = Path->PathPoints.Num();
    return true;
}

bool AStageDPlayerCharacter::TryIssueStageG0MoveFromCursor(bool bFromHold)
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!PlayerController || !GetWorld()) return false;
    FVector RayOrigin;
    FVector RayDirection;
    if (!PlayerController->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
    {
        ClickDebug.InvalidReason = TEXT("移動不可：カーソル座標を取得できません");
        return false;
    }
    FVector Destination;
    int32 PathPoints = 0;
    FString Reason;
    if (!ResolveStageG0MoveDestination(RayOrigin, RayDirection, Destination, PathPoints, Reason))
    {
        ClickDebug.GroundResult = TEXT("無効");
        ClickDebug.InvalidReason = Reason;
        if (PointMoveMarker) PointMoveMarker->SetVisibility(false, true);
        UE_LOG(LogTemp, Display, TEXT("STAGE_G0_CLICK_MOVE_REJECTED reason=%s"), *Reason);
        return false;
    }
    if (bFromHold && bStageGPointTargetValid && !ShouldUpdateClickDestination(
        StageGPointTarget, Destination, GetDefault<UStageG0Settings>()->ClickDestinationMinUpdateDistance))
        return false;

    StageGPointTarget = Destination;
    bStageGPointTargetValid = true;
    bStageGMoveCommandActive = true;
    bStageGReplanAttempted = false;
    StageGPointSampleLocation = GetActorLocation();
    StageGPointStuckSeconds = 0.0f;
    ClickDebug.GroundResult = TEXT("通行可能");
    ClickDebug.CurrentDestination = Destination;
    ClickDebug.PathStatus = TEXT("経路追従中");
    ClickDebug.PathPointCount = PathPoints;
    ClickDebug.bMoving = true;
    ClickDebug.InvalidReason.Reset();
    ++ClickDebug.DestinationUpdateCount;
    if (PointMoveMarker)
    {
        PointMoveMarker->SetWorldLocation(Destination);
        PointMoveMarker->SetWorldRotation(FRotator::ZeroRotator);
        PointMoveMarker->SetVisibility(true, true);
    }
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(PlayerController, Destination);
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G0_CLICK_MOVE destination=%s pointer=%s path_points=%d hold_update=%d"),
        *Destination.ToCompactString(), *PointMoveMarker->GetComponentLocation().ToCompactString(),
        PathPoints, bFromHold ? 1 : 0);
    return true;
}

void AStageDPlayerCharacter::StopStageG0Path(const FString& JapaneseReason, bool bHidePointer)
{
    if (UPathFollowingComponent* PathFollowing = GetController()
        ? GetController()->FindComponentByClass<UPathFollowingComponent>() : nullptr)
        PathFollowing->AbortMove(*this, FPathFollowingResultFlags::ForcedScript);
    if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
    bStageGMoveCommandActive = false;
    bStageGPointTargetValid = false;
    ClickDebug.bMoving = false;
    ClickDebug.PathStatus = JapaneseReason;
    if (!JapaneseReason.IsEmpty()) ClickDebug.InvalidReason = JapaneseReason;
    if (bHidePointer && PointMoveMarker) PointMoveMarker->SetVisibility(false, true);
}

void AStageDPlayerCharacter::TickStageGPointMove(float DeltaSeconds)
{
    if (!bStageGMoveCommandActive || !bStageGPointTargetValid || !GetCharacterMovement()) return;
    if (FVector::Dist2D(GetActorLocation(), StageGPointTarget) <=
        GetDefault<UStageG0Settings>()->ClickArrivalTolerance)
    {
        StopStageG0Path(TEXT("目的地へ到着"), true);
        return;
    }
    const float Progress = FVector::Dist2D(GetActorLocation(), StageGPointSampleLocation);
    if (Progress >= 14.0f)
    {
        StageGPointSampleLocation = GetActorLocation();
        StageGPointStuckSeconds = 0.0f;
    }
    else
    {
        StageGPointStuckSeconds += DeltaSeconds;
        if (StageGPointStuckSeconds >= GetDefault<UStageG0Settings>()->ClickStuckSeconds)
        {
            StageGPointStuckSeconds = 0.0f;
            StageGPointSampleLocation = GetActorLocation();
            if (!bStageGReplanAttempted)
            {
                bStageGReplanAttempted = true;
                ClickDebug.PathStatus = TEXT("停止検知：経路を1回再計算");
                UAIBlueprintHelperLibrary::SimpleMoveToLocation(GetController(), StageGPointTarget);
                UE_LOG(LogTemp, Warning, TEXT("STAGE_G0_CLICK_MOVE_REPLAN destination=%s attempt=1"),
                    *StageGPointTarget.ToCompactString());
            }
            else
            {
                StopStageG0Path(TEXT("移動不能：再計算後も進めないため停止しました"), true);
                if (AStageDGameMode* GameMode = GetWorld()->GetAuthGameMode<AStageDGameMode>())
                    GameMode->NotifyStageG0MovementRecovery(GetActorLocation());
            }
        }
    }
}

float AStageDPlayerCharacter::ClampStageGCameraPitch(float Pitch)
{
    const UStageG0Settings* Settings = GetDefault<UStageG0Settings>();
    return FMath::Clamp(Pitch, Settings->CameraMinPitch, Settings->CameraMaxPitch);
}

float AStageDPlayerCharacter::ClampStageGCameraZoom(float ArmLength)
{
    const UStageG0Settings* Settings = GetDefault<UStageG0Settings>();
    return FMath::Clamp(ArmLength, Settings->CameraMinArmLength, Settings->CameraMaxArmLength);
}

bool AStageDPlayerCharacter::ShouldRecoverFromFall(float Z, float Threshold) { return Z < Threshold; }

void AStageDPlayerCharacter::SetStageGCamera(float Yaw, float Pitch, float ArmLength)
{
    if (!CameraBoom) return;
    CameraBoom->SetRelativeRotation(FRotator(ClampStageGCameraPitch(Pitch), Yaw, 0.0f));
    CameraBoom->TargetArmLength = ClampStageGCameraZoom(ArmLength);
}

void AStageDPlayerCharacter::ResetStageGCamera()
{
    const UStageG0Settings* Settings = GetDefault<UStageG0Settings>();
    SetStageGCamera(Settings->CameraYaw, Settings->CameraPitch, Settings->CameraArmLength);
}

float AStageDPlayerCharacter::GetStageGCameraPitch() const { return CameraBoom ? CameraBoom->GetRelativeRotation().Pitch : 0.0f; }
float AStageDPlayerCharacter::GetStageGCameraYaw() const { return CameraBoom ? CameraBoom->GetRelativeRotation().Yaw : 0.0f; }
float AStageDPlayerCharacter::GetStageGCameraArmLength() const { return CameraBoom ? CameraBoom->TargetArmLength : 0.0f; }

void AStageDPlayerCharacter::SetBlobShadowVisible(bool bVisible)
{
    if (BlobShadow) BlobShadow->SetVisibility(bVisible, true);
}

bool AStageDPlayerCharacter::IsStageG0Map() const
{
    return GetWorld() && GetWorld()->GetMapName().Contains(TEXT("StageG0_VisualPoC"));
}

void AStageDPlayerCharacter::UpdateBlobShadow()
{
    if (!BlobShadow || !GetWorld()) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG0PlayerShadow), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + FVector(0, 0, 50),
        GetActorLocation() - FVector(0, 0, 260), ECC_Visibility, Params))
    {
        BlobShadow->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * 2.0f);
        BlobShadow->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector, Hit.ImpactNormal));
    }
}
void AStageDPlayerCharacter::Talk() { ExecuteCoreAction(TEXT("TALK")); }
void AStageDPlayerCharacter::Help() { ExecuteCoreAction(TEXT("HELP")); }
void AStageDPlayerCharacter::Harm() { ExecuteCoreAction(TEXT("HARM")); }
void AStageDPlayerCharacter::Trade() { ExecuteCoreAction(TEXT("TRADE")); }
void AStageDPlayerCharacter::Steal() { ExecuteCoreAction(TEXT("STEAL")); }
void AStageDPlayerCharacter::WaitAction() { ExecuteCoreAction(TEXT("WAIT")); }

void AStageDPlayerCharacter::ExecuteCoreAction(const FString& Action)
{
    StageGPlayerAction = Action;
    StageGPlayerActionRemaining = 1.2f;
    if (!GetGameInstance()) return;
    if (auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>())
    {
        if (Action != TEXT("WAIT") && StageG0SelectedTarget.IsValid())
        {
            if (StageG0SelectedTargetInfo.TargetType == TEXT("FANG_RAT"))
            {
                ClickDebug.bTargetActionPossible = false;
                ClickDebug.TargetBlockedReason = TEXT("牙鼠は描画fixture専用です。戦闘・因果行動は送信していません");
                UE_LOG(LogTemp, Display,
                    TEXT("STAGE_G0_FANG_RAT_ACTION_BLOCKED action=%s target=fang_rat_001 causal_events=0"), *Action);
                return;
            }
            Subsystem->SetTargetNpc(StageG0SelectedTargetInfo.TargetId);
        }
        const FString Target = Action == TEXT("WAIT") ? TEXT("") :
            (StageG0SelectedTargetInfo.TargetType == TEXT("FANG_RAT")
                ? TEXT("") : StageG0SelectedTargetInfo.TargetId.IsEmpty()
                    ? Subsystem->GetWorldView().TargetNpcId : StageG0SelectedTargetInfo.TargetId);
        Subsystem->SubmitPlayerAction(Action, Target, TEXT(""));
    }
}

void AStageDPlayerCharacter::MoveToNextLocation()
{
    if (!GetGameInstance()) return;
    auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;
    static const TArray<FString> Locations = {TEXT("capital"), TEXT("market"), TEXT("tavern"), TEXT("residential"), TEXT("gate")};
    const int32 Current = Locations.IndexOfByKey(Subsystem->GetWorldView().CurrentLocationId);
    const FString& Next = Locations[(Current + 1 + Locations.Num()) % Locations.Num()];
    bManualTargetSelection = false;
    if (Subsystem->EnterLocation(Next)) SetActorLocation(AStageDGameMode::GetLocationCenter(Next) + FVector(0.0f, 0.0f, 140.0f));
}

void AStageDPlayerCharacter::CycleTarget()
{
    if (!GetGameInstance()) return;
    auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;
    TArray<AStageDNpcActor*> Npcs;
    for (TActorIterator<AStageDNpcActor> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && FVector::Distance(GetActorLocation(), It->GetActorLocation()) <= Subsystem->GetInteractionRange())
        {
            Npcs.Add(*It);
        }
    }
    Npcs.Sort([](const AStageDNpcActor& A, const AStageDNpcActor& B) { return A.GetNpcId() < B.GetNpcId(); });
    if (Npcs.IsEmpty())
    {
        bManualTargetSelection = false;
        Subsystem->ClearTargetNpc(TEXT("No active NPC is within interaction range"));
        return;
    }
    const FString Current = Subsystem->GetWorldView().TargetNpcId;
    int32 Index = Npcs.IndexOfByPredicate([&](const AStageDNpcActor* Npc) { return Npc->GetNpcId() == Current; });
    Index = (Index + 1 + Npcs.Num()) % Npcs.Num();
    Subsystem->SetTargetNpc(Npcs[Index]->GetNpcId());
    SetStageG0SelectedTarget(Npcs[Index], Npcs[Index]->GetStageG0TargetInfo());
    bManualTargetSelection = true;
    TargetLostSeconds = 0.0f;
    UE_LOG(LogTemp, Display, TEXT("STAGE_D_TARGET_SELECTED npc=%s method=TAB"), *Npcs[Index]->GetNpcId());
}

void AStageDPlayerCharacter::ScanTarget()
{
    if (IsStageG0Map()) return;
    if (!FollowCamera || !GetGameInstance()) return;
    auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;

    // A target explicitly chosen with Tab remains selected while the core still
    // considers it valid. Range/location/despawn invalidation remains owned by
    // the subsystem; looking away must not silently cancel manual selection.
    if (bManualTargetSelection)
    {
        if (!Subsystem->GetWorldView().TargetNpcId.IsEmpty())
        {
            TargetLostSeconds = 0.0f;
            return;
        }
        bManualTargetSelection = false;
    }

    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * 1800.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageDTargetScan), false, this);
    bool bSawValidNpc = false;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (const auto* Npc = Cast<AStageDNpcActor>(Hit.GetActor()))
        {
            if (FVector::Distance(GetActorLocation(), Npc->GetActorLocation()) <= Subsystem->GetInteractionRange())
            {
                Subsystem->SetTargetNpc(Npc->GetNpcId());
                TargetLostSeconds = 0.0f;
                bSawValidNpc = true;
            }
        }
    }
    if (!bSawValidNpc)
    {
        TargetLostSeconds += 0.15f;
        if (TargetLostSeconds >= 1.0f)
        {
            if (!Subsystem->GetWorldView().TargetNpcId.IsEmpty())
            {
                Subsystem->ClearTargetNpc(TEXT("Target NPC was outside the view/range timeout"));
            }
            TargetLostSeconds = 0.0f;
        }
    }
}

void AStageDPlayerCharacter::SaveSimulation()
{
    if (GetGameInstance()) GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>()->SaveMidChain();
}

void AStageDPlayerCharacter::LoadSimulation()
{
    if (GetGameInstance()) GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>()->ReloadSave();
}

void AStageDPlayerCharacter::ToggleDebug()
{
    if (const auto* GameMode = GetWorld()->GetAuthGameMode<AStageDGameMode>())
    {
        if (GameMode->GetStageDHud()) GameMode->GetStageDHud()->ToggleDebug();
    }
}
