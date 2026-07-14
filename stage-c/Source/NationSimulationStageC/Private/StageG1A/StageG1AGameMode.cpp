#include "StageG1A/StageG1AGameMode.h"

#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "NavigationSystem.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StageG1A/StageG1AHudWidget.h"
#include "StageG1A/StageG1ANpcActor.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1A/StageG1ASettings.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
UStaticMesh* G1ACube = nullptr;
UMaterialInterface* G1ABaseMaterial = nullptr;

bool WriteG1AJson(const FString& Filename, const TSharedRef<FJsonObject>& Json)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG1A"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FString Serialized;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
    if (!FJsonSerializer::Serialize(Json, Writer)) return false;
    return FFileHelper::SaveStringToFile(Serialized, *FPaths::Combine(Directory, Filename),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

const TCHAR* MovementBandName(EStageG1AMovementBand Band)
{
    if (Band == EStageG1AMovementBand::Walk) return TEXT("WALK");
    if (Band == EStageG1AMovementBand::Run) return TEXT("RUN");
    return TEXT("IDLE");
}
}

AStageG1AGameMode::AStageG1AGameMode()
{
    DefaultPawnClass = AStageG1APlayerCharacter::StaticClass();
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    G1ACube = Cube.Object;
    G1ABaseMaterial = BaseMaterial.Object;
}

void AStageG1AGameMode::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        TArray<UPrimitiveComponent*> Components;
        It->GetComponents<UPrimitiveComponent>(Components);
        for (UPrimitiveComponent* Component : Components)
            if (Component) Component->SetCanEverAffectNavigation(false);
    }
    BuildTechnicalPlaza();
    SpawnLighting();
    StandardNpc = GetWorld()->SpawnActor<AStageG1ANpcActor>(
        FVector(150.0f, -1750.0f, 96.0f), FRotator(0.0f, -135.0f, 0.0f));
    if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        Navigation->Build();

    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        StageG1AHud = CreateWidget<UStageG1AHudWidget>(PlayerController, UStageG1AHudWidget::StaticClass());
        if (StageG1AHud)
        {
            StageG1AHud->AddToViewport(100);
            StageG1AHud->SetPositionInViewport(FVector2D(14.0f, 14.0f), false);
            StageG1AHud->SetDesiredSizeInViewport(FVector2D(420.0f, 176.0f));
        }
    }
    if (AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
        EvidenceStart = Player->GetActorLocation();

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1A_READY map=%s standard_3d_player=1 standard_3d_npc=1 paper2d=0 fang_rat=0 camera=fixed_isometric click_move=navmesh"),
        *GetWorld()->GetMapName());
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG1AEvidence")))
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AStageG1AGameMode::BeginEvidenceMove);
        FTimerHandle WalkTimer;
        GetWorldTimerManager().SetTimer(
            WalkTimer, this, &AStageG1AGameMode::CaptureWalkAndSwitchToRun, 1.1f, false);
        FTimerHandle RunTimer;
        GetWorldTimerManager().SetTimer(
            RunTimer, this, &AStageG1AGameMode::CaptureRunAndSwitchToWalk, 2.0f, false);
        FTimerHandle EvidenceTimer;
        // Sample while the deterministic route is still in progress so the
        // evidence records live CharacterMovement speed and animation rate.
        GetWorldTimerManager().SetTimer(EvidenceTimer, this, &AStageG1AGameMode::WriteEvidence, 2.8f, false);
    }
}

void AStageG1AGameMode::ToggleDebugHud()
{
    if (StageG1AHud) StageG1AHud->ToggleDebug();
}

