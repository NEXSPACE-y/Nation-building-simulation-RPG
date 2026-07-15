#include "StageG2A/StageG2ACameraTechnicalWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"

void UStageG2ACameraTechnicalWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StageG2ACameraTechnicalBorder"));
    Border->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.04f, 0.92f));
    Border->SetPadding(FMargin(12.0f, 8.0f));
    TechnicalText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageG2ACameraTechnicalText"));
    TechnicalText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.92f, 1.0f, 1.0f)));
    TechnicalText->SetAutoWrapText(true);
    Border->SetContent(TechnicalText);
    WidgetTree->RootWidget = Border;
    SetVisibility(ESlateVisibility::Collapsed);
}

void UStageG2ACameraTechnicalWidget::SetAdapter(AStageG2ACameraModeAdapter* InAdapter)
{
    Adapter = InAdapter;
}

void UStageG2ACameraTechnicalWidget::ToggleDebug()
{
    bVisible = !bVisible;
    SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UStageG2ACameraTechnicalWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bVisible || !TechnicalText) return;
    const AStageG2ACameraModeAdapter* Camera = Adapter.Get();
    if (!Camera) return;
    TechnicalText->SetText(FText::FromString(FString::Printf(
        TEXT("Stage G-2A カメラ再設計\nモード: %s\nYaw %.1f / Pitch %.1f\n距離 %.1f (実測 %.1f)  FOV %.1f\n制限: Pitch %.1f～%.1f / Zoom %.0f～%.0f\nCollision: %s  右ドラッグ: %s\nF6補助俯瞰 / PLAYER_M fallback=false"),
        *Camera->GetModeDisplayNameJa(), Camera->GetCameraYaw(), Camera->GetCameraPitch(),
        Camera->GetTargetArmLength(), Camera->GetActualCameraArmLength(), Camera->GetCurrentFov(),
        Camera->GetActiveMinPitch(), Camera->GetActiveMaxPitch(),
        Camera->GetActiveMinDistance(), Camera->GetActiveMaxDistance(),
        Camera->IsCameraCollisionShortened() ? TEXT("短縮中") : TEXT("有効"),
        Camera->IsRightDragActive() ? TEXT("入力中") : TEXT("停止"))));
}
