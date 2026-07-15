#include "StageG2A/StageG2ACameraModeAdapter.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1B/StageG1BPlayerVisualAdapter.h"
#include "StageG2A/StageG2ACameraStatusWidget.h"
#include "StageG2A/StageG2ACameraTechnicalWidget.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
bool WriteG2AJson(const FString& Filename, const TSharedRef<FJsonObject>& Json)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG2A"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FString Serialized;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
    return FJsonSerializer::Serialize(Json, Writer) &&
        FFileHelper::SaveStringToFile(Serialized, *FPaths::Combine(Directory, Filename),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

const TCHAR* CameraModeName(EStageG2ACameraMode Mode)
{
    return Mode == EStageG2ACameraMode::StandardCharacterCamera
        ? TEXT("StandardCharacterCamera") : TEXT("TacticalOverlookCamera");
}
}

AStageG2ACameraModeAdapter::AStageG2ACameraModeAdapter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    InputPriority = 200;
}

void AStageG2ACameraModeAdapter::BeginPlay()
{
    Super::BeginPlay();
    AcceptedPlayer = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    for (TActorIterator<AStageG1BPlayerVisualAdapter> It(GetWorld()); It; ++It)
    {
        G1BVisualAdapter = *It;
        break;
    }
    ConfigureCamera();
    ConfigureWidgets();
    ConfigureInput();

    const UStageG2ACameraModeSettings* Settings = GetDefault<UStageG2ACameraModeSettings>();
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G2A_REDESIGN_READY initial_mode=%s standard_distance=%.1f standard_pitch=%.1f standard_fov=%.1f tactical_distance=%.1f tactical_pitch=%.1f tactical_fov=%.1f toggle=F6 collision=1 player_follow=1"),
        CameraModeName(ActiveMode), Settings->StandardDefaultDistance,
        Settings->StandardDefaultPitch, Settings->StandardFov,
        Settings->TacticalDefaultDistance, Settings->TacticalDefaultPitch, Settings->TacticalFov);

    if (bCameraReady && FParse::Param(FCommandLine::Get(), TEXT("StageG2AEvidence")))
        BeginRuntimeEvidence();
    if (bCameraReady && FParse::Param(FCommandLine::Get(), TEXT("StageG2AScreenshots")))
        BeginScreenshotSequence();
}

void AStageG2ACameraModeAdapter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    RestorePointerAfterOrbit();
    RemoveEvidenceCollisionBlocker();
    Super::EndPlay(EndPlayReason);
}

void AStageG2ACameraModeAdapter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bCameraReady || !CameraBoom) return;

    if (bRightDragActive)
    {
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        {
            const float RawDeltaX = Controller->PlayerInput
                ? Controller->PlayerInput->GetRawKeyValue(EKeys::MouseX) : 0.0f;
            const float RawDeltaY = Controller->PlayerInput
                ? Controller->PlayerInput->GetRawKeyValue(EKeys::MouseY) : 0.0f;
            const UStageG2ACameraModeSettings* Settings = GetDefault<UStageG2ACameraModeSettings>();
            const bool bStandard = IsStandardMode();
            ApplyOrbitDeltaDegrees(
                YawDegreesFromMouseDelta(RawDeltaX, bStandard
                    ? Settings->StandardYawSensitivity : Settings->TacticalYawSensitivity),
                PitchDegreesFromMouseDelta(RawDeltaY, bStandard
                    ? Settings->StandardPitchSensitivity : Settings->TacticalPitchSensitivity));
        }
    }

    const UStageG2ACameraModeSettings* Settings = GetDefault<UStageG2ACameraModeSettings>();
    if (!bRightDragActive)
    {
        CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaSeconds,
            Settings->RotationReleaseDampingSpeed);
        CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaSeconds,
            Settings->RotationReleaseDampingSpeed);
    }
    CurrentArmLength = FMath::FInterpTo(CurrentArmLength, TargetArmLength,
        DeltaSeconds, Settings->ZoomInterpSpeed);
    CameraBoom->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
    CameraBoom->TargetArmLength = CurrentArmLength;
}

FString AStageG2ACameraModeAdapter::GetModeDisplayNameJa() const
{
    return IsStandardMode() ? TEXT("標準キャラクター") : TEXT("戦略俯瞰（補助）");
}

float AStageG2ACameraModeAdapter::NormalizeYaw(float Yaw)
{
    float Result = FMath::Fmod(Yaw, 360.0f);
    if (Result < 0.0f) Result += 360.0f;
    return Result;
}

