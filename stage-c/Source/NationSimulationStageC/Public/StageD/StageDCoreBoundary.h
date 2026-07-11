#pragma once

#include "CoreMinimal.h"
#include "nation_sim/simulation.hpp"

struct FStageDInteractionCheck
{
    bool bTargetExists = false;
    bool bSameLocation = false;
    bool bTargetActive = false;
    float Distance = TNumericLimits<float>::Max();
    float InteractionRange = 350.0f;
};

class NATIONSIMULATIONSTAGEC_API FStageDCoreBoundary final
{
public:
    static FString ResolveTalkDialogue(const nation_sim::Simulation& Simulation, const FString& NpcId);
    static bool HasPendingMoveTo(const nation_sim::Simulation& Simulation, const FString& DestinationId);
    static bool ShouldEnqueueMove(const nation_sim::Simulation& Simulation, const FString& DestinationId,
        FString& OutReason);
    static bool ValidateInteraction(const FStageDInteractionCheck& Check, FString& OutReason);
};
