#include "StageG1B/StageG1BMovementWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1B/StageG1BMeshyAnimInstance.h"
#include "StageG1B/StageG1BPlayerVisualAdapter.h"
#include "Styling/CoreStyle.h"

namespace
{
UTextBlock* AddText(UWidgetTree* Tree, UVerticalBox* Root, const FString& Text,
    int32 Size, const FLinearColor& Color)
{
    UTextBlock* Result = Tree->ConstructWidget<UTextBlock>();
    Result->SetText(FText::FromString(Text));
    Result->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size));
    Result->SetColorAndOpacity(FSlateColor(Color));
    Result->SetAutoWrapText(true);
    Root->AddChildToVerticalBox(Result)->SetPadding(FMargin(1.0f));
    return Result;
}
}

void UStageG1BMovementWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 1.0f));
    Border->SetPadding(FMargin(7.0f));
    WidgetTree->RootWidget = Border;
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
    Border->SetContent(Root);
    AddText(WidgetTree, Root, TEXT("Stage G-1B | PLAYER_M Meshy 3D PoC"), 12,
        FLinearColor(0.35f, 0.95f, 1.0f));
    AddText(WidgetTree, Root, TEXT("左クリック/長押し: NavMesh移動 | ホイール: ズーム | F1: 技術表示"),
        10, FLinearColor::White);
    MovementModeText = AddText(WidgetTree, Root, TEXT("移動：歩行"), 12,
        FLinearColor(0.6f, 0.95f, 1.0f));

    USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
    ButtonSize->SetHeightOverride(30.0f);
    Root->AddChildToVerticalBox(ButtonSize)->SetPadding(FMargin(1.0f));
    UButton* Button = WidgetTree->ConstructWidget<UButton>();
    Button->SetBackgroundColor(FLinearColor(0.08f, 0.42f, 0.56f, 1.0f));
    Button->OnClicked.AddDynamic(this, &UStageG1BMovementWidget::OnMovementModeButtonClicked);
    ButtonSize->SetContent(Button);
    MovementModeButtonText = WidgetTree->ConstructWidget<UTextBlock>();
    MovementModeButtonText->SetText(FText::FromString(TEXT("走行へ切替")));
    MovementModeButtonText->SetJustification(ETextJustify::Center);
    MovementModeButtonText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11));
    Button->SetContent(MovementModeButtonText);
    StatusText = AddText(WidgetTree, Root, TEXT("状態: IDLE | 待機"), 10,
        FLinearColor(1.0f, 0.85f, 0.45f));
}

void UStageG1BMovementWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    const AStageG1BPlayerVisualAdapter* VisualAdapter = Adapter.Get();
    if (!VisualAdapter || !MovementModeText || !MovementModeButtonText || !StatusText) return;
    MovementModeText->SetText(FText::FromString(FString::Printf(
        TEXT("移動：%s"), *VisualAdapter->GetMoveModeDisplayNameJa())));
    MovementModeButtonText->SetText(FText::FromString(FString::Printf(
        TEXT("%sへ切替"), *VisualAdapter->GetNextMoveModeDisplayNameJa())));
    const AStageG1APlayerCharacter* Player = VisualAdapter->GetAcceptedPlayer();
    const FString Status = Player ? Player->GetMovementView().Status : TEXT("初期化中");
    const UStageG1BMeshyAnimInstance* Anim = Player && Player->GetMesh()
        ? Cast<UStageG1BMeshyAnimInstance>(Player->GetMesh()->GetAnimInstance()) : nullptr;
    StatusText->SetText(FText::FromString(FString::Printf(TEXT("状態: %s | %s"),
        Anim ? *Anim->StateName : TEXT("INITIALIZING"), *Status)));
}

void UStageG1BMovementWidget::SetAdapter(AStageG1BPlayerVisualAdapter* InAdapter)
{
    Adapter = InAdapter;
}

void UStageG1BMovementWidget::OnMovementModeButtonClicked()
{
    if (AStageG1BPlayerVisualAdapter* VisualAdapter = Adapter.Get())
        VisualAdapter->CycleMovementMode();
}
