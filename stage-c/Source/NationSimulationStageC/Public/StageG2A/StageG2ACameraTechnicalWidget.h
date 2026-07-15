#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageG2ACameraTechnicalWidget.generated.h"

class AStageG2ACameraModeAdapter;
class UTextBlock;

/** F1 technical readout for measured Stage G-2A camera state. */
UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG2ACameraTechnicalWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetAdapter(AStageG2ACameraModeAdapter* InAdapter);
    void ToggleDebug();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY() TWeakObjectPtr<AStageG2ACameraModeAdapter> Adapter;
    UPROPERTY() TObjectPtr<UTextBlock> TechnicalText;
    bool bVisible = false;
};
