#include "StageG1A/StageG1ANpcActor.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "StageG1A/StageG1ASettings.h"
#include "UObject/ConstructorHelpers.h"

AStageG1ANpcActor::AStageG1ANpcActor()
{
    PrimaryActorTick.bCanEverTick = false;
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    GetCapsuleComponent()->SetCanEverAffectNavigation(true);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
    GetCharacterMovement()->DisableMovement();
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> QuinnMesh(
        TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    if (QuinnMesh.Succeeded()) GetMesh()->SetSkeletalMeshAsset(QuinnMesh.Object);
    static ConstructorHelpers::FClassFinder<UAnimInstance> StandardAnim(
        TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
    if (StandardAnim.Succeeded()) GetMesh()->SetAnimInstanceClass(StandardAnim.Class);
    GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, Settings->MeshOffsetZ));
    GetMesh()->SetRelativeRotation(FRotator(0.0f, Settings->MeshYaw, 0.0f));
    GetMesh()->SetRelativeScale3D(FVector(Settings->CharacterScale));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCastShadow(true);
    GetMesh()->bCastDynamicShadow = true;
    GetMesh()->bCastContactShadow = true;

    SelectionRing = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StageG1ASelectionRing"));
    SelectionRing->SetupAttachment(GetRootComponent());
    SelectionRing->SetRelativeLocation(FVector(0.0f, 0.0f, -94.0f));
    SelectionRing->SetRelativeScale3D(FVector(0.62f, 0.62f, 0.018f));
    SelectionRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SelectionRing->SetCastShadow(false);
    SelectionRing->SetVisibility(false, true);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> RingMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> BaseMaterial(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (RingMesh.Succeeded()) SelectionRing->SetStaticMesh(RingMesh.Object);
    if (BaseMaterial.Succeeded())
    {
        if (UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(
            BaseMaterial.Object, this, TEXT("StageG1ASelectionRingMaterial")))
        {
            Material->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.0f, 0.72f, 0.08f, 1.0f));
            SelectionRing->SetMaterial(0, Material);
        }
    }
}

FStageG0TargetInfo AStageG1ANpcActor::GetStageG0TargetInfo() const
{
    FStageG0TargetInfo Info;
    Info.TargetId = TEXT("g1a_standard_npc");
    Info.TargetType = TEXT("STANDARD_3D_NPC");
    Info.DisplayNameJa = TEXT("標準3D NPC");
    Info.LocationId = TEXT("stage_g1a_poc");
    Info.bIsActive = true;
    Info.bIsTargetable = true;
    Info.InteractionOrigin = GetActorLocation() + FVector(0.0f, 0.0f, 80.0f);
    return Info;
}

void AStageG1ANpcActor::SetStageG0TargetSelected(bool bSelected)
{
    SelectionRing->SetVisibility(bSelected, true);
}
