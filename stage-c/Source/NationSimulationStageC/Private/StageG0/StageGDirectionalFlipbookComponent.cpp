#include "StageG0/StageGDirectionalFlipbookComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Engine/Texture2D.h"
#include "Rendering/Texture2DResource.h"
#include "StageG0/StageG0Settings.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr int32 DirectionCount = 8;
constexpr int32 PlaceholderWidth = 96;
constexpr int32 PlaceholderHeight = 128;

FColor FamilyColor(const FString& AssetFamily)
{
    if (AssetFamily == TEXT("PLAYER")) return FColor(255, 225, 80, 255);
    if (AssetFamily == TEXT("GUARD")) return FColor(45, 125, 255, 255);
    if (AssetFamily == TEXT("CAPTAIN")) return FColor(235, 55, 55, 255);
    if (AssetFamily == TEXT("BROKER")) return FColor(170, 65, 220, 255);
    if (AssetFamily == TEXT("FANG_RAT")) return FColor(85, 85, 95, 255);
    return FColor(75, 190, 95, 255);
}

FVector2D ArrowVector(EStageGVisualDirection Direction)
{
    switch (Direction)
    {
    case EStageGVisualDirection::Front: return FVector2D(0.0f, 1.0f);
    case EStageGVisualDirection::FrontRight: return FVector2D(0.7071f, 0.7071f);
    case EStageGVisualDirection::Right: return FVector2D(1.0f, 0.0f);
    case EStageGVisualDirection::BackRight: return FVector2D(0.7071f, -0.7071f);
    case EStageGVisualDirection::Back: return FVector2D(0.0f, -1.0f);
    case EStageGVisualDirection::BackLeft: return FVector2D(-0.7071f, -0.7071f);
    case EStageGVisualDirection::Left: return FVector2D(-1.0f, 0.0f);
    case EStageGVisualDirection::FrontLeft: return FVector2D(-0.7071f, 0.7071f);
    }
    return FVector2D(0.0f, 1.0f);
}

float DistanceToSegment(const FVector2D& Point, const FVector2D& Start, const FVector2D& End)
{
    const FVector2D Segment = End - Start;
    const float LengthSquared = Segment.SizeSquared();
    const float T = LengthSquared > 0.0f
        ? FMath::Clamp(FVector2D::DotProduct(Point - Start, Segment) / LengthSquared, 0.0f, 1.0f)
        : 0.0f;
    return FVector2D::Distance(Point, Start + Segment * T);
}

float PlaybackRateFor(EStageGVisualAction Action)
{
    switch (Action)
    {
    case EStageGVisualAction::Flee: return 1.8f;
    case EStageGVisualAction::MonsterMove: return 1.2f;
    case EStageGVisualAction::MonsterCharge: return 1.6f;
    case EStageGVisualAction::Walk: return 1.0f;
    default: return 1.0f;
    }
}
}

UStageGDirectionalFlipbookComponent::UStageGDirectionalFlipbookComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCastShadow(false);
    SetLooping(true);
    SetPlayRate(1.0f);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaskedMaterial(
        TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial"));
    if (MaskedMaterial.Succeeded()) SetMaterial(0, MaskedMaterial.Object);
}

void UStageG0ProceduralSprite::InitializeRuntimeSprite(UTexture2D* InTexture, UMaterialInterface* InMaterial)
{
    BakedSourceTexture = InTexture;
    BakedSourceUV = FVector2D::ZeroVector;
    BakedSourceDimension = FVector2D(PlaceholderWidth, PlaceholderHeight);
    DefaultMaterial = InMaterial;
    PixelsPerUnrealUnit = 1.0f;
    AlternateMaterialSplitIndex = INDEX_NONE;
    BakedRenderData = {
        FVector4(-48.0f, 120.0f, 0.0f, 0.0f), FVector4(48.0f, 120.0f, 1.0f, 0.0f),
        FVector4(-48.0f, -8.0f, 0.0f, 1.0f), FVector4(48.0f, 120.0f, 1.0f, 0.0f),
        FVector4(48.0f, -8.0f, 1.0f, 1.0f), FVector4(-48.0f, -8.0f, 0.0f, 1.0f)};
}

