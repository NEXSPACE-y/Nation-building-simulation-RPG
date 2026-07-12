#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageG0WorldLabelWidget.generated.h"

class UTextBlock;

UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG0WorldLabelWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    void SetWorldLabel(const FString& Label, const FLinearColor& Color, int32 FontSize);

private:
    UPROPERTY() TObjectPtr<UTextBlock> LabelText;
};
