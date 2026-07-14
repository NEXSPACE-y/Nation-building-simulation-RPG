#include "StageG1A/StageG1AHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1A/StageG1ASettings.h"
#include "Styling/CoreStyle.h"

namespace
{
UTextBlock* AddG1AText(UWidgetTree* Tree, UVerticalBox* Parent, const FString& Text, int32 Size,
    const FLinearColor& Color)
{
    UTextBlock* Block = Tree->ConstructWidget<UTextBlock>();
    Block->SetText(FText::FromString(Text));
    Block->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size));
    Block->SetColorAndOpacity(FSlateColor(Color));
    Block->SetAutoWrapText(true);
    Block->SetWrapTextAt(420.0f);
    Parent->AddChildToVerticalBox(Block)->SetPadding(FMargin(2.0f));
    return Block;
}
}

void UStageG1AHudWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
    Border->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.78f));
    Border->SetPadding(FMargin(8.0f));
    WidgetTree->RootWidget = Border;
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
    Border->SetContent(Root);
    AddG1AText(WidgetTree, Root, TEXT("Stage G-1A | 標準3Dキャラクター技術PoC"), 15,
        FLinearColor(0.25f, 0.9f, 1.0f));
    AddG1AText(WidgetTree, Root, TEXT("左クリック/長押し: NavMesh移動  |  ホイール: ズーム  |  F1: 技術表示"),
        12, FLinearColor::White);
    MovementModeText = AddG1AText(WidgetTree, Root, TEXT("移動：歩行"), 12,
        FLinearColor(0.6f, 0.95f, 1.0f));
    USizeBox* ButtonSize = WidgetTree->ConstructWidget<USizeBox>();
    ButtonSize->SetWidthOverride(150.0f);
    ButtonSize->SetHeightOverride(32.0f);
    Root->AddChildToVerticalBox(ButtonSize)->SetPadding(FMargin(2.0f));
    MovementModeButton = WidgetTree->ConstructWidget<UButton>();
    MovementModeButton->SetBackgroundColor(FLinearColor(0.08f, 0.42f, 0.56f, 1.0f));
    ButtonSize->SetContent(MovementModeButton);
    MovementModeButtonText = WidgetTree->ConstructWidget<UTextBlock>();
    MovementModeButtonText->SetText(FText::FromString(TEXT("走行へ切替")));
    MovementModeButtonText->SetJustification(ETextJustify::Center);
    MovementModeButtonText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12));
    MovementModeButton->SetContent(MovementModeButtonText);
    MovementModeButton->OnClicked.AddDynamic(this, &UStageG1AHudWidget::OnMovementModeButtonClicked);
    StatusText = AddG1AText(WidgetTree, Root, TEXT("状態: 待機"), 12, FLinearColor(1.0f, 0.85f, 0.45f));
    DebugText = AddG1AText(WidgetTree, Root, TEXT(""), 11, FLinearColor(0.55f, 1.0f, 0.65f));
    DebugText->SetVisibility(ESlateVisibility::Collapsed);
}

void UStageG1AHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    const AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(GetOwningPlayerPawn());
    if (!Player) return;
    const FStageG1AMovementView& View = Player->GetMovementView();
    const TCHAR* Band = View.Band == EStageG1AMovementBand::Idle ? TEXT("IDLE") :
        View.Band == EStageG1AMovementBand::Walk ? TEXT("WALK") : TEXT("RUN");
    const bool bWalkMode = View.Mode == EStageG1AMovementMode::Walk;
    const TCHAR* ModeJa = bWalkMode ? TEXT("歩行") : TEXT("走行");
    MovementModeText->SetText(FText::FromString(FString::Printf(TEXT("移動：%s"), ModeJa)));
    MovementModeButtonText->SetText(FText::FromString(
        bWalkMode ? TEXT("走行へ切替") : TEXT("歩行へ切替")));
    StatusText->SetText(FText::FromString(FString::Printf(TEXT("状態: %s | %s"), Band, *View.Status)));
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    DebugText->SetText(FText::FromString(FString::Printf(
        TEXT("移動モード：%s | 設定歩行速度：%.0f | 設定走行速度：%.0f\n")
        TEXT("現在最大速度：%.0f | 現在水平速度：%.1f uu/s\n")
        TEXT("Animation状態：%s | Animation再生倍率：%.2f\n")
        TEXT("移動中：%s | 左クリック保持：%s | 目的地：%s\n")
        TEXT("目的地更新：%d | 選択対象：%s | 落下復帰：%d\n")
        TEXT("カメラ yaw=%.1f pitch=%.1f FOV=%.1f arm=%.1f | キャラクター倍率=%.2f"),
        ModeJa, Settings->WalkSpeed, Settings->RunSpeed,
        Player->GetCharacterMovement() ? Player->GetCharacterMovement()->MaxWalkSpeed : 0.0f,
        View.Speed, Band, View.PlayRate, View.bMoving ? TEXT("はい") : TEXT("いいえ"),
        View.bLeftHeld ? TEXT("true") : TEXT("false"), *View.Destination.ToCompactString(),
        View.DestinationUpdateCount, View.SelectedTargetId.IsEmpty() ? TEXT("none") : *View.SelectedTargetId,
        View.FallRecoveryCount, Settings->CameraYaw, Settings->CameraPitch, Settings->CameraFov,
        Player->GetCameraBoom() ? Player->GetCameraBoom()->TargetArmLength : 0.0f, Settings->CharacterScale)));
}

void UStageG1AHudWidget::OnMovementModeButtonClicked()
{
    if (AStageG1APlayerCharacter* Player = Cast<AStageG1APlayerCharacter>(GetOwningPlayerPawn()))
    {
        const EStageG1AMovementMode NextMode = Player->GetMovementMode() == EStageG1AMovementMode::Walk
            ? EStageG1AMovementMode::Run : EStageG1AMovementMode::Walk;
        Player->SetMovementMode(NextMode);
    }
}

void UStageG1AHudWidget::ToggleDebug()
{
    bDebugVisible = !bDebugVisible;
    DebugText->SetVisibility(bDebugVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    SetDesiredSizeInViewport(bDebugVisible ? FVector2D(500.0f, 300.0f) : FVector2D(420.0f, 176.0f));
}
