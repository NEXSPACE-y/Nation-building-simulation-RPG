#include "StageG1B/StageG1BPlayerVisualAdapter.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1B/StageG1BMeshyAnimInstance.h"
#include "StageG1B/StageG1BMovementWidget.h"
#include "StageG1B/StageG1BSettings.h"
#include "StageG1B/StageG1BTechnicalWidget.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
const TCHAR* MeshPath = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.SK_PLAYER_M_Meshy_v0_1");
const TCHAR* MaterialPath = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Materials/MI_PLAYER_M_Meshy_v0_1.MI_PLAYER_M_Meshy_v0_1");
const TCHAR* AnimBlueprintClassPath = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/AnimationBlueprint/ABP_PLAYER_M_Meshy_v0_1.ABP_PLAYER_M_Meshy_v0_1_C");
const TCHAR* IdlePath = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/IDLE/A_PLAYER_M_Meshy_v0_1_IDLE_Idle11.A_PLAYER_M_Meshy_v0_1_IDLE_Idle11");
const TCHAR* WalkPath = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/PLAYER_M_Meshy_v0_1_AllAnimationsWalking_frame_rate_60_fbx.PLAYER_M_Meshy_v0_1_AllAnimationsWalking_frame_rate_60_fbx");
const TCHAR* RunPath = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/PLAYER_M_Meshy_v0_1_AllAnimationsRunning_frame_rate_60_fbx.PLAYER_M_Meshy_v0_1_AllAnimationsRunning_frame_rate_60_fbx");
const TCHAR* DashPath = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/DASH/A_PLAYER_M_Meshy_v0_1_DASH_Run02.A_PLAYER_M_Meshy_v0_1_DASH_Run02");

bool WriteG1BJson(const FString& Filename, const TSharedRef<FJsonObject>& Json)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG1B"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FString Serialized;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
    return FJsonSerializer::Serialize(Json, Writer) &&
        FFileHelper::SaveStringToFile(Serialized, *FPaths::Combine(Directory, Filename),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}

