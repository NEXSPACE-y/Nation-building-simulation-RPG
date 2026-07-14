#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StageG1ASettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Stage G-1A Standard 3D Character PoC"))
class NATIONSIMULATIONSTAGEC_API UStageG1ASettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Character") float CharacterScale = 0.97f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float MeshOffsetZ = -90.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float MeshYaw = -90.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float WalkSpeed = 250.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float RunSpeed = 500.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float WalkStateThreshold = 8.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float RunStateThreshold = 330.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float RotationRateYaw = 420.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float AnimationNominalRunSpeed = 500.0f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float AnimationMinPlayRate = 0.72f;
    UPROPERTY(Config, EditAnywhere, Category="Character") float AnimationMaxPlayRate = 1.18f;

    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraYaw = 45.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraPitch = -50.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraArmLength = 1400.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraMinArmLength = 1050.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraMaxArmLength = 1750.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraZoomStep = 120.0f;
    UPROPERTY(Config, EditAnywhere, Category="Camera") float CameraFov = 30.0f;

    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float HoldUpdateSeconds = 0.075f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float DestinationMinUpdateDistance = 50.0f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float ArrivalTolerance = 45.0f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float StuckSeconds = 1.25f;
    UPROPERTY(Config, EditAnywhere, Category="ClickMove") float WorldBounds = 3400.0f;

    UPROPERTY(Config, EditAnywhere, Category="Safety") float FallRecoveryZ = -350.0f;
};
