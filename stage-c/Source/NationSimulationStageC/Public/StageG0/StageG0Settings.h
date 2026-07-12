#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StageG0Settings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Stage G-0 Visual PoC"))
class NATIONSIMULATIONSTAGEC_API UStageG0Settings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraYaw = 45.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraPitch = -55.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraArmLength = 1100.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraMinPitch = -70.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraMaxPitch = -35.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraMinArmLength = 700.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraMaxArmLength = 1500.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraOrbitSensitivity = 1.5f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraZoomStep = 120.0f;
    UPROPERTY(Config, EditAnywhere, Category="Safety") float FallRecoveryZ = -500.0f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float ClickHoldUpdateSeconds = 0.075f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float ClickDestinationMinUpdateDistance = 50.0f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float ClickArrivalTolerance = 45.0f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float ClickStuckSeconds = 1.0f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") bool bContinueToDestinationOnRelease = true;
    UPROPERTY(Config, EditAnywhere, Category="Targeting") bool bKeepTargetOnGroundClick = true;
    UPROPERTY(Config, EditAnywhere, Category="Direction") float MovementDirectionThreshold = 3.0f;
    UPROPERTY(Config, EditAnywhere, Category="Direction") float WalkSpeedThreshold = 8.0f;
    UPROPERTY(Config, EditAnywhere, Category="Animation") float NonLoopFallbackSeconds = 1.20f;
    UPROPERTY(Config, EditAnywhere, Category="Presentation") float HumanSpriteScale = 1.65f;
    UPROPERTY(Config, EditAnywhere, Category="Presentation") float MonsterSpriteScale = 1.15f;
};
