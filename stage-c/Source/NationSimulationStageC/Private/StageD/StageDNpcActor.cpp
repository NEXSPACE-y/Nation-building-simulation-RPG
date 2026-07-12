#include "StageD/StageDNpcActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageG0/StageGDirectionalFlipbookComponent.h"
#include "StageG0/StageG0Types.h"
#include "StageG0/StageG0WorldLabelWidget.h"
#include "UObject/ConstructorHelpers.h"

AStageDNpcActor::AStageDNpcActor()
{
    PrimaryActorTick.bCanEverTick = true;
    CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("StageGCollisionCapsule"));
    CollisionCapsule->InitCapsuleSize(42.0f, 95.0f);
    CollisionCapsule->SetCanEverAffectNavigation(false);
    CollisionCapsule->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    CollisionCapsule->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
    SetRootComponent(CollisionCapsule);

    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    Body->SetupAttachment(CollisionCapsule);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) Body->SetStaticMesh(Cylinder.Object);
    Body->SetRelativeScale3D(FVector(0.42f, 0.42f, 1.0f));
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->SetHiddenInGame(true);

    StageGVisual = CreateDefaultSubobject<UStageGDirectionalFlipbookComponent>(TEXT("StageGDirectionalVisual"));
    StageGVisual->SetupAttachment(CollisionCapsule);

    BlobShadow = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageGBlobShadow"));
    BlobShadow->SetupAttachment(CollisionCapsule);
    BlobShadow->SetRelativeLocation(FVector(0.0f, 0.0f, -94.0f));
    BlobShadow->SetRelativeScale3D(FVector(0.52f, 0.34f, 0.018f));
    BlobShadow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlobShadow->SetCastShadow(false);
    if (Cylinder.Succeeded()) BlobShadow->SetStaticMesh(Cylinder.Object);
    BlobShadow->ComponentTags.Add(TEXT("StageG0BlobShadow"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShadowBaseMaterial(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (ShadowBaseMaterial.Succeeded())
    {
        UMaterialInstanceDynamic* ShadowMaterial = UMaterialInstanceDynamic::Create(
            ShadowBaseMaterial.Object, this, TEXT("StageGNpcShadowMaterial"));
        if (ShadowMaterial)
        {
            ShadowMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.015f, 0.015f, 0.02f, 1.0f));
            BlobShadow->SetMaterial(0, ShadowMaterial);
        }
    }

    TargetMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageG0TargetMarker"));
    TargetMarker->SetupAttachment(CollisionCapsule);
    TargetMarker->SetRelativeLocation(FVector(0.0f, 0.0f, -94.0f));
    TargetMarker->SetRelativeScale3D(FVector(0.76f, 0.76f, 0.025f));
    TargetMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TargetMarker->SetCastShadow(false);
    TargetMarker->SetVisibility(false);
    if (Cylinder.Succeeded()) TargetMarker->SetStaticMesh(Cylinder.Object);
    if (ShadowBaseMaterial.Succeeded())
    {
        UMaterialInstanceDynamic* TargetMaterial = UMaterialInstanceDynamic::Create(
            ShadowBaseMaterial.Object, this, TEXT("StageG0NpcTargetMaterial"));
        if (TargetMaterial)
        {
            TargetMaterial->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.1f, 0.75f, 1.0f));
            TargetMarker->SetMaterial(0, TargetMaterial);
        }
    }

    IdentityLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("IdentityLabel"));
    IdentityLabel->SetupAttachment(CollisionCapsule);
    IdentityLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
    IdentityLabel->SetHorizontalAlignment(EHTA_Center);
    IdentityLabel->SetWorldSize(14.0f);
    IdentityLabel->SetTextRenderColor(FColor::White);

    StageGWorldLabel = CreateDefaultSubobject<UWidgetComponent>(TEXT("StageG0JapaneseWorldLabel"));
    StageGWorldLabel->SetupAttachment(CollisionCapsule);
    StageGWorldLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 135.0f));
    StageGWorldLabel->SetWidgetSpace(EWidgetSpace::Screen);
    StageGWorldLabel->SetDrawSize(FVector2D(360.0f, 48.0f));
    StageGWorldLabel->SetPivot(FVector2D(0.5f, 0.5f));
    StageGWorldLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StageGWorldLabel->SetWidgetClass(UStageG0WorldLabelWidget::StaticClass());

    ActionLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ActionLabel"));
    ActionLabel->SetupAttachment(CollisionCapsule);
    ActionLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 165.0f));
    ActionLabel->SetHorizontalAlignment(EHTA_Center);
    ActionLabel->SetWorldSize(18.0f);
    ActionLabel->SetTextRenderColor(FColor::Yellow);
    ActionLabel->SetHiddenInGame(true);

    TargetLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TargetLabel"));
    TargetLabel->SetupAttachment(CollisionCapsule);
    TargetLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
    TargetLabel->SetHorizontalAlignment(EHTA_Center);
    TargetLabel->SetWorldSize(20.0f);
    TargetLabel->SetTextRenderColor(FColor(255, 40, 220));
    TargetLabel->SetText(FText::FromString(TEXT("[ TARGET ]")));
    TargetLabel->SetHiddenInGame(true);
}

