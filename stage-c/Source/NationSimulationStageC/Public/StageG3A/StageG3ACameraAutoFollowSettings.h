#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StageG3ACameraAutoFollowSettings.generated.h"

/** G3A-only Standard camera auto-follow tuning. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Stage G-3A Standard Camera Auto Follow"))
class NATIONSIMULATIONSTAGEC_API UStageG3ACameraAutoFollowSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Auto Follow", meta=(ClampMin="0.0"))
    float YawInterpSpeed = 5.0f;

    UPROPERTY(Config, EditAnywhere, Category="Auto Follow", meta=(ClampMin="0.0"))
    float MovementSpeedThreshold = 10.0f;

    UPROPERTY(Config, EditAnywhere, Category="Auto Follow", meta=(ClampMin="0.0"))
    float DestinationDirectionMinDistance = 25.0f;

    UPROPERTY(Config, EditAnywhere, Category="Manual Override", meta=(ClampMin="1.0", ClampMax="1.5"))
    float ManualOverrideSeconds = 1.25f;
};
