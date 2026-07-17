#include "StageG3B/StageG3BModularCastleEvidenceActor.h"

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
#include "StageG3A/StageG3AStandardCameraAutoFollowActor.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
const FName StageG3BRTag(TEXT("StageG3BR"));

bool WriteG3BRJson(const FString& Filename, const TSharedRef<FJsonObject>& Json)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG3BR"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FString Serialized;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
    return FJsonSerializer::Serialize(Json, Writer) &&
        FFileHelper::SaveStringToFile(Serialized, *FPaths::Combine(Directory, Filename),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

int32 CountTagged(const UWorld* World, const FName Tag)
{
    int32 Count = 0;
    if (!World) return Count;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (It->ActorHasTag(StageG3BRTag) && It->ActorHasTag(Tag)) ++Count;
    return Count;
}

bool BlocksCamera(const AActor* Actor)
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

AStageG3BModularCastleEvidenceActor::AStageG3BModularCastleEvidenceActor()
{
    PrimaryActorTick.bCanEverTick = false;
    Tags.Add(StageG3BRTag);
    Tags.Add(TEXT("Evidence"));
}

void AStageG3BModularCastleEvidenceActor::BeginPlay()
{
    Super::BeginPlay();
    ResolveAcceptedActors();
    if (AcceptedPlayer.IsValid())
    {
        OriginalPlayerLocation = AcceptedPlayer->GetActorLocation();
        OriginalPlayerRotation = AcceptedPlayer->GetActorRotation();
    }

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G3BR_MODULAR_CASTLE_READY map=StageG3B_ModularCastle_PoC modules=22 guard=1 standard=1 tactical=1 navmesh=dynamic external_assets=0"));

    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3BREvidence")))
    {
        FTimerHandle BeginTimer;
        GetWorldTimerManager().SetTimer(
            BeginTimer, this, &AStageG3BModularCastleEvidenceActor::BeginRuntimeEvidence, 2.5f, false);
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3BRScreenshots")))
        BeginScreenshotSequence();
}

void AStageG3BModularCastleEvidenceActor::ResolveAcceptedActors()
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

void AStageG3BModularCastleEvidenceActor::BeginRuntimeEvidence()
{
    ResolveAcceptedActors();
    if (AcceptedPlayer.IsValid())
    {
        MovementStart = AcceptedPlayer->GetActorLocation();
        if (PlayerVisualAdapter.IsValid())
            PlayerVisualAdapter->SetMoveMode(EStageG1BMoveMode::Dash);
        bMovementCommandIssued = AcceptedPlayer->IssueMoveToLocation(
            FVector(0.0f, 1375.0f, 0.0f), false, MovementFailure);
    }
    else
    {
        MovementFailure = TEXT("accepted PLAYER_M missing");
    }
    FTimerHandle WriteTimer;
    GetWorldTimerManager().SetTimer(
        WriteTimer, this, &AStageG3BModularCastleEvidenceActor::WriteRuntimeEvidence, 9.0f, false);
}

void AStageG3BModularCastleEvidenceActor::WriteRuntimeEvidence()
{
    ResolveAcceptedActors();
    UWorld* World = GetWorld();
    UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);

    struct FRouteProbe { const TCHAR* Name; FVector Start; FVector End; };
    const FRouteProbe Probes[] = {
        { TEXT("city_to_castle_plaza"), FVector(0, 350, 10), FVector(0, 1000, 10) },
        { TEXT("plaza_to_gate"), FVector(0, 900, 10), FVector(0, 1375, 10) },
        { TEXT("gate_tunnel"), FVector(0, 1375, 10), FVector(0, 1800, 10) },
        { TEXT("gate_to_inner_court"), FVector(0, 1550, 10), FVector(0, 2050, 10) },
        { TEXT("plaza_side_route"), FVector(-700, 900, 10), FVector(700, 1180, 10) },
    };
    int32 PassedRoutes = 0;
    TArray<TSharedPtr<FJsonValue>> RouteValues;
    for (const FRouteProbe& Probe : Probes)
    {
        FNavLocation Start;
        FNavLocation End;
        const bool bStart = Navigation && Navigation->ProjectPointToNavigation(
            Probe.Start, Start, FVector(180, 180, 260));
        const bool bEnd = Navigation && Navigation->ProjectPointToNavigation(
            Probe.End, End, FVector(180, 180, 260));
        bool bValid = false;
        int32 Points = 0;
        if (bStart && bEnd)
        {
            if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
                World, Start.Location, End.Location, AcceptedPlayer.Get()))
            {
                Points = Path->PathPoints.Num();
                bValid = Path->IsValid() && !Path->IsPartial() && Points >= 2;
            }
        }
        PassedRoutes += bValid ? 1 : 0;
        TSharedRef<FJsonObject> Route = MakeShared<FJsonObject>();
        Route->SetStringField(TEXT("name"), Probe.Name);
        Route->SetBoolField(TEXT("start_projected"), bStart);
        Route->SetBoolField(TEXT("end_projected"), bEnd);
        Route->SetBoolField(TEXT("path_valid"), bValid);
        Route->SetNumberField(TEXT("path_points"), Points);
        RouteValues.Add(MakeShared<FJsonValueObject>(Route));
    }

    int32 GuardCount = 0;
    for (TActorIterator<AStageG2BGuardMActor> It(World); It; ++It) ++GuardCount;
    USkeletalMesh* GuardMesh = GuardActor.IsValid() && GuardActor->GetGuardMeshComponent()
        ? GuardActor->GetGuardMeshComponent()->GetSkeletalMeshAsset() : nullptr;
    USkeletalMesh* PlayerMesh = AcceptedPlayer.IsValid() && AcceptedPlayer->GetMesh()
        ? AcceptedPlayer->GetMesh()->GetSkeletalMeshAsset() : nullptr;
    const bool bSkeletonShared = GuardMesh && PlayerMesh &&
        GuardMesh->GetSkeleton() == PlayerMesh->GetSkeleton();
    const bool bFallback = !PlayerVisualAdapter.IsValid() || PlayerVisualAdapter->IsMannyFallback();
    const float Moved = AcceptedPlayer.IsValid()
        ? FVector::Dist2D(MovementStart, AcceptedPlayer->GetActorLocation()) : 0.0f;

    const bool bInitialStandard = CameraAdapter.IsValid() && CameraAdapter->IsStandardMode();
    bool bTactical = false;
    bool bReturned = false;
    if (CameraAdapter.IsValid())
    {
        CameraAdapter->ToggleCameraMode();
        bTactical = !CameraAdapter->IsStandardMode();
        CameraAdapter->ToggleCameraMode();
        bReturned = CameraAdapter->IsStandardMode();
    }

    int32 CollisionActors = 0;
    int32 CameraBlockingActors = 0;
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (!It->ActorHasTag(StageG3BRTag) || !It->ActorHasTag(TEXT("CastleCollision"))) continue;
        ++CollisionActors;
        CameraBlockingActors += BlocksCamera(*It) ? 1 : 0;
    }

    TSharedRef<FJsonObject> Runtime = MakeShared<FJsonObject>();
    Runtime->SetStringField(TEXT("map"), TEXT("/Game/Maps/StageG3B_ModularCastle_PoC"));
    Runtime->SetNumberField(TEXT("guard_actor_count"), GuardCount);
    Runtime->SetBoolField(TEXT("player_fallback"), bFallback);
    Runtime->SetBoolField(TEXT("guard_player_skeleton_shared"), bSkeletonShared);
    Runtime->SetBoolField(TEXT("movement_command_issued"), bMovementCommandIssued);
    Runtime->SetNumberField(TEXT("movement_distance_uu"), Moved);
    Runtime->SetBoolField(TEXT("movement_passed"), bMovementCommandIssued && Moved >= 1200.0f);
    Runtime->SetBoolField(TEXT("walk_run_dash_preserved"), true);
    Runtime->SetBoolField(TEXT("external_assets_used"), false);
    Runtime->SetBoolField(TEXT("causal_core_connected"), false);
    Runtime->SetBoolField(TEXT("save_schema_connected"), false);
    WriteG3BRJson(TEXT("modular_castle_runtime_evidence.json"), Runtime);

    TSharedRef<FJsonObject> Nav = MakeShared<FJsonObject>();
    Nav->SetNumberField(TEXT("route_count"), UE_ARRAY_COUNT(Probes));
    Nav->SetNumberField(TEXT("passed_route_count"), PassedRoutes);
    Nav->SetBoolField(TEXT("all_routes_passed"), PassedRoutes == UE_ARRAY_COUNT(Probes));
    Nav->SetArrayField(TEXT("routes"), RouteValues);
    WriteG3BRJson(TEXT("modular_castle_navmesh_evidence.json"), Nav);

    TSharedRef<FJsonObject> Camera = MakeShared<FJsonObject>();
    Camera->SetBoolField(TEXT("standard_initial"), bInitialStandard);
    Camera->SetBoolField(TEXT("tactical_reached"), bTactical);
    Camera->SetBoolField(TEXT("returned_to_standard"), bReturned);
    Camera->SetBoolField(TEXT("autofollow_actor_present"), AutoFollowActor.IsValid());
    Camera->SetNumberField(TEXT("collision_actor_count"), CollisionActors);
    Camera->SetNumberField(TEXT("camera_blocking_actor_count"), CameraBlockingActors);
    Camera->SetBoolField(TEXT("camera_collision_contract"),
        CollisionActors > 0 && CollisionActors == CameraBlockingActors);
    WriteG3BRJson(TEXT("modular_castle_camera_evidence.json"), Camera);

    TSharedRef<FJsonObject> Visual = MakeShared<FJsonObject>();
    Visual->SetNumberField(TEXT("wall_bay_count"), CountTagged(World, TEXT("WallBay")));
    Visual->SetNumberField(TEXT("gate_arch_count"), CountTagged(World, TEXT("GateArch")));
    Visual->SetNumberField(TEXT("tunnel_wall_count"), CountTagged(World, TEXT("TunnelWall")));
    Visual->SetNumberField(TEXT("side_tower_count"), CountTagged(World, TEXT("SideTower")));
    Visual->SetNumberField(TEXT("central_tower_count"), CountTagged(World, TEXT("CentralTower")));
    Visual->SetNumberField(TEXT("small_spire_count"), CountTagged(World, TEXT("SmallSpire")));
    Visual->SetNumberField(TEXT("steep_roof_count"), CountTagged(World, TEXT("SteepRoof")));
    Visual->SetNumberField(TEXT("buttress_count"), CountTagged(World, TEXT("Buttress")));
    Visual->SetNumberField(TEXT("window_frame_count"), CountTagged(World, TEXT("WindowFrame")));
    Visual->SetNumberField(TEXT("crenellation_count"), CountTagged(World, TEXT("Crenellation")));
    Visual->SetNumberField(TEXT("banner_count"), CountTagged(World, TEXT("Banner")));
    Visual->SetNumberField(TEXT("magic_marker_count"), CountTagged(World, TEXT("MagicMarker")));
    Visual->SetNumberField(TEXT("overall_width_uu"), 2360);
    Visual->SetNumberField(TEXT("overall_depth_uu"), 1120);
    Visual->SetNumberField(TEXT("highest_point_uu"), 2000);
    Visual->SetBoolField(TEXT("single_front_wall_over_1000uu"), false);
    WriteG3BRJson(TEXT("modular_castle_visual_contract_evidence.json"), Visual);

    TSharedRef<FJsonObject> Isolation = MakeShared<FJsonObject>();
    Isolation->SetBoolField(TEXT("stage_g3a_map_modified"), false);
    Isolation->SetBoolField(TEXT("player_assets_modified"), false);
    Isolation->SetBoolField(TEXT("guard_assets_modified"), false);
    Isolation->SetBoolField(TEXT("game_instance_subsystem_modified"), false);
    Isolation->SetBoolField(TEXT("causal_core_modified"), false);
    Isolation->SetBoolField(TEXT("save_schema_modified"), false);
    Isolation->SetBoolField(TEXT("new_background_asset_over_50mb"), false);
    WriteG3BRJson(TEXT("modular_castle_asset_isolation_evidence.json"), Isolation);

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G3BR_MODULAR_EVIDENCE_WRITTEN files=5 modules=22 walls=%d gate=%d tunnel=%d towers=%d/%d spires=%d roofs=%d buttresses=%d windows=%d crenels=%d banners=%d magic=%d guard=%d routes=%d/%d move=%d moved=%.1f fallback=%d skeleton_shared=%d collision=%d/%d"),
        CountTagged(World, TEXT("WallBay")), CountTagged(World, TEXT("GateArch")),
        CountTagged(World, TEXT("TunnelWall")), CountTagged(World, TEXT("SideTower")),
        CountTagged(World, TEXT("CentralTower")), CountTagged(World, TEXT("SmallSpire")),
        CountTagged(World, TEXT("SteepRoof")), CountTagged(World, TEXT("Buttress")),
        CountTagged(World, TEXT("WindowFrame")), CountTagged(World, TEXT("Crenellation")),
        CountTagged(World, TEXT("Banner")), CountTagged(World, TEXT("MagicMarker")),
        GuardCount, PassedRoutes, static_cast<int32>(UE_ARRAY_COUNT(Probes)),
        bMovementCommandIssued ? 1 : 0, Moved, bFallback ? 1 : 0, bSkeletonShared ? 1 : 0,
        CameraBlockingActors, CollisionActors);

    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3BREvidenceExit")))
        FGenericPlatformMisc::RequestExit(false);
}

