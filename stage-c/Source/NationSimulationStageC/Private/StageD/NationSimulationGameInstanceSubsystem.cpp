#include "StageD/NationSimulationGameInstanceSubsystem.h"

#include "HAL/PlatformFileManager.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "StageD/StageDCoreBoundary.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDSaveMetadata.h"
#include "StageE/StageESaveMigration.h"
#include "nation_sim/simulation.hpp"

#include <filesystem>

DEFINE_LOG_CATEGORY_STATIC(LogNationSimulationStageD, Log, All);

namespace
{
std::filesystem::path NativePath(const FString& Path)
{
    return std::filesystem::path(*Path);
}

FString ToFString(const std::string& Value)
{
    return UTF8_TO_TCHAR(Value.c_str());
}
}

struct UNationSimulationGameInstanceSubsystem::FCoreStorage
{
    TUniquePtr<nation_sim::Simulation> Simulation;
};

void UNationSimulationGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Core = new FCoreStorage();
    LoadCore(true);
}

void UNationSimulationGameInstanceSubsystem::Deinitialize()
{
    delete Core;
    Core = nullptr;
    Super::Deinitialize();
}

FString UNationSimulationGameInstanceSubsystem::ResolveFixturePath() const
{
    const TArray<FString> Candidates = {
        FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Data/stage_a_fixture.json")),
        FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("../data/stage_a_fixture.json")))
    };
    for (const FString& Candidate : Candidates)
    {
        if (FPaths::FileExists(Candidate)) return FPaths::ConvertRelativePathToFull(Candidate);
    }
    return Candidates.Last();
}

FString UNationSimulationGameInstanceSubsystem::ResolveStageEOverlayPath() const
{
    const TArray<FString> Candidates = {
        FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("Data/StageE/stage_e_state_definitions.json")),
        FPaths::ConvertRelativePathToFull(FPaths::Combine(
            FPaths::ProjectDir(), TEXT("Data/StageE/stage_e_state_definitions.json")))
    };
    for (const FString& Candidate : Candidates)
    {
        if (FPaths::FileExists(Candidate)) return FPaths::ConvertRelativePathToFull(Candidate);
    }
    return Candidates.Last();
}

FString UNationSimulationGameInstanceSubsystem::SavePath() const
{
    return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames/stage_d_save.json"));
}

FString UNationSimulationGameInstanceSubsystem::SaveMetadataPath() const
{
    return SavePath() + TEXT(".utc");
}

bool UNationSimulationGameInstanceSubsystem::LoadCore(bool bApplyOfflineElapsed)
{
    try
    {
        LastError.Reset();
        LastActionRejection.Reset();
        DeferredOfflineSeconds = 0;
        const FString Fixture = ResolveFixturePath();
        const FString Overlay = ResolveStageEOverlayPath();
        const FString Save = SavePath();
        FString DefinitionSha256;
        FString HashError;
        if (!FStageESaveMigration::Sha256File(Overlay, DefinitionSha256, HashError))
        {
            RecordErrorMessage(TEXT("LoadCore Stage E overlay"), HashError);
            return false;
        }
        if (FPaths::FileExists(Save))
        {
            const FStageESaveMigrationResult Migration = FStageESaveMigration::MigrateIfNeeded(
                Save, SaveMetadataPath(), Fixture, Overlay);
            if (!Migration.bSuccess)
            {
                RecordErrorMessage(TEXT("LoadCore Stage D to Stage E migration"), Migration.Error);
                return false;
            }
            Core->Simulation = MakeUnique<nation_sim::Simulation>(
                nation_sim::Simulation::load_save_with_overlay(
                    NativePath(Fixture), NativePath(Overlay), NativePath(Save), TCHAR_TO_UTF8(*DefinitionSha256)));
            if (Migration.bMigrated)
            {
                UE_LOG(LogNationSimulationStageD, Display,
                    TEXT("STAGE_E_SAVE_MIGRATED save=%s backup=%s metadata_backup=%s source_sha256=%s"),
                    *Save, *Migration.SaveBackupPath, *Migration.MetadataBackupPath, *Migration.SourceSaveSha256);
            }
            if (bApplyOfflineElapsed)
            {
                const FStageDSaveMetadataReadResult Metadata =
                    FStageDSaveMetadata::ReadValidated(Save, SaveMetadataPath());
                if (Metadata.bValid)
                {
                    DeferredOfflineSeconds = FMath::Max<int64>(
                        0, FDateTime::UtcNow().ToUnixTimestamp() - Metadata.SavedAtUtcEpoch);
                }
                else
                {
                    LastError = TEXT("Offline time not applied: ") + Metadata.Error;
                    UE_LOG(LogNationSimulationStageD, Error,
                        TEXT("STAGE_D_SAVE_METADATA_INVALID save=%s metadata=%s reason=%s"),
                        *Save, *SaveMetadataPath(), *Metadata.Error);
                }
            }
        }
        else
        {
            Core->Simulation = MakeUnique<nation_sim::Simulation>(
                nation_sim::Simulation::from_fixture_with_overlay(
                    NativePath(Fixture), NativePath(Overlay), TCHAR_TO_UTF8(*DefinitionSha256)));
        }
        RecentEvent = TEXT("Stage E causal core ready");
        UE_LOG(LogNationSimulationStageD, Display,
            TEXT("STAGE_E_CORE_READY overlay=stage_e_behavioral_depth_v1 definition_sha256=%s save=%s"),
            *DefinitionSha256, *Save);
        OnCoreStateChanged.Broadcast();
        return true;
    }
    catch (const std::exception& Exception)
    {
        RecordError(TEXT("LoadCore"), Exception);
        return false;
    }
}