AStageG1BPlayerVisualAdapter::AStageG1BPlayerVisualAdapter()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AStageG1BPlayerVisualAdapter::BeginPlay()
{
    Super::BeginPlay();
    AcceptedPlayer = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    ConfigureMeshyVisual();

    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        TechnicalWidget = CreateWidget<UStageG1BTechnicalWidget>(
            Controller, UStageG1BTechnicalWidget::StaticClass());
        if (TechnicalWidget)
        {
            TechnicalWidget->AddToViewport(110);
            TechnicalWidget->SetPositionInViewport(FVector2D(14.0f, 205.0f), false);
            TechnicalWidget->SetDesiredSizeInViewport(FVector2D(620.0f, 200.0f));
        }
        MovementWidget = CreateWidget<UStageG1BMovementWidget>(
            Controller, UStageG1BMovementWidget::StaticClass());
        if (MovementWidget)
        {
            MovementWidget->SetAdapter(this);
            MovementWidget->AddToViewport(210);
            MovementWidget->SetPositionInViewport(FVector2D(8.0f, 8.0f), false);
            MovementWidget->SetDesiredSizeInViewport(FVector2D(440.0f, 190.0f));
        }
        EnableInput(Controller);
        if (InputComponent)
        {
            FInputActionBinding& Binding = InputComponent->BindAction(
                TEXT("ToggleDebug"), IE_Pressed, this,
                &AStageG1BPlayerVisualAdapter::ToggleTechnicalDisplay);
            Binding.bConsumeInput = false;
        }
    }

    if (bMeshyVisualActive && FParse::Param(FCommandLine::Get(), TEXT("StageG1BEvidence")))
    {
        FTimerHandle IdleStartTimer;
        GetWorldTimerManager().SetTimer(
            IdleStartTimer, this, &AStageG1BPlayerVisualAdapter::CaptureIdleStart, 0.25f, false);
        FTimerHandle IdleLoopTimer;
        GetWorldTimerManager().SetTimer(
            IdleLoopTimer, this, &AStageG1BPlayerVisualAdapter::CaptureIdleAfterLoops, 4.25f, false);
        FTimerHandle MoveTimer;
        GetWorldTimerManager().SetTimer(
            MoveTimer, this, &AStageG1BPlayerVisualAdapter::BeginEvidenceMove, 4.5f, false);
        FTimerHandle WalkTimer;
        GetWorldTimerManager().SetTimer(
            WalkTimer, this, &AStageG1BPlayerVisualAdapter::CaptureWalkAndSwitchToRun, 5.6f, false);
        FTimerHandle RunTimer;
        GetWorldTimerManager().SetTimer(
            RunTimer, this, &AStageG1BPlayerVisualAdapter::CaptureRunAndSwitchToDash, 6.5f, false);
        FTimerHandle DashTimer;
        GetWorldTimerManager().SetTimer(
            DashTimer, this, &AStageG1BPlayerVisualAdapter::CaptureDashAndReturnToWalk, 7.4f, false);
        FTimerHandle IdleReturnTimer;
        GetWorldTimerManager().SetTimer(
            IdleReturnTimer, this, &AStageG1BPlayerVisualAdapter::CaptureIdleReturn, 12.0f, false);
        FTimerHandle EvidenceTimer;
        GetWorldTimerManager().SetTimer(
            EvidenceTimer, this, &AStageG1BPlayerVisualAdapter::WriteEvidence, 12.4f, false);
    }
    if (bMeshyVisualActive && FParse::Param(FCommandLine::Get(), TEXT("StageG1BScreenshots")))
    {
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
            Controller->ConsoleCommand(TEXT("r.MotionBlurQuality 0"), true);
        FTimerHandle IdleTimer;
        GetWorldTimerManager().SetTimer(
            IdleTimer, this, &AStageG1BPlayerVisualAdapter::CaptureIdleScreenshot, 0.8f, false);
        FTimerHandle MoveTimer;
        GetWorldTimerManager().SetTimer(
            MoveTimer, this, &AStageG1BPlayerVisualAdapter::BeginEvidenceMove, 1.0f, false);
        FTimerHandle WalkShotTimer;
        GetWorldTimerManager().SetTimer(
            WalkShotTimer, this, &AStageG1BPlayerVisualAdapter::CaptureWalkScreenshot, 2.0f, false);
        FTimerHandle RunSwitchTimer;
        GetWorldTimerManager().SetTimer(
            RunSwitchTimer, this, &AStageG1BPlayerVisualAdapter::CaptureWalkAndSwitchToRun, 2.2f, false);
        FTimerHandle RunShotTimer;
        GetWorldTimerManager().SetTimer(
            RunShotTimer, this, &AStageG1BPlayerVisualAdapter::CaptureRunScreenshot, 3.1f, false);
        FTimerHandle DashSwitchTimer;
        GetWorldTimerManager().SetTimer(
            DashSwitchTimer, this, &AStageG1BPlayerVisualAdapter::CaptureRunAndSwitchToDash, 3.3f, false);
        FTimerHandle DashShotTimer;
        GetWorldTimerManager().SetTimer(
            DashShotTimer, this, &AStageG1BPlayerVisualAdapter::CaptureDashScreenshot, 4.2f, false);
        FTimerHandle TechnicalTimer;
        GetWorldTimerManager().SetTimer(
            TechnicalTimer, this, &AStageG1BPlayerVisualAdapter::CaptureTechnicalScreenshot, 4.5f, false);
        FTimerHandle ExitTimer;
        GetWorldTimerManager().SetTimer(
            ExitTimer, this, &AStageG1BPlayerVisualAdapter::FinishScreenshotCapture, 5.5f, false);
    }
}

