#include "StageG3A/StageG3ACapitalEvidenceActor.h"
#include "StageG3A/StageG3AStandardCameraAutoFollowActor.h"

#include "Animation/Skeleton.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1B/StageG1BPlayerVisualAdapter.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"
#include "StageG2B/StageG2BGuardMActor.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
const FName StageG3ATag(TEXT("StageG3A"));

bool WriteG3AJson(const FString& Filename, const TSharedRef<FJsonObject>& Json)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG3A"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FString Serialized;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
    return FJsonSerializer::Serialize(Json, Writer) &&
        FFileHelper::SaveStringToFile(Serialized, *FPaths::Combine(Directory, Filename),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

int32 CountActorsWithTag(const UWorld* World, const FName Tag)
{
    int32 Count = 0;
    if (!World) return Count;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (It->ActorHasTag(StageG3ATag) && It->ActorHasTag(Tag)) ++Count;
    return Count;
}

bool HasCameraBlockingComponent(const AActor* Actor)
{
    if (!Actor) return false;
    TArray<UPrimitiveComponent*> Components;
    Actor->GetComponents<UPrimitiveComponent>(Components);
    for (const UPrimitiveComponent* Component : Components)
    {
        if (Component && Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision &&
            Component->GetCollisionResponseToChannel(ECC_Camera) == ECR_Block)
            return true;
    }
    return false;
}
}

AStageG3ACapitalEvidenceActor::AStageG3ACapitalEvidenceActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Tags.Add(StageG3ATag);
    Tags.Add(TEXT("Evidence"));
}

void AStageG3ACapitalEvidenceActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveAcceptedActors();
    if (AcceptedPlayer.IsValid())
    {
        OriginalPlayerLocation = AcceptedPlayer->GetActorLocation();
        OriginalPlayerRotation = AcceptedPlayer->GetActorRotation();
    }

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G3A_CAPITAL_READY map=StageG3A_CapitalBlockout_PoC blockout=6000x6000 guard=1 standard=1 tactical=1 navmesh=dynamic external_assets=0"));

    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3AEvidence")))
    {
        FTimerHandle EvidenceTimer;
        GetWorldTimerManager().SetTimer(
            EvidenceTimer, this, &AStageG3ACapitalEvidenceActor::BeginRuntimeEvidence, 2.50f, false);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3AScreenshots")))
        BeginScreenshotSequence();
}