bool UNationSimulationGameInstanceSubsystem::SubmitPlayerAction(
    const FString& Action, const FString& InTargetNpcId, const FString& DestinationId)
{
    if (!Core || !Core->Simulation) return false;
    if (Action == TEXT("HELP") || Action == TEXT("HARM") || Action == TEXT("TALK") ||
        Action == TEXT("TRADE") || Action == TEXT("STEAL"))
    {
        FString Reason;
        if (!ValidateInteractionTarget(InTargetNpcId, Reason))
        {
            RejectAction(Action, InTargetNpcId, Reason);
            return false;
        }
        if (Action == TEXT("TALK") && FStageDCoreBoundary::ResolveTalkDialogue(*Core->Simulation, InTargetNpcId).IsEmpty())
        {
            RejectAction(Action, InTargetNpcId, TEXT("会話データが定義されていません"));
            return false;
        }
    }
    try
    {
        Core->Simulation->enqueue_player_action(
            TCHAR_TO_UTF8(*Action), TCHAR_TO_UTF8(*InTargetNpcId), TCHAR_TO_UTF8(*DestinationId));
        RecentEvent = FString::Printf(TEXT("Queued %s target=%s destination=%s"), *Action, *InTargetNpcId, *DestinationId);
        if (Action == TEXT("TALK"))
        {
            const FString Dialogue = FStageDCoreBoundary::ResolveTalkDialogue(*Core->Simulation, InTargetNpcId);
            LastDialogue = FString::Printf(TEXT("%s: %s"), *InTargetNpcId, *Dialogue);
        }
        LastError.Reset();
        LastActionRejection.Reset();
        OnCoreStateChanged.Broadcast();
        return true;
    }
    catch (const std::exception& Exception)
    {
        RecordError(Action, Exception);
        return false;
    }
}

bool UNationSimulationGameInstanceSubsystem::EnterLocation(const FString& LocationId)
{
    if (!Core || !Core->Simulation || LocationId.IsEmpty()) return false;
    FString Reason;
    if (!FStageDCoreBoundary::ShouldEnqueueMove(*Core->Simulation, LocationId, Reason))
    {
        if (FStageDCoreBoundary::HasPendingMoveTo(*Core->Simulation, LocationId) ||
            ToFString(Core->Simulation->player().current_location_id) == LocationId)
        {
            RecentEvent = TEXT("MOVE_DUPLICATE_SUPPRESSED | ") + Reason;
            UE_LOG(LogNationSimulationStageD, Display,
                TEXT("STAGE_D_MOVE_SUPPRESSED player=%s destination=%s reason=%s"),
                *ToFString(Core->Simulation->player().player_id), *LocationId, *Reason);
            OnCoreStateChanged.Broadcast();
            return true;
        }
        RejectAction(TEXT("MOVE"), TEXT(""), Reason);
        return false;
    }
    ClearTargetNpc(TEXT("Target cleared because player location changed"));
    return SubmitPlayerAction(TEXT("MOVE"), TEXT(""), LocationId);
}

