#include "StageG0/StageG0FangRatActor.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StageG0/StageGDirectionalFlipbookComponent.h"
#include "StageG0/StageG0WorldLabelWidget.h"
#include "UObject/ConstructorHelpers.h"

AStageG0FangRatActor::AStageG0FangRatActor()
{
    PrimaryActorTick.bCanEverTick = true;
    CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("StageGCollisionCapsule"));
    CollisionCapsule->InitCapsuleSize(48.0f, 42.0f);
    CollisionCapsule->SetCanEverAffectNavigation(false);
    CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
    CollisionCapsule->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
    SetRootComponent(CollisionCapsule);

    TargetHitArea = CreateDefaultSubobject<USphereComponent>(TEXT("StageG0TargetHitArea"));
    TargetHitArea->SetupAttachment(CollisionCapsule);
    TargetHitArea->InitSphereRadius(135.0f);
    TargetHitArea->SetCanEverAffectNavigation(false);
    TargetHitArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TargetHitArea->SetCollisionResponseToAllChannels(ECR_Ignore);
    TargetHitArea->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);

    StageGVisual = CreateDefaultSubobject<UStageGDirectionalFlipbookComponent>(TEXT("StageGDirectionalVisual"));
    StageGVisual->SetupAttachment(CollisionCapsule);
    StageGVisual->ConfigureVisual(TEXT("fang_rat_001"), TEXT("FANG_RAT"), true);

    BlobShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageGBlobShadow"));
    BlobShadow->SetupAttachment(CollisionCapsule);
    BlobShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -41.0f));
    BlobShadow->SetRelativeScale3D(FVector(0.62f, 0.38f, 0.018f));
    BlobShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlobShadow->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) BlobShadow->SetStaticMesh(Cylinder.Object);
    BlobShadow->ComponentTags.Add(TEXT("StageG0BlobShadow"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShadowBaseMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (ShadowBaseMaterial.Succeeded())
    {
        UMaterialInstanceDynamic* ShadowMaterial = UMaterialInstanceDynamic::Create(
            ShadowBaseMaterial.Object, this, TEXT("StageGFangRatShadowMaterial"));
        if (ShadowMaterial)
        {
            ShadowMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.01f, 0.01f, 0.015f, 1.0f));
            BlobShadow->SetMaterial(0, ShadowMaterial);
        }
    }

    TargetMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageG0TargetMarker"));
    TargetMarker->SetupAttachment(CollisionCapsule);
    TargetMarker->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
    TargetMarker->SetRelativeScale3D(FVector(0.82f, 0.82f, 0.025f));
    TargetMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TargetMarker->SetCastShadow(false);
    TargetMarker->SetVisibility(false);
    if (Cylinder.Succeeded()) TargetMarker->SetStaticMesh(Cylinder.Object);
    if (ShadowBaseMaterial.Succeeded())
    {
        UMaterialInstanceDynamic* TargetMaterial = UMaterialInstanceDynamic::Create(
            ShadowBaseMaterial.Object, this, TEXT("StageG0FangRatTargetMaterial"));
        if (TargetMaterial)
        {
            TargetMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.1f, 0.75f, 1.0f));
            TargetMarker->SetMaterial(0, TargetMaterial);
        }
    }

    DebugLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StageGDebugLabel"));
    DebugLabel->SetupAttachment(CollisionCapsule);
    DebugLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
    DebugLabel->SetHorizontalAlignment(EHTA_Center);
    DebugLabel->SetWorldSize(15.0f);
    DebugLabel->SetTextRenderColor(FColor(255, 120, 90));
    DebugLabel->SetText(FText::FromString(TEXT("牙鼠 fang_rat_001\n仮素材")));

    StageGWorldLabel = CreateDefaultSubobject<UWidgetComponent>(TEXT("StageG0JapaneseWorldLabel"));
    StageGWorldLabel->SetupAttachment(CollisionCapsule);
    StageGWorldLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    StageGWorldLabel->SetWidgetSpace(EWidgetSpace::Screen);
    StageGWorldLabel->SetDrawSize(FVector2D(320.0f, 52.0f));
    StageGWorldLabel->SetPivot(FVector2D(0.5f, 0.5f));
    StageGWorldLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StageGWorldLabel->SetWidgetClass(UStageG0WorldLabelWidget::StaticClass());

    FixtureActions = {
        EStageGVisualAction::Idle,
        EStageGVisualAction::MonsterMove,
        EStageGVisualAction::MonsterCharge,
        EStageGVisualAction::MonsterBite,
        EStageGVisualAction::Hit,
        EStageGVisualAction::Death
    };
}

void AStageG0FangRatActor::BeginPlay()
{
    Super::BeginPlay();
    FixtureOrigin = GetActorLocation();
    PreviousLocation = FixtureOrigin;
    DebugLabel->SetHiddenInGame(true);
    StageGWorldLabel->InitWidget();
    if (auto* Label = Cast<UStageG0WorldLabelWidget>(StageGWorldLabel->GetUserWidgetObject()))
        Label->SetWorldLabel(TEXT("牙鼠 fang_rat_001"), FLinearColor(0.85f, 0.85f, 0.9f), 19);
    if (StageGVisual) StageGVisual->ApplyDebugVisualAction(FixtureActions[FixtureActionIndex]);
}