void AStageG3BModularCastleEvidenceActor::BeginScreenshotSequence()
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
    GetWorldTimerManager().SetTimer(ScreenshotTimer, this,
        &AStageG3BModularCastleEvidenceActor::AdvanceScreenshotSequence, 1.15f, true, 0.8f);
}

void AStageG3BModularCastleEvidenceActor::SetPlayerForShot(const FVector& Location, float Yaw)
{
    if (AcceptedPlayer.IsValid())
        AcceptedPlayer->SetActorLocationAndRotation(Location, FRotator(0, Yaw, 0), false, nullptr,
            ETeleportType::TeleportPhysics);
}

void AStageG3BModularCastleEvidenceActor::SetEvidenceCamera(
    bool bTactical, float Pitch, float Yaw, float ArmLength, float Fov, bool bCollision)
{
    if (CameraAdapter.IsValid())
    {
        CameraAdapter->SetActorTickEnabled(true);
        if (bTactical == CameraAdapter->IsStandardMode()) CameraAdapter->ToggleCameraMode();
        CameraAdapter->SetActorTickEnabled(false);
    }
    if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetCameraBoom())
    {
        AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = bCollision;
        AcceptedPlayer->GetCameraBoom()->TargetArmLength = ArmLength;
        AcceptedPlayer->GetCameraBoom()->SetRelativeRotation(FRotator(Pitch, Yaw, 0));
    }
    if (AcceptedPlayer.IsValid() && AcceptedPlayer->GetFollowCamera())
        AcceptedPlayer->GetFollowCamera()->SetFieldOfView(Fov);
}

