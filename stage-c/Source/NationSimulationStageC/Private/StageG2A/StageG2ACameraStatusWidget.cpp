#include "StageG2A/StageG2ACameraStatusWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"

void UStageG2ACameraStatusWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StageG2ACameraStatusBorder"));
    Border->SetBrushColor(FLinearColor(0.025f, 0.035f, 0.055f, 0.88f));
    Border->SetPadding(FMargin(12.0f, 8.0f));
    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StageG2ACameraStatusText"));
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.94f, 1.0f, 1.0f)));
    StatusText->SetAutoWrapText(true);
    Border->SetContent(StatusText);
    WidgetTree->RootWidget = Border;
    SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UStageG2ACameraStatusWidget::SetAdapter(AStageG2ACameraModeAdapter* InAdapter)
{
    Adapter = InAdapter;
}

void UStageG2ACameraStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!StatusText) return;
    const AStageG2ACameraModeAdapter* Camera = Adapter.Get();
    if (!Camera)
    {
        StatusText->SetText(FText::FromString(TEXT("Stage G-2A カメラ準備中")));
        return;
    }
    StatusText->SetText(FText::FromString(FString::Printf(
        TEXT("視点: %s\n[F6] 標準／戦略俯瞰切替  右ドラッグ: 回転  ホイール: ズーム"),
        *Camera->GetModeDisplayNameJa())));
}