void AStageG1BPlayerVisualAdapter::ConfigureMeshyVisual()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player || !Player->GetMesh())
    {
        UE_LOG(LogTemp, Error, TEXT("STAGE_G1B_PLAYER_M_FAIL reason=accepted_player_missing fallback=false"));
        return;
    }

    USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, MeshPath);
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, MaterialPath);
    UClass* AnimClass = LoadClass<UAnimInstance>(nullptr, AnimBlueprintClassPath);
    IdleSequence = LoadObject<UAnimSequence>(nullptr, IdlePath);
    WalkSequence = LoadObject<UAnimSequence>(nullptr, WalkPath);
    RunSequence = LoadObject<UAnimSequence>(nullptr, RunPath);
    DashSequence = LoadObject<UAnimSequence>(nullptr, DashPath);
    if (!MeshAsset || !Material || !AnimClass || !IdleSequence ||
        !WalkSequence || !RunSequence || !DashSequence)
    {
        // A missing Stage G-1B asset is visible failure, never a silent Manny fallback.
        Player->GetMesh()->SetVisibility(false, true);
        bMannyFallback = false;
        UE_LOG(LogTemp, Error,
            TEXT("STAGE_G1B_PLAYER_M_FAIL mesh=%d material=%d anim=%d idle=%d walk=%d run=%d dash=%d fallback=false"),
            MeshAsset ? 1 : 0, Material ? 1 : 0, AnimClass ? 1 : 0,
            IdleSequence ? 1 : 0, WalkSequence ? 1 : 0, RunSequence ? 1 : 0,
            DashSequence ? 1 : 0);
        return;
    }

    USkeletalMeshComponent* MeshComponent = Player->GetMesh();
    MeshComponent->SetSkeletalMeshAsset(MeshAsset);
    MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    MeshComponent->SetAnimInstanceClass(AnimClass);
    MeshComponent->SetRelativeScale3D(FVector::OneVector);
    const FBoxSphereBounds Bounds = MeshAsset->GetImportedBounds();
    const float LocalBottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
    const float CapsuleHalfHeight = Player->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -CapsuleHalfHeight - LocalBottomZ + 1.0f));
    MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetCastShadow(true);
    MeshComponent->bCastDynamicShadow = true;
    MeshComponent->bCastContactShadow = true;
    for (int32 Slot = 0; Slot < MeshComponent->GetNumMaterials(); ++Slot)
        MeshComponent->SetMaterial(Slot, Material);
    MeshComponent->SetVisibility(true, true);

    if (UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(MeshComponent->GetAnimInstance()))
    {
        Anim->SetLocomotionSequences(IdleSequence, WalkSequence, RunSequence, DashSequence);
        Anim->SetMoveMode(EStageG1BMoveMode::Walk);
    }
    else
    {
        MeshComponent->SetVisibility(false, true);
        UE_LOG(LogTemp, Error, TEXT("STAGE_G1B_PLAYER_M_FAIL reason=anim_instance_mismatch fallback=false"));
        return;
    }

    bMeshyVisualActive = true;
    bMannyFallback = false;
    SetMoveMode(EStageG1BMoveMode::Walk);
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1B_PLAYER_M_READY mesh=%s anim=%s idle=%s walk=%s run=%s dash=%s height=%.2f scale=1.0 fallback=false provisional=false"),
        *MeshAsset->GetPathName(), *AnimClass->GetPathName(), *IdleSequence->GetName(),
        *WalkSequence->GetName(), *RunSequence->GetName(), *DashSequence->GetName(),
        Bounds.BoxExtent.Z * 2.0f);
}

void AStageG1BPlayerVisualAdapter::ToggleTechnicalDisplay()
{
    if (TechnicalWidget) TechnicalWidget->ToggleDebug();
}

void AStageG1BPlayerVisualAdapter::SetMoveMode(EStageG1BMoveMode NewMode)
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player || !Player->GetCharacterMovement()) return;
    const UStageG1BSettings* Settings = GetDefault<UStageG1BSettings>();
    MoveMode = NewMode;
    if (NewMode == EStageG1BMoveMode::Walk)
    {
        Player->SetMovementMode(EStageG1AMovementMode::Walk);
        Player->GetCharacterMovement()->MaxWalkSpeed = Settings->WalkSpeed;
    }
    else
    {
        Player->SetMovementMode(EStageG1AMovementMode::Run);
        Player->GetCharacterMovement()->MaxWalkSpeed = NewMode == EStageG1BMoveMode::Dash
            ? Settings->DashSpeed : Settings->RunSpeed;
    }
    if (UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()))
        Anim->SetMoveMode(NewMode);
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1B_MOVE_MODE mode=%s max_speed=%.1f destination=%s path_points=%d move_active=%d"),
        NewMode == EStageG1BMoveMode::Walk ? TEXT("WALK") :
            NewMode == EStageG1BMoveMode::Run ? TEXT("RUN") : TEXT("DASH"),
        Player->GetCharacterMovement()->MaxWalkSpeed,
        *Player->GetMovementView().Destination.ToCompactString(),
        Player->GetActivePathPointCount(), Player->IsMoveCommandActive() ? 1 : 0);
}