void UStageGDirectionalFlipbookComponent::BeginPlay()
{
    Super::BeginPlay();
    BuildPlaceholderFlipbooks();
    SelectFlipbook(true);
}

void UStageGDirectionalFlipbookComponent::ConfigureVisual(
    const FString& InVisualActorId, const FString& InAssetFamily, bool bInMonster)
{
    VisualActorId = InVisualActorId;
    AssetFamily = InAssetFamily.ToUpper();
    bMonster = bInMonster;
    bAssetsReady = false;
    PlaceholderFlipbooks.Reset();
    PlaceholderTextures.Reset();
    DirectionalSprites.Reset();
    const float Scale = bMonster
        ? GetDefault<UStageG0Settings>()->MonsterSpriteScale
        : GetDefault<UStageG0Settings>()->HumanSpriteScale;
    SetRelativeScale3D(FVector(Scale));
    if (HasBegunPlay())
    {
        BuildPlaceholderFlipbooks();
        SelectFlipbook(true);
    }
}

FString UStageGDirectionalFlipbookComponent::DirectionalPlaceholderAssetId(
    const FString& Family, EStageGVisualDirection Direction)
{
    return FString::Printf(TEXT("VISUAL_PLACEHOLDER:%s:PROC_ARROW_%s"),
        *Family.ToUpper(), *StageGVisualDirectionSuffix(Direction));
}

int32 UStageGDirectionalFlipbookComponent::FlipbookKey(
    EStageGVisualAction Action, EStageGVisualDirection Direction)
{
    return static_cast<int32>(Action) * DirectionCount + static_cast<int32>(Direction);
}

void UStageGDirectionalFlipbookComponent::BuildPlaceholderFlipbooks()
{
    if (bAssetsReady) return;
    DirectionalSprites.SetNum(DirectionCount);
    for (int32 DirectionIndex = 0; DirectionIndex < DirectionCount; ++DirectionIndex)
    {
        DirectionalSprites[DirectionIndex] = BuildDirectionalSprite(
            static_cast<EStageGVisualDirection>(DirectionIndex));
        if (!DirectionalSprites[DirectionIndex])
        {
            UE_LOG(LogTemp, Error, TEXT("STAGE_G0_PROCEDURAL_PLACEHOLDER_FAILED actor=%s direction=%d"),
                *VisualActorId, DirectionIndex);
            return;
        }
    }

    for (int32 ActionIndex = 0; ActionIndex <= static_cast<int32>(EStageGVisualAction::Death); ++ActionIndex)
    {
        const EStageGVisualAction Action = static_cast<EStageGVisualAction>(ActionIndex);
        for (int32 DirectionIndex = 0; DirectionIndex < DirectionCount; ++DirectionIndex)
        {
            const EStageGVisualDirection Direction = static_cast<EStageGVisualDirection>(DirectionIndex);
            const FString Name = FString::Printf(TEXT("FB_VISUAL_PLACEHOLDER_%s_%s_%s"),
                *AssetFamily, *StageGVisualActionName(Action), *StageGVisualDirectionSuffix(Direction));
            UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(this, *Name, RF_Transient);
            FScopedFlipbookMutator Mutator(Flipbook);
            Mutator.FramesPerSecond = 8.0f;
            FPaperFlipbookKeyFrame& KeyFrame = Mutator.KeyFrames.AddDefaulted_GetRef();
            KeyFrame.Sprite = DirectionalSprites[DirectionIndex];
            KeyFrame.FrameRun = Action == EStageGVisualAction::Idle ? 4 : 2;
            PlaceholderFlipbooks.Add(FlipbookKey(Action, Direction), Flipbook);
        }
    }
    bAssetsReady = PlaceholderFlipbooks.Num() == (static_cast<int32>(EStageGVisualAction::Death) + 1) * DirectionCount;
}

