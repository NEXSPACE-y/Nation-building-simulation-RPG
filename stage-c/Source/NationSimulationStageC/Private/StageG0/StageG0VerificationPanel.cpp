#include "StageG0/StageG0VerificationPanel.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "StageD/StageDGameMode.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDPlayerCharacter.h"
#include "StageG0/StageG0FangRatActor.h"
#include "StageG0/StageG0Types.h"
#include "StageG0/StageGDirectionalFlipbookComponent.h"
#include "Styling/CoreStyle.h"

namespace
{
UTextBlock* AddPanelText(UWidgetTree* Tree, UVerticalBox* Parent, const FString& Value,
    int32 Size, const FLinearColor& Color)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Value));
    Text->SetColorAndOpacity(FSlateColor(Color));
    Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size));
    Text->SetAutoWrapText(true);
    Parent->AddChildToVerticalBox(Text)->SetPadding(FMargin(3.0f));
    return Text;
}

void AddCommandGroup(UWidgetTree* Tree, UVerticalBox* Root, UStageG0VerificationPanel* Owner,
    const FString& Title, const TArray<TPair<FString, FString>>& Commands)
{
    AddPanelText(Tree, Root, Title, 14, FLinearColor(0.35f, 0.9f, 1.0f));
    UWrapBox* Row = Tree->ConstructWidget<UWrapBox>();
    Row->SetInnerSlotPadding(FVector2D(3.0f, 3.0f));
    Root->AddChildToVerticalBox(Row)->SetPadding(FMargin(1.0f, 0.0f, 1.0f, 4.0f));
    for (const auto& Pair : Commands)
    {
        UStageG0VerificationButton* Button = Tree->ConstructWidget<UStageG0VerificationButton>();
        UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Pair.Key));
        Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        Label->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12));
        Button->SetContent(Label);
        Button->InitializeCommand(Owner, Pair.Value);
        Row->AddChildToWrapBox(Button)->SetPadding(FMargin(2.0f));
    }
}
}

void UStageG0VerificationButton::InitializeCommand(
    UStageG0VerificationPanel* InOwner, const FString& InCommand)
{
    CommandOwner = InOwner;
    Command = InCommand;
    OnClicked.AddDynamic(this, &UStageG0VerificationButton::HandleClicked);
}

void UStageG0VerificationButton::HandleClicked()
{
    if (CommandOwner) CommandOwner->ExecuteCommand(Command);
}