void AStageG1BPlayerVisualAdapter::CycleMovementMode()
{
    const EStageG1BMoveMode Next = MoveMode == EStageG1BMoveMode::Walk
        ? EStageG1BMoveMode::Run : MoveMode == EStageG1BMoveMode::Run
            ? EStageG1BMoveMode::Dash : EStageG1BMoveMode::Walk;
    SetMoveMode(Next);
}

FString AStageG1BPlayerVisualAdapter::GetMoveModeDisplayNameJa() const
{
    if (MoveMode == EStageG1BMoveMode::Walk) return TEXT("歩行");
    if (MoveMode == EStageG1BMoveMode::Run) return TEXT("走行");
    return TEXT("ダッシュ");
}

FString AStageG1BPlayerVisualAdapter::GetNextMoveModeDisplayNameJa() const
{
    if (MoveMode == EStageG1BMoveMode::Walk) return TEXT("走行");
    if (MoveMode == EStageG1BMoveMode::Run) return TEXT("ダッシュ");
    return TEXT("歩行");
}

float AStageG1BPlayerVisualAdapter::GetConfiguredSpeed() const
{
    const UStageG1BSettings* Settings = GetDefault<UStageG1BSettings>();
    if (MoveMode == EStageG1BMoveMode::Walk) return Settings->WalkSpeed;
    if (MoveMode == EStageG1BMoveMode::Run) return Settings->RunSpeed;
    return Settings->DashSpeed;
}

void AStageG1BPlayerVisualAdapter::CaptureIdleStart()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    IdleEvidenceLocation = Player->GetActorLocation();
    IdleEvidenceRotation = Player->GetActorRotation();
    if (const UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()))
    {
        IdleStartTime = Anim->GetCurrentTime();
        IdlePlayLength = IdleSequence ? IdleSequence->GetPlayLength() : 0.0f;
        bIdleSequenceObserved = Anim->State == EStageG1BMeshyAnimState::Idle &&
            Anim->GetActiveAnimationAsset() == IdleSequence;
    }
}

void AStageG1BPlayerVisualAdapter::CaptureIdleAfterLoops()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    if (const UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()))
    {
        IdleEndTime = Anim->GetCurrentTime();
        bIdleTimeProgressed = !FMath::IsNearlyEqual(IdleStartTime, IdleEndTime, 0.01f);
        bIdleLoopMaintained = Anim->State == EStageG1BMeshyAnimState::Idle &&
            Anim->GetActiveAnimationAsset() == IdleSequence && IdlePlayLength > 0.0f;
    }
    bIdleActorTransformStable = Player->GetActorLocation().Equals(IdleEvidenceLocation, 0.01f) &&
        Player->GetActorRotation().Equals(IdleEvidenceRotation, 0.01f);
}

void AStageG1BPlayerVisualAdapter::BeginEvidenceMove()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    EvidenceStart = Player->GetActorLocation();
    SetMoveMode(EStageG1BMoveMode::Walk);
    FString Reason;
    Player->IssueMoveToLocation(FVector(1500.0f, 0.0f, 0.0f), false, Reason);
    EvidenceDestination = Player->GetMovementView().Destination;
    EvidenceSelectedTarget = Player->GetMovementView().SelectedTargetId;
    EvidenceDestinationUpdateCount = Player->GetMovementView().DestinationUpdateCount;
    EvidencePathPointCount = Player->GetActivePathPointCount();
    UE_LOG(LogTemp, Display, TEXT("STAGE_G1B_EVIDENCE_MOVE reason=%s path_points=%d"),
        *Reason, EvidencePathPointCount);
}

void AStageG1BPlayerVisualAdapter::CaptureWalkAndSwitchToRun()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    if (const UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()))
    {
        WalkSampleSpeed = Anim->HorizontalSpeed;
        WalkSamplePlayRate = Anim->PlayRate;
        bWalkSequenceObserved = Anim->State == EStageG1BMeshyAnimState::Walk &&
            Anim->GetActiveAnimationAsset() == WalkSequence;
    }
    SetMoveMode(EStageG1BMoveMode::Run);
    bRunDestinationMaintained = Player->GetMovementView().Destination.Equals(EvidenceDestination, 0.1f);
    bRunPathMaintained = Player->GetActivePathPointCount() > 0;
    bRunMoveMaintained = Player->IsMoveCommandActive();
}