UPaperSprite* UStageGDirectionalFlipbookComponent::BuildDirectionalSprite(EStageGVisualDirection Direction)
{
    UTexture2D* Texture = UTexture2D::CreateTransient(
        PlaceholderWidth, PlaceholderHeight, PF_B8G8R8A8,
        *FString::Printf(TEXT("T_VISUAL_PLACEHOLDER_%s_%s"), *AssetFamily, *StageGVisualDirectionSuffix(Direction)));
    if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.IsEmpty()) return nullptr;
    Texture->CompressionSettings = TC_EditorIcon;
    Texture->Filter = TF_Nearest;
    Texture->SRGB = true;
    FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
    FColor* Pixels = static_cast<FColor*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
    if (!Pixels) return nullptr;
    FMemory::Memzero(Pixels, PlaceholderWidth * PlaceholderHeight * sizeof(FColor));

    const FColor Fill = FamilyColor(AssetFamily);
    const FColor Outline(12, 12, 18, 255);
    const FColor Arrow(255, 255, 255, 255);
    const FVector2D Center(48.0f, 64.0f);
    const FVector2D Vector = ArrowVector(Direction);
    const FVector2D Perpendicular(-Vector.Y, Vector.X);
    const FVector2D ArrowStart = Center - Vector * 8.0f;
    const FVector2D ArrowEnd = Center + Vector * 38.0f;

    for (int32 Y = 0; Y < PlaceholderHeight; ++Y)
    {
        for (int32 X = 0; X < PlaceholderWidth; ++X)
        {
            const FVector2D Point(static_cast<float>(X), static_cast<float>(Y));
            const float BodyOuter = FMath::Square((X - 48.0f) / 30.0f) + FMath::Square((Y - 79.0f) / 37.0f);
            const float BodyInner = FMath::Square((X - 48.0f) / 26.0f) + FMath::Square((Y - 79.0f) / 33.0f);
            const bool bFoot = Y >= 103 && Y <= 120 && FMath::Abs(X - 48) <= 15;
            if (BodyOuter <= 1.0f || bFoot) Pixels[Y * PlaceholderWidth + X] = Outline;
            if (BodyInner <= 1.0f || (Y >= 105 && Y <= 118 && FMath::Abs(X - 48) <= 11))
                Pixels[Y * PlaceholderWidth + X] = Fill;

            if (DistanceToSegment(Point, ArrowStart, ArrowEnd) <= 6.0f)
                Pixels[Y * PlaceholderWidth + X] = Outline;
            if (DistanceToSegment(Point, ArrowStart, ArrowEnd) <= 3.0f)
                Pixels[Y * PlaceholderWidth + X] = Arrow;
            const FVector2D FromTip = Point - ArrowEnd;
            const float Back = -FVector2D::DotProduct(FromTip, Vector);
            const float Side = FMath::Abs(FVector2D::DotProduct(FromTip, Perpendicular));
            if (Back >= -1.0f && Back <= 18.0f && Side <= Back * 0.72f + 1.0f)
                Pixels[Y * PlaceholderWidth + X] = Back <= 14.0f ? Arrow : Outline;
        }
    }
    Mip.BulkData.Unlock();
    Texture->UpdateResource();
    PlaceholderTextures.Add(Texture);

    UStageG0ProceduralSprite* Sprite = NewObject<UStageG0ProceduralSprite>(this,
        *FString::Printf(TEXT("SPR_VISUAL_PLACEHOLDER_%s_%s"), *AssetFamily, *StageGVisualDirectionSuffix(Direction)),
        RF_Transient);
    if (!Sprite) return nullptr;
    // Initialize runtime baked data directly so the same transparent procedural
    // sprite is available in Editor and packaged Development builds.
    UMaterialInterface* Material = GetMaterial(0);
    Sprite->InitializeRuntimeSprite(Texture, Material);
    return Sprite;
}