bool UNationSimulationGameInstanceSubsystem::SaveMidChain()
{
    if (!Core || !Core->Simulation) return false;
    try
    {
        Core->Simulation->save(NativePath(SavePath()));
        FString MetadataError;
        if (!FStageDSaveMetadata::WriteVerified(
            SavePath(), SaveMetadataPath(), FDateTime::UtcNow().ToUnixTimestamp(), MetadataError))
        {
            RecordErrorMessage(TEXT("SaveMidChain metadata"), MetadataError);
            return false;
        }
        RecentEvent = FString::Printf(TEXT("Saved with %d pending event(s)"), static_cast<int32>(Core->Simulation->pending_events().size()));
        LastError.Reset();
        OnCoreStateChanged.Broadcast();
        return true;
    }
    catch (const std::exception& Exception)
    {
        RecordError(TEXT("SaveMidChain"), Exception);
        return false;
    }
}

bool UNationSimulationGameInstanceSubsystem::ReloadSave()
{
    DeferredOfflineSeconds = 0;
    OfflineSecondsApplied = 0;
    return LoadCore(false);
}

void UNationSimulationGameInstanceSubsystem::SetTargetNpc(const FString& NpcId)
{
    if (NpcId.IsEmpty())
    {
        ClearTargetNpc(TEXT(""));
        return;
    }
    FString Reason;
    if (!ValidateInteractionTarget(NpcId, Reason))
    {
        RejectAction(TEXT("TARGET"), NpcId, Reason);
        return;
    }
    if (TargetNpcId == NpcId) return;
    TargetNpcId = NpcId;
    LastActionRejection.Reset();
    LastDialogue.Reset();
    OnCoreStateChanged.Broadcast();
}

void UNationSimulationGameInstanceSubsystem::ClearTargetNpc(const FString& Reason)
{
    if (TargetNpcId.IsEmpty()) return;
    const FString ClearedNpc = TargetNpcId;
    TargetNpcId.Reset();
    LastDialogue.Reset();
    if (!Reason.IsEmpty())
    {
        LastActionRejection = Reason;
        RecentEvent = FString::Printf(TEXT("TARGET_CLEARED | npc=%s | %s"), *ClearedNpc, *Reason);
        UE_LOG(LogNationSimulationStageD, Display,
            TEXT("STAGE_D_TARGET_CLEARED npc=%s reason=%s"), *ClearedNpc, *Reason);
    }
    OnCoreStateChanged.Broadcast();
}

void UNationSimulationGameInstanceSubsystem::AdvancePresentation(float DeltaSeconds)
{
    if (!Core || !Core->Simulation) return;
    EventAccumulator += DeltaSeconds;
    if (EventAccumulator < 0.30f) return;
    EventAccumulator = 0.0f;
    RefreshTargetValidity();
    try
    {
        if (!Core->Simulation->pending_events().empty())
        {
            const int32 EventCount = static_cast<int32>(Core->Simulation->event_history().size());
            const int32 AuditCount = static_cast<int32>(Core->Simulation->audit_log().size());
            Core->Simulation->process_pending(1);
            RefreshProjection(EventCount, AuditCount);
            OnCoreStateChanged.Broadcast();
            return;
        }
        if (DeferredOfflineSeconds > 0)
        {
            const int32 EventCount = static_cast<int32>(Core->Simulation->event_history().size());
            const int32 AuditCount = static_cast<int32>(Core->Simulation->audit_log().size());
            const auto Result = Core->Simulation->advance_offline(DeferredOfflineSeconds);
            OfflineSecondsApplied = Result.applied_real_seconds;
            DeferredOfflineSeconds = 0;
            RefreshProjection(EventCount, AuditCount);
            OnCoreStateChanged.Broadcast();
        }
    }
    catch (const std::exception& Exception)
    {
        RecordError(TEXT("AdvancePresentation"), Exception);
    }
}

