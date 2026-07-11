#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageDHudWidget.generated.h"

class UTextBlock;

UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageDHudWidget final : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    UFUNCTION(BlueprintCallable) void ToggleDebug();

private:
    UPROPERTY() TObjectPtr<UTextBlock> WorldText;
    UPROPERTY() TObjectPtr<UTextBlock> TargetText;
    UPROPERTY() TObjectPtr<UTextBlock> DialogueText;
    UPROPERTY() TObjectPtr<UTextBlock> EventText;
    UPROPERTY() TObjectPtr<UTextBlock> ReactionText;
    UPROPERTY() TObjectPtr<UTextBlock> ActionStatusText;
    UPROPERTY() TObjectPtr<UTextBlock> DebugText;
    bool bDebugVisible = false;

    void Submit(const FString& Action);
    UFUNCTION() void OnTalk();
    UFUNCTION() void OnHelp();
    UFUNCTION() void OnHarm();
    UFUNCTION() void OnTrade();
    UFUNCTION() void OnSteal();
    UFUNCTION() void OnWait();
    UFUNCTION() void OnMove();
    UFUNCTION() void OnSave();
    UFUNCTION() void OnLoad();
    UFUNCTION() void OnDebug();
};