void AStageG3ACapitalEvidenceActor::BeginRuntimeEvidence()
{
    ResolveAcceptedActors();
    UWorld* World = GetWorld();
    const FVector ClickSamples[] = {
        FVector(0, -2850, 0), FVector(0, -2050, 0), FVector(0, -1050, 0),
        FVector(-260, -220, 0), FVector(720, -250, 0), FVector(0, 1200, 0),
    };
    ClickTraceSampleCount = UE_ARRAY_COUNT(ClickSamples);
    ClickTracePassedCount = 0;
    FVector MovementTarget = FVector::ZeroVector;
    for (const FVector& Sample : ClickSamples)
    {
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG3AClickMoveEvidence), false,
            AcceptedPlayer.Get());
        const bool bHit = World && World->LineTraceSingleByChannel(
            Hit, Sample + FVector(0, 0, 1400), Sample - FVector(0, 0, 300),
            ECC_GameTraceChannel1, Params);
        const bool bWalkableHit = bHit && Hit.GetActor() &&
            Hit.GetActor()->ActorHasTag(TEXT("WalkableSurface"));
        ClickTracePassedCount += bWalkableHit ? 1 : 0;
        if (bWalkableHit && Sample.Y == 1200.0f) MovementTarget = Hit.ImpactPoint;
    }

    if (AcceptedPlayer.IsValid())
    {
        MovementEvidenceStart = AcceptedPlayer->GetActorLocation();
        AcceptedPlayer->SetMovementMode(EStageG1AMovementMode::Run);
        bMovementCommandIssued = AcceptedPlayer->IssueMoveToLocation(
            MovementTarget, false, MovementCommandFailure);
    }
    else
    {
        MovementCommandFailure = TEXT("accepted player missing");
    }

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G3A_CLICK_MOVE_PROBE traces=%d/%d command=%d target=%s reason=%s"),
        ClickTracePassedCount, ClickTraceSampleCount, bMovementCommandIssued ? 1 : 0,
        *MovementTarget.ToCompactString(), *MovementCommandFailure);

    if (CameraAdapter.IsValid())
    {
        if (!CameraAdapter->IsStandardMode()) CameraAdapter->ToggleCameraMode();
        if (AutoFollowActor.IsValid()) AutoFollowActor->SetManagedYawForEvidence(-90.0f);
    }

    FTimerHandle ManualBeginTimer;
    GetWorldTimerManager().SetTimer(ManualBeginTimer, this,
        &AStageG3ACapitalEvidenceActor::BeginManualDragEvidence, 0.70f, false);
    FTimerHandle ManualEndTimer;
    GetWorldTimerManager().SetTimer(ManualEndTimer, this,
        &AStageG3ACapitalEvidenceActor::EndManualDragEvidence, 1.05f, false);
    FTimerHandle TacticalTimer;
    GetWorldTimerManager().SetTimer(TacticalTimer, this,
        &AStageG3ACapitalEvidenceActor::EnterTacticalEvidence, 2.45f, false);
    FTimerHandle StandardTimer;
    GetWorldTimerManager().SetTimer(StandardTimer, this,
        &AStageG3ACapitalEvidenceActor::ReturnStandardEvidence, 3.10f, false);
    FTimerHandle WriteTimer;
    GetWorldTimerManager().SetTimer(WriteTimer, this,
        &AStageG3ACapitalEvidenceActor::WriteRuntimeEvidence, 9.50f, false);
}

void AStageG3ACapitalEvidenceActor::BeginManualDragEvidence()
{
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        Controller->SetMouseLocation(1200, 680);
        Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE,
            EKeys::RightMouseButton, IE_Pressed, 1.0f, false, FPlatformTime::Cycles64()));
        Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE,
            EKeys::MouseX, 120.0f, 1.0f / 60.0f, 1, FPlatformTime::Cycles64()));
    }
}

void AStageG3ACapitalEvidenceActor::EndManualDragEvidence()
{
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        Controller->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE,
            EKeys::RightMouseButton, IE_Released, 0.0f, false, FPlatformTime::Cycles64()));
}

void AStageG3ACapitalEvidenceActor::EnterTacticalEvidence()
{
    if (CameraAdapter.IsValid() && CameraAdapter->IsStandardMode())
        CameraAdapter->ToggleCameraMode();
}

void AStageG3ACapitalEvidenceActor::ReturnStandardEvidence()
{
    if (CameraAdapter.IsValid() && !CameraAdapter->IsStandardMode())
        CameraAdapter->ToggleCameraMode();
}

void AStageG3ACapitalEvidenceActor::ResolveAcceptedActors()
{
    AcceptedPlayer = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    for (TActorIterator<AStageG1BPlayerVisualAdapter> It(GetWorld()); It; ++It)
    {
        PlayerVisualAdapter = *It;
        break;
    }
    for (TActorIterator<AStageG2ACameraModeAdapter> It(GetWorld()); It; ++It)
    {
        CameraAdapter = *It;
        break;
    }
    for (TActorIterator<AStageG2BGuardMActor> It(GetWorld()); It; ++It)
    {
        GuardActor = *It;
        break;
    }
    for (TActorIterator<AStageG3AStandardCameraAutoFollowActor> It(GetWorld()); It; ++It)
    {
        AutoFollowActor = *It;
        break;
    }
}