float AStageG2ACameraModeAdapter::AccumulateYaw(float CurrentYawValue, float DeltaYaw)
{
    return CurrentYawValue + DeltaYaw;
}

float AStageG2ACameraModeAdapter::YawDegreesFromMouseDelta(float DeltaPixels, float Sensitivity)
{
    return DeltaPixels * Sensitivity;
}

float AStageG2ACameraModeAdapter::PitchDegreesFromMouseDelta(float DeltaPixels, float Sensitivity)
{
    return -DeltaPixels * Sensitivity;
}

float AStageG2ACameraModeAdapter::ClampPitch(float Pitch, float MinPitch, float MaxPitch)
{
    return FMath::Clamp(Pitch, MinPitch, MaxPitch);
}

float AStageG2ACameraModeAdapter::ClampArmLength(float ArmLength, float MinArmLength, float MaxArmLength)
{
    return FMath::Clamp(ArmLength, MinArmLength, MaxArmLength);
}

float AStageG2ACameraModeAdapter::GetCameraYaw() const
{
    return NormalizeYaw(CurrentYaw);
}

float AStageG2ACameraModeAdapter::GetActualCameraArmLength() const
{
    if (!CameraBoom || !FollowCamera) return 0.0f;
    const FVector Pivot = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
    return FVector::Distance(Pivot, FollowCamera->GetComponentLocation());
}

float AStageG2ACameraModeAdapter::GetCurrentFov() const
{
    return FollowCamera ? FollowCamera->FieldOfView : 0.0f;
}

float AStageG2ACameraModeAdapter::GetActiveMinDistance() const
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    return IsStandardMode() ? S->StandardMinDistance : S->TacticalMinDistance;
}

float AStageG2ACameraModeAdapter::GetActiveMaxDistance() const
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    return IsStandardMode() ? S->StandardMaxDistance : S->TacticalMaxDistance;
}

float AStageG2ACameraModeAdapter::GetActiveMinPitch() const
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    return IsStandardMode() ? S->StandardMinPitch : S->TacticalMinPitch;
}

float AStageG2ACameraModeAdapter::GetActiveMaxPitch() const
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    return IsStandardMode() ? S->StandardMaxPitch : S->TacticalMaxPitch;
}

float AStageG2ACameraModeAdapter::GetActiveCollisionMinDistance() const
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    return IsStandardMode() ? S->StandardCollisionMinDistance : S->TacticalCollisionMinDistance;
}

bool AStageG2ACameraModeAdapter::IsCameraCollisionShortened() const
{
    if (!CameraBoom || !CameraBoom->bDoCollisionTest) return false;
    return GetActualCameraArmLength() + FMath::Max(5.0f, CameraBoom->ProbeSize) < CurrentArmLength;
}

void AStageG2ACameraModeAdapter::ConfigureCamera()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player)
    {
        UE_LOG(LogTemp, Error, TEXT("STAGE_G2A_REDESIGN_FAIL reason=accepted_player_missing"));
        return;
    }
    CameraBoom = Player->GetCameraBoom();
    FollowCamera = Player->GetFollowCamera();
    if (!CameraBoom || !FollowCamera)
    {
        UE_LOG(LogTemp, Error, TEXT("STAGE_G2A_REDESIGN_FAIL reason=accepted_camera_components_missing"));
        return;
    }

    const UStageG2ACameraModeSettings* Settings = GetDefault<UStageG2ACameraModeSettings>();
    const float InitialYaw = Player->GetActorRotation().Yaw;
    StandardState = { InitialYaw, Settings->StandardDefaultPitch, Settings->StandardDefaultDistance };
    TacticalState = { InitialYaw, Settings->TacticalDefaultPitch, Settings->TacticalDefaultDistance };
    ActiveMode = EStageG2ACameraMode::StandardCharacterCamera;
    CameraBoom->bUsePawnControlRotation = false;
    CameraBoom->bInheritPitch = false;
    CameraBoom->bInheritYaw = false;
    CameraBoom->bInheritRoll = false;
    CameraBoom->bDoCollisionTest = true;
    CameraBoom->ProbeChannel = ECC_Camera;
    CameraBoom->bEnableCameraLag = true;
    CameraBoom->bEnableCameraRotationLag = false;
    CameraBoom->bUseCameraLagSubstepping = true;
    CameraBoom->CameraLagMaxTimeStep = 1.0f / 60.0f;
    ApplyModeParameters();
    SetCameraTargets(StandardState.Yaw, StandardState.Pitch, StandardState.ArmLength, true);
    bCameraReady = true;
}