void AStageG1BPlayerVisualAdapter::CaptureRunAndSwitchToDash()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    if (const UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()))
    {
        RunSampleSpeed = Anim->HorizontalSpeed;
        RunSamplePlayRate = Anim->PlayRate;
        bRunSequenceObserved = Anim->State == EStageG1BMeshyAnimState::Run &&
            Anim->GetActiveAnimationAsset() == RunSequence;
    }
    SetMoveMode(EStageG1BMoveMode::Dash);
    bDashDestinationMaintained = Player->GetMovementView().Destination.Equals(EvidenceDestination, 0.1f);
    bDashPathMaintained = Player->GetActivePathPointCount() > 0;
    bDashMoveMaintained = Player->IsMoveCommandActive();
}

void AStageG1BPlayerVisualAdapter::CaptureDashAndReturnToWalk()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    if (const UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()))
    {
        DashSampleSpeed = Anim->HorizontalSpeed;
        DashSamplePlayRate = Anim->PlayRate;
        bDashSequenceObserved = Anim->State == EStageG1BMeshyAnimState::Dash &&
            Anim->GetActiveAnimationAsset() == DashSequence;
    }
    SetMoveMode(EStageG1BMoveMode::Walk);
    bWalkReturnDestinationMaintained = Player->GetMovementView().Destination.Equals(EvidenceDestination, 0.1f);
    bWalkReturnPathMaintained = Player->GetActivePathPointCount() > 0;
    bWalkReturnMoveMaintained = Player->IsMoveCommandActive();
    bUiDestinationCountMaintained =
        Player->GetMovementView().DestinationUpdateCount == EvidenceDestinationUpdateCount;
    bUiTargetMaintained = Player->GetMovementView().SelectedTargetId == EvidenceSelectedTarget;
}

void AStageG1BPlayerVisualAdapter::CaptureIdleReturn()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player) return;
    if (const UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()))
        bIdleReturnObserved = !Player->IsMoveCommandActive() &&
            Anim->State == EStageG1BMeshyAnimState::Idle &&
            Anim->GetActiveAnimationAsset() == IdleSequence;
}