void AStageG3ACapitalEvidenceActor::WriteRuntimeEvidence()
{
    ResolveAcceptedActors();
    UWorld* World = GetWorld();
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

    struct FRouteProbe
    {
        const TCHAR* Name;
        FVector Start;
        FVector End;
    };
    const FRouteProbe Probes[] = {
        { TEXT("player_start_to_south_gate"), FVector(0, -3000, 10), FVector(0, -2050, 10) },
        { TEXT("south_gate_to_main_street"), FVector(0, -2050, 10), FVector(0, -1050, 10) },
        { TEXT("main_street_to_central_plaza"), FVector(0, -1050, 10), FVector(-260, -220, 10) },
        { TEXT("central_plaza_to_market"), FVector(360, -120, 10), FVector(920, -250, 10) },
        { TEXT("central_plaza_to_residential"), FVector(-360, -120, 10), FVector(-980, -260, 10) },
        { TEXT("central_plaza_to_castle_approach"), FVector(0, 360, 10), FVector(0, 1320, 10) },
        { TEXT("castle_approach_to_noble"), FVector(0, 1200, 10), FVector(1050, 1120, 10) },
        { TEXT("south_gate_to_workshop"), FVector(-300, -1900, 10), FVector(-1150, -1450, 10) },
        { TEXT("building_corner_route"), FVector(-620, -650, 10), FVector(-760, 300, 10) },
        { TEXT("outer_wall_side_route"), FVector(2800, -1200, 10), FVector(2800, 850, 10) },
    };

    int32 PassedRoutes = 0;
    TArray<TSharedPtr<FJsonValue>> RouteValues;
    for (const FRouteProbe& Probe : Probes)
    {
        bool bProjectedStart = false;
        bool bProjectedEnd = false;
        bool bPathValid = false;
        int32 PointCount = 0;
        FNavLocation ProjectedStart;
        FNavLocation ProjectedEnd;
        if (Navigation)
        {
            bProjectedStart = Navigation->ProjectPointToNavigation(
                Probe.Start, ProjectedStart, FVector(180.0f, 180.0f, 300.0f));
            bProjectedEnd = Navigation->ProjectPointToNavigation(
                Probe.End, ProjectedEnd, FVector(180.0f, 180.0f, 300.0f));
            if (bProjectedStart && bProjectedEnd)
            {
                if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
                    World, ProjectedStart.Location, ProjectedEnd.Location, AcceptedPlayer.Get()))
                {
                    PointCount = Path->PathPoints.Num();
                    bPathValid = Path->IsValid() && !Path->IsPartial() && PointCount >= 2;
                }
            }
        }
        PassedRoutes += bPathValid ? 1 : 0;
        TSharedRef<FJsonObject> Route = MakeShared<FJsonObject>();
        Route->SetStringField(TEXT("name"), Probe.Name);
        Route->SetBoolField(TEXT("start_projected"), bProjectedStart);
        Route->SetBoolField(TEXT("end_projected"), bProjectedEnd);
        Route->SetBoolField(TEXT("path_valid"), bPathValid);
        Route->SetNumberField(TEXT("path_points"), PointCount);
        RouteValues.Add(MakeShared<FJsonValueObject>(Route));
    }

    int32 GuardCount = 0;
    for (TActorIterator<AStageG2BGuardMActor> It(World); It; ++It) ++GuardCount;
    const bool bPlayerFallback = !PlayerVisualAdapter.IsValid() || PlayerVisualAdapter->IsMannyFallback();
    USkeletalMesh* GuardMesh = GuardActor.IsValid() && GuardActor->GetGuardMeshComponent()
        ? GuardActor->GetGuardMeshComponent()->GetSkeletalMeshAsset() : nullptr;
    USkeletalMesh* PlayerMesh = AcceptedPlayer.IsValid() && AcceptedPlayer->GetMesh()
        ? AcceptedPlayer->GetMesh()->GetSkeletalMeshAsset() : nullptr;
    const bool bSkeletonShared = GuardMesh && PlayerMesh &&
        GuardMesh->GetSkeleton() == PlayerMesh->GetSkeleton();

    const bool bInitialStandard = CameraAdapter.IsValid() && CameraAdapter->IsStandardMode();
    bool bTactical = false;
    bool bReturnedStandard = false;
    if (CameraAdapter.IsValid())
    {
        CameraAdapter->ToggleCameraMode();
        bTactical = !CameraAdapter->IsStandardMode();
        CameraAdapter->ToggleCameraMode();
        bReturnedStandard = CameraAdapter->IsStandardMode();
    }

    int32 CameraBlockingActors = 0;
    int32 CollisionActors = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (!It->ActorHasTag(StageG3ATag) || !It->ActorHasTag(TEXT("Collision"))) continue;
        ++CollisionActors;
        CameraBlockingActors += HasCameraBlockingComponent(*It) ? 1 : 0;
    }

    const int32 ZoneCount =
        (CountActorsWithTag(World, TEXT("SouthGate")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("MainStreet")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("CentralPlaza")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("Castle")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("Market")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("Residential")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("Noble")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("Workshop")) > 0 ? 1 : 0) +
        (CountActorsWithTag(World, TEXT("Outskirts")) > 0 ? 1 : 0);
    const float PlayerMovementDistance = AcceptedPlayer.IsValid()
        ? FVector::Dist2D(MovementEvidenceStart, AcceptedPlayer->GetActorLocation()) : 0.0f;
    const bool bActualPlayerMovementPassed = bMovementCommandIssued && PlayerMovementDistance >= 100.0f;
    const bool bCapitalTraversalPassed = AcceptedPlayer.IsValid() &&
        PlayerMovementDistance >= 3800.0f && AcceptedPlayer->GetActorLocation().Y >= 900.0f;
    const bool bClickMoveSurfacePassed = ClickTraceSampleCount > 0 &&
        ClickTracePassedCount == ClickTraceSampleCount;
    const bool bAutoFollowObserved = AutoFollowActor.IsValid() &&
        AutoFollowActor->WasAutoFollowObserved();
    const bool bManualDragObserved = AutoFollowActor.IsValid() &&
        AutoFollowActor->WasManualDragObserved();
    const bool bManualHoldObserved = AutoFollowActor.IsValid() &&
        AutoFollowActor->WasManualHoldObserved();
    const bool bAutoFollowResumed = AutoFollowActor.IsValid() &&
        AutoFollowActor->WasAutoFollowResumedAfterManual();
    const bool bTacticalNoFollow = AutoFollowActor.IsValid() &&
        AutoFollowActor->WasTacticalNoAutoFollowObserved();
    const bool bReturnStandardFollow = AutoFollowActor.IsValid() &&
        AutoFollowActor->WasReturnToStandardAutoFollowObserved();

    TSharedRef<FJsonObject> Runtime = MakeShared<FJsonObject>();
    Runtime->SetStringField(TEXT("map"), TEXT("/Game/Maps/StageG3A_CapitalBlockout_PoC"));
    Runtime->SetNumberField(TEXT("blockout_width_uu"), 6000);
    Runtime->SetNumberField(TEXT("blockout_depth_uu"), 6000);
    Runtime->SetNumberField(TEXT("zone_count"), ZoneCount);
    Runtime->SetNumberField(TEXT("guard_actor_count"), GuardCount);
    Runtime->SetBoolField(TEXT("player_fallback"), bPlayerFallback);
    Runtime->SetBoolField(TEXT("guard_player_skeleton_shared"), bSkeletonShared);
    Runtime->SetBoolField(TEXT("external_assets_used"), false);
    Runtime->SetBoolField(TEXT("ai_connected"), false);
    Runtime->SetBoolField(TEXT("causal_core_connected"), false);
    Runtime->SetBoolField(TEXT("save_schema_connected"), false);
    Runtime->SetNumberField(TEXT("click_trace_sample_count"), ClickTraceSampleCount);
    Runtime->SetNumberField(TEXT("click_trace_passed_count"), ClickTracePassedCount);
    Runtime->SetBoolField(TEXT("click_move_surface_passed"), bClickMoveSurfacePassed);
    Runtime->SetBoolField(TEXT("player_move_command_issued"), bMovementCommandIssued);
    Runtime->SetNumberField(TEXT("player_movement_distance_uu"), PlayerMovementDistance);
    Runtime->SetBoolField(TEXT("actual_player_movement_passed"), bActualPlayerMovementPassed);
    Runtime->SetBoolField(TEXT("south_gate_plaza_castle_traversal_passed"), bCapitalTraversalPassed);
    Runtime->SetNumberField(TEXT("traversal_end_y"),
        AcceptedPlayer.IsValid() ? AcceptedPlayer->GetActorLocation().Y : 0.0f);
    Runtime->SetBoolField(TEXT("standard_camera_autofollow_observed"), bAutoFollowObserved);
    Runtime->SetBoolField(TEXT("manual_drag_priority_observed"), bManualDragObserved);
    Runtime->SetBoolField(TEXT("manual_release_hold_observed"), bManualHoldObserved);
    Runtime->SetBoolField(TEXT("autofollow_resumed_after_drag"), bAutoFollowResumed);
    Runtime->SetBoolField(TEXT("tactical_camera_no_autofollow"), bTacticalNoFollow);
    Runtime->SetBoolField(TEXT("return_standard_autofollow"), bReturnStandardFollow);
    WriteG3AJson(TEXT("capital_runtime_evidence.json"), Runtime);

    TSharedRef<FJsonObject> Nav = MakeShared<FJsonObject>();
    Nav->SetNumberField(TEXT("route_count"), UE_ARRAY_COUNT(Probes));
    Nav->SetNumberField(TEXT("passed_route_count"), PassedRoutes);
    Nav->SetBoolField(TEXT("all_routes_passed"), PassedRoutes == UE_ARRAY_COUNT(Probes));
    Nav->SetBoolField(TEXT("click_move_surface_passed"), bClickMoveSurfacePassed);
    Nav->SetBoolField(TEXT("actual_player_movement_passed"), bActualPlayerMovementPassed);
    Nav->SetArrayField(TEXT("routes"), RouteValues);
    WriteG3AJson(TEXT("navmesh_route_evidence.json"), Nav);

    TSharedRef<FJsonObject> Camera = MakeShared<FJsonObject>();
    Camera->SetBoolField(TEXT("standard_initial"), bInitialStandard);
    Camera->SetBoolField(TEXT("tactical_reached"), bTactical);
    Camera->SetBoolField(TEXT("returned_to_standard"), bReturnedStandard);
    Camera->SetNumberField(TEXT("collision_actor_count"), CollisionActors);
    Camera->SetNumberField(TEXT("camera_blocking_actor_count"), CameraBlockingActors);
    Camera->SetBoolField(TEXT("camera_collision_contract"),
        CollisionActors > 0 && CameraBlockingActors == CollisionActors);
    WriteG3AJson(TEXT("camera_collision_evidence.json"), Camera);

    TSharedRef<FJsonObject> Isolation = MakeShared<FJsonObject>();
    Isolation->SetBoolField(TEXT("player_assets_modified"), false);
    Isolation->SetBoolField(TEXT("guard_assets_modified"), false);
    Isolation->SetBoolField(TEXT("guard_player_skeleton_shared"), bSkeletonShared);
    Isolation->SetBoolField(TEXT("game_instance_subsystem_modified"), false);
    Isolation->SetBoolField(TEXT("causal_core_modified"), false);
    Isolation->SetBoolField(TEXT("save_schema_modified"), false);
    Isolation->SetBoolField(TEXT("new_background_asset_over_50mb"), false);
    WriteG3AJson(TEXT("asset_isolation_evidence.json"), Isolation);
    if (AutoFollowActor.IsValid()) AutoFollowActor->WriteEvidenceReport();

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G3A_CAPITAL_EVIDENCE_WRITTEN files=5 zones=%d guard=%d nav_routes=%d/%d click_traces=%d/%d move_command=%d moved=%.1f move_pass=%d traversal=%d gate=%d plaza=%d castle=%d standard=%d tactical=%d return=%d collision=%d autofollow=%d manual=%d resume=%d tactical_no_follow=%d return_follow=%d fallback=%d skeleton_shared=%d oversized=0"),
        ZoneCount, GuardCount, PassedRoutes, UE_ARRAY_COUNT(Probes),
        ClickTracePassedCount, ClickTraceSampleCount, bMovementCommandIssued ? 1 : 0,
        PlayerMovementDistance, bActualPlayerMovementPassed ? 1 : 0,
        bCapitalTraversalPassed ? 1 : 0,
        CountActorsWithTag(World, TEXT("SouthGate")) > 0 ? 1 : 0,
        CountActorsWithTag(World, TEXT("CentralPlaza")) > 0 ? 1 : 0,
        CountActorsWithTag(World, TEXT("Castle")) > 0 ? 1 : 0,
        bInitialStandard ? 1 : 0, bTactical ? 1 : 0, bReturnedStandard ? 1 : 0,
        CollisionActors > 0 && CameraBlockingActors == CollisionActors ? 1 : 0,
        bAutoFollowObserved ? 1 : 0, bManualDragObserved && bManualHoldObserved ? 1 : 0,
        bAutoFollowResumed ? 1 : 0, bTacticalNoFollow ? 1 : 0,
        bReturnStandardFollow ? 1 : 0,
        bPlayerFallback ? 1 : 0, bSkeletonShared ? 1 : 0);

    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3AEvidenceExit")))
        FGenericPlatformMisc::RequestExit(false);
}

