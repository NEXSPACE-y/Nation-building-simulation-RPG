#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageG1BTechnicalWidget.generated.h"

class UTextBlock;

UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG1BTechnicalWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    void ToggleDebug();

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> DebugText;

    bool bDebugVisible = false;
};
