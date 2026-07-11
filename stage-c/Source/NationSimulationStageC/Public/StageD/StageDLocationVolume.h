#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StageDLocationVolume.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class UTextRenderComponent;

UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageDLocationVolume final : public AActor
{
    GENERATED_BODY()

public:
    AStageDLocationVolume();
    void InitializeLocation(const FString& InLocationId, const FVector& HalfExtent);

private:
    UFUNCTION()
    void OnLocationEntered(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Trigger;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY() FString LocationId;
};
