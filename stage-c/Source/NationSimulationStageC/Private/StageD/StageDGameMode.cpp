#include "StageD/StageDGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/SkyLight.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Components/SkyLightComponent.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDHudWidget.h"
#include "StageD/StageDLocationVolume.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDPlayerCharacter.h"
#include "StageG0/StageG0FangRatActor.h"
#include "StageG0/StageG0Settings.h"
#include "StageG0/StageGDirectionalFlipbookComponent.h"
#include "StageG0/StageG0VerificationPanel.h"
#include "StageG0/StageG0TestSignActor.h"
#include "UObject/UObjectIterator.h"

AStageDGameMode::AStageDGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AStageDPlayerCharacter::StaticClass();
}

FVector AStageDGameMode::GetLocationCenter(const FString& LocationId)
{
    if (LocationId == TEXT("market")) return FVector(2400.0f, 0.0f, 0.0f);
    if (LocationId == TEXT("tavern")) return FVector(0.0f, 2400.0f, 0.0f);
    if (LocationId == TEXT("residential")) return FVector(-2400.0f, 0.0f, 0.0f);
    if (LocationId == TEXT("gate")) return FVector(0.0f, -2400.0f, 0.0f);
    return FVector::ZeroVector;
}

void AStageDGameMode::BeginPlay()
{
    Super::BeginPlay();
    const FString StageG0PackageMarker = FPaths::Combine(FPaths::ProjectDir(), TEXT("StageG0Package.marker"));
    if (!GetWorld()->GetMapName().Contains(TEXT("StageG0_VisualPoC")) &&
        IFileManager::Get().FileExists(*StageG0PackageMarker))
    {
        UE_LOG(LogTemp, Display, TEXT("STAGE_G0_PACKAGE_DEFAULT_MAP_REDIRECT marker=%s"), *StageG0PackageMarker);
        UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/StageG0_VisualPoC")));
        return;
    }
    bStageG0VisualPoC = GetWorld()->GetMapName().Contains(TEXT("StageG0_VisualPoC"));
    if (bStageG0VisualPoC)
    {
        // PlayerStart remains in the cooked map after the default pawn has
        // spawned. Its capsule must not carve a hole in the runtime NavMesh.
        for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
        {
            TArray<UPrimitiveComponent*> Components;
            It->GetComponents<UPrimitiveComponent>(Components);
            for (UPrimitiveComponent* Component : Components)
                if (Component) Component->SetCanEverAffectNavigation(false);
        }
    }
    BuildCapitalBlock();
    if (bStageG0VisualPoC) BuildStageG0VisualPoC();
    if (bStageG0VisualPoC)
    {
        if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            Navigation->Build();
            UE_LOG(LogTemp, Display,
                TEXT("STAGE_G0_NAVIGATION_BUILD requested=1 runtime_generation=Dynamic bounds=StageG0_NavMeshBounds"));
        }
    }
    SpawnNpcPresentation();
    if (bStageG0VisualPoC) SpawnStageG0FangRat();

    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
    {
        const auto* CoreSubsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
        const FString InitialLocationId = CoreSubsystem && !CoreSubsystem->GetWorldView().CurrentLocationId.IsEmpty()
            ? CoreSubsystem->GetWorldView().CurrentLocationId
            : TEXT("capital");
        const FVector InitialPlayerLocation = GetLocationCenter(InitialLocationId) + FVector(0.0f, 0.0f, 140.0f);
        APawn* Pawn = Controller->GetPawn();
        if (!Pawn)
        {
            Pawn = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, InitialPlayerLocation, FRotator::ZeroRotator);
            if (Pawn) Controller->Possess(Pawn);
        }
        if (Pawn) Pawn->SetActorLocation(InitialPlayerLocation);
        StageDHud = CreateWidget<UStageDHudWidget>(Controller, UStageDHudWidget::StaticClass());
        if (StageDHud)
        {
            StageDHud->AddToViewport(100);
            StageDHud->SetVisibility(ESlateVisibility::Visible);
            StageDHud->SetRenderOpacity(1.0f);
            StageDHud->SetAlignmentInViewport(FVector2D(0.0f, 0.0f));
            StageDHud->SetPositionInViewport(FVector2D(14.0f, 14.0f), false);
            StageDHud->SetDesiredSizeInViewport(FVector2D(740.0f, 285.0f));
            StageDHud->ForceLayoutPrepass();
        }
        if (bStageG0VisualPoC)
        {
            SpawnStageG0TestSign(TEXT("門・衝突試験"), GetStageG0TestPoint(TEXT("gate_collision")));
            SpawnStageG0TestSign(TEXT("狭路試験"), GetStageG0TestPoint(TEXT("narrow")));
            SpawnStageG0TestSign(TEXT("傾斜試験"), GetStageG0TestPoint(TEXT("slope")));
            SpawnStageG0TestSign(TEXT("遮蔽試験"), GetStageG0TestPoint(TEXT("occlusion")));
            SpawnStageG0TestSign(TEXT("牙鼠動作試験"), GetStageG0TestPoint(TEXT("fang_rat")));
            StageG0VerificationPanel = CreateWidget<UStageG0VerificationPanel>(
                Controller, UStageG0VerificationPanel::StaticClass());
            if (StageG0VerificationPanel)
            {
                StageG0VerificationPanel->AddToViewport(150);
                StageG0VerificationPanel->SetPositionInViewport(FVector2D(1080.0f, 20.0f), false);
                StageG0VerificationPanel->SetDesiredSizeInViewport(FVector2D(500.0f, 680.0f));
                FInputModeGameAndUI InputMode;
                InputMode.SetWidgetToFocus(StageG0VerificationPanel->TakeWidget());
                InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                InputMode.SetHideCursorDuringCapture(false);
                Controller->SetInputMode(InputMode);
                Controller->bShowMouseCursor = true;
                UE_LOG(LogTemp, Display, TEXT("STAGE_G0_VERIFICATION_PANEL_READY language=ja targets=6 directions=8 human_actions=7 rat_actions=6 test_points=5 shadow_toggle=1 camera_follow=1 point_drag_move=1 causal_events=0"));
            }
        }
        UE_LOG(LogTemp, Display, TEXT("STAGE_D_PLAYABLE_READY pawn=%s npc_count=%d map=%s location=%s"),
            Pawn ? *Pawn->GetName() : TEXT("NONE"), CoreSubsystem ? CoreSubsystem->GetAllNpcViews().Num() : 0,
            *GetWorld()->GetMapName(), *InitialLocationId);
        if (bStageG0VisualPoC)
        {
            int32 VisualCount = 0;
            for (TObjectIterator<UStageGDirectionalFlipbookComponent> It; It; ++It)
                if (It->GetWorld() == GetWorld()) ++VisualCount;
            UE_LOG(LogTemp, Display,
                TEXT("STAGE_G0_VISUAL_READY map=StageG0_VisualPoC player=%s npc_count=40 fang_rat_count=1 paper2d=1 visual_placeholders=%d directional_placeholder=PROC_ARROW japanese_panel=1 camera_yaw=%.1f camera_pitch=%.1f"),
                Pawn ? *Pawn->GetName() : TEXT("NONE"), VisualCount,
                GetDefault<UStageG0Settings>()->CameraYaw, GetDefault<UStageG0Settings>()->CameraPitch);
        }
    }
}

void AStageDGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bStageG0VisualPoC && !bStageG0PerformanceWritten)
    {
        StageG0Elapsed += DeltaSeconds;
        StageG0FrameTimes.Add(DeltaSeconds);
        if (StageG0Elapsed >= 5.0f) WriteStageG0PerformanceEvidence();
    }
    if (GetGameInstance())
    {
        if (auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>())
        {
            Subsystem->AdvancePresentation(DeltaSeconds);
        }
    }
}

void AStageDGameMode::BuildStageG0VisualPoC()
{
    // Small technical cells only. These blocks are VISUAL_PLACEHOLDER geometry,
    // not production work for the seven capital districts.
    const FVector Market = GetLocationCenter(TEXT("market"));
    SpawnBlock(Market + FVector(0.0f, 760.0f, 160.0f), FVector(8.0f, 0.6f, 3.2f));
    SpawnBlock(Market + FVector(-360.0f, 120.0f, 100.0f), FVector(1.2f, 4.8f, 2.0f));
    SpawnBlock(Market + FVector(360.0f, 120.0f, 100.0f), FVector(1.2f, 4.8f, 2.0f));
    SpawnBlock(Market + FVector(0.0f, 120.0f, 270.0f), FVector(4.8f, 1.2f, 0.7f));
    for (int32 Index = 0; Index < 5; ++Index)
        SpawnBlock(Market + FVector(-520.0f + Index * 260.0f, -460.0f, 45.0f), FVector(0.8f, 1.5f, 0.9f));

    const FVector Gate = GetLocationCenter(TEXT("gate"));
    SpawnBlock(Gate + FVector(-350.0f, 0.0f, 180.0f), FVector(1.2f, 2.0f, 3.6f));
    SpawnBlock(Gate + FVector(350.0f, 0.0f, 180.0f), FVector(1.2f, 2.0f, 3.6f));
    SpawnBlock(Gate + FVector(0.0f, 0.0f, 390.0f), FVector(4.7f, 2.0f, 0.6f));
    SpawnBlock(Gate + FVector(0.0f, -900.0f, -35.0f), FVector(3.0f, 11.0f, 0.3f), FRotator::ZeroRotator, true);
    for (int32 Index = 0; Index < 7; ++Index)
    {
        const float Side = Index % 2 == 0 ? -1.0f : 1.0f;
        SpawnBlock(Gate + FVector(Side * 520.0f, -480.0f - Index * 170.0f, 80.0f), FVector(0.35f, 0.35f, 1.6f));
    }
    SpawnBlock(Gate + FVector(720.0f, -850.0f, 10.0f), FVector(4.5f, 4.5f, 0.35f), FRotator(0.0f, 12.0f, 8.0f), true);

    // Clearly separated manual verification stations. All geometry remains
    // technical placeholder content and is not part of the capital production map.
    const FVector Narrow = GetStageG0TestPoint(TEXT("narrow"));
    SpawnBlock(Narrow + FVector(-150.0f, 0.0f, 115.0f), FVector(0.35f, 4.5f, 2.3f));
    SpawnBlock(Narrow + FVector(150.0f, 0.0f, 115.0f), FVector(0.35f, 4.5f, 2.3f));
    const FVector Slope = GetStageG0TestPoint(TEXT("slope"));
    SpawnBlock(Slope + FVector(280.0f, 0.0f, 75.0f), FVector(6.0f, 2.2f, 0.28f), FRotator(0.0f, 0.0f, -15.0f), true);
    SpawnBlock(Slope + FVector(610.0f, 0.0f, 150.0f), FVector(1.2f, 2.2f, 0.25f), FRotator::ZeroRotator, true);
    const FVector Occlusion = GetStageG0TestPoint(TEXT("occlusion"));
    SpawnBlock(Occlusion + FVector(180.0f, 0.0f, 150.0f), FVector(0.5f, 4.0f, 3.0f));

    constexpr float Edge = 4300.0f;
    SpawnStageG0BlockingVolume(TEXT("StageG0BlockingNorth"), FVector(0.0f, Edge, 500.0f), FVector(4400.0f, 100.0f, 1000.0f));
    SpawnStageG0BlockingVolume(TEXT("StageG0BlockingSouth"), FVector(0.0f, -Edge, 500.0f), FVector(4400.0f, 100.0f, 1000.0f));
    SpawnStageG0BlockingVolume(TEXT("StageG0BlockingEast"), FVector(Edge, 0.0f, 500.0f), FVector(100.0f, 4400.0f, 1000.0f));
    SpawnStageG0BlockingVolume(TEXT("StageG0BlockingWest"), FVector(-Edge, 0.0f, 500.0f), FVector(100.0f, 4400.0f, 1000.0f));
    UE_LOG(LogTemp, Display, TEXT("STAGE_G0_ENVIRONMENT_READY geometry=VISUAL_PLACEHOLDER city_cell=1 outskirts_cell=1 occluders=1 collision=3D test_points=5 blocking_volumes=4 slope=1"));
}

