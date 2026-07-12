#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "StageG0/StageG0Types.h"
#include "StageGDirectionalFlipbookComponent.generated.h"

class UPaperFlipbook;
class UPaperSprite;
class UTexture2D;
class UMaterialInterface;

UCLASS()
class NATIONSIMULATIONSTAGEC_API UStageG0ProceduralSprite final : public UPaperSprite
{
    GENERATED_BODY()

public:
    void InitializeRuntimeSprite(UTexture2D* InTexture, UMaterialInterface* InMaterial);
};

UCLASS(ClassGroup=(StageG0), meta=(BlueprintSpawnableComponent))
class NATIONSIMULATIONSTAGEC_API UStageGDirectionalFlipbookComponent final : public UPaperFlipbookComponent
{
    GENERATED_BODY()

public:
    UStageGDirectionalFlipbookComponent();
    virtual void BeginPlay() override;

    void ConfigureVisual(const FString& InVisualActorId, const FString& InAssetFamily, bool bInMonster);
    void UpdatePresentation(const FVector& WorldVelocity, const FString& CoreAction,
        const FVector& CameraLocation, const FRotator& CameraRotation, float DeltaSeconds);
    void ApplyDebugVisualAction(EStageGVisualAction Action);
    void ApplyDebugVisualDirection(EStageGVisualDirection Direction);
    void ClearDebugFixtureOverride();
    FStageGVisualDebugView GetDebugView(const FVector& CameraLocation, const FRotator& CameraRotation);

    static EStageGVisualDirection QuantizeDirection(const FVector& WorldDirection,
        const FVector& CameraForward, const FVector& CameraRight, EStageGVisualDirection LastDirection,
        float DirectionThreshold = 3.0f);
    static EStageGVisualAction MapCoreAction(const FString& CoreAction, bool bMonster = false);
    static EStageGVisualAction ResolveMovementAction(float Speed, float Threshold, bool bMonster = false);
    static bool IsLoopingAction(EStageGVisualAction Action);
    static bool UsesMovementState(const FString& CoreAction);
    static bool FallsBackToIdle(EStageGVisualAction Action);

    EStageGVisualAction GetVisualAction() const { return CurrentAction; }
    EStageGVisualDirection GetVisualDirection() const { return CurrentDirection; }
    int32 GetFlipbookSwitchCount() const { return FlipbookSwitchCount; }
    bool IsVisualPlaceholder() const { return true; }
    const FString& GetVisualActorId() const { return VisualActorId; }
    const FString& GetAssetFamily() const { return AssetFamily; }
    bool IsDebugFixtureOverride() const { return bDebugActionOverride || bDebugDirectionOverride; }
    static FString DirectionalPlaceholderAssetId(const FString& Family, EStageGVisualDirection Direction);
    void InitializeVisualPlaceholderForAutomation();

private:
    UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> PlaceholderTextures;
    UPROPERTY(Transient) TArray<TObjectPtr<UPaperSprite>> DirectionalSprites;
    UPROPERTY(Transient) TMap<int32, TObjectPtr<UPaperFlipbook>> PlaceholderFlipbooks;
    UPROPERTY() FString VisualActorId;
    UPROPERTY() FString AssetFamily = TEXT("RESIDENT");
    UPROPERTY() FString LastRequestedCoreAction;
    EStageGVisualAction CurrentAction = EStageGVisualAction::Idle;
    EStageGVisualDirection CurrentDirection = EStageGVisualDirection::Front;
    bool bMonster = false;
    bool bAssetsReady = false;
    bool bDebugActionOverride = false;
    bool bDebugDirectionOverride = false;
    float ActionElapsed = 0.0f;
    int32 FlipbookSwitchCount = 0;

    void BuildPlaceholderFlipbooks();
    UPaperSprite* BuildDirectionalSprite(EStageGVisualDirection Direction);
    void SelectFlipbook(bool bRestart);
    static int32 FlipbookKey(EStageGVisualAction Action, EStageGVisualDirection Direction);
    FLinearColor PlaceholderColor(EStageGVisualAction Action, EStageGVisualDirection Direction) const;
};