EStageGVisualDirection UStageGDirectionalFlipbookComponent::QuantizeDirection(
    const FVector& WorldDirection, const FVector& CameraForward, const FVector& CameraRight,
    EStageGVisualDirection LastDirection, float DirectionThreshold)
{
    FVector Horizontal(WorldDirection.X, WorldDirection.Y, 0.0f);
    if (Horizontal.SizeSquared() < FMath::Square(DirectionThreshold)) return LastDirection;
    Horizontal.Normalize();
    FVector Forward(CameraForward.X, CameraForward.Y, 0.0f);
    FVector Right(CameraRight.X, CameraRight.Y, 0.0f);
    if (!Forward.Normalize() || !Right.Normalize()) return LastDirection;
    const float LocalRight = FVector::DotProduct(Horizontal, Right);
    const float LocalForward = FVector::DotProduct(Horizontal, Forward);
    const float Degrees = FMath::RadiansToDegrees(FMath::Atan2(LocalRight, LocalForward));
    const int32 Sector = (FMath::RoundToInt(Degrees / 45.0f) + 8) % 8;
    switch (Sector)
    {
    case 0: return EStageGVisualDirection::Front;
    case 1: return EStageGVisualDirection::FrontRight;
    case 2: return EStageGVisualDirection::Right;
    case 3: return EStageGVisualDirection::BackRight;
    case 4: return EStageGVisualDirection::Back;
    case 5: return EStageGVisualDirection::BackLeft;
    case 6: return EStageGVisualDirection::Left;
    case 7: return EStageGVisualDirection::FrontLeft;
    default: return LastDirection;
    }
}

EStageGVisualAction UStageGDirectionalFlipbookComponent::MapCoreAction(
    const FString& CoreAction, bool bIsMonster)
{
    const FString Action = CoreAction.ToUpper();
    if (bIsMonster)
    {
        if (Action == TEXT("MOVE")) return EStageGVisualAction::MonsterMove;
        if (Action == TEXT("CHARGE")) return EStageGVisualAction::MonsterCharge;
        if (Action == TEXT("BITE")) return EStageGVisualAction::MonsterBite;
        if (Action == TEXT("HIT")) return EStageGVisualAction::Hit;
        if (Action == TEXT("DEATH")) return EStageGVisualAction::Death;
        return EStageGVisualAction::Idle;
    }
    if (Action == TEXT("MOVE") || Action == TEXT("INVESTIGATE") || Action == TEXT("PROTECT"))
        return EStageGVisualAction::Walk;
    if (Action == TEXT("TALK") || Action == TEXT("REFUSE_TRADE") || Action == TEXT("HELP"))
        return EStageGVisualAction::Talk;
    if (Action == TEXT("WARN")) return EStageGVisualAction::Warn;
    if (Action == TEXT("REPORT")) return EStageGVisualAction::Report;
    if (Action == TEXT("ARREST")) return EStageGVisualAction::Arrest;
    if (Action == TEXT("FLEE")) return EStageGVisualAction::Flee;
    return EStageGVisualAction::Idle;
}

EStageGVisualAction UStageGDirectionalFlipbookComponent::ResolveMovementAction(
    float Speed, float Threshold, bool bIsMonster)
{
    if (Speed < Threshold) return EStageGVisualAction::Idle;
    return bIsMonster ? EStageGVisualAction::MonsterMove : EStageGVisualAction::Walk;
}

bool UStageGDirectionalFlipbookComponent::IsLoopingAction(EStageGVisualAction Action)
{
    return Action == EStageGVisualAction::Idle || Action == EStageGVisualAction::Walk ||
        Action == EStageGVisualAction::Flee || Action == EStageGVisualAction::MonsterMove;
}

bool UStageGDirectionalFlipbookComponent::UsesMovementState(const FString& CoreAction)
{
    const FString Action = CoreAction.ToUpper();
    return Action.IsEmpty() || Action == TEXT("WAIT") || Action == TEXT("MOVE") ||
        Action == TEXT("INVESTIGATE") || Action == TEXT("PROTECT");
}

