#include "StageD/StageDHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDPlayerCharacter.h"
#include "Styling/CoreStyle.h"

namespace
{
UTextBlock* AddText(UWidgetTree* Tree, UVerticalBox* Parent, const FString& Initial, int32 Size, const FLinearColor& Color)
{
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Initial));
    Text->SetColorAndOpacity(FSlateColor(Color));
    Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), Size));
    Text->SetAutoWrapText(true);
    Text->SetWrapTextAt(700.0f);
    Parent->AddChildToVerticalBox(Text)->SetPadding(FMargin(2.0f));
    return Text;
}

UButton* AddButton(UWidgetTree* Tree, UHorizontalBox* Parent, const FString& Label)
{
    UButton* Button = Tree->ConstructWidget<UButton>();
    UTextBlock* Text = Tree->ConstructWidget<UTextBlock>();
    Text->SetText(FText::FromString(Label));
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    Text->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12));
    Button->AddChild(Text);
    Parent->AddChildToHorizontalBox(Button)->SetPadding(FMargin(2.0f));
    return Button;
}
}

void UStageDHudWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // The hierarchy must exist before UUserWidget rebuilds its Slate widget.
    // Creating the root in NativeConstruct is too late for a native-only widget
    // and produces an invisible HUD in a packaged build.
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>();
    Backdrop->SetBrushColor(FLinearColor(0.012f, 0.016f, 0.024f, 0.88f));
    Backdrop->SetPadding(FMargin(9.0f));
    WidgetTree->RootWidget = Backdrop;

    UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>();
    Backdrop->SetContent(Root);
    AddText(WidgetTree, Root, TEXT("STAGE F | 本番規模ランタイム基盤"), 16, FLinearColor(0.2f, 0.85f, 1.0f));
    WorldText = AddText(WidgetTree, Root, TEXT("因果コアを読み込み中..."), 13, FLinearColor::White);
    TargetText = AddText(WidgetTree, Root, TEXT("対象: なし | NPCへ近づいてTab"), 15, FLinearColor(1.0f, 0.35f, 0.9f));

    UHorizontalBox* Actions = WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(Actions)->SetPadding(FMargin(0.0f, 3.0f));
    UButton* Talk = AddButton(WidgetTree, Actions, TEXT("1 会話")); Talk->OnClicked.AddDynamic(this, &UStageDHudWidget::OnTalk);
    UButton* Help = AddButton(WidgetTree, Actions, TEXT("2 支援")); Help->OnClicked.AddDynamic(this, &UStageDHudWidget::OnHelp);
    UButton* Harm = AddButton(WidgetTree, Actions, TEXT("3 危害")); Harm->OnClicked.AddDynamic(this, &UStageDHudWidget::OnHarm);
    UButton* Trade = AddButton(WidgetTree, Actions, TEXT("4 取引")); Trade->OnClicked.AddDynamic(this, &UStageDHudWidget::OnTrade);
    UButton* Steal = AddButton(WidgetTree, Actions, TEXT("5 盗む")); Steal->OnClicked.AddDynamic(this, &UStageDHudWidget::OnSteal);
    UButton* Wait = AddButton(WidgetTree, Actions, TEXT("6 待機")); Wait->OnClicked.AddDynamic(this, &UStageDHudWidget::OnWait);
    UButton* Move = AddButton(WidgetTree, Actions, TEXT("7 場所移動")); Move->OnClicked.AddDynamic(this, &UStageDHudWidget::OnMove);

    UHorizontalBox* Utilities = WidgetTree->ConstructWidget<UHorizontalBox>();
    Root->AddChildToVerticalBox(Utilities)->SetPadding(FMargin(0.0f, 1.0f, 0.0f, 3.0f));
    UButton* Save = AddButton(WidgetTree, Utilities, TEXT("F5 中間保存")); Save->OnClicked.AddDynamic(this, &UStageDHudWidget::OnSave);
    UButton* Load = AddButton(WidgetTree, Utilities, TEXT("F9 読込")); Load->OnClicked.AddDynamic(this, &UStageDHudWidget::OnLoad);
    UButton* Debug = AddButton(WidgetTree, Utilities, TEXT("F1 デバッグ")); Debug->OnClicked.AddDynamic(this, &UStageDHudWidget::OnDebug);

    DialogueText = AddText(WidgetTree, Root, TEXT("会話: -"), 13, FLinearColor(1.0f, 0.85f, 0.55f));
    EventText = AddText(WidgetTree, Root, TEXT("直近イベント: -"), 12, FLinearColor(0.8f, 0.8f, 0.85f));
    ReactionText = AddText(WidgetTree, Root, TEXT("AI反応: -"), 12, FLinearColor(0.35f, 0.9f, 1.0f));
    ActionStatusText = AddText(WidgetTree, Root, TEXT("行動可能"), 12, FLinearColor(0.45f, 1.0f, 0.55f));
    DebugText = AddText(WidgetTree, Root, TEXT(""), 12, FLinearColor(1.0f, 0.45f, 0.3f));
    DebugText->SetVisibility(ESlateVisibility::Collapsed);
}

