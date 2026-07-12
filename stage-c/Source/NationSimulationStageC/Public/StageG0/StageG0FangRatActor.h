#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG0/StageG0Types.h"
#include "StageG0/StageG0Targetable.h"
#include "StageG0FangRatActor.generated.h"

class UCapsuleComponent;
class USphereComponent;
class UStageGDirectionalFlipbookComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG0FangRatActor final : public AActor, public IStageG0Targetable
{
    GENERATED_BODY()

public:
    AStageG0FangRatActor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UStageGDirectionalFlipbookComponent* GetStageGVisual() const { return StageGVisual; }
    EStageGVisualAction GetFixtureAction() const { return FixtureActions[FixtureActionIndex]; }
    static constexpr int32 FixtureActionCount = 6;
    static bool IsSupportedFixtureAction(EStageGVisualAction Action);
    void SelectFixtureAction(EStageGVisualAction Action);
    void SetAutoCycle(bool bEnabled);
    bool IsAutoCycleEnabled() const { return bAutoCycle; }
    void SetBlobShadowVisible(bool bVisible);
    void SetTargeted(bool bTargeted);
    bool IsTargeted() const;
    virtual FStageG0TargetInfo GetStageG0TargetInfo() const override;
    virtual void SetStageG0TargetSelected(bool bSelected) override { SetTargeted(bSelected); }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> CollisionCapsule;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> TargetHitArea;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStageGDirectionalFlipbookComponent> StageGVisual;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> BlobShadow;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> TargetMarker;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> DebugLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<class UWidgetComponent> StageGWorldLabel;
    TArray<EStageGVisualAction> FixtureActions;
    int32 FixtureActionIndex = 0;
    float FixtureActionElapsed = 0.0f;
    bool bAutoCycle = true;
    FVector FixtureOrigin = FVector::ZeroVector;
    FVector PreviousLocation = FVector::ZeroVector;
    void UpdateBlobShadow();
};
