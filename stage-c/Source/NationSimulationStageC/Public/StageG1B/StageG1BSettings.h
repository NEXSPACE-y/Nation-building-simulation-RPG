#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StageG1BSettings.generated.h"

/** Stage G-1B-only locomotion presentation speeds. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Stage G-1B Settings"))
class NATIONSIMULATIONSTAGEC_API UStageG1BSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Locomotion", meta=(ClampMin="1.0"))
    float WalkSpeed = 250.0f;

    UPROPERTY(Config, EditAnywhere, Category="Locomotion", meta=(ClampMin="1.0"))
    float RunSpeed = 500.0f;

    UPROPERTY(Config, EditAnywhere, Category="Locomotion", meta=(ClampMin="1.0"))
    float DashSpeed = 750.0f;

    UPROPERTY(Config, EditAnywhere, Category="Animation", meta=(ClampMin="0.1"))
    float AnimationMinPlayRate = 0.72f;

    UPROPERTY(Config, EditAnywhere, Category="Animation", meta=(ClampMin="0.1"))
    float AnimationMaxPlayRate = 1.18f;
};