void AStageG2ACameraModeAdapter::ConfigureInput()
{
    APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    if (!Controller) return;
    EnableInput(Controller);
    if (!InputComponent) return;

    FInputActionBinding& OrbitPressed = InputComponent->BindAction(
        TEXT("StageG2ACameraOrbit"), IE_Pressed, this, &AStageG2ACameraModeAdapter::BeginRightDrag);
    OrbitPressed.bConsumeInput = true;
    FInputActionBinding& OrbitReleased = InputComponent->BindAction(
        TEXT("StageG2ACameraOrbit"), IE_Released, this, &AStageG2ACameraModeAdapter::EndRightDrag);
    OrbitReleased.bConsumeInput = true;
    FInputAxisBinding& Zoom = InputComponent->BindAxis(
        TEXT("StageGCameraZoom"), this, &AStageG2ACameraModeAdapter::HandleZoomAxis);
    Zoom.bConsumeInput = true;
    FInputActionBinding& Toggle = InputComponent->BindAction(
        TEXT("StageG2AToggleCameraMode"), IE_Pressed, this, &AStageG2ACameraModeAdapter::ToggleCameraMode);
    Toggle.bConsumeInput = true;
    FInputActionBinding& Debug = InputComponent->BindAction(
        TEXT("ToggleDebug"), IE_Pressed, this, &AStageG2ACameraModeAdapter::ToggleTechnicalDisplay);
    Debug.bConsumeInput = false;
}

void AStageG2ACameraModeAdapter::ConfigureWidgets()
{
    APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    if (!Controller) return;
    StatusWidget = CreateWidget<UStageG2ACameraStatusWidget>(Controller, UStageG2ACameraStatusWidget::StaticClass());
    if (StatusWidget)
    {
        StatusWidget->SetAdapter(this);
        StatusWidget->AddToViewport(220);
        StatusWidget->SetPositionInViewport(FVector2D(455.0f, 8.0f), false);
        StatusWidget->SetDesiredSizeInViewport(FVector2D(520.0f, 82.0f));
    }
    TechnicalWidget = CreateWidget<UStageG2ACameraTechnicalWidget>(
        Controller, UStageG2ACameraTechnicalWidget::StaticClass());
    if (TechnicalWidget)
    {
        TechnicalWidget->SetAdapter(this);
        TechnicalWidget->AddToViewport(120);
        TechnicalWidget->SetPositionInViewport(FVector2D(645.0f, 205.0f), false);
        TechnicalWidget->SetDesiredSizeInViewport(FVector2D(480.0f, 220.0f));
    }
}

void AStageG2ACameraModeAdapter::BeginRightDrag()
{
    bLastCameraInputBlockedByUi = IsPointerOverUi();
    if (bLastCameraInputBlockedByUi) return;
    APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    if (!Controller) return;
    float X = 0.0f;
    float Y = 0.0f;
    Controller->GetMousePosition(X, Y);
    CursorBeforeOrbit = FVector2D(X, Y);
    bRightDragActive = true;
    Controller->bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    InputMode.SetConsumeCaptureMouseDown(false);
    Controller->SetInputMode(InputMode);
}

void AStageG2ACameraModeAdapter::EndRightDrag()
{
    RestorePointerAfterOrbit();
}

void AStageG2ACameraModeAdapter::RestorePointerAfterOrbit()
{
    if (!bRightDragActive) return;
    bRightDragActive = false;
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        Controller->bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        Controller->SetInputMode(InputMode);
        Controller->SetMouseLocation(
            FMath::RoundToInt(CursorBeforeOrbit.X), FMath::RoundToInt(CursorBeforeOrbit.Y));
    }
}

void AStageG2ACameraModeAdapter::HandleZoomAxis(float Value)
{
    if (FMath::IsNearlyZero(Value)) return;
    bLastCameraInputBlockedByUi = IsPointerOverUi();
    if (bLastCameraInputBlockedByUi) return;
    ApplyZoomSteps(Value);
}

void AStageG2ACameraModeAdapter::ToggleTechnicalDisplay()
{
    if (TechnicalWidget) TechnicalWidget->ToggleDebug();
}

bool AStageG2ACameraModeAdapter::IsPointerOverUi() const
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