void UNationSimulationGameInstanceSubsystem::RefreshProjection(int32 PreviousEventCount, int32 PreviousAuditCount)
{
    const auto& Events = Core->Simulation->event_history();
    if (static_cast<int32>(Events.size()) > PreviousEventCount)
    {
        const auto& Event = Events.back();
        RecentEvent = FString::Printf(TEXT("%s | %s | actor=%s"),
            *ToFString(Event.event_id), *ToFString(Event.event_type), *ToFString(Event.actor_id));
    }
    const auto& Audits = Core->Simulation->audit_log();
    if (static_cast<int32>(Audits.size()) > PreviousAuditCount && !Audits.back().selected_dialogue.empty() &&
        (TargetNpcId.IsEmpty() || ToFString(Audits.back().decision_npc_id) == TargetNpcId))
    {
        const auto& Audit = Audits.back();
        const auto& Npc = Core->Simulation->ai_npc(Audit.decision_npc_id);
        if (Npc.current_state_id >= 1 && Npc.current_state_id <= static_cast<int>(Npc.states.size()))
        {
            const auto& State = Npc.states[static_cast<size_t>(Npc.current_state_id - 1)];
            const auto It = std::find_if(State.dialogue_candidates.begin(), State.dialogue_candidates.end(),
                [&](const nation_sim::DialogueCandidate& Dialogue) { return Dialogue.dialogue_id == Audit.selected_dialogue; });
            if (It != State.dialogue_candidates.end()) LastDialogue = ToFString(It->text);
        }
    }
}

FStageDWorldView UNationSimulationGameInstanceSubsystem::GetWorldView() const
{
    FStageDWorldView View;
    View.TargetNpcId = TargetNpcId;
    View.RecentEvent = LastError.IsEmpty() ? RecentEvent : LastError;
    View.Dialogue = LastDialogue;
    View.OfflineRealSecondsApplied = OfflineSecondsApplied;
    View.ActionBlockedReason = LastActionRejection;
    if (!Core || !Core->Simulation) return View;
    const auto& Simulation = *Core->Simulation;
    View.CurrentLocationId = ToFString(Simulation.player().current_location_id);
    View.Security = Simulation.country().security;
    View.CrimeLevel = Simulation.country().crime_level;
    View.SimulationTick = Simulation.simulation_tick();
    View.PendingEventCount = static_cast<int32>(Simulation.pending_events().size());
    View.TargetRole = GetNpcView(TargetNpcId).Role;
    return View;
}

