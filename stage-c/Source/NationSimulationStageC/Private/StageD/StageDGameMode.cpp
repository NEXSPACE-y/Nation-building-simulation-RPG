#include "StageD/StageDGameMode.h"

#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDHudWidget.h"
#include "StageD/StageDLocationVolume.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDPlayerCharacter.h"

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
    BuildCapitalBlock();
    SpawnNpcPresentation();

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
        UE_LOG(LogTemp, Display, TEXT("STAGE_D_PLAYABLE_READY pawn=%s npc_count=%d map=%s location=%s"),
            Pawn ? *Pawn->GetName() : TEXT("NONE"), CoreSubsystem ? CoreSubsystem->GetAllNpcViews().Num() : 0,
            *GetWorld()->GetMapName(), *InitialLocationId);
    }
}

void AStageDGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetGameInstance())
    {
        if (auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>())
        {
            Subsystem->AdvancePresentation(DeltaSeconds);
        }
    }
}

void AStageDGameMode::SpawnBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube) return;
    AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    Block->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    Block->GetStaticMeshComponent()->SetStaticMesh(Cube);
    Block->SetActorScale3D(Scale);
}

void AStageDGameMode::BuildCapitalBlock()
{
    const TArray<FString> Locations = {TEXT("capital"), TEXT("market"), TEXT("tavern"), TEXT("residential"), TEXT("gate")};
    for (const FString& LocationId : Locations)
    {
        const FVector Center = GetLocationCenter(LocationId);
        SpawnBlock(Center + FVector(0.0f, 0.0f, -55.0f), FVector(18.0f, 18.0f, 1.0f));
        auto* Volume = GetWorld()->SpawnActor<AStageDLocationVolume>(Center + FVector(0.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
        Volume->InitializeLocation(LocationId, FVector(900.0f, 900.0f, 250.0f));
        SpawnBlock(Center + FVector(650.0f, 650.0f, 180.0f), FVector(3.5f, 3.5f, 3.5f));
        SpawnBlock(Center + FVector(-650.0f, 650.0f, 130.0f), FVector(2.5f, 2.5f, 2.5f));
    }
    SpawnBlock(FVector(1200.0f, 0.0f, -40.0f), FVector(24.0f, 3.0f, 0.8f));
    SpawnBlock(FVector(-1200.0f, 0.0f, -40.0f), FVector(24.0f, 3.0f, 0.8f));
    SpawnBlock(FVector(0.0f, 1200.0f, -40.0f), FVector(3.0f, 24.0f, 0.8f));
    SpawnBlock(FVector(0.0f, -1200.0f, -40.0f), FVector(3.0f, 24.0f, 0.8f));

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
        if (Npc) Npc->InitializeNpc(View.NpcId, View.Role, View.bIsAi);
        ++Index;
    }
}