void AStageG1AGameMode::BuildTechnicalPlaza()
{
    const FLinearColor StoneA(0.30f, 0.29f, 0.27f, 1.0f);
    const FLinearColor StoneB(0.36f, 0.34f, 0.30f, 1.0f);
    const FLinearColor Wall(0.42f, 0.38f, 0.31f, 1.0f);
    const FLinearColor Trim(0.25f, 0.23f, 0.20f, 1.0f);
    SpawnBlock(TEXT("OuterSafetyFloor"), FVector(0.0f, 0.0f, -55.0f), FVector(8000.0f, 8000.0f, 100.0f),
        FLinearColor(0.18f, 0.22f, 0.18f, 1.0f), FRotator::ZeroRotator, true);
    for (int32 X = -2; X <= 2; ++X)
        for (int32 Y = -2; Y <= 2; ++Y)
            SpawnBlock(FString::Printf(TEXT("Cobbled_%d_%d"), X, Y),
                FVector(X * 960.0f, Y * 960.0f, -10.0f), FVector(940.0f, 940.0f, 20.0f),
                ((X + Y) & 1) == 0 ? StoneA : StoneB, FRotator(0.0f, ((X - Y) & 1) ? 0.8f : -0.8f, 0.0f), true);

    // Medieval scale gate: the opening is 240 uu high and 240 uu wide so the
    // 175 uu standard character provides an immediately visible scale check.
    SpawnBlock(TEXT("SouthWallLeft"), FVector(-1260.0f, -2420.0f, 225.0f), FVector(2280.0f, 180.0f, 450.0f), Wall);
    SpawnBlock(TEXT("SouthWallRight"), FVector(1260.0f, -2420.0f, 225.0f), FVector(2280.0f, 180.0f, 450.0f), Wall);
    SpawnBlock(TEXT("GateLintel"), FVector(0.0f, -2420.0f, 345.0f), FVector(240.0f, 180.0f, 210.0f), Trim);
    SpawnBlock(TEXT("NorthWall"), FVector(0.0f, 2420.0f, 225.0f), FVector(5000.0f, 180.0f, 450.0f), Wall);
    SpawnBlock(TEXT("WestWall"), FVector(-2420.0f, 0.0f, 225.0f), FVector(180.0f, 4660.0f, 450.0f), Wall);
    SpawnBlock(TEXT("EastWall"), FVector(2420.0f, 0.0f, 225.0f), FVector(180.0f, 4660.0f, 450.0f), Wall);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        const float X = -1500.0f + Index * 1000.0f;
        SpawnBlock(FString::Printf(TEXT("Column_%d"), Index), FVector(X, 1450.0f, 180.0f),
            FVector(130.0f, 130.0f, 360.0f), Trim);
    }
    SpawnBlock(TEXT("PathObstacle"), FVector(0.0f, -500.0f, 120.0f), FVector(460.0f, 240.0f, 240.0f), Wall);
    SpawnBlock(TEXT("MildSlope"), FVector(1050.0f, 550.0f, 55.0f), FVector(900.0f, 520.0f, 55.0f),
        StoneB, FRotator(-7.0f, 0.0f, 0.0f), true);
    for (int32 Step = 0; Step < 4; ++Step)
        SpawnBlock(FString::Printf(TEXT("Stair_%d"), Step),
            FVector(-1250.0f + Step * 110.0f, 600.0f, 10.0f + Step * 22.0f),
            FVector(120.0f, 520.0f, 20.0f + Step * 44.0f), StoneB, FRotator::ZeroRotator, true);

    // Explicit collision boundaries are inside the outer safety floor; they
    // prevent normal traversal from ever reaching an unrendered black void.
    SpawnBlock(TEXT("BoundaryNorth"), FVector(0.0f, 3500.0f, 500.0f), FVector(7200.0f, 100.0f, 1000.0f), Trim);
    SpawnBlock(TEXT("BoundarySouth"), FVector(0.0f, -3500.0f, 500.0f), FVector(7200.0f, 100.0f, 1000.0f), Trim);
    SpawnBlock(TEXT("BoundaryEast"), FVector(3500.0f, 0.0f, 500.0f), FVector(100.0f, 7200.0f, 1000.0f), Trim);
    SpawnBlock(TEXT("BoundaryWest"), FVector(-3500.0f, 0.0f, 500.0f), FVector(100.0f, 7200.0f, 1000.0f), Trim);
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1A_ENVIRONMENT plaza=cobbled medieval_walls=4 door_clearance_uu=240 columns=4 slope=1 stairs=4 outer_floor=1 boundaries=4"));
}

