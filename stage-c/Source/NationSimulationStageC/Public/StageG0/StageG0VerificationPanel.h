#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "StageG0VerificationPanel.generated.h"

class UStageG0VerificationPanel;
class UStageGDirectionalFlipbookComponent;
class UTextBlock;

UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG0VerificationButton final : public UButton
{
    GENERATED_BODY()

public:
    void InitializeCommand(UStageG0VerificationPanel* InOwner, const FString& InCommand);

private:
    UPROPERTY() TObjectPtr<UStageG0VerificationPanel> CommandOwner;
    UPROPERTY() FString Command;
    UFUNCTION() void HandleClicked();
};

UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG0VerificationPanel final : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    void ExecuteCommand(const FString& Command);
    void SelectTargetFromWorld(const FString& TargetKey);
    const FString& GetLastCommand() const { return LastCommand; }
    static bool IsPresentationOnlyCommand(const FString& Command);

private:
    UPROPERTY() TObjectPtr<UTextBlock> StatusText;
    UPROPERTY() TObjectPtr<UStageGDirectionalFlipbookComponent> SelectedVisual;
    FString LastCommand;

    void SelectVisualTarget(const FString& TargetKey);
    void SetStatus(const FString& Message);
};
