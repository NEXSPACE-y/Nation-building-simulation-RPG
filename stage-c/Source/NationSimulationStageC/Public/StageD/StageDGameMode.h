#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StageDGameMode.generated.h"

class UStageDHudWidget;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageDGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    AStageDGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    static FVector GetLocationCenter(const FString& LocationId);
    UStageDHudWidget* GetStageDHud() const { return StageDHud; }

private:
    UPROPERTY() TObjectPtr<UStageDHudWidget> StageDHud;
    void BuildCapitalBlock();
    void SpawnNpcPresentation();
    void SpawnBlock(const FVector& Location, const FVector& Scale, const FRotator& Rotation = FRotator::ZeroRotator);
};
