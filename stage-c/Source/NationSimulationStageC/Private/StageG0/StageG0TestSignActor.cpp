#include "StageG0/StageG0TestSignActor.h"

#include "Components/WidgetComponent.h"
#include "StageG0/StageG0WorldLabelWidget.h"

AStageG0TestSignActor::AStageG0TestSignActor()
{
    SignWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("StageG0JapaneseTestSign"));
    SetRootComponent(SignWidget);
    SignWidget->SetWidgetSpace(EWidgetSpace::Screen);
    SignWidget->SetDrawSize(FVector2D(540.0f, 72.0f));
    SignWidget->SetPivot(FVector2D(0.5f, 0.5f));
    SignWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SignWidget->SetWidgetClass(UStageG0WorldLabelWidget::StaticClass());
}

void AStageG0TestSignActor::InitializeSign(const FString& Label)
{
    Tags.AddUnique(TEXT("StageG0TestPoint"));
    SignWidget->InitWidget();
    if (auto* WorldLabel = Cast<UStageG0WorldLabelWidget>(SignWidget->GetUserWidgetObject()))
        WorldLabel->SetWorldLabel(Label, FLinearColor(1.0f, 0.8f, 0.08f), 30);
}