void UStageDHudWidget::NativeConstruct()
{
    Super::NativeConstruct();
}

void UStageDHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!GetGameInstance()) return;
    const auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;
    const FStageDWorldView World = Subsystem->GetWorldView();
    const FStageFRuntimeView StageF = Subsystem->GetStageFRuntimeView();
    const FStageDNpcView Npc = Subsystem->GetNpcView(World.TargetNpcId);
    WorldText->SetText(FText::FromString(FString::Printf(
        TEXT("現在地: %s | 治安 %d | 犯罪 %d | tick %lld | 未処理 %d | オフライン +%lld秒"),
        *World.CurrentLocationId, World.Security, World.CrimeLevel, World.SimulationTick,
        World.PendingEventCount, World.OfflineRealSecondsApplied)));
    TargetText->SetText(FText::FromString(World.TargetNpcId.IsEmpty()
        ? TEXT("対象: なし | NPCへ近づいてTab")
        : FString::Printf(TEXT("対象: %s | %s | Tabで次へ"), *World.TargetNpcId, *World.TargetRole)));
    DialogueText->SetText(FText::FromString(TEXT("会話: ") + (World.Dialogue.IsEmpty() ? TEXT("-（1キーで会話）") : World.Dialogue)));
    EventText->SetText(FText::FromString(TEXT("直近イベント: ") + World.RecentEvent));
    TArray<FString> Reactions;
    for (const FStageDNpcView& View : Subsystem->GetAllNpcViews())
    {
        const bool bRequiredReaction =
            View.SelectedAction == TEXT("REPORT") ||
            View.SelectedAction == TEXT("WARN") ||
            View.SelectedAction == TEXT("REFUSE_TRADE") ||
            View.SelectedAction == TEXT("FLEE");
        if (!bRequiredReaction) continue;
        Reactions.Add(View.SelectedTargetId.IsEmpty()
            ? FString::Printf(TEXT("%s %s"), *View.NpcId, *View.SelectedAction)
            : FString::Printf(TEXT("%s -> %s %s"), *View.NpcId, *View.SelectedTargetId, *View.SelectedAction));
    }
    Reactions.Sort();
    ReactionText->SetText(FText::FromString(Reactions.IsEmpty()
        ? TEXT("AI反応: -")
        : TEXT("AI反応: ") + FString::Join(Reactions, TEXT(" | "))));
    ActionStatusText->SetText(FText::FromString(World.ActionBlockedReason.IsEmpty()
        ? TEXT("行動可能")
        : TEXT("行動不可: ") + World.ActionBlockedReason));
    const FString TimedTransition = Npc.NextTimedTransitionAt < 0
        ? TEXT("なし")
        : FString::Printf(TEXT("game minute %lld"), Npc.NextTimedTransitionAt);
    DebugText->SetText(FText::FromString(FString::Printf(
        TEXT("状態: %d / %s | 滞在 %lld分 | 次の時間遷移: %s\n")
        TEXT("目標: %s | player_evaluation=%d\n")
        TEXT("主要関係値: %s\n")
        TEXT("証拠評価: %s\n")
        TEXT("候補規則: %s\n")
        TEXT("採用規則: %s | 遷移理由: %s\n")
        TEXT("不採用理由: %s\n")
        TEXT("選択行動: %s | root_event_id=%s\n")
        TEXT("Stage F: countries=%d | AI=%d | ACTIVE=%d | BACKGROUND=%d | DORMANT=%d\n")
        TEXT("NON AI: materialized=%d | promoted=%d | pending=%d | due=%d\n")
        TEXT("save generation=%lld | loaded shards=%d | cache=%d\n")
        TEXT("offline=%lld秒 | save=%lldms | load=%lldms | data=%s"),
        Npc.CurrentStateId, *Npc.CurrentStateName, Npc.StateResidenceMinutes, *TimedTransition,
        *Npc.CurrentGoal, Npc.PlayerEvaluation,
        Npc.MajorRelationships.IsEmpty() ? TEXT("-") : *Npc.MajorRelationships,
        *Npc.EvidenceEvaluation,
        Npc.CandidateRules.IsEmpty() ? TEXT("-") : *Npc.CandidateRules,
        Npc.SelectedRule.IsEmpty() ? TEXT("-") : *Npc.SelectedRule,
        Npc.LastTransitionReason.IsEmpty() ? TEXT("-") : *Npc.LastTransitionReason,
        Npc.RejectedReasons.IsEmpty() ? TEXT("-") : *Npc.RejectedReasons,
        Npc.SelectedAction.IsEmpty() ? TEXT("-") : *Npc.SelectedAction,
        Npc.RootEventId.IsEmpty() ? TEXT("-") : *Npc.RootEventId,
        StageF.LoadedCountryCount, StageF.AiNpcTotalCount, StageF.ActiveCount, StageF.BackgroundCount, StageF.DormantCount,
        StageF.MaterializedNonAiCount, StageF.PromotedNonAiCount, StageF.PendingEventCount, StageF.NextDueCount,
        StageF.CurrentSaveGeneration, StageF.LoadedStateShardCount, StageF.StateCacheSize,
        StageF.LastOfflineDurationSeconds, StageF.LastSaveDurationMilliseconds, StageF.LastLoadDurationMilliseconds,
        StageF.DatasetSha256.IsEmpty() ? TEXT("-") : *StageF.DatasetSha256)));
}