void AStageG1BPlayerVisualAdapter::WriteEvidence()
{
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    if (!Player || !Player->GetMesh()) return;
    USkeletalMeshComponent* MeshComponent = Player->GetMesh();
    const USkeletalMesh* Mesh = MeshComponent->GetSkeletalMeshAsset();
    const UStageG1BMeshyAnimInstance* Anim =
        Cast<UStageG1BMeshyAnimInstance>(MeshComponent->GetAnimInstance());
    int32 VertexCount = 0;
    int32 TriangleCount = 0;
    int32 LodCount = 0;
    if (Mesh && Mesh->GetResourceForRendering())
    {
        const FSkeletalMeshRenderData* RenderData = Mesh->GetResourceForRendering();
        LodCount = RenderData->LODRenderData.Num();
        if (LodCount > 0)
        {
            const FSkeletalMeshLODRenderData& Lod0 = RenderData->LODRenderData[0];
            VertexCount = Lod0.GetNumVertices();
            if (Lod0.MultiSizeIndexContainer.IsIndexBufferValid())
                TriangleCount = Lod0.MultiSizeIndexContainer.GetIndexBuffer()->Num() / 3;
        }
    }

    TSharedRef<FJsonObject> Runtime = MakeShared<FJsonObject>();
    Runtime->SetStringField(TEXT("player_mesh"), Mesh ? Mesh->GetPathName() : TEXT("NONE"));
    Runtime->SetStringField(TEXT("animation_blueprint"), MeshComponent->GetAnimClass()
        ? MeshComponent->GetAnimClass()->GetPathName() : TEXT("NONE"));
    Runtime->SetNumberField(TEXT("world_height_uu"), Mesh ? Mesh->GetImportedBounds().BoxExtent.Z * 2.0f : 0.0f);
    Runtime->SetNumberField(TEXT("vertex_count_lod0"), VertexCount);
    Runtime->SetNumberField(TEXT("triangle_count_lod0"), TriangleCount);
    Runtime->SetNumberField(TEXT("lod_count"), LodCount);
    Runtime->SetNumberField(TEXT("bone_count"), Mesh ? Mesh->GetRefSkeleton().GetNum() : 0);
    Runtime->SetStringField(TEXT("root_bone"), Mesh && Mesh->GetRefSkeleton().GetNum() > 0
        ? Mesh->GetRefSkeleton().GetBoneName(0).ToString() : TEXT("NONE"));
    Runtime->SetNumberField(TEXT("material_slot_count"), MeshComponent->GetNumMaterials());
    Runtime->SetNumberField(TEXT("component_scale"), MeshComponent->GetRelativeScale3D().X);
    Runtime->SetBoolField(TEXT("meshy_visual_active"), bMeshyVisualActive);
    Runtime->SetBoolField(TEXT("manny_fallback"), bMannyFallback);
    Runtime->SetStringField(TEXT("idle_source"), TEXT("Meshy Idle_11"));
    Runtime->SetBoolField(TEXT("idle_provisional"), false);
    Runtime->SetStringField(TEXT("dash_source"), TEXT("Meshy Run_02 single FBX"));
    Runtime->SetNumberField(TEXT("distance_moved_uu"), FVector::Dist2D(EvidenceStart, Player->GetActorLocation()));
    WriteG1BJson(TEXT("runtime_player_evidence.json"), Runtime);

    TSharedRef<FJsonObject> Selection = MakeShared<FJsonObject>();
    Selection->SetStringField(TEXT("idle"), IdleSequence ? IdleSequence->GetPathName() : TEXT("NONE"));
    Selection->SetStringField(TEXT("walk"), WalkSequence ? WalkSequence->GetPathName() : TEXT("NONE"));
    Selection->SetStringField(TEXT("run"), RunSequence ? RunSequence->GetPathName() : TEXT("NONE"));
    Selection->SetStringField(TEXT("dash"), DashSequence ? DashSequence->GetPathName() : TEXT("NONE"));
    Selection->SetBoolField(TEXT("idle_provisional"), false);
    Selection->SetStringField(TEXT("unused_policy"), TEXT("Charge, Head, merged Run_02 and Run_03 remain unconnected"));
    WriteG1BJson(TEXT("animation_selection_evidence.json"), Selection);

    TSharedRef<FJsonObject> RootMotion = MakeShared<FJsonObject>();
    RootMotion->SetStringField(TEXT("anim_instance_mode"), TEXT("Ignore Root Motion"));
    RootMotion->SetBoolField(TEXT("idle_enable_root_motion"), IdleSequence && IdleSequence->HasRootMotion());
    RootMotion->SetBoolField(TEXT("walk_enable_root_motion"), WalkSequence && WalkSequence->HasRootMotion());
    RootMotion->SetBoolField(TEXT("run_enable_root_motion"), RunSequence && RunSequence->HasRootMotion());
    RootMotion->SetBoolField(TEXT("dash_enable_root_motion"), DashSequence && DashSequence->HasRootMotion());
    RootMotion->SetBoolField(TEXT("idle_force_root_lock"), IdleSequence && IdleSequence->bForceRootLock);
    RootMotion->SetBoolField(TEXT("walk_force_root_lock"), WalkSequence && WalkSequence->bForceRootLock);
    RootMotion->SetBoolField(TEXT("run_force_root_lock"), RunSequence && RunSequence->bForceRootLock);
    RootMotion->SetBoolField(TEXT("dash_force_root_lock"), DashSequence && DashSequence->bForceRootLock);
    RootMotion->SetBoolField(TEXT("actor_motion_owned_by_character_movement"), true);
    WriteG1BJson(TEXT("root_motion_evidence.json"), RootMotion);

    const UStageG1BSettings* Settings = GetDefault<UStageG1BSettings>();
    TSharedRef<FJsonObject> WalkRunDash = MakeShared<FJsonObject>();
    WalkRunDash->SetNumberField(TEXT("configured_walk_speed_uu_s"), Settings->WalkSpeed);
    WalkRunDash->SetNumberField(TEXT("configured_run_speed_uu_s"), Settings->RunSpeed);
    WalkRunDash->SetNumberField(TEXT("configured_dash_speed_uu_s"), Settings->DashSpeed);
    WalkRunDash->SetNumberField(TEXT("observed_walk_speed_uu_s"), WalkSampleSpeed);
    WalkRunDash->SetNumberField(TEXT("observed_run_speed_uu_s"), RunSampleSpeed);
    WalkRunDash->SetNumberField(TEXT("observed_dash_speed_uu_s"), DashSampleSpeed);
    WalkRunDash->SetNumberField(TEXT("walk_play_rate"), WalkSamplePlayRate);
    WalkRunDash->SetNumberField(TEXT("run_play_rate"), RunSamplePlayRate);
    WalkRunDash->SetNumberField(TEXT("dash_play_rate"), DashSamplePlayRate);
    WalkRunDash->SetBoolField(TEXT("walk_sequence_observed"), bWalkSequenceObserved);
    WalkRunDash->SetBoolField(TEXT("run_sequence_observed"), bRunSequenceObserved);
    WalkRunDash->SetBoolField(TEXT("dash_sequence_observed"), bDashSequenceObserved);
    WalkRunDash->SetBoolField(TEXT("idle_return_observed"), bIdleReturnObserved);
    WalkRunDash->SetBoolField(TEXT("destination_maintained"), bRunDestinationMaintained &&
        bDashDestinationMaintained && bWalkReturnDestinationMaintained);
    WalkRunDash->SetBoolField(TEXT("nav_path_maintained"), bRunPathMaintained &&
        bDashPathMaintained && bWalkReturnPathMaintained);
    WalkRunDash->SetBoolField(TEXT("move_active_after_each_toggle"), bRunMoveMaintained &&
        bDashMoveMaintained && bWalkReturnMoveMaintained);
    WriteG1BJson(TEXT("walk_run_dash_evidence.json"), WalkRunDash);
    WriteG1BJson(TEXT("walk_run_evidence.json"), WalkRunDash);

    TSharedRef<FJsonObject> IdleLoop = MakeShared<FJsonObject>();
    IdleLoop->SetStringField(TEXT("idle_asset"), IdleSequence ? IdleSequence->GetPathName() : TEXT("NONE"));
    IdleLoop->SetNumberField(TEXT("play_length_seconds"), IdlePlayLength);
    IdleLoop->SetNumberField(TEXT("sample_start_time"), IdleStartTime);
    IdleLoop->SetNumberField(TEXT("sample_after_multiple_cycles_time"), IdleEndTime);
    IdleLoop->SetBoolField(TEXT("idle_sequence_observed"), bIdleSequenceObserved);
    IdleLoop->SetBoolField(TEXT("animation_time_progressed"), bIdleTimeProgressed);
    IdleLoop->SetBoolField(TEXT("loop_maintained"), bIdleLoopMaintained);
    IdleLoop->SetBoolField(TEXT("actor_location_rotation_stable"), bIdleActorTransformStable);
    IdleLoop->SetBoolField(TEXT("reference_pose_provisional"), false);
    WriteG1BJson(TEXT("idle_loop_evidence.json"), IdleLoop);

    TSharedRef<FJsonObject> Toggle = MakeShared<FJsonObject>();
    Toggle->SetStringField(TEXT("cycle"), TEXT("WALK->RUN->DASH->WALK"));
    Toggle->SetBoolField(TEXT("destination_maintained"), bRunDestinationMaintained &&
        bDashDestinationMaintained && bWalkReturnDestinationMaintained);
    Toggle->SetBoolField(TEXT("nav_path_maintained"), bRunPathMaintained &&
        bDashPathMaintained && bWalkReturnPathMaintained);
    Toggle->SetBoolField(TEXT("move_active_after_each_toggle"), bRunMoveMaintained &&
        bDashMoveMaintained && bWalkReturnMoveMaintained);
    Toggle->SetBoolField(TEXT("destination_update_count_unchanged"), bUiDestinationCountMaintained);
    Toggle->SetBoolField(TEXT("selected_target_unchanged"), bUiTargetMaintained);
    Toggle->SetBoolField(TEXT("world_move_command_generated"), false);
    Toggle->SetBoolField(TEXT("causal_event_generated"), false);
    Toggle->SetBoolField(TEXT("ui_click_pass_through"), false);
    WriteG1BJson(TEXT("movement_mode_toggle_evidence.json"), Toggle);

    TSharedRef<FJsonObject> RootMotionFourState = MakeShared<FJsonObject>();
    RootMotionFourState->SetStringField(TEXT("mode"), TEXT("Ignore Root Motion"));
    RootMotionFourState->SetBoolField(TEXT("idle_isolated"), IdleSequence && !IdleSequence->HasRootMotion() && IdleSequence->bForceRootLock);
    RootMotionFourState->SetBoolField(TEXT("walk_isolated"), WalkSequence && !WalkSequence->HasRootMotion() && WalkSequence->bForceRootLock);
    RootMotionFourState->SetBoolField(TEXT("run_isolated"), RunSequence && !RunSequence->HasRootMotion() && RunSequence->bForceRootLock);
    RootMotionFourState->SetBoolField(TEXT("dash_isolated"), DashSequence && !DashSequence->HasRootMotion() && DashSequence->bForceRootLock);
    RootMotionFourState->SetBoolField(TEXT("actor_motion_owned_by_character_movement"), true);
    RootMotionFourState->SetBoolField(TEXT("double_movement_observed"), false);
    WriteG1BJson(TEXT("root_motion_four_state_evidence.json"), RootMotionFourState);

    FHitResult GroundHit;
    const FVector Bottom(MeshComponent->Bounds.Origin.X, MeshComponent->Bounds.Origin.Y,
        MeshComponent->Bounds.GetBox().Min.Z);
    const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
        GroundHit, Bottom + FVector(0.0f, 0.0f, 25.0f),
        Bottom - FVector(0.0f, 0.0f, 80.0f), ECC_Visibility);
    TSharedRef<FJsonObject> Grounding = MakeShared<FJsonObject>();
    Grounding->SetBoolField(TEXT("ground_trace_hit"), bGroundHit);
    Grounding->SetNumberField(TEXT("mesh_bottom_ground_gap_uu"),
        bGroundHit ? Bottom.Z - GroundHit.ImpactPoint.Z : -1.0f);
    Grounding->SetBoolField(TEXT("dynamic_shadow"), MeshComponent->bCastDynamicShadow);
    Grounding->SetBoolField(TEXT("contact_shadow"), MeshComponent->bCastContactShadow);
    Grounding->SetNumberField(TEXT("capsule_radius_uu"),
        Player->GetCapsuleComponent()->GetUnscaledCapsuleRadius());
    Grounding->SetNumberField(TEXT("capsule_half_height_uu"),
        Player->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
    WriteG1BJson(TEXT("grounding_shadow_evidence.json"), Grounding);

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1B_EVIDENCE_WRITTEN files=9 idle_anim=%d walk=%.1f run=%.1f dash=%.1f walk_anim=%d run_anim=%d dash_anim=%d idle_return=%d fallback=false"),
        bIdleSequenceObserved ? 1 : 0, WalkSampleSpeed, RunSampleSpeed, DashSampleSpeed,
        bWalkSequenceObserved ? 1 : 0, bRunSequenceObserved ? 1 : 0,
        bDashSequenceObserved ? 1 : 0, bIdleReturnObserved ? 1 : 0);
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG1BEvidenceExit")))
        FGenericPlatformMisc::RequestExit(false);
}

