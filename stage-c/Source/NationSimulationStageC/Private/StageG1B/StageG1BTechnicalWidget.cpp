#include "StageG1B/StageG1BTechnicalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "StageG1B/StageG1BMeshyAnimInstance.h"
#include "Styling/CoreStyle.h"

void UStageG1BTechnicalWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.82f));
    Border->SetPadding(FMargin(8.0f));
    WidgetTree->RootWidget = Border;
    DebugText = WidgetTree->ConstructWidget<UTextBlock>();
    DebugText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11));
    DebugText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 1.0f, 0.65f)));
    DebugText->SetAutoWrapText(true);
    DebugText->SetWrapTextAt(420.0f);
    Border->SetContent(DebugText);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UStageG1BTechnicalWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bDebugVisible || !DebugText) return;
    const APawn* Pawn = GetOwningPlayerPawn();
    const USkeletalMeshComponent* Mesh = Pawn ? Pawn->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
    const UStageG1BMeshyAnimInstance* Anim = Mesh
        ? Cast<UStageG1BMeshyAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
    const TCHAR* ModeJa = !Anim || Anim->MoveMode == EStageG1BMoveMode::Walk ? TEXT("歩行") :
        Anim->MoveMode == EStageG1BMoveMode::Run ? TEXT("走行") : TEXT("ダッシュ");
    DebugText->SetText(FText::FromString(FString::Printf(
        TEXT("Visual Model: PLAYER_M Meshy v0.1\n")
        TEXT("Skeleton: Meshy 24 bones\n")
        TEXT("Move Mode: %s\n")
        TEXT("Current Animation: %s\n")
        TEXT("Animation Source: %s\n")
        TEXT("Configured Speed: %.0f | Horizontal Speed: %.1f | Play Rate: %.2f\n")
        TEXT("Root Motion: Ignored\n")
        TEXT("Manny Fallback: false\n")
        TEXT("Idle Provisional: false"),
        ModeJa,
        Anim ? *Anim->StateName : TEXT("INITIALIZING"),
        Anim ? *Anim->GetAnimationSourceName() : TEXT("NONE"),
        Anim ? Anim->GetConfiguredSpeed() : 0.0f,
        Anim ? Anim->HorizontalSpeed : 0.0f,
        Anim ? Anim->PlayRate : 1.0f)));
}

void UStageG1BTechnicalWidget::ToggleDebug()
{
    bDebugVisible = !bDebugVisible;
    SetVisibility(bDebugVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