void UStageDHudWidget::Submit(const FString& Action)
{
    if (!GetOwningPlayerPawn()) return;
    if (auto* Character = Cast<AStageDPlayerCharacter>(GetOwningPlayerPawn())) Character->ExecuteCoreAction(Action);
}

void UStageDHudWidget::OnTalk() { Submit(TEXT("TALK")); }
void UStageDHudWidget::OnHelp() { Submit(TEXT("HELP")); }
void UStageDHudWidget::OnHarm() { Submit(TEXT("HARM")); }
void UStageDHudWidget::OnTrade() { Submit(TEXT("TRADE")); }
void UStageDHudWidget::OnSteal() { Submit(TEXT("STEAL")); }
void UStageDHudWidget::OnWait() { Submit(TEXT("WAIT")); }
void UStageDHudWidget::OnMove() { if (auto* Character = Cast<AStageDPlayerCharacter>(GetOwningPlayerPawn())) Character->MoveToNextLocation(); }
void UStageDHudWidget::OnSave() { if (GetGameInstance()) GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>()->SaveMidChain(); }
void UStageDHudWidget::OnLoad() { if (GetGameInstance()) GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>()->ReloadSave(); }
void UStageDHudWidget::OnDebug() { ToggleDebug(); }

void UStageDHudWidget::ToggleDebug()
{
    bDebugVisible = !bDebugVisible;
    if (DebugText) DebugText->SetVisibility(bDebugVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