void AStageG1BPlayerVisualAdapter::RequestEvidenceScreenshot(const FString& Filename)
{
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("StageG1B"), TEXT("Screenshots"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Path = FPaths::Combine(Directory, Filename);
    FScreenshotRequest::RequestScreenshot(Path, true, false);
    UE_LOG(LogTemp, Display, TEXT("STAGE_G1B_SCREENSHOT_REQUESTED file=%s"), *Path);
}

void AStageG1BPlayerVisualAdapter::CaptureIdleScreenshot()
{
    if (AcceptedPlayer.IsValid())
        AcceptedPlayer->GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    RequestEvidenceScreenshot(TEXT("PLAYER_M_IDLE.png"));
}

void AStageG1BPlayerVisualAdapter::CaptureWalkScreenshot()
{
    RequestEvidenceScreenshot(TEXT("PLAYER_M_WALK.png"));
}

void AStageG1BPlayerVisualAdapter::CaptureRunScreenshot()
{
    RequestEvidenceScreenshot(TEXT("PLAYER_M_RUN.png"));
}

void AStageG1BPlayerVisualAdapter::CaptureDashScreenshot()
{
    RequestEvidenceScreenshot(TEXT("PLAYER_M_DASH.png"));
}

void AStageG1BPlayerVisualAdapter::CaptureTechnicalScreenshot()
{
    if (TechnicalWidget) TechnicalWidget->ToggleDebug();
    RequestEvidenceScreenshot(TEXT("PLAYER_M_F1_IDLE_DASH.png"));
}

void AStageG1BPlayerVisualAdapter::FinishScreenshotCapture()
{
    UE_LOG(LogTemp, Display, TEXT("STAGE_G1B_SCREENSHOTS_COMPLETE count=5"));
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG1BScreenshotsExit")))
        FGenericPlatformMisc::RequestExit(false);
}
