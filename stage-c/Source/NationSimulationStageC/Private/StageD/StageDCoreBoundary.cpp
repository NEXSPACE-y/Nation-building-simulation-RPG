#include "StageD/StageDCoreBoundary.h"

#include <algorithm>

FString FStageDCoreBoundary::ResolveTalkDialogue(
    const nation_sim::Simulation& Simulation, const FString& NpcId)
{
    const std::string Id = TCHAR_TO_UTF8(*NpcId);
    const auto AiIt = std::find_if(Simulation.ai_npcs().begin(), Simulation.ai_npcs().end(),
        [&](const nation_sim::AiNpcState& Npc) { return Npc.npc_id == Id; });
    if (AiIt != Simulation.ai_npcs().end())
    {
        const auto StateIt = std::find_if(AiIt->states.begin(), AiIt->states.end(),
            [&](const nation_sim::StateDefinition& State) { return State.state_id == AiIt->current_state_id; });
        if (StateIt == AiIt->states.end() || StateIt->undefined) return FString();
        const auto DialogueIt = std::max_element(
            StateIt->dialogue_candidates.begin(), StateIt->dialogue_candidates.end(),
            [](const nation_sim::DialogueCandidate& Left, const nation_sim::DialogueCandidate& Right)
            {
                if (Left.priority != Right.priority) return Left.priority < Right.priority;
                return Left.dialogue_id > Right.dialogue_id;
            });
        return DialogueIt == StateIt->dialogue_candidates.end()
            ? FString()
            : UTF8_TO_TCHAR(DialogueIt->text.c_str());
    }

    const auto NonAiIt = std::find_if(Simulation.non_ai_npcs().begin(), Simulation.non_ai_npcs().end(),
        [&](const nation_sim::NonAiNpcState& Npc) { return Npc.npc_id == Id; });
    return NonAiIt == Simulation.non_ai_npcs().end()
        ? FString()
        : UTF8_TO_TCHAR(NonAiIt->basic_profile.c_str());
}

bool FStageDCoreBoundary::HasPendingMoveTo(
    const nation_sim::Simulation& Simulation, const FString& DestinationId)
{
    const std::string Destination = TCHAR_TO_UTF8(*DestinationId);
    for (const nation_sim::Event& Event : Simulation.pending_events())
    {
        const auto DestinationIt = Event.payload.find("destination_id");
        if (Event.event_type == "PLAYER_LEFT_LOCATION" &&
            Event.actor_id == Simulation.player().player_id &&
            DestinationIt != Event.payload.end() && DestinationIt->second == Destination)
        {
            return true;
        }
    }
    return false;
}

bool FStageDCoreBoundary::ShouldEnqueueMove(const nation_sim::Simulation& Simulation,
    const FString& DestinationId, FString& OutReason)
{
    OutReason.Reset();
    if (DestinationId.IsEmpty())
    {
        OutReason = TEXT("MOVE destination is empty");
        return false;
    }
    if (HasPendingMoveTo(Simulation, DestinationId))
    {
        OutReason = FString::Printf(TEXT("MOVE to %s is already pending"), *DestinationId);
        return false;
    }
    if (UTF8_TO_TCHAR(Simulation.player().current_location_id.c_str()) == DestinationId)
    {
        OutReason = FString::Printf(TEXT("player is already at %s"), *DestinationId);
        return false;
    }
    return true;
}

bool FStageDCoreBoundary::ValidateInteraction(const FStageDInteractionCheck& Check, FString& OutReason)
{
    OutReason.Reset();
    if (!Check.bTargetExists)
    {
        OutReason = TEXT("Target NPC does not exist in the causal core or presentation world");
        return false;
    }
    if (!Check.bTargetActive)
    {
        OutReason = TEXT("Target NPC is inactive, despawned, or being destroyed");
        return false;
    }
    if (!Check.bSameLocation)
    {
        OutReason = TEXT("Target NPC is in a different location");
        return false;
    }
    if (Check.InteractionRange <= 0.0f)
    {
        OutReason = TEXT("Interaction range configuration must be positive");
        return false;
    }
    if (!FMath::IsFinite(Check.Distance) || Check.Distance > Check.InteractionRange)
    {
        OutReason = FString::Printf(TEXT("Target NPC is out of interaction range (%.1f > %.1f uu)"),
            Check.Distance, Check.InteractionRange);
        return false;
    }
    return true;
}