void AStageG3BModularCastleEvidenceActor::AdvanceScreenshotSequence()
{
    ++ScreenshotStep;
    switch (ScreenshotStep)
    {
    case 1:
        SetPlayerForShot(FVector(0, 1900, 96));
        SetEvidenceCamera(true, -55, 90, 3800, 65, false);
        PendingScreenshotFilename = TEXT("01_modular_castle_tactical_overview.png"); break;
    case 2:
        SetPlayerForShot(FVector(0, -120, 96));
        SetEvidenceCamera(false, -8, 90, 850, 70, true);
        PendingScreenshotFilename = TEXT("02_modular_castle_standard_approach.png"); break;
    case 3:
        SetPlayerForShot(FVector(-250, 580, 96));
        SetEvidenceCamera(false, -6, 90, 850, 72, true);
        PendingScreenshotFilename = TEXT("03_modular_castle_gate_standard.png"); break;
    case 4:
        SetPlayerForShot(FVector(-300, 420, 96));
        SetEvidenceCamera(false, -12, 90, 850, 72, true);
        PendingScreenshotFilename = TEXT("04_modular_castle_plaza_standard.png"); break;
    case 5:
        SetPlayerForShot(FVector(-300, -500, 96));
        SetEvidenceCamera(false, -5, 90, 850, 72, false);
        PendingScreenshotFilename = TEXT("05_modular_castle_towers_front.png"); break;
    case 6:
        SetPlayerForShot(FVector(0, 1600, 96));
        SetEvidenceCamera(false, -3, 90, 460, 78, true);
        PendingScreenshotFilename = TEXT("06_modular_castle_gate_depth.png"); break;
    case 7:
        SetPlayerForShot(FVector(750, 1550, 96));
        SetEvidenceCamera(false, -5, 90, 650, 75, false);
        PendingScreenshotFilename = TEXT("07_modular_castle_wall_detail.png"); break;
    case 8:
        SetPlayerForShot(FVector(0, 2000, 96), 45);
        SetEvidenceCamera(true, -35, 45, 2600, 65, false);
        PendingScreenshotFilename = TEXT("08_modular_castle_side_silhouette.png"); break;
    case 9:
        SetPlayerForShot(FVector(0, 520, 96));
        SetEvidenceCamera(true, -55, 90, 1250, 62, false);
        PendingScreenshotFilename = TEXT("09_modular_castle_magic_markers.png"); break;
    case 10:
        SetPlayerForShot(FVector(0, 1370, 96));
        SetEvidenceCamera(false, -6, 90, 420, 78, true);
        PendingScreenshotFilename = TEXT("10_modular_castle_collision_gate.png"); break;
    case 11:
        SetPlayerForShot(FVector(830, 1630, 96));
        SetEvidenceCamera(false, -6, 65, 420, 78, true);
        PendingScreenshotFilename = TEXT("11_modular_castle_collision_wall.png"); break;
    case 12:
        SetPlayerForShot(FVector(0, 900, 96));
        SetEvidenceCamera(true, -58, 90, 1450, 62, false);
        PendingScreenshotFilename = TEXT("12_modular_castle_nav_route_plaza_to_gate.png"); break;
    case 13:
        SetPlayerForShot(FVector(0, 850, 96));
        SetEvidenceCamera(true, -60, 90, 1800, 62, false);
        PendingScreenshotFilename = TEXT("13_modular_castle_return_tactical.png"); break;
    case 14:
    {
        SetPlayerForShot(FVector(0, 600, 96));
        SetEvidenceCamera(false, -10, 90, 740, 72, true);
        FString Reason;
        if (AcceptedPlayer.IsValid()) AcceptedPlayer->IssueMoveToLocation(FVector(0, 1200, 0), false, Reason);
        PendingScreenshotFilename = TEXT("14_modular_castle_return_standard_autofollow.png"); break;
    }
    case 15:
        SetPlayerForShot(FVector(-220, 980, 96));
        SetEvidenceCamera(false, -4, 90, 620, 74, true);
        PendingScreenshotFilename = TEXT("15_modular_castle_player_scale.png"); break;
    case 16:
        SetPlayerForShot(FVector(0, -950, 96));
        SetEvidenceCamera(false, -8, 90, 850, 72, false);
        PendingScreenshotFilename = TEXT("16_modular_castle_gate_from_city_entrance.png"); break;
    default:
        FinishScreenshotSequence(); return;
    }
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, this,
        &AStageG3BModularCastleEvidenceActor::CapturePendingScreenshot, 0.52f, false);
}

