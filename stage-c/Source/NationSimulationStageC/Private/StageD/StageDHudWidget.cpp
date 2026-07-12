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
#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDPlayerCharacter.h"
#include "StageD/StageDGameMode.h"
#include "StageG0/StageG0Types.h"
#include "StageG0/StageGDirectionalFlipbookComponent.h"
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

FString CoreActionJapanese(const FString& Action)
{
    if (Action == TEXT("REPORT")) return TEXT("報告");
    if (Action == TEXT("WARN")) return TEXT("警告");
    if (Action == TEXT("REFUSE_TRADE")) return TEXT("取引拒否");
    if (Action == TEXT("FLEE")) return TEXT("逃走");
    if (Action == TEXT("ARREST")) return TEXT("拘束");
    if (Action == TEXT("TALK")) return TEXT("会話");
    if (Action == TEXT("WAIT")) return TEXT("待機");
    return Action.IsEmpty() ? TEXT("-") : Action;
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
    const bool bStageG0 = GetWorld() && GetWorld()->GetMapName().Contains(TEXT("StageG0_VisualPoC"));
    AddText(WidgetTree, Root, bStageG0 ? TEXT("Stage G-0 | 2.5D描画検証") : TEXT("STAGE F | 本番規模ランタイム基盤"),
        16, FLinearColor(0.2f, 0.85f, 1.0f));
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
    UStageGDirectionalFlipbookComponent* Visual = nullptr;
    if (!World.TargetNpcId.IsEmpty())
    {
        for (TActorIterator<AStageDNpcActor> It(GetWorld()); It; ++It)
            if (It->GetNpcId() == World.TargetNpcId) { Visual = It->GetStageGVisual(); break; }
    }
    if (!Visual)
        if (const auto* Player = Cast<AStageDPlayerCharacter>(GetOwningPlayerPawn())) Visual = Player->GetStageGVisual();
    FStageGVisualDebugView StageG;
    if (Visual)
        if (const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0))
            StageG = Visual->GetDebugView(Camera->GetCameraLocation(), Camera->GetCameraRotation());
    WorldText->SetText(FText::FromString(FString::Printf(
        TEXT("現在地: %s | 治安 %d | 犯罪 %d | tick %lld | 未処理 %d | オフライン +%lld秒"),
        *World.CurrentLocationId, World.Security, World.CrimeLevel, World.SimulationTick,
        World.PendingEventCount, World.OfflineRealSecondsApplied)));
    const AStageDGameMode* VisualGameMode = GetWorld()->GetAuthGameMode<AStageDGameMode>();
    const FString VisualFixtureTarget = VisualGameMode ? VisualGameMode->GetStageG0SelectedVisualTarget() : TEXT("");
    const AStageDPlayerCharacter* StageG0Player = Cast<AStageDPlayerCharacter>(GetOwningPlayerPawn());
    const FStageG0ClickDebugView* Click = StageG0Player ? &StageG0Player->GetStageG0ClickDebugView() : nullptr;
    TargetText->SetText(FText::FromString(Click && !Click->Target.TargetId.IsEmpty()
        ? FString::Printf(TEXT("選択対象: %s | 種別: %s | %s"),
            *Click->Target.DisplayNameJa, *Click->Target.TargetType,
            Click->bTargetActionPossible ? TEXT("行動可能") : *FString::Printf(TEXT("行動不可: %s"), *Click->TargetBlockedReason))
        : !VisualFixtureTarget.IsEmpty()
        ? FString::Printf(TEXT("描画対象: %s | 非因果fixture・右パネルで動作選択"), *VisualFixtureTarget)
        : World.TargetNpcId.IsEmpty()
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
            ? FString::Printf(TEXT("%s %s"), *View.NpcId, *CoreActionJapanese(View.SelectedAction))
            : FString::Printf(TEXT("%s -> %s %s"), *View.NpcId, *View.SelectedTargetId, *CoreActionJapanese(View.SelectedAction)));
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
    const FString VisualActionJapanese = Visual ? StageGVisualActionJapanese(Visual->GetVisualAction()) : TEXT("-");
    const FString VisualDirectionJapanese = Visual ? StageGVisualDirectionJapanese(Visual->GetVisualDirection()) : TEXT("-");
    const AStageDGameMode* StageG0GameMode = GetWorld()->GetAuthGameMode<AStageDGameMode>();
    const int32 RecoveryCount = StageG0GameMode ? StageG0GameMode->GetStageG0RecoveryCount() : 0;
    const FString RecoveryMessage = StageG0GameMode && !StageG0GameMode->GetStageG0RecoveryMessage().IsEmpty()
        ? StageG0GameMode->GetStageG0RecoveryMessage() : TEXT("なし");
    DebugText->SetText(FText::FromString(FString::Printf(
        TEXT("状態: %d / %s | 滞在 %lld分 | 次の時間遷移: %s\n")
        TEXT("目標: %s | プレイヤー評価(player_evaluation)=%d\n")
        TEXT("主要関係値: %s\n")
        TEXT("証拠評価: %s\n")
        TEXT("候補規則: %s\n")
        TEXT("採用規則: %s | 遷移理由: %s\n")
        TEXT("不採用理由: %s\n")
        TEXT("選択行動: %s | ルートイベントID(root_event_id)=%s\n")
        TEXT("Stage F: 国家=%d | AI=%d | 活動中=%d | 背景=%d | 休眠=%d\n")
        TEXT("NON AI: 実体化=%d | 昇格=%d | 未処理=%d | 期限到来=%d\n")
        TEXT("保存世代=%lld | 読込shard=%d | cache=%d\n")
        TEXT("オフライン=%lld秒 | 保存=%lldms | 読込=%lldms | data=%s\n")
        TEXT("Stage G-0: 表示対象ID(visual_actor_id)=%s | 表示素材ID(visual_asset_id)=%s\n")
        TEXT("表示動作(visual_action)=%s (%s) | 表示方向(visual_direction)=%s (%s)\n")
        TEXT("Flipbook名=%s | frame=%d | 再生速度=%.2f\n")
        TEXT("仮素材(is_placeholder)=%s | sprite=%s | capsule=%s\n")
        TEXT("camera yaw=%.1f | pitch=%.1f | 距離=%.1f | 遮蔽状態=%s | 切替回数=%d\n")
        TEXT("落下復帰回数=%d | 復帰監査=%s"),
        Npc.CurrentStateId, *Npc.CurrentStateName, Npc.StateResidenceMinutes, *TimedTransition,
        *Npc.CurrentGoal, Npc.PlayerEvaluation,
        Npc.MajorRelationships.IsEmpty() ? TEXT("-") : *Npc.MajorRelationships,
        *Npc.EvidenceEvaluation,
        Npc.CandidateRules.IsEmpty() ? TEXT("-") : *Npc.CandidateRules,
        Npc.SelectedRule.IsEmpty() ? TEXT("-") : *Npc.SelectedRule,
        Npc.LastTransitionReason.IsEmpty() ? TEXT("-") : *Npc.LastTransitionReason,
        Npc.RejectedReasons.IsEmpty() ? TEXT("-") : *Npc.RejectedReasons,
        *CoreActionJapanese(Npc.SelectedAction),
        Npc.RootEventId.IsEmpty() ? TEXT("-") : *Npc.RootEventId,
        StageF.LoadedCountryCount, StageF.AiNpcTotalCount, StageF.ActiveCount, StageF.BackgroundCount, StageF.DormantCount,
        StageF.MaterializedNonAiCount, StageF.PromotedNonAiCount, StageF.PendingEventCount, StageF.NextDueCount,
        StageF.CurrentSaveGeneration, StageF.LoadedStateShardCount, StageF.StateCacheSize,
        StageF.LastOfflineDurationSeconds, StageF.LastSaveDurationMilliseconds, StageF.LastLoadDurationMilliseconds,
        StageF.DatasetSha256.IsEmpty() ? TEXT("-") : *StageF.DatasetSha256,
        StageG.VisualActorId.IsEmpty() ? TEXT("-") : *StageG.VisualActorId,
        StageG.VisualAssetId.IsEmpty() ? TEXT("-") : *StageG.VisualAssetId,
        *VisualActionJapanese, StageG.VisualAction.IsEmpty() ? TEXT("-") : *StageG.VisualAction,
        *VisualDirectionJapanese, StageG.VisualDirection.IsEmpty() ? TEXT("-") : *StageG.VisualDirection,
        StageG.FlipbookName.IsEmpty() ? TEXT("-") : *StageG.FlipbookName,
        StageG.FlipbookFrame, StageG.PlayRate, StageG.bIsPlaceholder ? TEXT("true") : TEXT("false"),
        *StageG.SpriteWorldLocation.ToCompactString(), *StageG.CapsuleWorldLocation.ToCompactString(),
        StageG.CameraYaw, StageG.CameraPitch, StageG.DistanceToCamera,
        StageG.OcclusionState.IsEmpty() ? TEXT("-") : *StageG.OcclusionState,
        StageG.FlipbookSwitchCount, RecoveryCount, *RecoveryMessage)));
    if (Click)
    {
        const FString ClickDetails = FString::Printf(
            TEXT("\nクリック地点=%s | 地面判定=%s | NavMesh投影地点=%s\n")
            TEXT("現在の移動先=%s | 経路状態=%s | 経路点数=%d | %s\n")
            TEXT("左クリック保持中=%s | 移動先更新回数=%d | 移動不可理由=%s\n")
            TEXT("対象ID=%s | 対象種別=%s | 表示名=%s\n")
            TEXT("選択可能=%s | 行動可能=%s | 距離=%.1f | 同一地点=%s | 行動不可理由=%s"),
            *Click->ClickLocation.ToCompactString(), *Click->GroundResult,
            *Click->NavigationLocation.ToCompactString(), *Click->CurrentDestination.ToCompactString(),
            *Click->PathStatus, Click->PathPointCount, Click->bMoving ? TEXT("移動中") : TEXT("停止中"),
            Click->bLeftHeld ? TEXT("はい") : TEXT("いいえ"), Click->DestinationUpdateCount,
            Click->InvalidReason.IsEmpty() ? TEXT("なし") : *Click->InvalidReason,
            Click->Target.TargetId.IsEmpty() ? TEXT("なし") : *Click->Target.TargetId,
            Click->Target.TargetType.IsEmpty() ? TEXT("なし") : *Click->Target.TargetType,
            Click->Target.DisplayNameJa.IsEmpty() ? TEXT("なし") : *Click->Target.DisplayNameJa,
            Click->Target.bIsTargetable ? TEXT("はい") : TEXT("いいえ"),
            Click->bTargetActionPossible ? TEXT("はい") : TEXT("いいえ"), Click->TargetDistance,
            Click->bSameLocation ? TEXT("はい") : TEXT("いいえ"),
            Click->TargetBlockedReason.IsEmpty() ? TEXT("なし") : *Click->TargetBlockedReason);
        DebugText->SetText(FText::FromString(DebugText->GetText().ToString() + ClickDetails));
    }
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
    SetDesiredSizeInViewport(bDebugVisible ? FVector2D(900.0f, 650.0f) : FVector2D(740.0f, 285.0f));
}
