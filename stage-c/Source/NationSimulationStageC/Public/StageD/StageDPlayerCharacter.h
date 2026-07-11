#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "StageDPlayerCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UStaticMeshComponent;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageDPlayerCharacter final : public ACharacter
{
    GENERATED_BODY()

public:
    AStageDPlayerCharacter();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    void ExecuteCoreAction(const FString& Action);
    void MoveToNextLocation();
    void CycleTarget();

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> FollowCamera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PlaceholderBody;
    float TargetScanAccumulator = 0.0f;
    float TargetLostSeconds = 0.0f;
    bool bManualTargetSelection = false;

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Turn(float Value);
    void LookUp(float Value);
    void Talk();
    void Help();
    void Harm();
    void Trade();
    void Steal();
    void WaitAction();
    void SaveSimulation();
    void LoadSimulation();
    void ToggleDebug();
    void ScanTarget();
};