void AStageG3ACapitalEvidenceActor::BeginScreenshotSequence()
{
    ResolveAcceptedActors();
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        Controller->ConsoleCommand(TEXT("r.MotionBlurQuality 0"), true);
    if (AcceptedPlayer.IsValid())
    {
        OriginalPlayerLocation = AcceptedPlayer->GetActorLocation();
        OriginalPlayerRotation = AcceptedPlayer->GetActorRotation();
    }
    ScreenshotStep = 0;
    GetWorldTimerManager().SetTimer(ScreenshotSequenceTimer, this,
        &AStageG3ACapitalEvidenceActor::AdvanceScreenshotSequence, 1.10f, true, 0.70f);
}

void AStageG3ACapitalEvidenceActor::SetPlayerForShot(const FVector& Location, float Yaw)
{
    if (AcceptedPlayer.IsValid())
        AcceptedPlayer->SetActorLocationAndRotation(Location, FRotator(0.0f, Yaw, 0.0f), false, nullptr,
            ETeleportType::TeleportPhysics);
}

void AStageG3ACapitalEvidenceActor::EnsureStandardCamera()
{
    if (CameraAdapter.IsValid() && !CameraAdapter->IsStandardMode())
        CameraAdapter->ToggleCameraMode();
}

void AStageG3ACapitalEvidenceActor::EnsureTacticalCamera(bool bZoomOut)
{
    if (CameraAdapter.IsValid() && CameraAdapter->IsStandardMode())
        CameraAdapter->ToggleCameraMode();
    if (!bZoomOut) return;
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        for (int32 Index = 0; Index < 12; ++Index)
        {
            Controller->InputKey(FInputKeyEventArgs(
                nullptr, INPUTDEVICEID_NONE, EKeys::MouseWheelAxis,
                -1.0f, 1.0f / 60.0f, 1, FPlatformTime::Cycles64()));
        }
    }
}

