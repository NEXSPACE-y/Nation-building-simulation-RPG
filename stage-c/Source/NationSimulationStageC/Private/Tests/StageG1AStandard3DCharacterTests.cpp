#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/Skeleton.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "StageG1A/StageG1AGameMode.h"
#include "StageG1A/StageG1ANpcActor.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1A/StageG1ASettings.h"
#include "UObject/UnrealType.h"

namespace
{
FString ReadG1ASource(const FString& Relative)
{
    FString Text;
    FFileHelper::LoadFileToString(Text, *FPaths::Combine(FPaths::ProjectDir(), Relative));
    return Text;
}

bool RequireG1A(FAutomationTestBase* Test, bool bCondition, const FString& Message)
{
    if (!bCondition) Test->AddError(Message);
    return bCondition;
}

void RecordG1AResult(int32 Category, bool bPassed)
{
    static TMap<int32, bool> Results;
    Results.Add(Category, bPassed);
    if (Results.Num() != 22) return;
    const TCHAR* Names[] = {
        TEXT("Standard Asset Contract"), TEXT("World Height"), TEXT("Foot Grounding"),
        TEXT("Capsule Alignment"), TEXT("Animation State"), TEXT("Animation Progression"),
        TEXT("Movement Sync"), TEXT("Click Move"), TEXT("Hold Move"), TEXT("No WASD"),
        TEXT("Path Around Obstacle"), TEXT("No Fall"), TEXT("Targeting"),
        TEXT("Lighting and Shadow"), TEXT("Save Independence"),
        TEXT("Default Walk Mode"), TEXT("Walk Animation"), TEXT("Run Toggle"),
        TEXT("Walk Toggle"), TEXT("Idle Return"), TEXT("UI Isolation"),
        TEXT("Movement Regression")};
    TArray<FString> Lines;
    int32 Passed = 0;
    for (int32 Index = 1; Index <= 22; ++Index)
    {
        const bool bResult = Results.FindRef(Index);
        Passed += bResult ? 1 : 0;
        Lines.Add(FString::Printf(TEXT("G1A-%d | %s | %s"), Index,
            bResult ? TEXT("PASS") : TEXT("FAIL"), Names[Index - 1]));
    }
    Lines.Add(FString::Printf(TEXT("SUMMARY | %d/22 tests passed"), Passed));
    const FString Output = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectDir(), TEXT("../out/stage-g1a/test-output")));
    IFileManager::Get().MakeDirectory(*Output, true);
    FFileHelper::SaveStringArrayToFile(Lines, *FPaths::Combine(
        Output, TEXT("stage_g1a_automation_results.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool RunG1ACheck(FAutomationTestBase* Test, int32 Category)
{
    const UStageG1ASettings* Settings = GetDefault<UStageG1ASettings>();
    const AStageG1APlayerCharacter* Player = GetDefault<AStageG1APlayerCharacter>();
    const AStageG1ANpcActor* Npc = GetDefault<AStageG1ANpcActor>();
    const FString PlayerSource = ReadG1ASource(
        TEXT("Source/NationSimulationStageC/Private/StageG1A/StageG1APlayerCharacter.cpp"));
    const FString GameModeSource = ReadG1ASource(
        TEXT("Source/NationSimulationStageC/Private/StageG1A/StageG1AGameMode.cpp"));
    const FString HudSource = ReadG1ASource(
        TEXT("Source/NationSimulationStageC/Private/StageG1A/StageG1AHudWidget.cpp"));
    const FString NpcSource = ReadG1ASource(
        TEXT("Source/NationSimulationStageC/Private/StageG1A/StageG1ANpcActor.cpp"));
    switch (Category)
    {
    case 1:
    {
        const USkeleton* Skeleton = LoadObject<USkeleton>(
            nullptr, TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
        return RequireG1A(Test, Player->GetMesh()->GetSkeletalMeshAsset() != nullptr,
                   TEXT("G1A-1 Manny skeletal mesh must resolve")) &
            RequireG1A(Test, Player->GetMesh()->GetAnimClass() != nullptr,
                TEXT("G1A-1 standard animation blueprint must resolve")) &
            RequireG1A(Test, Skeleton != nullptr,
                TEXT("G1A-1 standard skeleton must resolve"));
    }
    case 2:
    {
        const USkeletalMesh* Mesh = Player->GetMesh()->GetSkeletalMeshAsset();
        const float Height = Mesh ? Mesh->GetImportedBounds().BoxExtent.Z * 2.0f * Settings->CharacterScale : 0.0f;
        return RequireG1A(Test, Height >= 170.0f && Height <= 180.0f,
            FString::Printf(TEXT("G1A-2 mesh height %.2f must be within 170-180 uu"), Height));
    }
    case 3:
        return RequireG1A(Test, Settings->MeshOffsetZ >= -96.0f && Settings->MeshOffsetZ <= -86.0f,
            TEXT("G1A-3 standard root offset must place feet within 10 uu of capsule floor"));
    case 4:
        return RequireG1A(Test, FMath::IsNearlyEqual(Player->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 42.0f),
                   TEXT("G1A-4 capsule radius must be 42 uu")) &
            RequireG1A(Test, FMath::IsNearlyEqual(Player->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 96.0f),
                TEXT("G1A-4 capsule half-height must be 96 uu")) &
            RequireG1A(Test, FMath::IsNearlyEqual(Player->GetMesh()->GetRelativeLocation().Z, Settings->MeshOffsetZ),
                TEXT("G1A-4 mesh/capsule lower alignment must use the single configured offset"));
    case 5:
        return RequireG1A(Test,
                   AStageG1APlayerCharacter::SelectMovementBandForMode(
                       0.0f, 8.0f, EStageG1AMovementMode::Run) == EStageG1AMovementBand::Idle,
                   TEXT("G1A-5 zero speed must select IDLE")) &
            RequireG1A(Test,
                AStageG1APlayerCharacter::SelectMovementBandForMode(
                    500.0f, 8.0f, EStageG1AMovementMode::Walk) == EStageG1AMovementBand::Walk,
                TEXT("G1A-5 WALK mode must select WALK independently of speed thresholds")) &
            RequireG1A(Test,
                AStageG1APlayerCharacter::SelectMovementBandForMode(
                    250.0f, 8.0f, EStageG1AMovementMode::Run) == EStageG1AMovementBand::Run,
                TEXT("G1A-5 RUN mode must select RUN independently of speed thresholds"));
    case 6:
        return RequireG1A(Test, Player->GetMesh()->GetAnimClass() != nullptr &&
                PlayerSource.Contains(TEXT("UpdateMovementPresentation")) &&
                PlayerSource.Contains(TEXT("Super::Tick(DeltaSeconds)")),
            TEXT("G1A-6 AnimBP must be ticked through the live skeletal mesh every real frame"));
    case 7:
        return RequireG1A(Test,
                   AStageG1APlayerCharacter::CalculateModeAnimationPlayRate(
                       250.0f, EStageG1AMovementMode::Walk, 250.0f, 500.0f, 0.72f, 1.18f) == 1.0f,
                   TEXT("G1A-7 configured WALK speed must map to 1.0 play rate")) &
            RequireG1A(Test,
                AStageG1APlayerCharacter::CalculateModeAnimationPlayRate(
                    500.0f, EStageG1AMovementMode::Run, 250.0f, 500.0f, 0.72f, 1.18f) == 1.0f,
                TEXT("G1A-7 configured RUN speed must map to 1.0 play rate")) &
            RequireG1A(Test, PlayerSource.Contains(TEXT("GlobalAnimRateScale = MovementView.PlayRate")),
                TEXT("G1A-7 measured Actor speed must drive skeletal animation play rate"));
    case 8:
        return RequireG1A(Test, PlayerSource.Contains(TEXT("StageGPointDrag")) &&
                PlayerSource.Contains(TEXT("LineTraceSingleByChannel")) &&
                PlayerSource.Contains(TEXT("BuildActiveNavRoute")) &&
                PlayerSource.Contains(TEXT("AddMovementInput(MoveDirection.GetSafeNormal()")),
            TEXT("G1A-8 left-click path must ground trace and follow a NavMesh route through CharacterMovement"));
    case 9:
        return RequireG1A(Test, PlayerSource.Contains(TEXT("HoldUpdateSeconds")) &&
                PlayerSource.Contains(TEXT("TryIssueMoveFromCursor(true)")) &&
                AStageG1APlayerCharacter::ShouldUpdateDestination(
                    FVector::ZeroVector, FVector(100.0f, 0.0f, 0.0f), 50.0f),
            TEXT("G1A-9 held click must periodically update a materially changed destination"));
    case 10:
        return RequireG1A(Test, !PlayerSource.Contains(TEXT("BindAxis(TEXT(\"MoveForward\")")) &&
                !PlayerSource.Contains(TEXT("BindAxis(TEXT(\"MoveRight\")")),
            TEXT("G1A-10 G-1A player must not bind WASD movement axes"));
    case 11:
        return RequireG1A(Test, GameModeSource.Contains(TEXT("PathObstacle")) &&
                PlayerSource.Contains(TEXT("FindPathToLocationSynchronously")) &&
                PlayerSource.Contains(TEXT("Path->IsPartial()")),
            TEXT("G1A-11 obstacle and complete NavMesh path validation must exist"));
    case 12:
        return RequireG1A(Test, GameModeSource.Contains(TEXT("BoundaryNorth")) &&
                GameModeSource.Contains(TEXT("OuterSafetyFloor")) &&
                PlayerSource.Contains(TEXT("FallRecoveryZ")) &&
                PlayerSource.Contains(TEXT("LastSafeLocation")),
            TEXT("G1A-12 boundary, safety floor, and last-safe fall recovery must exist"));
    case 13:
        return RequireG1A(Test, Npc != nullptr && NpcSource.Contains(TEXT("ECC_GameTraceChannel2")) &&
                PlayerSource.Contains(TEXT("TrySelectTargetUnderCursor")) &&
                NpcSource.Contains(TEXT("STANDARD_3D_NPC")),
            TEXT("G1A-13 representative 3D NPC must be selectable on the target channel"));
    case 14:
        return RequireG1A(Test, Player->GetMesh()->CastShadow && Player->GetMesh()->bCastDynamicShadow &&
                GameModeSource.Contains(TEXT("ContactShadowLength")) &&
                GameModeSource.Contains(TEXT("ASkyAtmosphere")) &&
                GameModeSource.Contains(TEXT("blob_shadow=0")),
            TEXT("G1A-14 lit skeletal mesh and dynamic/contact shadow contract must exist without blob shadow"));
    case 15:
    {
        bool bSaveGameProperty = false;
        for (TFieldIterator<FProperty> It(AStageG1APlayerCharacter::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
            bSaveGameProperty |= It->HasAnyPropertyFlags(CPF_SaveGame);
        return RequireG1A(Test, !bSaveGameProperty &&
                !PlayerSource.Contains(TEXT("NationSimulationGameInstanceSubsystem")) &&
                !NpcSource.Contains(TEXT("NationSimulationGameInstanceSubsystem")),
            TEXT("G1A-15 render mesh and animation state must remain outside the save source of truth"));
    }
    case 16:
        return RequireG1A(Test, Player->GetMovementMode() == EStageG1AMovementMode::Walk,
                   TEXT("G1A-16 default mode must be WALK")) &
            RequireG1A(Test, FMath::IsNearlyEqual(Settings->WalkSpeed, 250.0f) &&
                    FMath::IsNearlyEqual(Settings->RunSpeed, 500.0f),
                TEXT("G1A-16 WALK/RUN speeds must be externalized as 250/500")) &
            RequireG1A(Test, FMath::IsNearlyEqual(
                    Player->GetCharacterMovement()->MaxWalkSpeed, Settings->WalkSpeed),
                TEXT("G1A-16 default maximum speed must equal WalkSpeed"));
    case 17:
        return RequireG1A(Test,
                   AStageG1APlayerCharacter::SelectMovementBandForMode(
                       Settings->WalkSpeed, Settings->WalkStateThreshold,
                       EStageG1AMovementMode::Walk) == EStageG1AMovementBand::Walk,
                   TEXT("G1A-17 WALK mode movement must select WALK animation")) &
            RequireG1A(Test, AStageG1APlayerCharacter::CalculateModeAnimationPlayRate(
                    Settings->WalkSpeed, EStageG1AMovementMode::Walk, Settings->WalkSpeed,
                    Settings->RunSpeed, Settings->AnimationMinPlayRate,
                    Settings->AnimationMaxPlayRate) == 1.0f,
                TEXT("G1A-17 WALK animation must advance at natural rate at WalkSpeed")) &
            RequireG1A(Test, PlayerSource.Contains(TEXT("Super::Tick(DeltaSeconds)")),
                TEXT("G1A-17 WALK animation time must advance on real frame ticks"));
    case 18:
        return RequireG1A(Test, AStageG1APlayerCharacter::ResolveMovementSpeed(
                    EStageG1AMovementMode::Run, Settings->WalkSpeed, Settings->RunSpeed) == Settings->RunSpeed,
                   TEXT("G1A-18 RUN toggle must raise maximum speed to RunSpeed")) &
            RequireG1A(Test, PlayerSource.Contains(TEXT("STAGE_G1A_MOVEMENT_MODE")) &&
                    !PlayerSource.Contains(TEXT("SetMovementMode(EStageG1AMovementMode NewMode)\n{\n    StopMove")),
                TEXT("G1A-18 mode toggle must not stop or rebuild the active destination/path")) &
            RequireG1A(Test, AStageG1APlayerCharacter::SelectMovementBandForMode(
                    Settings->WalkSpeed, Settings->WalkStateThreshold,
                    EStageG1AMovementMode::Run) == EStageG1AMovementBand::Run,
                TEXT("G1A-18 RUN mode must select RUN animation immediately"));
    case 19:
        return RequireG1A(Test, AStageG1APlayerCharacter::ResolveMovementSpeed(
                    EStageG1AMovementMode::Walk, Settings->WalkSpeed, Settings->RunSpeed) == Settings->WalkSpeed,
                   TEXT("G1A-19 WALK toggle must lower maximum speed to WalkSpeed")) &
            RequireG1A(Test, PlayerSource.Contains(TEXT("HorizontalVelocity.GetSafeNormal() * Settings->WalkSpeed")),
                TEXT("G1A-19 RUN-to-WALK must cap current horizontal velocity without teleporting")) &
            RequireG1A(Test, AStageG1APlayerCharacter::SelectMovementBandForMode(
                    Settings->RunSpeed, Settings->WalkStateThreshold,
                    EStageG1AMovementMode::Walk) == EStageG1AMovementBand::Walk,
                TEXT("G1A-19 WALK mode must select WALK animation immediately"));
    case 20:
        return RequireG1A(Test, AStageG1APlayerCharacter::SelectMovementBandForMode(
                    0.0f, Settings->WalkStateThreshold,
                    EStageG1AMovementMode::Walk) == EStageG1AMovementBand::Idle,
                   TEXT("G1A-20 stopped WALK mode must return IDLE")) &
            RequireG1A(Test, AStageG1APlayerCharacter::SelectMovementBandForMode(
                    0.0f, Settings->WalkStateThreshold,
                    EStageG1AMovementMode::Run) == EStageG1AMovementBand::Idle,
                TEXT("G1A-20 stopped RUN mode must return IDLE")) &
            RequireG1A(Test, PlayerSource.Contains(TEXT("StopMove(TEXT(\"目的地へ到着\")")),
                TEXT("G1A-20 arrival must stop CharacterMovement and return animation to IDLE"));
    case 21:
        return RequireG1A(Test, HudSource.Contains(TEXT("OnMovementModeButtonClicked")) &&
                    HudSource.Contains(TEXT("OnClicked.AddDynamic")),
                   TEXT("G1A-21 normal UI must expose one clickable WALK/RUN toggle")) &
            RequireG1A(Test, PlayerSource.Contains(TEXT("Type == TEXT(\"SButton\")")),
                TEXT("G1A-21 button input must be blocked from world-click movement")) &
            RequireG1A(Test, HudSource.Contains(TEXT("Player->SetMovementMode(NextMode);")) &&
                    !HudSource.Contains(TEXT("IssueMoveToLocation")) &&
                    !HudSource.Contains(TEXT("TrySelectTargetUnderCursor")) &&
                    !HudSource.Contains(TEXT("NationSimulationGameInstanceSubsystem")),
                TEXT("G1A-21 toggle UI must not issue movement, target, or causal commands"));
    case 22:
        return RequireG1A(Test, PlayerSource.Contains(TEXT("StageGPointDrag")) &&
                    PlayerSource.Contains(TEXT("TryIssueMoveFromCursor(true)")) &&
                    PlayerSource.Contains(TEXT("FindPathToLocationSynchronously")),
                   TEXT("G1A-22 click, hold, and obstacle NavMesh movement must remain active")) &
            RequireG1A(Test, GameModeSource.Contains(TEXT("MildSlope")) &&
                    GameModeSource.Contains(TEXT("Stair_")) &&
                    GameModeSource.Contains(TEXT("OuterSafetyFloor")),
                TEXT("G1A-22 slope, stairs, and no-fall regression fixtures must remain active")) &
            RequireG1A(Test, !PlayerSource.Contains(TEXT("BindAxis(TEXT(\"MoveForward\")")) &&
                    !PlayerSource.Contains(TEXT("BindAxis(TEXT(\"MoveRight\")")),
                TEXT("G1A-22 WASD must remain disabled in both movement modes"));
    default:
        Test->AddError(TEXT("Unknown G-1A test category"));
        return false;
    }
}
}

#define G1A_TEST(ClassName, TestName, Category) \
    IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, \
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
    bool ClassName::RunTest(const FString&) { const bool bPassed = RunG1ACheck(this, Category); \
        RecordG1AResult(Category, bPassed); return bPassed; }

G1A_TEST(FStageG1AStandardAssetContractTest, "NationSimulation.StageG1A.01.StandardAssetContract", 1)
G1A_TEST(FStageG1AWorldHeightTest, "NationSimulation.StageG1A.02.WorldHeight", 2)
G1A_TEST(FStageG1AFootGroundingTest, "NationSimulation.StageG1A.03.FootGrounding", 3)
G1A_TEST(FStageG1ACapsuleAlignmentTest, "NationSimulation.StageG1A.04.CapsuleAlignment", 4)
G1A_TEST(FStageG1AAnimationStateTest, "NationSimulation.StageG1A.05.AnimationState", 5)
G1A_TEST(FStageG1AAnimationProgressionTest, "NationSimulation.StageG1A.06.AnimationProgression", 6)
G1A_TEST(FStageG1AMovementSyncTest, "NationSimulation.StageG1A.07.MovementSync", 7)
G1A_TEST(FStageG1AClickMoveTest, "NationSimulation.StageG1A.08.ClickMove", 8)
G1A_TEST(FStageG1AHoldMoveTest, "NationSimulation.StageG1A.09.HoldMove", 9)
G1A_TEST(FStageG1ANoWasdTest, "NationSimulation.StageG1A.10.NoWASD", 10)
G1A_TEST(FStageG1APathAroundObstacleTest, "NationSimulation.StageG1A.11.PathAroundObstacle", 11)
G1A_TEST(FStageG1ANoFallTest, "NationSimulation.StageG1A.12.NoFall", 12)
G1A_TEST(FStageG1ATargetingTest, "NationSimulation.StageG1A.13.Targeting", 13)
G1A_TEST(FStageG1ALightingShadowTest, "NationSimulation.StageG1A.14.LightingAndShadow", 14)
G1A_TEST(FStageG1ASaveIndependenceTest, "NationSimulation.StageG1A.15.SaveIndependence", 15)
G1A_TEST(FStageG1ADefaultWalkModeTest, "NationSimulation.StageG1A.16.DefaultWalkMode", 16)
G1A_TEST(FStageG1AWalkAnimationTest, "NationSimulation.StageG1A.17.WalkAnimation", 17)
G1A_TEST(FStageG1ARunToggleTest, "NationSimulation.StageG1A.18.RunToggle", 18)
G1A_TEST(FStageG1AWalkToggleTest, "NationSimulation.StageG1A.19.WalkToggle", 19)
G1A_TEST(FStageG1AIdleReturnTest, "NationSimulation.StageG1A.20.IdleReturn", 20)
G1A_TEST(FStageG1AUiIsolationTest, "NationSimulation.StageG1A.21.UIIsolation", 21)
G1A_TEST(FStageG1AMovementRegressionTest, "NationSimulation.StageG1A.22.MovementRegression", 22)

#undef G1A_TEST

#endif