void AStageDGameMode::SpawnStageG0FangRat()
{
    const FVector SpawnLocation = GetStageG0TestPoint(TEXT("fang_rat")) + FVector(320.0f, 320.0f, -65.0f);
    StageG0FangRat = GetWorld()->SpawnActor<AStageG0FangRatActor>(SpawnLocation, FRotator::ZeroRotator);
}

TArray<FString> AStageDGameMode::GetStageG0TestPointIds()
{
    return {TEXT("gate_collision"), TEXT("narrow"), TEXT("slope"), TEXT("occlusion"), TEXT("fang_rat")};
}

FVector AStageDGameMode::GetStageG0TestPoint(const FString& PointId)
{
    const FVector Gate = GetLocationCenter(TEXT("gate"));
    if (PointId == TEXT("gate_collision")) return Gate + FVector(0.0f, 420.0f, 140.0f);
    if (PointId == TEXT("narrow")) return Gate + FVector(-900.0f, 420.0f, 140.0f);
    if (PointId == TEXT("slope")) return Gate + FVector(650.0f, -650.0f, 140.0f);
    if (PointId == TEXT("occlusion")) return Gate + FVector(850.0f, 650.0f, 140.0f);
    if (PointId == TEXT("fang_rat")) return Gate + FVector(0.0f, -850.0f, 140.0f);
    return FVector(0.0f, 0.0f, 140.0f);
}

