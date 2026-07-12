#pragma once

#include "CoreMinimal.h"
#include "StageG0Types.generated.h"

UENUM(BlueprintType)
enum class EStageGVisualDirection : uint8
{
    Front,
    FrontRight,
    Right,
    BackRight,
    Back,
    BackLeft,
    Left,
    FrontLeft
};

UENUM(BlueprintType)
enum class EStageGVisualAction : uint8
{
    Idle,
    Walk,
    Talk,
    Warn,
    Report,
    Arrest,
    Flee,
    MonsterMove,
    MonsterCharge,
    MonsterBite,
    Hit,
    Death
};

USTRUCT(BlueprintType)
struct FStageGVisualDebugView
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) FString VisualActorId;
    UPROPERTY(BlueprintReadOnly) FString VisualAssetId;
    UPROPERTY(BlueprintReadOnly) FString VisualAction;
    UPROPERTY(BlueprintReadOnly) FString VisualDirection;
    UPROPERTY(BlueprintReadOnly) FString FlipbookName;
    UPROPERTY(BlueprintReadOnly) int32 FlipbookFrame = 0;
    UPROPERTY(BlueprintReadOnly) float PlayRate = 1.0f;
    UPROPERTY(BlueprintReadOnly) bool bIsPlaceholder = true;
    UPROPERTY(BlueprintReadOnly) FVector SpriteWorldLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector CapsuleWorldLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float CameraYaw = 0.0f;
    UPROPERTY(BlueprintReadOnly) float CameraPitch = 0.0f;
    UPROPERTY(BlueprintReadOnly) float DistanceToCamera = 0.0f;
    UPROPERTY(BlueprintReadOnly) FString OcclusionState = TEXT("UNKNOWN");
    UPROPERTY(BlueprintReadOnly) int32 FlipbookSwitchCount = 0;
};

NATIONSIMULATIONSTAGEC_API FString StageGVisualDirectionName(EStageGVisualDirection Direction);
NATIONSIMULATIONSTAGEC_API FString StageGVisualDirectionSuffix(EStageGVisualDirection Direction);
NATIONSIMULATIONSTAGEC_API FString StageGVisualActionName(EStageGVisualAction Action);
NATIONSIMULATIONSTAGEC_API FString StageGVisualDirectionJapanese(EStageGVisualDirection Direction);
NATIONSIMULATIONSTAGEC_API FString StageGVisualActionJapanese(EStageGVisualAction Action);
NATIONSIMULATIONSTAGEC_API FString StageGVisualFamilyJapanese(const FString& AssetFamily);
