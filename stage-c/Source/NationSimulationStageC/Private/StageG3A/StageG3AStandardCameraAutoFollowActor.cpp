#include "StageG3A/StageG3AStandardCameraAutoFollowActor.h"

#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"
#include "StageG3A/StageG3ACameraAutoFollowSettings.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

AStageG3AStandardCameraAutoFollowActor::AStageG3AStandardCameraAutoFollowActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    AutoFollowPivot = CreateDefaultSubobject<USceneComponent>(TEXT("StageG3AAutoFollowYawPivot"));
    SetRootComponent(AutoFollowPivot);
    Tags.Add(TEXT("StageG3A"));
    Tags.Add(TEXT("StandardCameraAutoFollow"));
}

void AStageG3AStandardCameraAutoFollowActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveAcceptedActors();
    if (CameraAdapter.IsValid()) AddTickPrerequisiteActor(CameraAdapter.Get());
    if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetRootComponent() &&
        AcceptedPlayer->GetCameraBoom() && AutoFollowPivot)
    {
        AutoFollowPivot->AttachToComponent(AcceptedPlayer->GetRootComponent(),
            FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        AcceptedPlayer->GetCameraBoom()->AttachToComponent(AutoFollowPivot,
            FAttachmentTransformRules::KeepRelativeTransform);
    }
}

void AStageG3AStandardCameraAutoFollowActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetRootComponent() &&
        AcceptedPlayer->GetCameraBoom())
    {
        AcceptedPlayer->GetCameraBoom()->AttachToComponent(AcceptedPlayer->GetRootComponent(),
            FAttachmentTransformRules::KeepWorldTransform);
        AcceptedPlayer->GetCameraBoom()->bInheritYaw = false;
    }
    Super::EndPlay(EndPlayReason);
}

float AStageG3AStandardCameraAutoFollowActor::InterpolateYaw(
    float CurrentYaw, float DesiredYaw, float DeltaSeconds, float InterpSpeed)
{
    const float NearestDesired = CurrentYaw + FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw);
    return FMath::FInterpTo(CurrentYaw, NearestDesired, DeltaSeconds, InterpSpeed);
}

void AStageG3AStandardCameraAutoFollowActor::ResolveAcceptedActors()
{
    if (!AcceptedPlayer.IsValid())
    {
        for (TActorIterator<AStageG1APlayerCharacter> It(GetWorld()); It; ++It)
        {
            AcceptedPlayer = *It;
            break;
        }
    }
    if (!CameraAdapter.IsValid())
    {
        for (TActorIterator<AStageG2ACameraModeAdapter> It(GetWorld()); It; ++It)
        {
            CameraAdapter = *It;
            break;
        }
    }
}

bool AStageG3AStandardCameraAutoFollowActor::ResolveDesiredYaw(float& OutYaw) const
{
    const AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return false;
    const UStageG3ACameraAutoFollowSettings* Settings = GetDefault<UStageG3ACameraAutoFollowSettings>();
    const FVector Velocity2D(Player->GetVelocity().X, Player->GetVelocity().Y, 0.0f);
    if (Velocity2D.SizeSquared() >= FMath::Square(Settings->MovementSpeedThreshold))
    {
        OutYaw = Velocity2D.Rotation().Yaw;
        return true;
    }

    const FStageG1AMovementView& Movement = Player->GetMovementView();
    if (!Player->IsMoveCommandActive() && !Movement.bMoving && Player->GetActivePathPointCount() <= 0)
        return false;
    const FVector ToDestination = Movement.Destination - Player->GetActorLocation();
    if (ToDestination.SizeSquared2D() < FMath::Square(Settings->DestinationDirectionMinDistance))
        return false;
    OutYaw = ToDestination.Rotation().Yaw;
    return true;
}

bool AStageG3AStandardCameraAutoFollowActor::IsPointerOverUi() const
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

bool AStageG3AStandardCameraAutoFollowActor::IsUiInputActive() const
{
    const APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
    const bool bMouseInputActive = Controller &&
        (Controller->IsInputKeyDown(EKeys::LeftMouseButton) ||
         Controller->IsInputKeyDown(EKeys::RightMouseButton) ||
         Controller->IsInputKeyDown(EKeys::MiddleMouseButton));
    return ShouldBlockForUiInput(IsPointerOverUi(), bMouseInputActive);
}

