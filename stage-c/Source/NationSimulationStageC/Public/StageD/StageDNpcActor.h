#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG0/StageG0Targetable.h"
#include "StageDNpcActor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UCapsuleComponent;
class UStageGDirectionalFlipbookComponent;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageDNpcActor final : public AActor, public IStageG0Targetable
{
    GENERATED_BODY()

public:
    AStageDNpcActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void InitializeNpc(const FString& InNpcId, const FString& InRole, bool bInIsAi, const FString& InLocationId);
    const FString& GetNpcId() const { return NpcId; }
    UStageGDirectionalFlipbookComponent* GetStageGVisual() const { return StageGVisual; }
    void SetBlobShadowVisible(bool bVisible);
    virtual FStageG0TargetInfo GetStageG0TargetInfo() const override;
    virtual void SetStageG0TargetSelected(bool bSelected) override;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> CollisionCapsule;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStageGDirectionalFlipbookComponent> StageGVisual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BlobShadow;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TargetMarker;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> IdentityLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UWidgetComponent> StageGWorldLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> ActionLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> TargetLabel;
    UPROPERTY() FString NpcId;
    UPROPERTY() FString NpcRole;
    UPROPERTY() FString LocationId;
    bool bIsAi = false;
    bool bStageG0ClickSelected = false;
    FString LastVisualizedAction;
    FVector RestLocation = FVector::ZeroVector;
    float ActionTime = 0.0f;
    FVector PreviousVisualLocation = FVector::ZeroVector;
    void UpdateBlobShadow();
};
