#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StageG2ACameraModeSettings.generated.h"

UENUM(BlueprintType)
enum class EStageG2ACameraMode : uint8
{
    StandardCharacterCamera,
    TacticalOverlookCamera
};

/** Single source of truth for the redesigned Stage G-2A camera modes. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Stage G-2A Camera Redesign"))
class NATIONSIMULATIONSTAGEC_API UStageG2ACameraModeSettings final : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardDefaultDistance = 520.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardMinDistance = 320.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardMaxDistance = 850.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardDefaultPitch = -15.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardMinPitch = -30.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardMaxPitch = 8.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardFov = 75.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardCenterZ = 115.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardProbeSize = 12.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardCollisionMinDistance = 180.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardCollisionRecoverySeconds = 0.20f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardCameraLagSpeed = 14.0f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardYawSensitivity = 0.50f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardPitchSensitivity = 0.15f;
    UPROPERTY(Config, EditAnywhere, Category="Standard") float StandardZoomStep = 60.0f;

    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalDefaultDistance = 1200.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalMinDistance = 800.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalMaxDistance = 2000.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalDefaultPitch = -55.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalMinPitch = -65.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalMaxPitch = -45.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalFov = 60.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalCenterZ = 120.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalProbeSize = 16.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalCollisionMinDistance = 400.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalCollisionRecoverySeconds = 0.30f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalCameraLagSpeed = 12.0f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalYawSensitivity = 1.25f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalPitchSensitivity = 0.18f;
    UPROPERTY(Config, EditAnywhere, Category="Tactical") float TacticalZoomStep = 100.0f;

    UPROPERTY(Config, EditAnywhere, Category="Interpolation") float RotationReleaseDampingSpeed = 12.0f;
    UPROPERTY(Config, EditAnywhere, Category="Interpolation") float ZoomInterpSpeed = 10.0f;
};