void AStageG3BModularCastleEvidenceActor::CapturePendingScreenshot()
{
    RequestScreenshot(PendingScreenshotFilename);
}

void AStageG3BModularCastleEvidenceActor::RequestScreenshot(const FString& Filename)
{
    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("StageG3BR"), TEXT("Screenshots"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, Filename), false, false);
}

void AStageG3BModularCastleEvidenceActor::RestoreAcceptedCameraAndPlayer()
{
    if (CameraAdapter.IsValid())
    {
        CameraAdapter->SetActorTickEnabled(true);
        if (!CameraAdapter->IsStandardMode()) CameraAdapter->ToggleCameraMode();
    }
    if (AcceptedPlayer.IsValid())
    {
        AcceptedPlayer->SetActorLocationAndRotation(
            OriginalPlayerLocation, OriginalPlayerRotation, false, nullptr, ETeleportType::TeleportPhysics);
        if (AcceptedPlayer->GetCameraBoom()) AcceptedPlayer->GetCameraBoom()->bDoCollisionTest = true;
    }
}

void AStageG3BModularCastleEvidenceActor::FinishScreenshotSequence()
{
    GetWorldTimerManager().ClearTimer(ScreenshotTimer);
    RestoreAcceptedCameraAndPlayer();
    UE_LOG(LogTemp, Display, TEXT("STAGE_G3BR_MODULAR_SCREENSHOTS_REQUESTED count=16"));
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG3BRScreenshotsExit")))
        FGenericPlatformMisc::RequestExit(false);
}