void UStageG0VerificationPanel::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
    Backdrop->SetBrushColor(FLinearColor(0.015f, 0.02f, 0.03f, 0.94f));
    Backdrop->SetPadding(FMargin(9.0f));
    WidgetTree->RootWidget = Backdrop;
    UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
    Backdrop->SetContent(Scroll);
    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
    Scroll->AddChild(Root);

    AddPanelText(WidgetTree, Root, TEXT("Stage G-0 描画検証パネル"), 18, FLinearColor(1.0f, 0.8f, 0.2f));
    AddPanelText(WidgetTree, Root,
        TEXT("表示fixture専用。因果イベント・状態遷移・国家状態は変更しません。\n左クリック保持: ポイント移動 / ホイール: ズーム / カメラ: Player自動追従"),
        11, FLinearColor(0.85f, 0.85f, 0.9f));

    AddCommandGroup(WidgetTree, Root, this, TEXT("表示対象"), {
        {TEXT("プレイヤー"), TEXT("TARGET_PLAYER")}, {TEXT("衛兵"), TEXT("TARGET_GUARD")},
        {TEXT("隊長"), TEXT("TARGET_CAPTAIN")}, {TEXT("商人"), TEXT("TARGET_BROKER")},
        {TEXT("一般住民"), TEXT("TARGET_RESIDENT")}, {TEXT("牙鼠"), TEXT("TARGET_FANG_RAT")}});
    AddCommandGroup(WidgetTree, Root, this, TEXT("人物方向"), {
        {TEXT("正面"), TEXT("DIR_0")}, {TEXT("右前"), TEXT("DIR_1")}, {TEXT("右"), TEXT("DIR_2")},
        {TEXT("右後"), TEXT("DIR_3")}, {TEXT("背面"), TEXT("DIR_4")}, {TEXT("左後"), TEXT("DIR_5")},
        {TEXT("左"), TEXT("DIR_6")}, {TEXT("左前"), TEXT("DIR_7")}});
    AddCommandGroup(WidgetTree, Root, this, TEXT("人物動作"), {
        {TEXT("待機"), TEXT("HUMAN_0")}, {TEXT("歩行"), TEXT("HUMAN_1")}, {TEXT("会話"), TEXT("HUMAN_2")},
        {TEXT("警告"), TEXT("HUMAN_3")}, {TEXT("報告"), TEXT("HUMAN_4")}, {TEXT("逃走"), TEXT("HUMAN_6")},
        {TEXT("拘束"), TEXT("HUMAN_5")}});
    AddCommandGroup(WidgetTree, Root, this, TEXT("牙鼠動作"), {
        {TEXT("待機"), TEXT("RAT_0")}, {TEXT("移動"), TEXT("RAT_7")}, {TEXT("突進"), TEXT("RAT_8")},
        {TEXT("噛みつき"), TEXT("RAT_9")}, {TEXT("被弾"), TEXT("RAT_10")}, {TEXT("死亡"), TEXT("RAT_11")},
        {TEXT("6動作を自動再生"), TEXT("RAT_AUTO")}});
    AddCommandGroup(WidgetTree, Root, this, TEXT("検証地点へ移動"), {
        {TEXT("門・衝突"), TEXT("MOVE_gate_collision")}, {TEXT("狭路"), TEXT("MOVE_narrow")},
        {TEXT("傾斜"), TEXT("MOVE_slope")}, {TEXT("遮蔽"), TEXT("MOVE_occlusion")},
        {TEXT("牙鼠"), TEXT("MOVE_fang_rat")}});
    AddCommandGroup(WidgetTree, Root, this, TEXT("表示設定"), {
        {TEXT("カメラ初期位置へ戻す"), TEXT("CAMERA_RESET")},
        {TEXT("足元影 ON"), TEXT("SHADOW_ON")}, {TEXT("足元影 OFF"), TEXT("SHADOW_OFF")},
        {TEXT("自動表示へ戻す"), TEXT("FIXTURE_CLEAR")}});
    StatusText = AddPanelText(WidgetTree, Root, TEXT("選択中: プレイヤー"), 13, FLinearColor(0.55f, 1.0f, 0.6f));
    SelectVisualTarget(TEXT("PLAYER"));
}

bool UStageG0VerificationPanel::IsPresentationOnlyCommand(const FString& Command)
{
    return Command.StartsWith(TEXT("TARGET_")) || Command.StartsWith(TEXT("DIR_")) ||
        Command.StartsWith(TEXT("HUMAN_")) || Command.StartsWith(TEXT("RAT_")) ||
        Command.StartsWith(TEXT("MOVE_")) || Command == TEXT("CAMERA_RESET") ||
        Command.StartsWith(TEXT("SHADOW_")) || Command == TEXT("FIXTURE_CLEAR");
}