FStageDNpcView UNationSimulationGameInstanceSubsystem::GetNpcView(const FString& NpcId) const
{
    FStageDNpcView View;
    View.NpcId = NpcId;
    if (!Core || !Core->Simulation || NpcId.IsEmpty()) return View;
    const std::string Id = TCHAR_TO_UTF8(*NpcId);
    const auto& Simulation = *Core->Simulation;
    const auto AiIt = std::find_if(Simulation.ai_npcs().begin(), Simulation.ai_npcs().end(),
        [&](const nation_sim::AiNpcState& Npc) { return Npc.npc_id == Id; });
    if (AiIt != Simulation.ai_npcs().end())
    {
        View.bIsAi = true;
        View.Role = ToFString(AiIt->role);
        View.LocationId = ToFString(AiIt->current_location_id);
        View.CurrentStateId = AiIt->current_state_id;
        if (AiIt->current_state_id >= 1 && AiIt->current_state_id <= static_cast<int>(AiIt->states.size()))
        {
            View.CurrentStateName = ToFString(AiIt->states[static_cast<size_t>(AiIt->current_state_id - 1)].state_name);
        }
        View.CurrentGoal = ToFString(AiIt->current_goal);
        View.PlayerEvaluation = AiIt->player_evaluation;
        View.LastTransitionReason = ToFString(AiIt->last_transition_reason);
        View.StateResidenceMinutes = FMath::Max<int64>(0, Simulation.current_world_time_minutes() - AiIt->state_entered_at);
        View.NextTimedTransitionAt = AiIt->timed_transition_at ? *AiIt->timed_transition_at : -1;
        TArray<FString> RelationshipRows;
        for (const auto& [TargetId, Metrics] : AiIt->relationships)
        {
            TArray<FString> MetricRows;
            for (const auto& [Metric, Value] : Metrics)
            {
                MetricRows.Add(FString::Printf(TEXT("%s=%d"), *ToFString(Metric), Value));
            }
            if (!MetricRows.IsEmpty())
            {
                RelationshipRows.Add(FString::Printf(TEXT("%s{%s}"), *ToFString(TargetId), *FString::Join(MetricRows, TEXT(", "))));
            }
        }
        View.MajorRelationships = FString::Join(RelationshipRows, TEXT(" | "));
        View.EvidenceEvaluation = FString::Printf(TEXT("source=%s | credibility=%.3f | evidence=%.3f | perception=%s"),
            *ToFString(AiIt->evidence_evaluation.source_event_id), AiIt->evidence_evaluation.credibility,
            AiIt->evidence_evaluation.evidence_level, *ToFString(AiIt->evidence_evaluation.perception));
        TArray<FString> CandidateRows;
        TArray<FString> RejectedRows;
        for (const nation_sim::RuleEvaluation& Evaluation : AiIt->last_rule_evaluations)
        {
            CandidateRows.Add(ToFString(Evaluation.rule_id));
            if (!Evaluation.selected)
            {
                RejectedRows.Add(FString::Printf(TEXT("%s: %s"), *ToFString(Evaluation.rule_id), *ToFString(Evaluation.reason)));
            }
        }
        View.CandidateRules = FString::Join(CandidateRows, TEXT(", "));
        View.RejectedReasons = FString::Join(RejectedRows, TEXT(" | "));
        for (auto It = Simulation.audit_log().rbegin(); It != Simulation.audit_log().rend(); ++It)
        {
            if (It->decision_npc_id != Id) continue;
            View.SelectedRule = ToFString(It->selected_rule);
            View.SelectedAction = ToFString(It->selected_action);
            View.SelectedTargetId = ToFString(It->target_id);
            View.RootEventId = ToFString(It->root_event_id);
            if (View.RejectedReasons.IsEmpty())
            {
                TArray<FString> RejectedAuditRows;
                for (const std::string& Rejected : It->rejected_rules) RejectedAuditRows.Add(ToFString(Rejected));
                View.RejectedReasons = FString::Join(RejectedAuditRows, TEXT(" | "));
            }
            if (!It->selected_dialogue.empty() && AiIt->current_state_id >= 1 && AiIt->current_state_id <= static_cast<int>(AiIt->states.size()))
            {
                const auto& State = AiIt->states[static_cast<size_t>(AiIt->current_state_id - 1)];
                const auto DialogueIt = std::find_if(State.dialogue_candidates.begin(), State.dialogue_candidates.end(),
                    [&](const nation_sim::DialogueCandidate& Dialogue) { return Dialogue.dialogue_id == It->selected_dialogue; });
                if (DialogueIt != State.dialogue_candidates.end()) View.Dialogue = ToFString(DialogueIt->text);
            }
            break;
        }
        return View;
    }
    const auto NonAiIt = std::find_if(Simulation.non_ai_npcs().begin(), Simulation.non_ai_npcs().end(),
        [&](const nation_sim::NonAiNpcState& Npc) { return Npc.npc_id == Id; });
    if (NonAiIt != Simulation.non_ai_npcs().end())
    {
        View.Role = ToFString(NonAiIt->occupation);
        View.LocationId = ToFString(NonAiIt->current_location_id);
        View.SelectedAction = ToFString(NonAiIt->current_activity);
    }
    return View;
}

TArray<FStageDNpcView> UNationSimulationGameInstanceSubsystem::GetAllNpcViews() const
{
    TArray<FStageDNpcView> Result;
    if (!Core || !Core->Simulation) return Result;
    Result.Reserve(static_cast<int32>(Core->Simulation->ai_npcs().size() + Core->Simulation->non_ai_npcs().size()));
    for (const auto& Npc : Core->Simulation->ai_npcs()) Result.Add(GetNpcView(ToFString(Npc.npc_id)));
    for (const auto& Npc : Core->Simulation->non_ai_npcs()) Result.Add(GetNpcView(ToFString(Npc.npc_id)));
    return Result;
}