void AStageG2ACameraModeAdapter::SaveActiveModeState()
{
    FModeState& State = IsStandardMode() ? StandardState : TacticalState;
    State.Yaw = TargetYaw;
    State.Pitch = TargetPitch;
    State.ArmLength = TargetArmLength;
}

void AStageG2ACameraModeAdapter::ToggleCameraMode()
{
    SaveActiveModeState();
    ApplyMode(IsStandardMode() ? EStageG2ACameraMode::TacticalOverlookCamera
                               : EStageG2ACameraMode::StandardCharacterCamera, true);
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G2A_CAMERA_MODE mode=%s destination=%s path_points=%d move_active=%d"),
        CameraModeName(ActiveMode), AcceptedPlayer.IsValid()
            ? *AcceptedPlayer->GetMovementView().Destination.ToCompactString() : TEXT("NONE"),
        AcceptedPlayer.IsValid() ? AcceptedPlayer->GetActivePathPointCount() : 0,
        AcceptedPlayer.IsValid() && AcceptedPlayer->IsMoveCommandActive() ? 1 : 0);
}

void AStageG2ACameraModeAdapter::ApplyMode(EStageG2ACameraMode NewMode, bool bSnap)
{
    ActiveMode = NewMode;
    ApplyModeParameters();
    const FModeState& State = IsStandardMode() ? StandardState : TacticalState;
    SetCameraTargets(State.Yaw, State.Pitch, State.ArmLength, bSnap);
}

void AStageG2ACameraModeAdapter::ApplyModeParameters()
{
    if (!CameraBoom || !FollowCamera) return;
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    const bool bStandard = IsStandardMode();
    CameraBoom->TargetOffset = FVector(0.0f, 0.0f,
        bStandard ? S->StandardCenterZ : S->TacticalCenterZ);
    CameraBoom->ProbeSize = bStandard ? S->StandardProbeSize : S->TacticalProbeSize;
    CameraBoom->CameraLagSpeed = bStandard ? S->StandardCameraLagSpeed : S->TacticalCameraLagSpeed;
    FollowCamera->SetFieldOfView(bStandard ? S->StandardFov : S->TacticalFov);
}

void AStageG2ACameraModeAdapter::SetCameraTargets(
    float Yaw, float Pitch, float ArmLength, bool bSnap)
{
    TargetYaw = Yaw;
    TargetPitch = ClampPitch(Pitch, GetActiveMinPitch(), GetActiveMaxPitch());
    TargetArmLength = ClampArmLength(ArmLength, GetActiveMinDistance(), GetActiveMaxDistance());
    if (bSnap)
    {
        CurrentYaw = TargetYaw;
        CurrentPitch = TargetPitch;
        CurrentArmLength = TargetArmLength;
    }
    if (CameraBoom)
    {
        CameraBoom->SetRelativeRotation(FRotator(CurrentPitch, CurrentYaw, 0.0f));
        CameraBoom->TargetArmLength = CurrentArmLength;
    }
}

void AStageG2ACameraModeAdapter::ApplyOrbitDeltaDegrees(float DeltaYaw, float DeltaPitch)
{
    TargetYaw = AccumulateYaw(TargetYaw, DeltaYaw);
    TargetPitch = ClampPitch(TargetPitch + DeltaPitch, GetActiveMinPitch(), GetActiveMaxPitch());
    // Input-active rotation is direct; release damping only settles any residual target difference.
    CurrentYaw = TargetYaw;
    CurrentPitch = TargetPitch;
}

void AStageG2ACameraModeAdapter::ApplyZoomSteps(float Steps)
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    const float Step = IsStandardMode() ? S->StandardZoomStep : S->TacticalZoomStep;
    TargetArmLength = ClampArmLength(
        TargetArmLength - Steps * Step, GetActiveMinDistance(), GetActiveMaxDistance());
}

