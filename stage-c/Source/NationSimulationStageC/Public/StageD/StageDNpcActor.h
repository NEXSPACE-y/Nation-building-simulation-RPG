#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageDNpcActor.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageDNpcActor final : public AActor
{
    GENERATED_BODY()

public:
    AStageDNpcActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void InitializeNpc(const FString& InNpcId, const FString& InRole, bool bInIsAi);
    const FString& GetNpcId() const { return NpcId; }

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> IdentityLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> ActionLabel;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> TargetLabel;
    UPROPERTY() FString NpcId;
    UPROPERTY() FString NpcRole;
    bool bIsAi = false;
    FString LastVisualizedAction;
    FVector RestLocation = FVector::ZeroVector;
    float ActionTime = 0.0f;
};