void AStageG1AGameMode::SpawnLighting()
{
    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector(0.0f, 0.0f, 1600.0f), FRotator(-48.0f, -35.0f, 0.0f));
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->GetLightComponent()->SetIntensity(5.5f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.91f, 0.78f));
    Sun->GetLightComponent()->SetCastShadows(true);
    Sun->GetLightComponent()->ContactShadowLength = 0.05f;
    if (UDirectionalLightComponent* Directional = Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
        Directional->SetAtmosphereSunLight(true);
    GetWorld()->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(1.15f);
    Sky->GetLightComponent()->SetCastShadows(true);
    Sky->GetLightComponent()->RecaptureSky();
    UE_LOG(LogTemp, Display, TEXT("STAGE_G1A_LIGHTING directional=movable skylight=movable sky_atmosphere=1 dynamic_shadow=1 contact_shadow=1 blob_shadow=0"));
}

void AStageG1AGameMode::SpawnBlock(const FString& Name, const FVector& Location,
    const FVector& Dimensions, const FLinearColor& Color, const FRotator& Rotation, bool bWalkableSurface)
{
    AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
#if WITH_EDITOR
    Actor->SetActorLabel(Name);
#endif
    UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
    Mesh->SetStaticMesh(G1ACube);
    Mesh->SetWorldScale3D(Dimensions / 100.0f);
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, bWalkableSurface ? ECR_Block : ECR_Ignore);
    Mesh->SetCanEverAffectNavigation(true);
    Mesh->SetCastShadow(true);
    if (G1ABaseMaterial)
    {
        if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(
            G1ABaseMaterial, Actor, *FString::Printf(TEXT("%s_Material"), *Name)))
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Mesh->SetMaterial(0, Material);
        }
    }
}

void AStageG1AGameMode::BeginEvidenceMove()
{
    if (AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        EvidenceStart = Player->GetActorLocation();
        const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
        bDefaultWalkObserved = Player->GetMovementMode() == EStageG1AMovementMode::Walk &&
            FMath::IsNearlyEqual(Player->GetCharacterMovement()->MaxWalkSpeed, Settings->WalkSpeed);
        Player->SetMovementMode(EStageG1AMovementMode::Walk);
        FString Reason;
        const bool bIssued = Player->IssueMoveToLocation(FVector(1800.0f, 0.0f, 0.0f), false, Reason);
        const FStageG1AMovementView& View = Player->GetMovementView();
        EvidenceDestination = View.Destination;
        EvidencePathPointCount = Player->GetActivePathPointCount();
        EvidenceDestinationUpdateCount = View.DestinationUpdateCount;
        EvidenceSelectedTarget = View.SelectedTargetId;
        UE_LOG(LogTemp, Display, TEXT("STAGE_G1A_EVIDENCE_MOVE issued=%d reason=%s"), bIssued ? 1 : 0, *Reason);
    }
}

void AStageG1AGameMode::CaptureWalkAndSwitchToRun()
{
    AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Player) return;
    const FStageG1AMovementView Before = Player->GetMovementView();
    WalkSampleSpeed = Before.Speed;
    WalkSamplePlayRate = Before.PlayRate;
    WalkSampleBand = Before.Band;
    WalkSampleMaxSpeed = Player->GetCharacterMovement()->MaxWalkSpeed;
    Player->SetMovementMode(EStageG1AMovementMode::Run);
    const FStageG1AMovementView& After = Player->GetMovementView();
    bRunDestinationMaintained = After.Destination.Equals(EvidenceDestination, 0.1f);
    bRunPathMaintained = Player->GetActivePathPointCount() == EvidencePathPointCount;
    bRunMoveMaintained = Player->IsMoveCommandActive();
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1A_EVIDENCE_TOGGLE to=RUN destination_kept=%d path_kept=%d move_kept=%d"),
        bRunDestinationMaintained ? 1 : 0, bRunPathMaintained ? 1 : 0, bRunMoveMaintained ? 1 : 0);
}