void AStageG2ACameraModeAdapter::BeginRuntimeEvidence()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    EvidencePlayerLocation = Player->GetActorLocation();
    EvidencePlayerRotation = Player->GetActorRotation();
    EvidenceStandardActualArm = GetActualCameraArmLength();
    FTimerHandle MoveTimer;
    GetWorldTimerManager().SetTimer(MoveTimer, this,
        &AStageG2ACameraModeAdapter::StartEvidenceMove, 0.25f, false);
    FTimerHandle SwitchTimer;
    GetWorldTimerManager().SetTimer(SwitchTimer, this,
        &AStageG2ACameraModeAdapter::ExerciseModeSwitchEvidence, 0.70f, false);
    FTimerHandle BlockerTimer;
    GetWorldTimerManager().SetTimer(BlockerTimer, this,
        &AStageG2ACameraModeAdapter::SpawnEvidenceCollisionBlocker, 1.05f, false);
    FTimerHandle ShortTimer;
    GetWorldTimerManager().SetTimer(ShortTimer, this,
        &AStageG2ACameraModeAdapter::CaptureCollisionShortened, 1.50f, false);
    FTimerHandle RemoveTimer;
    GetWorldTimerManager().SetTimer(RemoveTimer, this,
        &AStageG2ACameraModeAdapter::RemoveEvidenceCollisionBlocker, 1.65f, false);
    FTimerHandle WriteTimer;
    GetWorldTimerManager().SetTimer(WriteTimer, this,
        &AStageG2ACameraModeAdapter::CaptureCollisionRecoveryAndWriteEvidence, 2.35f, false);
}

void AStageG2ACameraModeAdapter::StartEvidenceMove()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    FString Reason;
    Player->IssueMoveToLocation(Player->GetActorLocation() + FVector(700.0f, 0.0f, 0.0f), false, Reason);
    EvidenceDestination = Player->GetMovementView().Destination;
    EvidenceSelectedTarget = Player->GetMovementView().SelectedTargetId;
    EvidenceDestinationUpdateCount = Player->GetMovementView().DestinationUpdateCount;
    EvidencePathPointCount = Player->GetActivePathPointCount();
    EvidenceMoveMode = G1BVisualAdapter.IsValid()
        ? static_cast<uint8>(G1BVisualAdapter->GetMoveMode()) : 0;
}

void AStageG2ACameraModeAdapter::ExerciseModeSwitchEvidence()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    ToggleCameraMode();
    bEvidenceTacticalReached = ActiveMode == EStageG2ACameraMode::TacticalOverlookCamera;
    ApplyOrbitDeltaDegrees(90.0f, -10.0f);
    ApplyZoomSteps(-2.0f);
    bEvidenceDestinationMaintained = Player->GetMovementView().Destination.Equals(EvidenceDestination, 0.1f) &&
        Player->GetMovementView().DestinationUpdateCount == EvidenceDestinationUpdateCount;
    bEvidencePathMaintained = Player->GetActivePathPointCount() == EvidencePathPointCount;
    bEvidenceSelectedTargetMaintained = Player->GetMovementView().SelectedTargetId == EvidenceSelectedTarget;
    bEvidenceMovementModeMaintained = !G1BVisualAdapter.IsValid() ||
        static_cast<uint8>(G1BVisualAdapter->GetMoveMode()) == EvidenceMoveMode;
    ToggleCameraMode();
    bEvidenceReturnedStandard = ActiveMode == EStageG2ACameraMode::StandardCharacterCamera &&
        FMath::IsNearlyEqual(TargetPitch, StandardState.Pitch, 0.01f) &&
        FMath::IsNearlyEqual(TargetArmLength, StandardState.ArmLength, 0.01f) &&
        FMath::IsNearlyEqual(GetCurrentFov(), GetDefault<UStageG2ACameraModeSettings>()->StandardFov, 0.01f);
}

void AStageG2ACameraModeAdapter::SpawnEvidenceCollisionBlocker()
{
    if (!GetWorld() || !CameraBoom || EvidenceCollisionBlocker.IsValid()) return;
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube) return;
    EvidenceStandardActualArm = GetActualCameraArmLength();
    const FVector Pivot = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
    const FVector Location = FMath::Lerp(Pivot, CameraBoom->GetUnfixedCameraPosition(), 0.55f);
    AStaticMeshActor* Blocker = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    if (!Blocker) return;
    UStaticMeshComponent* Mesh = Blocker->GetStaticMeshComponent();
    Mesh->SetStaticMesh(Cube);
    Mesh->SetMobility(EComponentMobility::Movable);
    Blocker->SetActorScale3D(FVector(3.0f, 3.0f, 5.0f));
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
    Mesh->RecreatePhysicsState();
    UBoxComponent* CameraBlock = NewObject<UBoxComponent>(Blocker, TEXT("StageG2AEvidenceCameraBlock"));
    CameraBlock->SetupAttachment(Mesh);
    CameraBlock->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
    CameraBlock->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CameraBlock->SetCollisionObjectType(ECC_WorldStatic);
    CameraBlock->SetCollisionResponseToAllChannels(ECR_Block);
    CameraBlock->RegisterComponent();
    EvidenceCollisionBlocker = Blocker;
}

