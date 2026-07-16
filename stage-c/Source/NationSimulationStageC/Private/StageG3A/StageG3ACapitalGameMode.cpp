#include "StageG3A/StageG3ACapitalGameMode.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "NavigationSystem.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "Components/SkyAtmosphereComponent.h"

AStageG3ACapitalGameMode::AStageG3ACapitalGameMode()
{
    DefaultPawnClass = AStageG1APlayerCharacter::StaticClass();
}

void AStageG3ACapitalGameMode::BeginPlay()
{
    Super::BeginPlay();
    for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
    {
        TArray<UPrimitiveComponent*> Components;
        It->GetComponents<UPrimitiveComponent>(Components);
        for (UPrimitiveComponent* Component : Components)
            if (Component) Component->SetCanEverAffectNavigation(false);
    }
    const int32 WalkableSurfaceCount = ConfigureCapitalWalkableSurfaces();
    SpawnCapitalLighting();
    if (UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        Navigation->Build();
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G3A_CAPITAL_GAMEMODE_READY player=StageG1APlayerCharacter navmesh=dynamic lighting=1 walkable_surfaces=%d click_trace_block=1 legacy_plaza=0 quinn=0"),
        WalkableSurfaceCount);
}

int32 AStageG3ACapitalGameMode::ConfigureCapitalWalkableSurfaces()
{
    int32 WalkableSurfaceCount = 0;
    for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
    {
        AStaticMeshActor* Actor = *It;
        if (!Actor || !Actor->ActorHasTag(TEXT("StageG3A"))) continue;
        UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
        if (!Mesh) continue;

        // ClickMoveSurface is default-ignore by contract. Only the physical city
        // ground may answer the cursor trace; scenery remains a navigation obstacle
        // and Camera blocker but never steals the point-move hit.
        if (Actor->ActorHasTag(TEXT("WalkableSurface")))
        {
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
            Mesh->SetCanEverAffectNavigation(true);
            ++WalkableSurfaceCount;
        }
        else
        {
            Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
        }
    }
    return WalkableSurfaceCount;
}

void AStageG3ACapitalGameMode::SpawnCapitalLighting()
{
    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector(0.0f, 0.0f, 1800.0f), FRotator(-48.0f, -35.0f, 0.0f));
    if (Sun)
    {
        Sun->SetMobility(EComponentMobility::Movable);
        Sun->GetLightComponent()->SetIntensity(2.8f);
        Sun->GetLightComponent()->SetLightColor(FLinearColor(1.0f, 0.82f, 0.66f));
        Sun->GetLightComponent()->SetCastShadows(true);
        Sun->GetLightComponent()->ContactShadowLength = 0.05f;
        if (UDirectionalLightComponent* Directional =
            Cast<UDirectionalLightComponent>(Sun->GetLightComponent()))
            Directional->SetAtmosphereSunLight(true);
    }
    GetWorld()->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector, FRotator::ZeroRotator);
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (Sky)
    {
        Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        Sky->GetLightComponent()->SetIntensity(0.75f);
        Sky->GetLightComponent()->SetCastShadows(true);
        Sky->GetLightComponent()->RecaptureSky();
    }
}
