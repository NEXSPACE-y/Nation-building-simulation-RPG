#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StageG0/StageG0Targetable.h"
#include "StageG1ANpcActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG1ANpcActor final : public ACharacter, public IStageG0Targetable
{
    GENERATED_BODY()

public:
    AStageG1ANpcActor();
    virtual FStageG0TargetInfo GetStageG0TargetInfo() const override;
    virtual void SetStageG0TargetSelected(bool bSelected) override;

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SelectionRing;
};