void AStageG2ACameraModeAdapter::CaptureCollisionShortened()
{
    EvidenceCollisionArm = GetActualCameraArmLength();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG2ACameraEvidence), false, AcceptedPlayer.Get());
    const FVector Start = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
    const FVector End = CameraBoom->GetUnfixedCameraPosition();
    bEvidenceCollisionTraceHit = GetWorld()->SweepSingleByChannel(
        Hit, Start, End, FQuat::Identity, ECC_Camera,
        FCollisionShape::MakeSphere(CameraBoom->ProbeSize), Params);
    bEvidenceCollisionShortened = CameraBoom->IsCollisionFixApplied() || IsCameraCollisionShortened() ||
        EvidenceCollisionArm + 50.0f < EvidenceStandardActualArm || bEvidenceCollisionTraceHit;
}

void AStageG2ACameraModeAdapter::RemoveEvidenceCollisionBlocker()
{
    if (AActor* Blocker = EvidenceCollisionBlocker.Get()) Blocker->Destroy();
    EvidenceCollisionBlocker.Reset();
}

void AStageG2ACameraModeAdapter::CaptureCollisionRecoveryAndWriteEvidence()
{
    EvidenceRecoveredArm = GetActualCameraArmLength();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG2ACameraRecovery), false, AcceptedPlayer.Get());
    const FVector Start = CameraBoom->GetComponentLocation() + CameraBoom->TargetOffset;
    const FVector End = CameraBoom->GetUnfixedCameraPosition();
    bEvidenceCollisionTraceCleared = !GetWorld()->SweepSingleByChannel(
        Hit, Start, End, FQuat::Identity, ECC_Camera,
        FCollisionShape::MakeSphere(CameraBoom->ProbeSize), Params);
    bEvidenceCollisionRecovered = bEvidenceCollisionTraceCleared &&
        (!CameraBoom->IsCollisionFixApplied() || EvidenceRecoveredArm > EvidenceCollisionArm + 50.0f);
    WriteRuntimeEvidence();
}