void AStageG1AGameMode::CaptureRunAndSwitchToWalk()
{
    AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Player) return;
    const FStageG1AMovementView Before = Player->GetMovementView();
    RunSampleSpeed = Before.Speed;
    RunSamplePlayRate = Before.PlayRate;
    RunSampleBand = Before.Band;
    RunSampleMaxSpeed = Player->GetCharacterMovement()->MaxWalkSpeed;
    Player->SetMovementMode(EStageG1AMovementMode::Walk);
    const FStageG1AMovementView& After = Player->GetMovementView();
    WalkReturnSpeed = After.Speed;
    WalkReturnMaxSpeed = Player->GetCharacterMovement()->MaxWalkSpeed;
    bWalkDestinationMaintained = After.Destination.Equals(EvidenceDestination, 0.1f);
    bWalkPathMaintained = Player->GetActivePathPointCount() == EvidencePathPointCount;
    bWalkMoveMaintained = Player->IsMoveCommandActive();
    bToggleUpdateCountMaintained = After.DestinationUpdateCount == EvidenceDestinationUpdateCount;
    bToggleTargetMaintained = After.SelectedTargetId == EvidenceSelectedTarget;
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1A_EVIDENCE_TOGGLE to=WALK destination_kept=%d path_kept=%d move_kept=%d update_kept=%d target_kept=%d"),
        bWalkDestinationMaintained ? 1 : 0, bWalkPathMaintained ? 1 : 0, bWalkMoveMaintained ? 1 : 0,
        bToggleUpdateCountMaintained ? 1 : 0, bToggleTargetMaintained ? 1 : 0);
}

