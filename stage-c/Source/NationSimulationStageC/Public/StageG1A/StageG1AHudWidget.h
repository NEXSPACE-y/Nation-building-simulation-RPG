#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageG1AHudWidget.generated.h"

class UTextBlock;
class UButton;

UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG1AHudWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    void ToggleDebug();

private:
    UFUNCTION() void OnMovementModeButtonClicked();

    UPROPERTY() TObjectPtr<UTextBlock> MovementModeText;
    UPROPERTY() TObjectPtr<UButton> MovementModeButton;
    UPROPERTY() TObjectPtr<UTextBlock> MovementModeButtonText;
    UPROPERTY() TObjectPtr<UTextBlock> StatusText;
    UPROPERTY() TObjectPtr<UTextBlock> DebugText;
    bool bDebugVisible = false;
};