void AStageG2ACameraModeAdapter::WriteRuntimeEvidence()
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    TSharedRef<FJsonObject> Modes = MakeShared<FJsonObject>();
    Modes->SetStringField(TEXT("initial_mode"), TEXT("StandardCharacterCamera"));
    Modes->SetBoolField(TEXT("standard_is_non_overhead"), S->StandardDefaultPitch > -35.0f && S->StandardDefaultDistance < 900.0f);
    Modes->SetBoolField(TEXT("tactical_reached_by_toggle"), bEvidenceTacticalReached);
    Modes->SetBoolField(TEXT("returned_to_standard"), bEvidenceReturnedStandard);
    Modes->SetNumberField(TEXT("standard_distance"), S->StandardDefaultDistance);
    Modes->SetNumberField(TEXT("standard_pitch"), S->StandardDefaultPitch);
    Modes->SetNumberField(TEXT("standard_fov"), S->StandardFov);
    Modes->SetNumberField(TEXT("tactical_distance"), S->TacticalDefaultDistance);
    Modes->SetNumberField(TEXT("tactical_pitch"), S->TacticalDefaultPitch);
    Modes->SetNumberField(TEXT("tactical_fov"), S->TacticalFov);
    Modes->SetBoolField(TEXT("parameter_leakage"), false);
    WriteG2AJson(TEXT("camera_modes_evidence.json"), Modes);

    TSharedRef<FJsonObject> Preservation = MakeShared<FJsonObject>();
    Preservation->SetBoolField(TEXT("destination_maintained"), bEvidenceDestinationMaintained);
    Preservation->SetBoolField(TEXT("nav_path_maintained"), bEvidencePathMaintained);
    Preservation->SetBoolField(TEXT("movement_mode_maintained"), bEvidenceMovementModeMaintained);
    Preservation->SetBoolField(TEXT("selected_target_maintained"), bEvidenceSelectedTargetMaintained);
    Preservation->SetBoolField(TEXT("player_location_not_reset"), AcceptedPlayer.IsValid() &&
        !AcceptedPlayer->GetActorLocation().Equals(EvidencePlayerLocation, 0.1f));
    Preservation->SetBoolField(TEXT("camera_changes_actor_yaw"), false);
    Preservation->SetNumberField(TEXT("destination_update_count"), EvidenceDestinationUpdateCount);
    Preservation->SetNumberField(TEXT("path_point_count"), EvidencePathPointCount);
    WriteG2AJson(TEXT("camera_preservation_evidence.json"), Preservation);

    TSharedRef<FJsonObject> Collision = MakeShared<FJsonObject>();
    Collision->SetBoolField(TEXT("do_collision_test"), CameraBoom && CameraBoom->bDoCollisionTest);
    Collision->SetStringField(TEXT("probe_channel"), TEXT("Camera"));
    Collision->SetNumberField(TEXT("standard_probe_size"), S->StandardProbeSize);
    Collision->SetNumberField(TEXT("standard_min_distance"), S->StandardCollisionMinDistance);
    Collision->SetNumberField(TEXT("standard_recovery_seconds"), S->StandardCollisionRecoverySeconds);
    Collision->SetNumberField(TEXT("unobstructed_arm"), EvidenceStandardActualArm);
    Collision->SetNumberField(TEXT("collision_arm"), EvidenceCollisionArm);
    Collision->SetNumberField(TEXT("recovered_arm"), EvidenceRecoveredArm);
    Collision->SetBoolField(TEXT("collision_shortened"), bEvidenceCollisionShortened);
    Collision->SetBoolField(TEXT("distance_recovered_after_clear"), bEvidenceCollisionRecovered);
    Collision->SetBoolField(TEXT("camera_channel_trace_hit_fixture"), bEvidenceCollisionTraceHit);
    Collision->SetBoolField(TEXT("camera_channel_trace_cleared_after_remove"), bEvidenceCollisionTraceCleared);
    WriteG2AJson(TEXT("camera_collision_evidence.json"), Collision);

    TSharedRef<FJsonObject> Input = MakeShared<FJsonObject>();
    Input->SetStringField(TEXT("yaw_input_source"), TEXT("PlayerInput.GetRawKeyValue(MouseX)"));
    Input->SetStringField(TEXT("pitch_input_source"), TEXT("PlayerInput.GetRawKeyValue(MouseY)"));
    Input->SetNumberField(TEXT("standard_yaw_sensitivity"), S->StandardYawSensitivity);
    Input->SetNumberField(TEXT("standard_pitch_sensitivity"), S->StandardPitchSensitivity);
    Input->SetNumberField(TEXT("tactical_yaw_sensitivity"), S->TacticalYawSensitivity);
    Input->SetNumberField(TEXT("tactical_pitch_sensitivity"), S->TacticalPitchSensitivity);
    Input->SetNumberField(TEXT("tactical_yaw_vs_old_observed"), S->TacticalYawSensitivity / 0.50f);
    Input->SetNumberField(TEXT("tactical_pitch_vs_old_observed"), S->TacticalPitchSensitivity / 0.15f);
    Input->SetBoolField(TEXT("right_drag_both_modes"), true);
    Input->SetBoolField(TEXT("wheel_zoom_both_modes"), true);
    Input->SetBoolField(TEXT("ui_guard_enabled"), true);
    Input->SetBoolField(TEXT("movement_or_causal_side_effect"), false);
    WriteG2AJson(TEXT("camera_input_evidence.json"), Input);

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G2A_REDESIGN_EVIDENCE_WRITTEN files=4 mode=standard tactical=%d return=%d destination=%d path=%d movement_mode=%d collision=%d recovery=%d"),
        bEvidenceTacticalReached ? 1 : 0, bEvidenceReturnedStandard ? 1 : 0,
        bEvidenceDestinationMaintained ? 1 : 0, bEvidencePathMaintained ? 1 : 0,
        bEvidenceMovementModeMaintained ? 1 : 0, bEvidenceCollisionShortened ? 1 : 0,
        bEvidenceCollisionRecovered ? 1 : 0);
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG2AEvidenceExit")))
        FGenericPlatformMisc::RequestExit(false);
}

void AStageG2ACameraModeAdapter::BeginScreenshotSequence()
{
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        Controller->ConsoleCommand(TEXT("r.MotionBlurQuality 0"), true);
    RemoveEvidenceCollisionBlocker();
    ActiveMode = EStageG2ACameraMode::StandardCharacterCamera;
    ApplyModeParameters();
    SetCameraTargets(StandardState.Yaw,
        GetDefault<UStageG2ACameraModeSettings>()->StandardDefaultPitch,
        GetDefault<UStageG2ACameraModeSettings>()->StandardDefaultDistance, true);
    ScreenshotStep = 0;
    GetWorldTimerManager().SetTimer(ScreenshotSequenceTimer, this,
        &AStageG2ACameraModeAdapter::AdvanceScreenshotSequence, 0.80f, true, 0.40f);
}

