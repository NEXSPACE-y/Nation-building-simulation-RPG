#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageG2ACameraStatusWidget.generated.h"

class AStageG2ACameraModeAdapter;
class UTextBlock;

/** Small always-visible Japanese operation guide for the two camera modes. */
UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG2ACameraStatusWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetAdapter(AStageG2ACameraModeAdapter* InAdapter);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    UPROPERTY() TWeakObjectPtr<AStageG2ACameraModeAdapter> Adapter;
    UPROPERTY() TObjectPtr<UTextBlock> StatusText;
};