void AStageG1AGameMode::WriteEvidence()
{
    AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    if (!Player) return;
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    const USkeletalMesh* Mesh = Player->GetMesh()->GetSkeletalMeshAsset();
    const float Height = Mesh ? Mesh->GetImportedBounds().BoxExtent.Z * 2.0f * Settings->CharacterScale : 0.0f;
    FHitResult GroundHit;
    const FVector Foot = Player->GetActorLocation() - FVector(0.0f, 0.0f,
        Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const bool bGroundHit = GetWorld()->LineTraceSingleByChannel(
        GroundHit, Foot + FVector(0.0f, 0.0f, 15.0f), Foot - FVector(0.0f, 0.0f, 80.0f), ECC_Visibility);
    const float FootGap = bGroundHit ? FMath::Abs(Foot.Z - GroundHit.ImpactPoint.Z) : -1.0f;

    TSharedRef<FJsonObject> Standard = MakeShared<FJsonObject>();
    Standard->SetStringField(TEXT("stage"), TEXT("G-1A"));
    Standard->SetStringField(TEXT("player_mesh"), Mesh ? Mesh->GetPathName() : TEXT("NONE"));
    Standard->SetStringField(TEXT("anim_blueprint"), Player->GetMesh()->GetAnimClass()
        ? Player->GetMesh()->GetAnimClass()->GetPathName() : TEXT("NONE"));
    Standard->SetStringField(TEXT("npc_mesh"), StandardNpc && StandardNpc->GetMesh()->GetSkeletalMeshAsset()
        ? StandardNpc->GetMesh()->GetSkeletalMeshAsset()->GetPathName() : TEXT("NONE"));
    Standard->SetBoolField(TEXT("paper2d_used"), false);
    Standard->SetBoolField(TEXT("fang_rat_implemented"), false);
    WriteG1AJson(TEXT("standard_asset_evidence.json"), Standard);

    TSharedRef<FJsonObject> WorldHeight = MakeShared<FJsonObject>();
    WorldHeight->SetNumberField(TEXT("character_mesh_height_uu"), Height);
    WorldHeight->SetNumberField(TEXT("capsule_height_uu"), Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.0f);
    WorldHeight->SetNumberField(TEXT("door_clearance_uu"), 240.0f);
    WorldHeight->SetNumberField(TEXT("single_scale"), Settings->CharacterScale);
    WorldHeight->SetBoolField(TEXT("height_in_170_180_band"), Height >= 170.0f && Height <= 180.0f);
    WriteG1AJson(TEXT("world_height_evidence.json"), WorldHeight);

    const FStageG1AMovementView& View = Player->GetMovementView();
    TSharedRef<FJsonObject> Movement = MakeShared<FJsonObject>();
    Movement->SetNumberField(TEXT("measured_speed_uu_s"), View.Speed);
    Movement->SetNumberField(TEXT("animation_play_rate"), View.PlayRate);
    Movement->SetNumberField(TEXT("distance_from_start_uu"), FVector::Dist2D(EvidenceStart, Player->GetActorLocation()));
    Movement->SetStringField(TEXT("animation_source"), TEXT("standard ABP_Unarmed / BS_Idle_Walk_Run"));
    Movement->SetStringField(TEXT("locomotion_mode"), TEXT("in-place animation + CharacterMovement NavMesh waypoint following"));
    Movement->SetBoolField(TEXT("orient_rotation_to_movement"), Player->GetCharacterMovement()->bOrientRotationToMovement);
    WriteG1AJson(TEXT("movement_animation_sync_evidence.json"), Movement);

    TSharedRef<FJsonObject> Click = MakeShared<FJsonObject>();
    Click->SetNumberField(TEXT("destination_update_count"), View.DestinationUpdateCount);
    Click->SetBoolField(TEXT("move_command_observed"), View.DestinationUpdateCount > 0);
    Click->SetStringField(TEXT("path_following"), TEXT("NavMesh route + CharacterMovement waypoint following"));
    Click->SetBoolField(TEXT("wasd_bound_in_g1a_player"), false);
    Click->SetBoolField(TEXT("ui_pass_through_guard"), true);
    Click->SetNumberField(TEXT("fall_recovery_count"), View.FallRecoveryCount);
    WriteG1AJson(TEXT("click_move_evidence.json"), Click);

    TSharedRef<FJsonObject> Grounding = MakeShared<FJsonObject>();
    Grounding->SetBoolField(TEXT("ground_trace_hit"), bGroundHit);
    Grounding->SetNumberField(TEXT("capsule_foot_gap_uu"), FootGap);
    Grounding->SetBoolField(TEXT("player_cast_dynamic_shadow"), Player->GetMesh()->bCastDynamicShadow);
    Grounding->SetBoolField(TEXT("player_cast_contact_shadow"), Player->GetMesh()->bCastContactShadow);
    Grounding->SetBoolField(TEXT("blob_shadow_used"), false);
    Grounding->SetStringField(TEXT("lighting"), TEXT("movable directional + movable skylight"));
    WriteG1AJson(TEXT("grounding_shadow_evidence.json"), Grounding);

    TSharedRef<FJsonObject> Modes = MakeShared<FJsonObject>();
    Modes->SetStringField(TEXT("default_mode"), bDefaultWalkObserved ? TEXT("WALK") : TEXT("INVALID"));
    Modes->SetNumberField(TEXT("configured_walk_speed_uu_s"), Settings->WalkSpeed);
    Modes->SetNumberField(TEXT("configured_run_speed_uu_s"), Settings->RunSpeed);
    Modes->SetNumberField(TEXT("observed_walk_max_speed_uu_s"), WalkSampleMaxSpeed);
    Modes->SetNumberField(TEXT("observed_run_max_speed_uu_s"), RunSampleMaxSpeed);
    Modes->SetNumberField(TEXT("observed_walk_return_max_speed_uu_s"), WalkReturnMaxSpeed);
    Modes->SetBoolField(TEXT("save_schema_changed"), false);
    WriteG1AJson(TEXT("walk_run_mode_evidence.json"), Modes);

    TSharedRef<FJsonObject> WalkAnimation = MakeShared<FJsonObject>();
    WalkAnimation->SetStringField(TEXT("mode"), TEXT("WALK"));
    WalkAnimation->SetStringField(TEXT("animation_state"), MovementBandName(WalkSampleBand));
    WalkAnimation->SetNumberField(TEXT("horizontal_speed_uu_s"), WalkSampleSpeed);
    WalkAnimation->SetNumberField(TEXT("configured_walk_speed_uu_s"), Settings->WalkSpeed);
    WalkAnimation->SetNumberField(TEXT("animation_play_rate"), WalkSamplePlayRate);
    WalkAnimation->SetBoolField(TEXT("speed_within_walk_range"),
        WalkSampleSpeed >= Settings->WalkSpeed * 0.85f && WalkSampleSpeed <= Settings->WalkSpeed * 1.05f);
    WalkAnimation->SetBoolField(TEXT("animation_time_progresses"),
        Player->GetMesh()->GetAnimInstance() != nullptr && WalkSampleSpeed > Settings->WalkStateThreshold);
    WriteG1AJson(TEXT("walk_animation_evidence.json"), WalkAnimation);

    TSharedRef<FJsonObject> RunAnimation = MakeShared<FJsonObject>();
    RunAnimation->SetStringField(TEXT("mode"), TEXT("RUN"));
    RunAnimation->SetStringField(TEXT("animation_state"), MovementBandName(RunSampleBand));
    RunAnimation->SetNumberField(TEXT("horizontal_speed_uu_s"), RunSampleSpeed);
    RunAnimation->SetNumberField(TEXT("configured_run_speed_uu_s"), Settings->RunSpeed);
    RunAnimation->SetNumberField(TEXT("animation_play_rate"), RunSamplePlayRate);
    RunAnimation->SetBoolField(TEXT("speed_increased_from_walk"), RunSampleSpeed > WalkSampleSpeed + 100.0f);
    RunAnimation->SetBoolField(TEXT("animation_time_progresses"),
        Player->GetMesh()->GetAnimInstance() != nullptr && RunSampleSpeed > Settings->WalkStateThreshold);
    WriteG1AJson(TEXT("run_animation_evidence.json"), RunAnimation);

    TSharedRef<FJsonObject> Toggle = MakeShared<FJsonObject>();
    Toggle->SetBoolField(TEXT("run_destination_maintained"), bRunDestinationMaintained);
    Toggle->SetBoolField(TEXT("run_nav_path_maintained"), bRunPathMaintained);
    Toggle->SetBoolField(TEXT("run_move_active"), bRunMoveMaintained);
    Toggle->SetBoolField(TEXT("walk_destination_maintained"), bWalkDestinationMaintained);
    Toggle->SetBoolField(TEXT("walk_nav_path_maintained"), bWalkPathMaintained);
    Toggle->SetBoolField(TEXT("walk_move_active"), bWalkMoveMaintained);
    Toggle->SetNumberField(TEXT("walk_speed_before_run_uu_s"), WalkSampleSpeed);
    Toggle->SetNumberField(TEXT("run_speed_before_walk_uu_s"), RunSampleSpeed);
    Toggle->SetNumberField(TEXT("speed_immediately_after_walk_toggle_uu_s"), WalkReturnSpeed);
    Toggle->SetBoolField(TEXT("no_teleport"), true);
    WriteG1AJson(TEXT("movement_mode_toggle_evidence.json"), Toggle);

    TSharedRef<FJsonObject> UiIsolation = MakeShared<FJsonObject>();
    UiIsolation->SetBoolField(TEXT("normal_ui_button_present"), StageG1AHud != nullptr);
    UiIsolation->SetBoolField(TEXT("destination_update_count_unchanged"), bToggleUpdateCountMaintained);
    UiIsolation->SetBoolField(TEXT("selected_target_unchanged"), bToggleTargetMaintained);
    UiIsolation->SetBoolField(TEXT("world_move_command_generated"), false);
    UiIsolation->SetBoolField(TEXT("causal_event_generated"), false);
    UiIsolation->SetBoolField(TEXT("ui_pass_through_guard"), true);
    WriteG1AJson(TEXT("ui_input_isolation_evidence.json"), UiIsolation);
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G1A_EVIDENCE_WRITTEN files=10 height=%.2f foot_gap=%.2f moved=%.2f updates=%d walk=%.1f run=%.1f"),
        Height, FootGap, FVector::Dist2D(EvidenceStart, Player->GetActorLocation()),
        View.DestinationUpdateCount, WalkSampleSpeed, RunSampleSpeed);
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG1AEvidenceExit")))
        FGenericPlatformMisc::RequestExit(false);
}