void AStageG0FangRatActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateBlobShadow();
    FixtureActionElapsed += DeltaSeconds;
    if (bAutoCycle && FixtureActionElapsed >= 2.0f)
    {
        FixtureActionElapsed = 0.0f;
        FixtureActionIndex = (FixtureActionIndex + 1) % FixtureActions.Num();
        if (StageGVisual) StageGVisual->ApplyDebugVisualAction(FixtureActions[FixtureActionIndex]);
    }

    const EStageGVisualAction Action = FixtureActions[FixtureActionIndex];
    if (Action == EStageGVisualAction::MonsterMove || Action == EStageGVisualAction::MonsterCharge)
    {
        const float Speed = Action == EStageGVisualAction::MonsterCharge ? 260.0f : 100.0f;
        const float Travel = FMath::Sin(GetGameTimeSinceCreation() * Speed / 180.0f) * 180.0f;
        SetActorLocation(FixtureOrigin + FVector(Travel, Travel * 0.35f, 0.0f), true);
    }

    if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
    {
        const FVector Velocity = DeltaSeconds > KINDA_SMALL_NUMBER
            ? (GetActorLocation() - PreviousLocation) / DeltaSeconds
            : FVector::ZeroVector;
        StageGVisual->UpdatePresentation(Velocity, StageGVisualActionName(Action),
            Camera->GetCameraLocation(), Camera->GetCameraRotation(), DeltaSeconds);
        const float LabelYaw = (Camera->GetCameraLocation() - DebugLabel->GetComponentLocation()).Rotation().Yaw;
        DebugLabel->SetWorldRotation(FRotator(0.0f, LabelYaw, 0.0f));
        DebugLabel->SetText(FText::FromString(FString::Printf(TEXT("牙鼠 fang_rat_001\n%s%s"),
            *StageGVisualActionJapanese(Action), bAutoCycle ? TEXT("（自動再生）") : TEXT("（手動選択）"))));
    }
    PreviousLocation = GetActorLocation();
}

void AStageG0FangRatActor::SelectFixtureAction(EStageGVisualAction Action)
{
    const int32 Found = FixtureActions.IndexOfByKey(Action);
    if (Found == INDEX_NONE) return;
    bAutoCycle = false;
    FixtureActionIndex = Found;
    FixtureActionElapsed = 0.0f;
    if (StageGVisual) StageGVisual->ApplyDebugVisualAction(Action);
}

bool AStageG0FangRatActor::IsSupportedFixtureAction(EStageGVisualAction Action)
{
    return Action == EStageGVisualAction::Idle || Action == EStageGVisualAction::MonsterMove ||
        Action == EStageGVisualAction::MonsterCharge || Action == EStageGVisualAction::MonsterBite ||
        Action == EStageGVisualAction::Hit || Action == EStageGVisualAction::Death;
}

void AStageG0FangRatActor::SetAutoCycle(bool bEnabled)
{
    bAutoCycle = bEnabled;
    FixtureActionElapsed = 0.0f;
}

void AStageG0FangRatActor::SetBlobShadowVisible(bool bVisible)
{
    if (BlobShadow) BlobShadow->SetVisibility(bVisible, true);
}

void AStageG0FangRatActor::SetTargeted(bool bTargeted)
{
    if (TargetMarker) TargetMarker->SetVisibility(bTargeted, true);
    if (StageGWorldLabel)
    {
        if (auto* Label = Cast<UStageG0WorldLabelWidget>(StageGWorldLabel->GetUserWidgetObject()))
            Label->SetWorldLabel(bTargeted ? TEXT("【選択中】牙鼠 fang_rat_001") : TEXT("牙鼠 fang_rat_001"),
                bTargeted ? FLinearColor(1.0f, 0.25f, 0.8f) : FLinearColor(0.85f, 0.85f, 0.9f), 19);
    }
}

bool AStageG0FangRatActor::IsTargeted() const
{
    return TargetMarker && TargetMarker->IsVisible();
}

FStageG0TargetInfo AStageG0FangRatActor::GetStageG0TargetInfo() const
{
    FStageG0TargetInfo Info;
    Info.TargetId = TEXT("fang_rat_001");
    Info.TargetType = TEXT("FANG_RAT");
    Info.DisplayNameJa = TEXT("牙鼠 fang_rat_001");
    Info.CountryId = TEXT("country_001");
    Info.LocationId = TEXT("gate");
    Info.bIsActive = !IsActorBeingDestroyed();
    Info.bIsTargetable = Info.bIsActive;
    Info.InteractionOrigin = GetActorLocation();
    return Info;
}

void AStageG0FangRatActor::UpdateBlobShadow()
{
    if (!BlobShadow || !GetWorld()) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG0FangRatShadow), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + FVector(0, 0, 35),
        GetActorLocation() - FVector(0, 0, 180), ECC_Visibility, Params))
    {
        BlobShadow->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * 2.0f);
        BlobShadow->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector, Hit.ImpactNormal));
    }
}