bool UStageGDirectionalFlipbookComponent::FallsBackToIdle(EStageGVisualAction Action)
{
    return !IsLoopingAction(Action) && Action != EStageGVisualAction::Death;
}

FLinearColor UStageGDirectionalFlipbookComponent::PlaceholderColor(
    EStageGVisualAction Action, EStageGVisualDirection Direction) const
{
    FLinearColor Color(0.45f, 0.85f, 0.55f, 1.0f);
    if (AssetFamily == TEXT("PLAYER")) Color = FLinearColor(0.15f, 0.75f, 1.0f, 1.0f);
    else if (AssetFamily == TEXT("GUARD")) Color = FLinearColor(0.20f, 0.50f, 1.0f, 1.0f);
    else if (AssetFamily == TEXT("CAPTAIN")) Color = FLinearColor(0.65f, 0.35f, 1.0f, 1.0f);
    else if (AssetFamily == TEXT("BROKER")) Color = FLinearColor(1.0f, 0.70f, 0.20f, 1.0f);
    else if (AssetFamily == TEXT("FANG_RAT")) Color = FLinearColor(0.45f, 0.08f, 0.08f, 1.0f);
    if (Action == EStageGVisualAction::Warn || Action == EStageGVisualAction::MonsterCharge)
        Color = FLinearColor(1.0f, 0.15f, 0.05f, 1.0f);
    else if (Action == EStageGVisualAction::Report) Color = FLinearColor(1.0f, 0.9f, 0.1f, 1.0f);
    else if (Action == EStageGVisualAction::Death) Color = FLinearColor(0.12f, 0.12f, 0.12f, 1.0f);
    const float DirectionShade = 0.82f + static_cast<int32>(Direction) * 0.02f;
    Color.R *= DirectionShade; Color.G *= DirectionShade; Color.B *= DirectionShade;
    return Color;
}

void UStageGDirectionalFlipbookComponent::SelectFlipbook(bool bRestart)
{
    BuildPlaceholderFlipbooks();
    const TObjectPtr<UPaperFlipbook>* Found = PlaceholderFlipbooks.Find(FlipbookKey(CurrentAction, CurrentDirection));
    if (!Found || !*Found) return;
    if (GetFlipbook() == *Found && !bRestart) return;
    SetLooping(IsLoopingAction(CurrentAction));
    SetPlayRate(PlaybackRateFor(CurrentAction));
    SetSpriteColor(CurrentAction == EStageGVisualAction::Death
        ? FLinearColor(0.35f, 0.35f, 0.35f, 1.0f)
        : FLinearColor::White);
    SetFlipbook(*Found);
    if (bRestart) PlayFromStart(); else Play();
    ++FlipbookSwitchCount;
}

void UStageGDirectionalFlipbookComponent::UpdatePresentation(
    const FVector& WorldVelocity, const FString& CoreAction, const FVector& CameraLocation,
    const FRotator& CameraRotation, float DeltaSeconds)
{
    ActionElapsed += DeltaSeconds;
    const FString NormalizedAction = CoreAction.ToUpper();
    if (!bDebugActionOverride && NormalizedAction != LastRequestedCoreAction)
    {
        LastRequestedCoreAction = NormalizedAction;
        CurrentAction = MapCoreAction(NormalizedAction, bMonster);
        ActionElapsed = 0.0f;
        SelectFlipbook(true);
    }

    if (!bDebugActionOverride && UsesMovementState(NormalizedAction))
    {
        const EStageGVisualAction Desired = ResolveMovementAction(
            WorldVelocity.Size2D(), GetDefault<UStageG0Settings>()->WalkSpeedThreshold, bMonster);
        if (Desired != CurrentAction)
        {
            CurrentAction = Desired;
            ActionElapsed = 0.0f;
            SelectFlipbook(true);
        }
    }
    else if (!bDebugActionOverride && FallsBackToIdle(CurrentAction) &&
        ActionElapsed >= GetDefault<UStageG0Settings>()->NonLoopFallbackSeconds)
    {
        CurrentAction = EStageGVisualAction::Idle;
        SelectFlipbook(true);
    }

    const EStageGVisualDirection NewDirection = bDebugDirectionOverride ? CurrentDirection : QuantizeDirection(
        WorldVelocity, CameraRotation.Vector(), FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y),
        CurrentDirection, GetDefault<UStageG0Settings>()->MovementDirectionThreshold);
    if (NewDirection != CurrentDirection)
    {
        CurrentDirection = NewDirection;
        SelectFlipbook(false);
    }

    const FVector ToCamera = CameraLocation - GetComponentLocation();
    SetWorldRotation(FRotator(0.0f, ToCamera.Rotation().Yaw - 90.0f, 0.0f));
}