void AStageDNpcActor::InitializeNpc(
    const FString& InNpcId, const FString& InRole, bool bInIsAi, const FString& InLocationId)
{
    NpcId = InNpcId;
    NpcRole = InRole;
    bIsAi = bInIsAi;
    LocationId = InLocationId;
    RestLocation = GetActorLocation();
    PreviousVisualLocation = RestLocation;
    FString AssetFamily = TEXT("RESIDENT");
    if (NpcId == TEXT("ai_npc_001") || NpcRole.Contains(TEXT("GUARD"), ESearchCase::IgnoreCase)) AssetFamily = TEXT("GUARD");
    if (NpcId == TEXT("ai_npc_002") || NpcRole.Contains(TEXT("CAPTAIN"), ESearchCase::IgnoreCase)) AssetFamily = TEXT("CAPTAIN");
    if (NpcId == TEXT("ai_npc_012") || NpcRole.Contains(TEXT("BROKER"), ESearchCase::IgnoreCase)) AssetFamily = TEXT("BROKER");
    if (StageGVisual) StageGVisual->ConfigureVisual(NpcId, AssetFamily, false);
    IdentityLabel->SetText(FText::FromString(FString::Printf(TEXT("%s %s"),
        *StageGVisualFamilyJapanese(AssetFamily), *NpcId)));
    if (AssetFamily == TEXT("GUARD")) IdentityLabel->SetTextRenderColor(FColor(75, 145, 255));
    else if (AssetFamily == TEXT("CAPTAIN")) IdentityLabel->SetTextRenderColor(FColor(255, 85, 85));
    else if (AssetFamily == TEXT("BROKER")) IdentityLabel->SetTextRenderColor(FColor(200, 95, 255));
    else IdentityLabel->SetTextRenderColor(FColor(135, 255, 145));
    const bool bStageG0Map = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("StageG0_VisualPoC"));
    if (bStageG0Map && CollisionCapsule)
    {
        // Forty presentation actors must remain mouse-queryable without forming
        // an accidental physical cage around the playable verification path.
        CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
        CollisionCapsule->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
    }
    IdentityLabel->SetHiddenInGame(bStageG0Map);
    StageGWorldLabel->SetVisibility(bStageG0Map);
    if (bStageG0Map) StageGWorldLabel->InitWidget();
    if (bStageG0Map)
    if (auto* Label = Cast<UStageG0WorldLabelWidget>(StageGWorldLabel->GetUserWidgetObject()))
    {
        FLinearColor LabelColor(0.45f, 1.0f, 0.5f);
        if (AssetFamily == TEXT("GUARD")) LabelColor = FLinearColor(0.25f, 0.55f, 1.0f);
        else if (AssetFamily == TEXT("CAPTAIN")) LabelColor = FLinearColor(1.0f, 0.25f, 0.25f);
        else if (AssetFamily == TEXT("BROKER")) LabelColor = FLinearColor(0.8f, 0.3f, 1.0f);
        Label->SetWorldLabel(FString::Printf(TEXT("%s %s"), *StageGVisualFamilyJapanese(AssetFamily), *NpcId), LabelColor, 18);
    }
}

void AStageDNpcActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (NpcId.IsEmpty() || !GetGameInstance()) return;
    UpdateBlobShadow();

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
    const bool bIsTarget = bStageG0ClickSelected || Subsystem->GetWorldView().TargetNpcId == NpcId;
    const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    const float PlayerDistance = Player ? FVector::Distance(Player->GetActorLocation(), GetActorLocation()) : TNumericLimits<float>::Max();
    const bool bStageG0Map = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("StageG0_VisualPoC"));
    StageGWorldLabel->SetVisibility(bStageG0Map && (bIsTarget || PlayerDistance <= 650.0f));
    IdentityLabel->SetHiddenInGame(bStageG0Map || (!bIsTarget && PlayerDistance > 650.0f));
    TargetLabel->SetHiddenInGame(!bIsTarget);
    const FStageDNpcView View = Subsystem->GetNpcView(NpcId);
    if (View.SelectedAction != LastVisualizedAction)
    {
        LastVisualizedAction = View.SelectedAction;
        ActionTime = 0.0f;
        ActionLabel->SetText(FText::FromString(StageGVisualActionJapanese(
            UStageGDirectionalFlipbookComponent::MapCoreAction(LastVisualizedAction, false))));
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

    if (StageGVisual)
    {
        if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
        {
            const FVector VisualVelocity = DeltaSeconds > KINDA_SMALL_NUMBER
                ? (GetActorLocation() - PreviousVisualLocation) / DeltaSeconds
                : FVector::ZeroVector;
            StageGVisual->UpdatePresentation(VisualVelocity, LastVisualizedAction,
                Camera->GetCameraLocation(), Camera->GetCameraRotation(), DeltaSeconds);
        }
    }
    PreviousVisualLocation = GetActorLocation();
}

FStageG0TargetInfo AStageDNpcActor::GetStageG0TargetInfo() const
{
    FStageG0TargetInfo Info;
    Info.TargetId = NpcId;
    Info.TargetType = bIsAi ? TEXT("AI_NPC") : TEXT("NON_AI_NPC");
    FString Family = TEXT("RESIDENT");
    if (NpcId == TEXT("ai_npc_001") || NpcRole.Contains(TEXT("GUARD"), ESearchCase::IgnoreCase)) Family = TEXT("GUARD");
    if (NpcId == TEXT("ai_npc_002") || NpcRole.Contains(TEXT("CAPTAIN"), ESearchCase::IgnoreCase)) Family = TEXT("CAPTAIN");
    if (NpcId == TEXT("ai_npc_012") || NpcRole.Contains(TEXT("BROKER"), ESearchCase::IgnoreCase)) Family = TEXT("BROKER");
    Info.DisplayNameJa = FString::Printf(TEXT("%s %s"), *StageGVisualFamilyJapanese(Family), *NpcId);
    Info.CountryId = TEXT("country_001");
    Info.LocationId = LocationId;
    Info.bIsActive = !IsActorBeingDestroyed();
    Info.bIsTargetable = Info.bIsActive && !NpcId.IsEmpty();
    Info.InteractionOrigin = GetActorLocation();
    return Info;
}

void AStageDNpcActor::SetStageG0TargetSelected(bool bSelected)
{
    bStageG0ClickSelected = bSelected;
    if (TargetMarker) TargetMarker->SetVisibility(bSelected, true);
}

void AStageDNpcActor::SetBlobShadowVisible(bool bVisible)
{
    if (BlobShadow) BlobShadow->SetVisibility(bVisible, true);
}

void AStageDNpcActor::UpdateBlobShadow()
{
    if (!BlobShadow || !GetWorld()) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG0NpcShadow), false, this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + FVector(0, 0, 40),
        GetActorLocation() - FVector(0, 0, 240), ECC_Visibility, Params))
    {
        BlobShadow->SetWorldLocation(Hit.ImpactPoint + Hit.ImpactNormal * 2.0f);
        BlobShadow->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector, Hit.ImpactNormal));
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
