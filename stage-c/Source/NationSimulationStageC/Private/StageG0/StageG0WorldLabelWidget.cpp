#include "StageG0/StageG0WorldLabelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Styling/CoreStyle.h"

void UStageG0WorldLabelWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
    Backdrop->SetBrushColor(FLinearColor(0.005f, 0.008f, 0.012f, 0.82f));
    Backdrop->SetPadding(FMargin(8.0f, 3.0f));
    WidgetTree->RootWidget = Backdrop;
    LabelText = WidgetTree->ConstructWidget<UTextBlock>();
    LabelText->SetJustification(ETextJustify::Center);
    LabelText->SetShadowOffset(FVector2D(1.5f, 1.5f));
    LabelText->SetShadowColorAndOpacity(FLinearColor::Black);
    Backdrop->SetContent(LabelText);
}

void UStageG0WorldLabelWidget::SetWorldLabel(
    const FString& Label, const FLinearColor& Color, int32 FontSize)
{
    if (!LabelText) return;
    LabelText->SetText(FText::FromString(Label));
    LabelText->SetColorAndOpacity(FSlateColor(Color));
    LabelText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), FontSize));
}