void UStageG0VerificationPanel::ExecuteCommand(const FString& Command)
{
    if (!IsPresentationOnlyCommand(Command)) { SetStatus(TEXT("未定義の表示fixture操作です")); return; }
    LastCommand = Command;
    AStageDGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AStageDGameMode>() : nullptr;
    if (Command.StartsWith(TEXT("TARGET_")))
    {
        const FString TargetKey = Command.Mid(7);
        if (GameMode) GameMode->SelectStageG0VisualTarget(TargetKey);
        else SelectVisualTarget(TargetKey);
        return;
    }
    if (Command.StartsWith(TEXT("DIR_")) && SelectedVisual)
    {
        const int32 Index = FCString::Atoi(*Command.Mid(4));
        if (Index >= 0 && Index < 8)
        {
            const auto Direction = static_cast<EStageGVisualDirection>(Index);
            SelectedVisual->ApplyDebugVisualDirection(Direction);
            SetStatus(FString::Printf(TEXT("表示方向: %s"), *StageGVisualDirectionJapanese(Direction)));
        }
        return;
    }
    if (Command.StartsWith(TEXT("HUMAN_")) && SelectedVisual)
    {
        if (SelectedVisual->GetAssetFamily() == TEXT("FANG_RAT"))
        {
            SetStatus(TEXT("人物動作は人物を選択して確認してください")); return;
        }
        const int32 Index = FCString::Atoi(*Command.Mid(6));
        if (Index >= 0 && Index <= static_cast<int32>(EStageGVisualAction::Flee))
        {
            const auto Action = static_cast<EStageGVisualAction>(Index);
            SelectedVisual->ApplyDebugVisualAction(Action);
            SetStatus(FString::Printf(TEXT("表示動作: %s"), *StageGVisualActionJapanese(Action)));
        }
        return;
    }

    if (Command.StartsWith(TEXT("RAT_")) && GameMode && GameMode->GetStageG0FangRat())
    {
        if (Command == TEXT("RAT_AUTO"))
        {
            GameMode->GetStageG0FangRat()->SetAutoCycle(true);
            SetStatus(TEXT("牙鼠: 6動作を自動再生"));
        }
        else
        {
            const int32 Index = FCString::Atoi(*Command.Mid(4));
            const auto Action = static_cast<EStageGVisualAction>(Index);
            GameMode->GetStageG0FangRat()->SelectFixtureAction(Action);
            SetStatus(FString::Printf(TEXT("牙鼠動作: %s"), *StageGVisualActionJapanese(Action)));
        }
        return;
    }
    if (Command.StartsWith(TEXT("MOVE_")) && GameMode)
    {
        GameMode->TeleportPlayerToStageG0TestPoint(Command.Mid(5));
        SetStatus(TEXT("検証地点へ移動しました")); return;
    }
    if (Command == TEXT("CAMERA_RESET"))
    {
        if (auto* Player = Cast<AStageDPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0))) Player->ResetStageGCamera();
        SetStatus(TEXT("カメラを初期位置へ戻しました")); return;
    }
    if (Command == TEXT("SHADOW_ON") || Command == TEXT("SHADOW_OFF"))
    {
        if (GameMode) GameMode->SetStageG0ShadowsEnabled(Command == TEXT("SHADOW_ON"));
        SetStatus(Command == TEXT("SHADOW_ON") ? TEXT("足元影: ON") : TEXT("足元影: OFF")); return;
    }
    if (Command == TEXT("FIXTURE_CLEAR"))
    {
        if (SelectedVisual) SelectedVisual->ClearDebugFixtureOverride();
        SetStatus(TEXT("選択対象を因果コア／移動速度による自動表示へ戻しました"));
    }
}

void UStageG0VerificationPanel::SelectTargetFromWorld(const FString& TargetKey)
{
    SelectVisualTarget(TargetKey);
    SetStatus(TargetKey == TEXT("FANG_RAT")
        ? TEXT("牙鼠を選択しました。牙鼠動作ボタンで6動作を非因果再生できます")
        : TEXT("ワールド上の表示対象を選択しました"));
}

void UStageG0VerificationPanel::SelectVisualTarget(const FString& TargetKey)
{
    SelectedVisual = nullptr;
    if (TargetKey == TEXT("PLAYER"))
    {
        if (const auto* Player = Cast<AStageDPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0)))
            SelectedVisual = Player->GetStageGVisual();
    }
    else if (TargetKey == TEXT("FANG_RAT"))
    {
        for (TActorIterator<AStageG0FangRatActor> It(GetWorld()); It; ++It) { SelectedVisual = It->GetStageGVisual(); break; }
    }
    else
    {
        const FString RequiredId = TargetKey.StartsWith(TEXT("ai_npc_")) || TargetKey.StartsWith(TEXT("non_ai_npc_"))
            ? TargetKey
            : TargetKey == TEXT("GUARD") ? TEXT("ai_npc_001") :
            TargetKey == TEXT("CAPTAIN") ? TEXT("ai_npc_002") :
            TargetKey == TEXT("BROKER") ? TEXT("ai_npc_012") : TEXT("non_ai_npc_001");
        for (TActorIterator<AStageDNpcActor> It(GetWorld()); It; ++It)
            if (It->GetNpcId() == RequiredId) { SelectedVisual = It->GetStageGVisual(); break; }
    }
    SetStatus(SelectedVisual
        ? FString::Printf(TEXT("選択中: %s %s"), *StageGVisualFamilyJapanese(SelectedVisual->GetAssetFamily()), *SelectedVisual->GetVisualActorId())
        : TEXT("表示対象が見つかりません"));
}

void UStageG0VerificationPanel::SetStatus(const FString& Message)
{
    if (StatusText) StatusText->SetText(FText::FromString(Message));
}