void UStageGDirectionalFlipbookComponent::ApplyDebugVisualAction(EStageGVisualAction Action)
{
    bDebugActionOverride = true;
    if (CurrentAction == Action) return;
    LastRequestedCoreAction = StageGVisualActionName(Action);
    CurrentAction = Action;
    ActionElapsed = 0.0f;
    SelectFlipbook(true);
}

void UStageGDirectionalFlipbookComponent::ApplyDebugVisualDirection(EStageGVisualDirection Direction)
{
    bDebugDirectionOverride = true;
    if (CurrentDirection == Direction) return;
    CurrentDirection = Direction;
    SelectFlipbook(false);
}

void UStageGDirectionalFlipbookComponent::ClearDebugFixtureOverride()
{
    bDebugActionOverride = false;
    bDebugDirectionOverride = false;
    LastRequestedCoreAction.Reset();
}

void UStageGDirectionalFlipbookComponent::InitializeVisualPlaceholderForAutomation()
{
    BuildPlaceholderFlipbooks();
    SelectFlipbook(true);
}

FStageGVisualDebugView UStageGDirectionalFlipbookComponent::GetDebugView(
    const FVector& CameraLocation, const FRotator& CameraRotation)
{
    FStageGVisualDebugView View;
    View.VisualActorId = VisualActorId;
    View.VisualAssetId = DirectionalPlaceholderAssetId(AssetFamily, CurrentDirection);
    View.VisualAction = StageGVisualActionName(CurrentAction);
    View.VisualDirection = StageGVisualDirectionName(CurrentDirection);
    View.FlipbookName = GetFlipbook() ? GetFlipbook()->GetName() : TEXT("VISUAL_PLACEHOLDER_MISSING");
    View.FlipbookFrame = GetPlaybackPositionInFrames();
    View.PlayRate = GetPlayRate();
    View.bIsPlaceholder = true;
    View.SpriteWorldLocation = GetComponentLocation();
    View.CameraYaw = CameraRotation.Yaw;
    View.CameraPitch = CameraRotation.Pitch;
    View.DistanceToCamera = FVector::Distance(CameraLocation, View.SpriteWorldLocation);
    View.FlipbookSwitchCount = FlipbookSwitchCount;
    if (const AActor* Owner = GetOwner())
    {
        if (const UCapsuleComponent* Capsule = Owner->FindComponentByClass<UCapsuleComponent>())
            View.CapsuleWorldLocation = Capsule->GetComponentLocation();
        else View.CapsuleWorldLocation = Owner->GetActorLocation();

        if (const UWorld* World = GetWorld())
        {
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(StageG0Occlusion), false, Owner);
            const bool bHit = World->LineTraceSingleByChannel(
                Hit, CameraLocation, View.SpriteWorldLocation, ECC_Visibility, Params);
            View.OcclusionState = bHit && Hit.GetActor() != Owner ? TEXT("OCCLUDED_BY_3D") : TEXT("VISIBLE_DEPTH_TESTED");
        }
    }
    return View;
}