void AStageG3AStandardCameraAutoFollowActor::ApplyManagedYaw() const
{
    if (!AcceptedPlayer.IsValid() || !AcceptedPlayer->GetCameraBoom() || !AutoFollowPivot) return;
    USpringArmComponent* Boom = AcceptedPlayer->GetCameraBoom();
    Boom->bInheritYaw = true;
    const float PlayerYaw = AcceptedPlayer->GetActorRotation().Yaw;
    const float BoomRelativeYaw = Boom->GetRelativeRotation().Yaw;
    FRotator PivotRotation = AutoFollowPivot->GetRelativeRotation();
    PivotRotation.Yaw = FMath::FindDeltaAngleDegrees(0.0f,
        ManagedYaw - PlayerYaw - BoomRelativeYaw);
    AutoFollowPivot->SetRelativeRotation(PivotRotation);
}

void AStageG3AStandardCameraAutoFollowActor::SetManagedYawForEvidence(float Yaw)
{
    ManagedYaw = Yaw;
    bManagedYawInitialized = true;
    if (CameraAdapter.IsValid() && CameraAdapter->IsStandardMode()) ApplyManagedYaw();
}

void AStageG3AStandardCameraAutoFollowActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ResolveAcceptedActors();
    AStageG2ACameraModeAdapter* Adapter = CameraAdapter.Get();
    if (!Adapter || !AcceptedPlayer.IsValid()) return;

    if (!bManagedYawInitialized)
    {
        ManagedYaw = Adapter->GetCameraYaw();
        bManagedYawInitialized = true;
    }

    const UStageG3ACameraAutoFollowSettings* Settings = GetDefault<UStageG3ACameraAutoFollowSettings>();
    const bool bRightDrag = Adapter->IsRightDragActive();
    bManualDragObservedNow = bRightDrag;
    if (bRightDrag)
    {
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        {
            const float RawDeltaX = Controller->PlayerInput
                ? Controller->PlayerInput->GetRawKeyValue(EKeys::MouseX) : 0.0f;
            const UStageG2ACameraModeSettings* CameraSettings =
                GetDefault<UStageG2ACameraModeSettings>();
            ManagedYaw += AStageG2ACameraModeAdapter::YawDegreesFromMouseDelta(
                RawDeltaX, CameraSettings->StandardYawSensitivity);
        }
        ManualOverrideRemaining = Settings->ManualOverrideSeconds;
        bWasRightDragActive = true;
        bEvidenceManualDragObserved = true;
        bAutoFollowActive = false;
        ApplyManagedYaw();
        return;
    }
    if (bWasRightDragActive)
    {
        bWasRightDragActive = false;
        ManualOverrideRemaining = Settings->ManualOverrideSeconds;
    }

    if (!Adapter->IsStandardMode())
    {
        if (AutoFollowPivot) AutoFollowPivot->SetRelativeRotation(FRotator::ZeroRotator);
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
            AcceptedPlayer->GetCameraBoom()->bInheritYaw = false;
        bAutoFollowActive = false;
        if (!bWasTactical)
        {
            TacticalEntryYaw = Adapter->GetCameraYaw();
            TacticalMaxUncommandedYawDelta = 0.0f;
            TacticalObservedFrames = 0;
        }
        bWasTactical = true;
        ++TacticalObservedFrames;
        TacticalMaxUncommandedYawDelta = FMath::Max(TacticalMaxUncommandedYawDelta,
            FMath::Abs(FMath::FindDeltaAngleDegrees(TacticalEntryYaw, Adapter->GetCameraYaw())));
        bEvidenceTacticalNoFollowObserved = TacticalObservedFrames >= 10 &&
            TacticalMaxUncommandedYawDelta <= 0.10f;
        return;
    }

    const bool bReturnedFromTactical = bWasTactical;
    bWasTactical = false;
    if (ManualOverrideRemaining > 0.0f)
    {
        ManualOverrideRemaining = FMath::Max(0.0f, ManualOverrideRemaining - DeltaSeconds);
        bEvidenceManualHoldObserved = bEvidenceManualHoldObserved || bEvidenceManualDragObserved;
        bAutoFollowActive = false;
        ApplyManagedYaw();
        return;
    }
    if (IsUiInputActive())
    {
        bAutoFollowActive = false;
        ApplyManagedYaw();
        return;
    }

    float DesiredYaw = Adapter->GetCameraYaw();
    if (!ResolveDesiredYaw(DesiredYaw))
    {
        bAutoFollowActive = false;
        ApplyManagedYaw();
        return;
    }

    const float BeforeVisualYaw = AcceptedPlayer->GetCameraBoom()
        ? AcceptedPlayer->GetCameraBoom()->GetTargetRotation().Yaw : ManagedYaw;
    const float BeforeYaw = ManagedYaw;
    ManagedYaw = InterpolateYaw(BeforeYaw, DesiredYaw, DeltaSeconds, Settings->YawInterpSpeed);
    ApplyManagedYaw();
    LastVisualTargetYaw = AcceptedPlayer->GetCameraBoom()
        ? AStageG2ACameraModeAdapter::NormalizeYaw(
            AcceptedPlayer->GetCameraBoom()->GetTargetRotation().Yaw)
        : AStageG2ACameraModeAdapter::NormalizeYaw(ManagedYaw);

    LastDesiredYaw = AStageG2ACameraModeAdapter::NormalizeYaw(DesiredYaw);
    bAutoFollowActive = true;
    ++AutoFollowApplyCount;
    const bool bYawChanged = FMath::Abs(FMath::FindDeltaAngleDegrees(
        BeforeVisualYaw, LastVisualTargetYaw)) > 0.01f;
    bEvidenceAutoFollowObserved = bEvidenceAutoFollowObserved || bYawChanged;
    bStandardFollowSeenBeforeTactical = true;
    if (bEvidenceManualHoldObserved) bEvidenceResumedAfterManual = true;
    if (bReturnedFromTactical && bStandardFollowSeenBeforeTactical)
        bEvidenceReturnedStandardFollow = true;
}

