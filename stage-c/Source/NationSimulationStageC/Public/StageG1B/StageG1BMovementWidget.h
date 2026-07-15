#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageG1BMovementWidget.generated.h"

class AStageG1BPlayerVisualAdapter;
class UButton;
class UTextBlock;

/** G1B-only three-mode UI layered over the unchanged accepted G1A HUD. */
UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG1BMovementWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    void SetAdapter(AStageG1BPlayerVisualAdapter* InAdapter);

private:
    UFUNCTION()
    void OnMovementModeButtonClicked();

    UPROPERTY()
    TWeakObjectPtr<AStageG1BPlayerVisualAdapter> Adapter;

    UPROPERTY()
    TObjectPtr<UTextBlock> MovementModeText;

    UPROPERTY()
    TObjectPtr<UTextBlock> MovementModeButtonText;

    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;
};
