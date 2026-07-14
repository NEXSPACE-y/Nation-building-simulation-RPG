#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1AGameMode.generated.h"

class UStageG1AHudWidget;
class AStageG1ANpcActor;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG1AGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    AStageG1AGameMode();
    virtual void BeginPlay() override;
    void ToggleDebugHud();

private:
    UPROPERTY() TObjectPtr<UStageG1AHudWidget> StageG1AHud;
    UPROPERTY() TObjectPtr<AStageG1ANpcActor> StandardNpc;
    FVector EvidenceStart = FVector::ZeroVector;
    FVector EvidenceDestination = FVector::ZeroVector;
    FString EvidenceSelectedTarget;
    int32 EvidencePathPointCount = 0;
    int32 EvidenceDestinationUpdateCount = 0;
    float WalkSampleSpeed = 0.0f;
    float WalkSamplePlayRate = 1.0f;
    float WalkSampleMaxSpeed = 0.0f;
    float RunSampleSpeed = 0.0f;
    float RunSamplePlayRate = 1.0f;
    float RunSampleMaxSpeed = 0.0f;
    float WalkReturnSpeed = 0.0f;
    float WalkReturnMaxSpeed = 0.0f;
    EStageG1AMovementBand WalkSampleBand = EStageG1AMovementBand::Idle;
    EStageG1AMovementBand RunSampleBand = EStageG1AMovementBand::Idle;
    bool bDefaultWalkObserved = false;
    bool bRunDestinationMaintained = false;
    bool bRunPathMaintained = false;
    bool bRunMoveMaintained = false;
    bool bWalkDestinationMaintained = false;
    bool bWalkPathMaintained = false;
    bool bWalkMoveMaintained = false;
    bool bToggleUpdateCountMaintained = false;
    bool bToggleTargetMaintained = false;

    void BuildTechnicalPlaza();
    void SpawnLighting();
    void BeginEvidenceMove();
    void CaptureWalkAndSwitchToRun();
    void CaptureRunAndSwitchToWalk();
    void WriteEvidence();
    void SpawnBlock(const FString& Name, const FVector& Location, const FVector& Dimensions,
        const FLinearColor& Color, const FRotator& Rotation = FRotator::ZeroRotator,
        bool bWalkableSurface = false);
};
