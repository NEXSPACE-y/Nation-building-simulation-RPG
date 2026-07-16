#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "StageG3ACapitalGameMode.generated.h"

/** G3A-only map host. It keeps the accepted Player class without spawning the legacy test plaza. */
UCLASS()
class NATIONSIMULATIONSTAGEC_API AStageG3ACapitalGameMode final : public AGameModeBase
{
    GENERATED_BODY()

public:
    AStageG3ACapitalGameMode();
    virtual void BeginPlay() override;

private:
    int32 ConfigureCapitalWalkableSurfaces();
    void SpawnCapitalLighting();
};
