#include "StageD/StageDNpcActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "UObject/ConstructorHelpers.h"

AStageDNpcActor::AStageDNpcActor()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    SetRootComponent(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) Body->SetStaticMesh(Cylinder.Object);
    Body->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.0f));
    Body->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    IdentityLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IdentityLabel"));
    IdentityLabel->SetupAttachment(Body);
    IdentityLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
    IdentityLabel->SetHorizontalAlignment(EHTA_Center);
    IdentityLabel->SetWorldSize(14.0f);
    IdentityLabel->SetTextRenderColor(FColor::White);

    ActionLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ActionLabel"));
    ActionLabel->SetupAttachment(Body);
    ActionLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 165.0f));
    ActionLabel->SetHorizontalAlignment(EHTA_Center);
    ActionLabel->SetWorldSize(18.0f);
    ActionLabel->SetTextRenderColor(FColor::Yellow);
    ActionLabel->SetHiddenInGame(true);

    TargetLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TargetLabel"));
    TargetLabel->SetupAttachment(Body);
    TargetLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
    TargetLabel->SetHorizontalAlignment(EHTA_Center);
    TargetLabel->SetWorldSize(20.0f);
    TargetLabel->SetTextRenderColor(FColor(255, 40, 220));
    TargetLabel->SetText(FText::FromString(TEXT("[ TARGET ]")));
    TargetLabel->SetHiddenInGame(true);
}

void AStageDNpcActor::InitializeNpc(const FString& InNpcId, const FString& InRole, bool bInIsAi)
{
    NpcId = InNpcId;
    NpcRole = InRole;
    bIsAi = bInIsAi;
    RestLocation = GetActorLocation();
    IdentityLabel->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s | %s"), *NpcId, bIsAi ? TEXT("AI") : TEXT("NON AI"), *NpcRole)));
    IdentityLabel->SetTextRenderColor(bIsAi ? FColor(100, 220, 255) : FColor(180, 255, 150));
}

void AStageDNpcActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (NpcId.IsEmpty() || !GetGameInstance()) return;

    // Keep all temporary identification text readable from the active camera.
    // These components are presentation-only and do not affect core decisions.
    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        const auto FaceCamera = [Camera](UTextRenderComponent* Label)
        {
            if (!Label) return;
            const float Yaw = (Camera->GetCameraLocation() - Label->GetComponentLocation()).Rotation().Yaw;
            Label->SetWorldRotation(FRotator(0.0f, Yaw, 0.0f));
        };
        FaceCamera(IdentityLabel);
        FaceCamera(ActionLabel);
        FaceCamera(TargetLabel);
    }

    const auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;
    const bool bIsTarget = Subsystem->GetWorldView().TargetNpcId == NpcId;
    const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    const float PlayerDistance = Player ? FVector::Distance(Player->GetActorLocation(), GetActorLocation()) : TNumericLimits<float>::Max();
    IdentityLabel->SetHiddenInGame(!bIsTarget && PlayerDistance > 650.0f);
    TargetLabel->SetHiddenInGame(!bIsTarget);
    const FStageDNpcView View = Subsystem->GetNpcView(NpcId);
    if (View.SelectedAction != LastVisualizedAction)
    {
        LastVisualizedAction = View.SelectedAction;
        ActionTime = 0.0f;
        ActionLabel->SetText(FText::FromString(LastVisualizedAction));
    }
    ActionTime += DeltaSeconds;

    const bool bImportantCausalAction =
        LastVisualizedAction == TEXT("REPORT") ||
        LastVisualizedAction == TEXT("WARN") ||
        LastVisualizedAction == TEXT("REFUSE_TRADE") ||
        LastVisualizedAction == TEXT("FLEE");
    ActionLabel->SetHiddenInGame(!bImportantCausalAction || PlayerDistance > 900.0f);

    // This maps core-selected actions to presentation only; it never selects an action.
    if (LastVisualizedAction == TEXT("FLEE"))
    {
        const float Distance = FMath::Min(ActionTime * 120.0f, 350.0f);
        SetActorLocation(RestLocation + GetActorRightVector() * Distance);
    }
    else if (LastVisualizedAction == TEXT("WARN"))
    {
        const float Pulse = 1.0f + 0.08f * FMath::Sin(ActionTime * 8.0f);
        Body->SetRelativeScale3D(FVector(0.42f * Pulse, 0.42f * Pulse, Pulse));
    }
    else
    {
        Body->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.0f));
    }
}

void AStageDNpcActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (!NpcId.IsEmpty() && GetGameInstance())
    {
        if (auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>())
        {
            if (Subsystem->GetWorldView().TargetNpcId == NpcId)
            {
                Subsystem->ClearTargetNpc(TEXT("Target NPC Actor was destroyed"));
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}
