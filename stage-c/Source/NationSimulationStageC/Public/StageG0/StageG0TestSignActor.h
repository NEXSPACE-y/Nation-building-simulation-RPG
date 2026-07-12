#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageG0TestSignActor.generated.h"

class UWidgetComponent;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG0TestSignActor final : public AActor
{
    GENERATED_BODY()

public:
    AStageG0TestSignActor();
    void InitializeSign(const FString& Label);

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<UWidgetComponent> SignWidget;
};