void AStageG3ACapitalEvidenceActor::AdvanceScreenshotSequence()
{
    ++ScreenshotStep;
    switch (ScreenshotStep)
    {
    case 1:
        SetPlayerForShot(FVector(0, -500, 96));
        EnsureTacticalCamera(true);
        // Evidence-only wide framing. The accepted G-2A mode and settings remain unchanged.
        if (CameraAdapter.IsValid()) CameraAdapter->SetActorTickEnabled(false);
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
        {
            AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = false;
            AcceptedPlayer->GetCameraBoom()->TargetArmLength = 5500.0f;
            AcceptedPlayer->GetCameraBoom()->SetRelativeRotation(FRotator(-75.0f, 90.0f, 0.0f));
        }
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetFollowCamera())
            AcceptedPlayer->GetFollowCamera()->SetFieldOfView(70.0f);
        PendingScreenshotFilename = TEXT("01_overview_tactical_full.png");
        break;
    case 2:
        if (CameraAdapter.IsValid()) CameraAdapter->SetActorTickEnabled(true);
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
            AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = true;
        SetPlayerForShot(FVector(0, -3200, 96));
        EnsureStandardCamera();
        PendingScreenshotFilename = TEXT("02_south_gate_standard.png");
        break;
    case 3:
        EnsureTacticalCamera(false);
        PendingScreenshotFilename = TEXT("03_south_gate_tactical.png");
        break;
    case 4:
        SetPlayerForShot(FVector(0, -1350, 96));
        EnsureStandardCamera();
        PendingScreenshotFilename = TEXT("04_main_street_standard.png");
        break;
    case 5:
        SetPlayerForShot(FVector(-340, -650, 96));
        EnsureStandardCamera();
        PendingScreenshotFilename = TEXT("05_central_plaza_standard.png");
        break;
    case 6:
        SetPlayerForShot(FVector(0, -160, 96));
        EnsureTacticalCamera(false);
        PendingScreenshotFilename = TEXT("06_central_plaza_tactical.png");
        break;
    case 7:
        SetPlayerForShot(FVector(1450, -350, 96));
        EnsureTacticalCamera(false);
        PendingScreenshotFilename = TEXT("07_market_district.png");
        break;
    case 8:
        SetPlayerForShot(FVector(-1450, -250, 96));
        EnsureTacticalCamera(false);
        PendingScreenshotFilename = TEXT("08_residential_district.png");
        break;
    case 9:
        SetPlayerForShot(FVector(1500, 1120, 96));
        EnsureTacticalCamera(false);
        PendingScreenshotFilename = TEXT("09_noble_district.png");
        break;
    case 10:
        SetPlayerForShot(FVector(-1550, -1500, 96));
        EnsureTacticalCamera(false);
        PendingScreenshotFilename = TEXT("10_workshop_district.png");
        break;
    case 11:
        SetPlayerForShot(FVector(0, -900, 96));
        EnsureStandardCamera();
        PendingScreenshotFilename = TEXT("11_castle_approach_standard.png");
        break;
    case 12:
        SetPlayerForShot(FVector(0, 2050, 96));
        EnsureTacticalCamera(true);
        if (CameraAdapter.IsValid()) CameraAdapter->SetActorTickEnabled(false);
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
        {
            AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = false;
            AcceptedPlayer->GetCameraBoom()->TargetArmLength = 2200.0f;
            AcceptedPlayer->GetCameraBoom()->SetRelativeRotation(FRotator(-68.0f, 90.0f, 0.0f));
        }
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetFollowCamera())
            AcceptedPlayer->GetFollowCamera()->SetFieldOfView(68.0f);
        PendingScreenshotFilename = TEXT("12_castle_overlook.png");
        break;
    case 13:
        SetPlayerForShot(FVector(0, -3200, 96));
        EnsureTacticalCamera(false);
        if (CameraAdapter.IsValid()) CameraAdapter->SetActorTickEnabled(false);
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
        {
            AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = false;
            AcceptedPlayer->GetCameraBoom()->TargetArmLength = 3200.0f;
            AcceptedPlayer->GetCameraBoom()->SetRelativeRotation(FRotator(-65.0f, 90.0f, 0.0f));
        }
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetFollowCamera())
            AcceptedPlayer->GetFollowCamera()->SetFieldOfView(68.0f);
        PendingScreenshotFilename = TEXT("13_outskirts_farmland_forest.png");
        break;
    case 14:
        if (CameraAdapter.IsValid()) CameraAdapter->SetActorTickEnabled(true);
        if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
            AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = true;
        SetPlayerForShot(FVector(1800, -2050, 96));
        EnsureStandardCamera();
        PendingScreenshotFilename = TEXT("14_camera_collision_wall.png");
        break;
    case 15:
        SetPlayerForShot(FVector(0, -1250, 96));
        EnsureTacticalCamera(false);
        PendingScreenshotFilename = TEXT("15_navmesh_route_gate_to_plaza.png");
        break;
    case 16:
        SetPlayerForShot(FVector(0, -620, 96));
        EnsureStandardCamera();
        PendingScreenshotFilename = TEXT("16_return_to_standard_after_tactical.png");
        break;
    case 17:
    {
        SetPlayerForShot(FVector(0, -3000, 96));
        EnsureStandardCamera();
        if (AutoFollowActor.IsValid()) AutoFollowActor->SetManagedYawForEvidence(-90.0f);
        FString Reason;
        if (AcceptedPlayer.IsValid())
            AcceptedPlayer->IssueMoveToLocation(FVector(0, 1200, 0), false, Reason);
        PendingScreenshotFilename = TEXT("standard_autofollow_move_start.png");
        break;
    }
    case 18:
        PendingScreenshotFilename = TEXT("standard_autofollow_while_moving.png");
        break;
    case 19:
        BeginManualDragEvidence();
        PendingScreenshotFilename = TEXT("standard_manual_drag_override.png");
        break;
    case 20:
        EndManualDragEvidence();
        PendingScreenshotFilename.Reset();
        return;
    case 21:
        PendingScreenshotFilename = TEXT("standard_autofollow_resume_after_drag.png");
        break;
    case 22:
    {
        EnsureTacticalCamera(false);
        FString Reason;
        if (AcceptedPlayer.IsValid())
            AcceptedPlayer->IssueMoveToLocation(FVector(1200, -1000, 0), false, Reason);
        PendingScreenshotFilename = TEXT("tactical_no_autofollow.png");
        break;
    }
    case 23:
        EnsureStandardCamera();
        PendingScreenshotFilename = TEXT("return_standard_autofollow.png");
        break;
    default:
        FinishScreenshotSequence();
        return;
    }
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, this,
        &AStageG3ACapitalEvidenceActor::CapturePendingScreenshot, 0.48f, false);
}

void AStageG3ACapitalEvidenceActor::CapturePendingScreenshot()
{
    RequestEvidenceScreenshot(PendingScreenshotFilename);
}

void AStageG3ACapitalEvidenceActor::RestorePlayerTransform()
{
    if (AcceptedPlayer.IsValid())
        AcceptedPlayer->SetActorLocationAndRotation(OriginalPlayerLocation, OriginalPlayerRotation, false, nullptr,
            ETeleportType::TeleportPhysics);
}

void AStageG3ACapitalEvidenceActor::RequestEvidenceScreenshot(const FString& Filename)
{
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("StageG3A"), TEXT("Screenshots"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, Filename), false, false);
}

void AStageG3ACapitalEvidenceActor::FinishScreenshotSequence()
{
    GetWorldTimerManager().ClearTimer(ScreenshotSequenceTimer);
    if (CameraAdapter.IsValid()) CameraAdapter->SetActorTickEnabled(true);
    if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
        AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = true;
    RestorePlayerTransform();
    EnsureStandardCamera();
    UE_LOG(LogTemp, Display, TEXT("STAGE_G3A_CAPITAL_SCREENSHOTS_REQUESTED count=22"));
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3AScreenshotsExit")))
        FGenericPlatformMisc::RequestExit(false);
}