void AStageDGameMode::TeleportPlayerToStageG0TestPoint(const FString& PointId)
{
    if (!bStageG0VisualPoC || !GetStageG0TestPointIds().Contains(PointId)) return;
    if (AStageDPlayerCharacter* Player = Cast<AStageDPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        StageG0IgnoreLocationEntriesUntil = GetWorld()->GetTimeSeconds() + 0.5;
        Player->SetActorLocation(GetStageG0TestPoint(PointId), false, nullptr, ETeleportType::TeleportPhysics);
        if (Player->GetCharacterMovement()) Player->GetCharacterMovement()->StopMovementImmediately();
        StageG0RecoveryMessage = FString::Printf(TEXT("検証地点へ移動: %s"), *PointId);
        UE_LOG(LogTemp, Display, TEXT("STAGE_G0_TEST_POINT_TELEPORT point=%s location=%s"),
            *PointId, *Player->GetActorLocation().ToCompactString());
    }
}

bool AStageDGameMode::ShouldIgnoreStageG0LocationEntry() const
{
    return bStageG0VisualPoC && GetWorld() && GetWorld()->GetTimeSeconds() <= StageG0IgnoreLocationEntriesUntil;
}

void AStageDGameMode::SetStageG0ShadowsEnabled(bool bEnabled)
{
    if (AStageDPlayerCharacter* Player = Cast<AStageDPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
        Player->SetBlobShadowVisible(bEnabled);
    for (TActorIterator<AStageDNpcActor> It(GetWorld()); It; ++It) It->SetBlobShadowVisible(bEnabled);
    if (StageG0FangRat) StageG0FangRat->SetBlobShadowVisible(bEnabled);
    UE_LOG(LogTemp, Display, TEXT("STAGE_G0_SHADOW visibility=%s collision=NoCollision"), bEnabled ? TEXT("ON") : TEXT("OFF"));
}

void AStageDGameMode::NotifyStageG0FallRecovery(int32 Count, const FVector& SafeLocation)
{
    StageG0RecoveryCount = Count;
    StageG0RecoveryMessage = TEXT("落下を検知したため最後の安全位置へ戻しました");
    UE_LOG(LogTemp, Warning, TEXT("STAGE_G0_FALL_RECOVERY count=%d safe_location=%s message=%s"),
        Count, *SafeLocation.ToCompactString(), *StageG0RecoveryMessage);
}

void AStageDGameMode::NotifyStageG0MovementRecovery(const FVector& SafeLocation)
{
    StageG0RecoveryMessage = TEXT("移動不能：経路再計算後も進めないため現在地で停止しました");
    UE_LOG(LogTemp, Warning, TEXT("STAGE_G0_MOVEMENT_STOP location=%s teleported=0 message=%s"),
        *SafeLocation.ToCompactString(), *StageG0RecoveryMessage);
}

void AStageDGameMode::SelectStageG0VisualTarget(const FString& TargetKey)
{
    if (bStageG0VisualPoC)
    {
        StageG0SelectedVisualTarget = TargetKey == TEXT("FANG_RAT") ? TEXT("牙鼠 fang_rat_001") : TargetKey;
        if (StageG0FangRat) StageG0FangRat->SetTargeted(TargetKey == TEXT("FANG_RAT"));
        if (StageG0VerificationPanel)
            StageG0VerificationPanel->SelectTargetFromWorld(TargetKey);
    }
}

void AStageDGameMode::SpawnStageG0TestSign(const FString& Label, const FVector& Location)
{
    AStageG0TestSignActor* Sign = GetWorld()->SpawnActor<AStageG0TestSignActor>(
        Location + FVector(0.0f, 0.0f, 230.0f), FRotator::ZeroRotator);
    if (!Sign) return;
    Sign->InitializeSign(Label);
}

void AStageDGameMode::SpawnStageG0BlockingVolume(
    const FString& Name, const FVector& Location, const FVector& Extent)
{
    AActor* Volume = GetWorld()->SpawnActor<AActor>(Location, FRotator::ZeroRotator);
    if (!Volume) return;
    Volume->Rename(*Name);
    Volume->Tags.Add(TEXT("StageG0BlockingVolume"));
    UBoxComponent* Box = NewObject<UBoxComponent>(Volume, TEXT("BlockingVolumeBox"));
    Volume->SetRootComponent(Box);
    Box->SetBoxExtent(Extent);
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->SetHiddenInGame(true);
    Box->RegisterComponent();
    // A dynamically-created root component starts at the world origin even
    // when the owner was spawned elsewhere. Reapply the actor transform after
    // registration so the four boundary boxes do not intersect at the map
    // center and split the NavMesh into disconnected quadrants.
    Volume->SetActorLocation(Location);
}

void AStageDGameMode::WriteStageG0PerformanceEvidence()
{
    bStageG0PerformanceWritten = true;
    if (StageG0FrameTimes.IsEmpty()) return;
    double Total = 0.0;
    for (const float FrameTime : StageG0FrameTimes) Total += FrameTime;
    TArray<float> Sorted = StageG0FrameTimes;
    Sorted.Sort([](float Left, float Right) { return Left > Right; });
    const int32 LowIndex = FMath::Clamp(FMath::FloorToInt(Sorted.Num() * 0.01f), 0, Sorted.Num() - 1);
    const double AverageFrameSeconds = Total / StageG0FrameTimes.Num();
    const double AverageFps = AverageFrameSeconds > 0.0 ? 1.0 / AverageFrameSeconds : 0.0;
    const double OnePercentLowFps = Sorted[LowIndex] > 0.0f ? 1.0 / Sorted[LowIndex] : 0.0;
    FIntPoint Resolution(0, 0);
    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
        Resolution = GEngine->GameViewport->Viewport->GetSizeXY();
    FString WindowMode = TEXT("UNKNOWN");
    if (GEngine && GEngine->GetGameUserSettings())
    {
        switch (GEngine->GetGameUserSettings()->GetFullscreenMode())
        {
        case EWindowMode::Fullscreen: WindowMode = TEXT("Fullscreen"); break;
        case EWindowMode::WindowedFullscreen: WindowMode = TEXT("WindowedFullscreen"); break;
        case EWindowMode::Windowed: WindowMode = TEXT("Windowed"); break;
        default: break;
        }
    }

    int32 VisualCount = 0;
    for (TObjectIterator<UStageGDirectionalFlipbookComponent> It; It; ++It)
        if (It->GetWorld() == GetWorld()) ++VisualCount;
    const FPlatformMemoryStats Memory = FPlatformMemory::GetStats();
    bool bNavigationReady = false;
    int32 NavigationPathPoints = 0;
    bool bNavigationPathValid = false;
    bool bNavigationPathPartial = true;
    FVector NavigationStart = FVector::ZeroVector;
    FVector NavigationEnd = FVector::ZeroVector;
    if (AStageDPlayerCharacter* Player = Cast<AStageDPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation ProjectedStart;
            FNavLocation Projected;
            // The positive-Y quadrant contains the deliberate capital obstacle;
            // probe a clear point so this validates navigation generation rather
            // than intentionally asking for a partial path onto obstacle geometry.
            const FVector Probe = Player->GetActorLocation() + FVector(500.0f, -300.0f, 0.0f);
            if (Navigation->ProjectPointToNavigation(Player->GetActorLocation(), ProjectedStart,
                    FVector(100.0f, 100.0f, 350.0f)) &&
                Navigation->ProjectPointToNavigation(Probe, Projected, FVector(100.0f, 100.0f, 350.0f)))
            {
                if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
                    GetWorld(), ProjectedStart.Location, Projected.Location, Player))
                {
                    NavigationStart = ProjectedStart.Location;
                    NavigationEnd = Projected.Location;
                    bNavigationPathValid = Path->IsValid();
                    bNavigationPathPartial = Path->IsPartial();
                    bNavigationReady = bNavigationPathValid && !bNavigationPathPartial && Path->PathPoints.Num() >= 2;
                    NavigationPathPoints = Path->PathPoints.Num();
                }
            }
        }
    }
    if (bNavigationReady)
    {
        UE_LOG(LogTemp, Display,
            TEXT("STAGE_G0_NAVIGATION_READY ready=1 path_points=%d click_move_surface=GameTraceChannel1 click_targetable=GameTraceChannel2"),
            NavigationPathPoints);
    }
    else
    {
        UE_LOG(LogTemp, Error,
            TEXT("STAGE_G0_NAVIGATION_READY ready=0 path_points=%d valid=%d partial=%d start=%s end=%s click_move_surface=GameTraceChannel1 click_targetable=GameTraceChannel2"),
            NavigationPathPoints, bNavigationPathValid ? 1 : 0, bNavigationPathPartial ? 1 : 0,
            *NavigationStart.ToCompactString(), *NavigationEnd.ToCompactString());
        for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        {
            if (FVector::Dist2D(It->GetActorLocation(), FVector::ZeroVector) > 600.0f) continue;
            TArray<UPrimitiveComponent*> PrimitiveComponents;
            It->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
            for (const UPrimitiveComponent* Primitive : PrimitiveComponents)
            {
                if (Primitive && Primitive->CanEverAffectNavigation())
                    UE_LOG(LogTemp, Warning,
                        TEXT("STAGE_G0_NAV_RELEVANT owner=%s component=%s class=%s location=%s extent=%s collision=%d"),
                        *It->GetName(), *Primitive->GetName(), *Primitive->GetClass()->GetName(),
                        *Primitive->Bounds.Origin.ToCompactString(), *Primitive->Bounds.BoxExtent.ToCompactString(),
                        static_cast<int32>(Primitive->GetCollisionEnabled()));
            }
        }
    }
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("resolution"), FString::Printf(TEXT("%dx%d"), Resolution.X, Resolution.Y));
    Root->SetStringField(TEXT("window_mode"), WindowMode);
    Root->SetStringField(TEXT("gpu"), FPlatformMisc::GetPrimaryGPUBrand());
    Root->SetStringField(TEXT("cpu"), FPlatformMisc::GetCPUBrand());
    Root->SetNumberField(TEXT("ram_bytes"), static_cast<double>(Memory.TotalPhysical));
    Root->SetNumberField(TEXT("average_fps"), AverageFps);
    Root->SetNumberField(TEXT("average_frame_time_ms"), AverageFrameSeconds * 1000.0);
    Root->SetNumberField(TEXT("one_percent_low_fps"), OnePercentLowFps);
    Root->SetNumberField(TEXT("vram_usage_bytes"), -1.0);
    Root->SetStringField(TEXT("vram_measurement"), TEXT("NOT_AVAILABLE_IN_AUTOMATED_POC"));
    Root->SetNumberField(TEXT("rendered_character_count"), VisualCount);
    Root->SetNumberField(TEXT("draw_calls"), -1.0);
    Root->SetNumberField(TEXT("sprite_count"), VisualCount);
    Root->SetNumberField(TEXT("flipbook_count"), VisualCount);
    Root->SetBoolField(TEXT("visual_placeholder"), true);
    Root->SetBoolField(TEXT("navigation_ready"), bNavigationReady);
    Root->SetNumberField(TEXT("navigation_probe_path_points"), NavigationPathPoints);
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Root, Writer);
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG0"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FFileHelper::SaveStringToFile(Json + TEXT("\n"), *FPaths::Combine(Directory, TEXT("stage_g0_performance.json")));
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G0_PERFORMANCE avg_fps=%.2f avg_frame_ms=%.3f one_percent_low_fps=%.2f characters=%d sprites=%d flipbooks=%d vram=NOT_AVAILABLE"),
        AverageFps, AverageFrameSeconds * 1000.0, OnePercentLowFps, VisualCount, VisualCount, VisualCount);
}