void AStageG3AStandardCameraAutoFollowActor::WriteEvidenceReport() const
{
    const UStageG3ACameraAutoFollowSettings* Settings = GetDefault<UStageG3ACameraAutoFollowSettings>();
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("scope"), TEXT("Stage G-3A StandardCharacterCamera auto Yaw follow"));
    Root->SetNumberField(TEXT("yaw_interp_speed"), Settings->YawInterpSpeed);
    Root->SetNumberField(TEXT("manual_override_seconds"), Settings->ManualOverrideSeconds);
    Root->SetNumberField(TEXT("movement_speed_threshold"), Settings->MovementSpeedThreshold);
    Root->SetNumberField(TEXT("destination_direction_min_distance"), Settings->DestinationDirectionMinDistance);
    Root->SetBoolField(TEXT("standard_autofollow_observed"), bEvidenceAutoFollowObserved);
    Root->SetBoolField(TEXT("manual_drag_priority_observed"), bEvidenceManualDragObserved);
    Root->SetBoolField(TEXT("manual_release_hold_observed"), bEvidenceManualHoldObserved);
    Root->SetBoolField(TEXT("standard_autofollow_resumed_after_drag"), bEvidenceResumedAfterManual);
    Root->SetBoolField(TEXT("tactical_autofollow_disabled"), bEvidenceTacticalNoFollowObserved);
    Root->SetBoolField(TEXT("return_standard_autofollow_observed"), bEvidenceReturnedStandardFollow);
    Root->SetNumberField(TEXT("tactical_max_uncommanded_yaw_delta"), TacticalMaxUncommandedYawDelta);
    Root->SetNumberField(TEXT("autofollow_apply_count"), AutoFollowApplyCount);
    Root->SetNumberField(TEXT("visual_springarm_target_yaw"), LastVisualTargetYaw);
    Root->SetNumberField(TEXT("visual_target_error_degrees"),
        FMath::Abs(FMath::FindDeltaAngleDegrees(LastVisualTargetYaw, LastDesiredYaw)));
    Root->SetStringField(TEXT("target_priority"), TEXT("velocity_then_destination_then_hold_current"));

    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Root, Writer);
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG3A"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FFileHelper::SaveStringToFile(Json,
        *FPaths::Combine(Directory, TEXT("camera_autofollow_evidence.json")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