bool UNationSimulationGameInstanceSubsystem::HasPendingEvents() const
{
    return Core && Core->Simulation && !Core->Simulation->pending_events().empty();
}

bool UNationSimulationGameInstanceSubsystem::ValidateInteractionTarget(
    const FString& NpcId, FString& OutReason) const
{
    FStageDInteractionCheck Check;
    Check.InteractionRange = InteractionRange;
    if (!Core || !Core->Simulation || NpcId.IsEmpty())
    {
        return FStageDCoreBoundary::ValidateInteraction(Check, OutReason);
    }

    const std::string Id = TCHAR_TO_UTF8(*NpcId);
    std::string NpcLocation;
    bool bCoreActive = false;
    const auto AiIt = std::find_if(Core->Simulation->ai_npcs().begin(), Core->Simulation->ai_npcs().end(),
        [&](const nation_sim::AiNpcState& Npc) { return Npc.npc_id == Id; });
    if (AiIt != Core->Simulation->ai_npcs().end())
    {
        Check.bTargetExists = true;
        NpcLocation = AiIt->current_location_id;
        bCoreActive = AiIt->status != "DESPAWNED" && AiIt->status != "DISABLED";
    }
    else
    {
        const auto NonAiIt = std::find_if(Core->Simulation->non_ai_npcs().begin(), Core->Simulation->non_ai_npcs().end(),
            [&](const nation_sim::NonAiNpcState& Npc) { return Npc.npc_id == Id; });
        if (NonAiIt != Core->Simulation->non_ai_npcs().end())
        {
            Check.bTargetExists = true;
            NpcLocation = NonAiIt->current_location_id;
            bCoreActive = true;
        }
    }
    Check.bSameLocation = Check.bTargetExists && NpcLocation == Core->Simulation->player().current_location_id;

    const AStageDNpcActor* TargetActor = nullptr;
    if (const UWorld* World = GetWorld())
    {
        for (TActorIterator<AStageDNpcActor> It(World); It; ++It)
        {
            if (It->GetNpcId() == NpcId)
            {
                TargetActor = *It;
                break;
            }
        }
    }
    Check.bTargetExists = Check.bTargetExists && IsValid(TargetActor);
    Check.bTargetActive = bCoreActive && IsValid(TargetActor) && !TargetActor->IsActorBeingDestroyed() && !TargetActor->IsHidden();
    const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (PlayerPawn && TargetActor)
    {
        Check.Distance = FVector::Distance(PlayerPawn->GetActorLocation(), TargetActor->GetActorLocation());
    }
    return FStageDCoreBoundary::ValidateInteraction(Check, OutReason);
}

void UNationSimulationGameInstanceSubsystem::RejectAction(
    const FString& Action, const FString& NpcId, const FString& Reason)
{
    LastActionRejection = Reason;
    RecentEvent = FString::Printf(TEXT("ACTION_REJECTED | %s | target=%s | %s"), *Action, *NpcId, *Reason);
    UE_LOG(LogNationSimulationStageD, Warning,
        TEXT("STAGE_D_ACTION_REJECTED action=%s target=%s reason=%s"), *Action, *NpcId, *Reason);
    if (!NpcId.IsEmpty() && TargetNpcId == NpcId) TargetNpcId.Reset();
    OnCoreStateChanged.Broadcast();
}

void UNationSimulationGameInstanceSubsystem::RefreshTargetValidity()
{
    if (TargetNpcId.IsEmpty()) return;
    FString Reason;
    if (!ValidateInteractionTarget(TargetNpcId, Reason)) ClearTargetNpc(Reason);
}

void UNationSimulationGameInstanceSubsystem::RecordError(const FString& Context, const std::exception& Exception)
{
    RecordErrorMessage(Context, UTF8_TO_TCHAR(Exception.what()));
}

void UNationSimulationGameInstanceSubsystem::RecordErrorMessage(const FString& Context, const FString& Message)
{
    LastError = FString::Printf(TEXT("%s failed: %s"), *Context, *Message);
    UE_LOG(LogNationSimulationStageD, Error, TEXT("%s"), *LastError);
    OnCoreStateChanged.Broadcast();
}