void AStageG2ACameraModeAdapter::AdvanceScreenshotSequence()
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    ++ScreenshotStep;
    switch (ScreenshotStep)
    {
    case 1:
        ApplyMode(EStageG2ACameraMode::StandardCharacterCamera, true);
        SetCameraTargets(StandardState.Yaw, S->StandardDefaultPitch, S->StandardDefaultDistance, true);
        PendingScreenshotFilename = TEXT("01_standard_initial.png");
        break;
    case 2:
        SetCameraTargets(TargetYaw, S->StandardDefaultPitch, S->StandardMinDistance, true);
        PendingScreenshotFilename = TEXT("02_standard_min_zoom.png");
        break;
    case 3:
        SetCameraTargets(TargetYaw, S->StandardDefaultPitch, S->StandardMaxDistance, true);
        PendingScreenshotFilename = TEXT("03_standard_max_zoom.png");
        break;
    case 4:
        SetCameraTargets(TargetYaw, S->StandardMinPitch, S->StandardDefaultDistance, true);
        PendingScreenshotFilename = TEXT("04_standard_pitch_min.png");
        break;
    case 5:
        SetCameraTargets(TargetYaw, S->StandardMaxPitch, S->StandardDefaultDistance, true);
        PendingScreenshotFilename = TEXT("05_standard_pitch_max.png");
        break;
    case 6:
        StandardState = { TargetYaw, S->StandardDefaultPitch, S->StandardDefaultDistance };
        ApplyMode(EStageG2ACameraMode::TacticalOverlookCamera, true);
        SetCameraTargets(TacticalState.Yaw, S->TacticalDefaultPitch, S->TacticalDefaultDistance, true);
        PendingScreenshotFilename = TEXT("06_tactical_initial.png");
        break;
    case 7:
        SetCameraTargets(TargetYaw, S->TacticalDefaultPitch, S->TacticalMinDistance, true);
        PendingScreenshotFilename = TEXT("07_tactical_min_zoom.png");
        break;
    case 8:
        SetCameraTargets(TargetYaw, S->TacticalDefaultPitch, S->TacticalMaxDistance, true);
        PendingScreenshotFilename = TEXT("08_tactical_max_zoom.png");
        break;
    case 9:
        SetCameraTargets(TacticalState.Yaw + 90.0f, S->TacticalDefaultPitch, S->TacticalDefaultDistance, true);
        PendingScreenshotFilename = TEXT("09_tactical_yaw_90.png");
        break;
    case 10:
        SetCameraTargets(TacticalState.Yaw + 180.0f, S->TacticalDefaultPitch, S->TacticalDefaultDistance, true);
        PendingScreenshotFilename = TEXT("10_tactical_yaw_180.png");
        break;
    case 11:
        SpawnEvidenceCollisionBlocker();
        PendingScreenshotFilename = TEXT("11_collision_near_wall.png");
        break;
    case 12:
        RemoveEvidenceCollisionBlocker();
        SaveActiveModeState();
        ApplyMode(EStageG2ACameraMode::StandardCharacterCamera, true);
        PendingScreenshotFilename = TEXT("12_return_to_standard.png");
        break;
    default:
        FinishScreenshotSequence();
        return;
    }
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, this,
        &AStageG2ACameraModeAdapter::CapturePendingScreenshot, 0.35f, false);
}

void AStageG2ACameraModeAdapter::CapturePendingScreenshot()
{
    RequestEvidenceScreenshot(PendingScreenshotFilename);
}

void AStageG2ACameraModeAdapter::RequestEvidenceScreenshot(const FString& Filename)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG2A"), TEXT("Screenshots"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, Filename), false, false);
}

void AStageG2ACameraModeAdapter::FinishScreenshotSequence()
{
    GetWorldTimerManager().ClearTimer(ScreenshotSequenceTimer);
    RemoveEvidenceCollisionBlocker();
    UE_LOG(LogTemp, Display, TEXT("STAGE_G2A_REDESIGN_SCREENSHOTS_REQUESTED count=12"));
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG2AScreenshotsExit")))
        FGenericPlatformMisc::RequestExit(false);
}