void AStageDGameMode::SpawnBlock(
    const FVector& Location, const FVector& Scale, const FRotator& Rotation, bool bWalkableSurface)
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube) return;
    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    UStaticMeshComponent* Component = Block->GetStaticMeshComponent();
    Component->SetMobility(EComponentMobility::Movable);
    Component->SetStaticMesh(Cube);
    Component->SetCanEverAffectNavigation(true);
    Component->SetCollisionResponseToChannel(ECC_GameTraceChannel1,
        bWalkableSurface ? ECR_Block : ECR_Ignore);
    Block->Tags.Add(bWalkableSurface ? TEXT("ClickMoveSurface") : TEXT("StageG0NonWalkable"));
    Block->SetActorScale3D(Scale);
}

void AStageDGameMode::BuildCapitalBlock()
{
    const TArray<FString> Locations = {TEXT("capital"), TEXT("market"), TEXT("tavern"), TEXT("residential"), TEXT("gate")};
    for (const FString& LocationId : Locations)
    {
        const FVector Center = GetLocationCenter(LocationId);
        // Keep all district and connecting-road top faces on Z=0. A five-unit
        // overlap previously produced two Recast layers at intersections in
        // packaged builds and yielded partial paths from the capital center.
        SpawnBlock(Center + FVector(0.0f, 0.0f, -50.0f), FVector(18.0f, 18.0f, 1.0f), FRotator::ZeroRotator, true);
        auto* Volume = GetWorld()->SpawnActor<AStageDLocationVolume>(Center + FVector(0.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
        Volume->InitializeLocation(LocationId, FVector(900.0f, 900.0f, 250.0f));
        SpawnBlock(Center + FVector(650.0f, 650.0f, 180.0f), FVector(3.5f, 3.5f, 3.5f));
        SpawnBlock(Center + FVector(-650.0f, 650.0f, 130.0f), FVector(2.5f, 2.5f, 2.5f));
    }
    SpawnBlock(FVector(1200.0f, 0.0f, -40.0f), FVector(24.0f, 8.0f, 0.8f), FRotator::ZeroRotator, true);
    SpawnBlock(FVector(-1200.0f, 0.0f, -40.0f), FVector(24.0f, 8.0f, 0.8f), FRotator::ZeroRotator, true);
    SpawnBlock(FVector(0.0f, 1200.0f, -40.0f), FVector(8.0f, 24.0f, 0.8f), FRotator::ZeroRotator, true);
    SpawnBlock(FVector(0.0f, -1200.0f, -40.0f), FVector(8.0f, 24.0f, 0.8f), FRotator::ZeroRotator, true);

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(FVector(0.0f, 0.0f, 1500.0f), FRotator(-50.0f, -35.0f, 0.0f));
    if (Sun) Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (Sky)
    {
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->SetIntensity(1.2f);
    }
}

void AStageDGameMode::SpawnNpcPresentation()
{
    if (!GetGameInstance()) return;
    const auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;
    TMap<FString, int32> Counts;
    for (const FStageDNpcView& View : Subsystem->GetAllNpcViews())
    {
        int32& Index = Counts.FindOrAdd(View.LocationId);
        int32 PresentationSlot = Index;
        // The accepted Stage D scenario starts with non_ai_npc_002 at market.
        // Swap two presentation slots so that target is immediately within the
        // configured 350uu interaction range after location travel. Core IDs,
        // locations, ordering and causal state remain untouched.
        if (View.LocationId == TEXT("market") && View.NpcId == TEXT("non_ai_npc_002")) PresentationSlot = 7;
        else if (View.LocationId == TEXT("market") && View.NpcId == TEXT("non_ai_npc_017")) PresentationSlot = 4;
        const int32 Column = PresentationSlot % 5;
        const int32 Row = PresentationSlot / 5;
        const FVector Offset(-500.0f + Column * 240.0f, -420.0f + Row * 280.0f, 100.0f);
        AStageDNpcActor* Npc = GetWorld()->SpawnActor<AStageDNpcActor>(GetLocationCenter(View.LocationId) + Offset, FRotator::ZeroRotator);
        if (Npc) Npc->InitializeNpc(View.NpcId, View.Role, View.bIsAi, View.LocationId);
        ++Index;
    }
}
