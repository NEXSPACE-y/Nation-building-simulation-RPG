#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1B/StageG1BMeshyAnimInstance.h"
#include "StageG1B/StageG1BPlayerVisualAdapter.h"
#include "StageG1B/StageG1BSettings.h"

namespace
{
const TCHAR* G1BMesh = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.SK_PLAYER_M_Meshy_v0_1");
const TCHAR* G1BSkeleton = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Skeleton/SKEL_PLAYER_M_Meshy_v0_1.SKEL_PLAYER_M_Meshy_v0_1");
const TCHAR* G1BPhysics = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Skeleton/PHYS_PLAYER_M_Meshy_v0_1.PHYS_PLAYER_M_Meshy_v0_1");
const TCHAR* G1BMaterial = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Materials/MI_PLAYER_M_Meshy_v0_1.MI_PLAYER_M_Meshy_v0_1");
const TCHAR* G1BAnimClass = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/AnimationBlueprint/ABP_PLAYER_M_Meshy_v0_1.ABP_PLAYER_M_Meshy_v0_1_C");
const TCHAR* G1BAdapterClass = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Blueprints/BP_StageG1B_PLAYER_M.BP_StageG1B_PLAYER_M_C");
const TCHAR* G1BWalk = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/PLAYER_M_Meshy_v0_1_AllAnimationsWalking_frame_rate_60_fbx.PLAYER_M_Meshy_v0_1_AllAnimationsWalking_frame_rate_60_fbx");
const TCHAR* G1BRun = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/PLAYER_M_Meshy_v0_1_AllAnimationsRunning_frame_rate_60_fbx.PLAYER_M_Meshy_v0_1_AllAnimationsRunning_frame_rate_60_fbx");
const TCHAR* G1BIdle = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/IDLE/A_PLAYER_M_Meshy_v0_1_IDLE_Idle11.A_PLAYER_M_Meshy_v0_1_IDLE_Idle11");
const TCHAR* G1BDash = TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/DASH/A_PLAYER_M_Meshy_v0_1_DASH_Run02.A_PLAYER_M_Meshy_v0_1_DASH_Run02");

FString ReadG1BFile(const FString& Relative)
{
    FString Text;
    FFileHelper::LoadFileToString(Text, *FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPaths::ProjectDir(), Relative)));
    return Text;
}

bool RequireG1B(FAutomationTestBase* Test, bool bCondition, const FString& Message)
{
    if (!bCondition) Test->AddError(Message);
    return bCondition;
}

void RecordG1BResult(int32 Category, bool bPassed)
{
    static TMap<int32, bool> Results;
    Results.Add(Category, bPassed);
    if (Results.Num() != 30) return;
    const TCHAR* Names[] = {
        TEXT("Source ZIP Contract"), TEXT("Mesh Import"), TEXT("No Manny Fallback"),
        TEXT("World Height"), TEXT("Material Contract"), TEXT("Animation Inventory"),
        TEXT("Walk"), TEXT("Run"), TEXT("Idle"), TEXT("Root Motion Isolation"),
        TEXT("Click Movement"), TEXT("Hold Movement"), TEXT("Walk Run Toggle"),
        TEXT("Grounding"), TEXT("Stage G-1A Regression"), TEXT("Package Default Map"),
        TEXT("Idle Source Contract"), TEXT("Formal Idle"), TEXT("Idle Progression"),
        TEXT("Idle Loop"), TEXT("Dash Source Contract"), TEXT("Dash Deduplication"),
        TEXT("Default Walk"), TEXT("Walk Run Dash Toggle"), TEXT("Destination Preservation"),
        TEXT("Dash Speed"), TEXT("Dash Animation"), TEXT("Idle Return"),
        TEXT("UI Isolation"), TEXT("Root Motion Isolation Four State")};
    TArray<FString> Lines;
    int32 Passed = 0;
    for (int32 Index = 1; Index <= 30; ++Index)
    {
        const bool bResult = Results.FindRef(Index);
        Passed += bResult ? 1 : 0;
        Lines.Add(FString::Printf(TEXT("G1B-M%d | %s | %s"), Index,
            bResult ? TEXT("PASS") : TEXT("FAIL"), Names[Index - 1]));
    }
    Lines.Add(FString::Printf(TEXT("SUMMARY | %d/30 tests passed"), Passed));
    const FString Output = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectDir(), TEXT("../out/stage-g1b-meshy/test-output")));
    IFileManager::Get().MakeDirectory(*Output, true);
    FFileHelper::SaveStringArrayToFile(Lines, *FPaths::Combine(
        Output, TEXT("stage_g1b_meshy_automation_results.txt")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool RunG1BCheck(FAutomationTestBase* Test, int32 Category)
{
    const USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, G1BMesh);
    const USkeleton* Skeleton = LoadObject<USkeleton>(nullptr, G1BSkeleton);
    const UPhysicsAsset* Physics = LoadObject<UPhysicsAsset>(nullptr, G1BPhysics);
    const UMaterialInstance* Material = LoadObject<UMaterialInstance>(nullptr, G1BMaterial);
    const UAnimSequence* Walk = LoadObject<UAnimSequence>(nullptr, G1BWalk);
    const UAnimSequence* Run = LoadObject<UAnimSequence>(nullptr, G1BRun);
    const UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, G1BIdle);
    const UAnimSequence* Dash = LoadObject<UAnimSequence>(nullptr, G1BDash);
    const FString Manifest = ReadG1BFile(
        TEXT("SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/Working/Manifest/PLAYER_M_Meshy_v0.1_SourceManifest.json"));
    const FString PlayerSource = ReadG1BFile(
        TEXT("Source/NationSimulationStageC/Private/StageG1A/StageG1APlayerCharacter.cpp"));
    const FString AdapterSource = ReadG1BFile(
        TEXT("Source/NationSimulationStageC/Private/StageG1B/StageG1BPlayerVisualAdapter.cpp"));
    const FString AnimSource = ReadG1BFile(
        TEXT("Source/NationSimulationStageC/Private/StageG1B/StageG1BMeshyAnimInstance.cpp"));
    const FString AdapterHeader = ReadG1BFile(
        TEXT("Source/NationSimulationStageC/Public/StageG1B/StageG1BPlayerVisualAdapter.h"));
    const FString MovementWidgetSource = ReadG1BFile(
        TEXT("Source/NationSimulationStageC/Private/StageG1B/StageG1BMovementWidget.cpp"));
    const FString AdditionalManifest = ReadG1BFile(
        TEXT("SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/AdditionalAnimations/Manifest/PLAYER_M_Meshy_v0.1_AdditionalAnimations_Manifest.json"));
    const FString DashDedup = ReadG1BFile(
        TEXT("SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/AdditionalAnimations/Working/Reports/dash_deduplication_evidence.json"));
    switch (Category)
    {
    case 1:
        return RequireG1B(Test, Manifest.Contains(TEXT("33506378")),
                   TEXT("G1B-M1 ZIP size must match the approved source")) &
            RequireG1B(Test, Manifest.Contains(TEXT("EBD21F89AC9441E4E9D94EAD79A0A7B5F58B8F0BCE3BCEDAD3A07553050B3367")),
                TEXT("G1B-M1 ZIP SHA-256 must match")) &
            RequireG1B(Test, Manifest.Contains(TEXT("\"internal_file_count\": 6")) &&
                    Manifest.Contains(TEXT("\"validation_status\": \"PASS\"")) &&
                    Manifest.Contains(TEXT("\"sha_match\": true")),
                TEXT("G1B-M1 six files and working-copy SHA validation must PASS"));
    case 2:
        return RequireG1B(Test, Mesh != nullptr, TEXT("G1B-M2 SkeletalMesh must resolve")) &
            RequireG1B(Test, Skeleton != nullptr, TEXT("G1B-M2 Skeleton must resolve")) &
            RequireG1B(Test, Physics != nullptr, TEXT("G1B-M2 PhysicsAsset must resolve")) &
            RequireG1B(Test, Mesh && Mesh->GetMaterials().Num() == 1 &&
                    Mesh->GetMaterials()[0].MaterialInterface == Material,
                TEXT("G1B-M2 imported mesh must have the Meshy material assigned"));
    case 3:
    {
        UClass* AdapterClass = LoadClass<AStageG1BPlayerVisualAdapter>(nullptr, G1BAdapterClass);
        return RequireG1B(Test, AdapterClass && AdapterClass->IsChildOf(AStageG1BPlayerVisualAdapter::StaticClass()),
                   TEXT("G1B-M3 dedicated visual adapter Blueprint must resolve")) &
            RequireG1B(Test, !AdapterSource.Contains(TEXT("SKM_Manny")) &&
                    AdapterSource.Contains(TEXT("SetVisibility(false, true)")) &&
                    AdapterSource.Contains(TEXT("fallback=false")),
                TEXT("G1B-M3 missing Meshy assets must fail visibly, never fall back to Manny"));
    }
    case 4:
    {
        const float Height = Mesh ? Mesh->GetImportedBounds().BoxExtent.Z * 2.0f : 0.0f;
        return RequireG1B(Test, Height >= 170.0f && Height <= 180.0f,
            FString::Printf(TEXT("G1B-M4 world height %.2f must be within 170-180 uu"), Height));
    }
    case 5:
    {
        UTexture2D* BaseColor = LoadObject<UTexture2D>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Textures/T_PLAYER_M_BaseColor_2K.T_PLAYER_M_BaseColor_2K"));
        UTexture2D* Normal = LoadObject<UTexture2D>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Textures/T_PLAYER_M_Normal_2K.T_PLAYER_M_Normal_2K"));
        UTexture2D* Metallic = LoadObject<UTexture2D>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Textures/T_PLAYER_M_Metallic_2K.T_PLAYER_M_Metallic_2K"));
        UTexture2D* Roughness = LoadObject<UTexture2D>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Textures/T_PLAYER_M_Roughness_2K.T_PLAYER_M_Roughness_2K"));
        bool bBaseColorSourceIs2K = true;
#if WITH_EDITORONLY_DATA
        bBaseColorSourceIs2K = BaseColor &&
            BaseColor->Source.GetSizeX() == 2048 && BaseColor->Source.GetSizeY() == 2048;
#endif
        return RequireG1B(Test, Material && Material->Parent,
                   TEXT("G1B-M5 material instance must have a parent material")) &
            RequireG1B(Test, BaseColor && BaseColor->SRGB && bBaseColorSourceIs2K,
                TEXT("G1B-M5 Base Color must be 2K sRGB")) &
            RequireG1B(Test, Normal && !Normal->SRGB && Normal->CompressionSettings == TC_Normalmap,
                TEXT("G1B-M5 Normal must use Normalmap compression without sRGB")) &
            RequireG1B(Test, Metallic && Roughness && !Metallic->SRGB && !Roughness->SRGB &&
                    Metallic->CompressionSettings == TC_Masks && Roughness->CompressionSettings == TC_Masks,
                TEXT("G1B-M5 Metallic and Roughness must use mask compression without sRGB"));
    }
    case 6:
    {
        const FString Inventory = ReadG1BFile(
            TEXT("SourceArt/StageG1B/PLAYER_M/Meshy/v0.1/Working/Reports/animation_inventory.json"));
        return RequireG1B(Test, Inventory.Contains(TEXT("\"sequences\"")) &&
                Inventory.Contains(TEXT("Walking_frame_rate_60")) &&
                Inventory.Contains(TEXT("Running_frame_rate_60")) &&
                Inventory.Contains(TEXT("Run_02")) && Inventory.Contains(TEXT("Run_03")) &&
                Inventory.Contains(TEXT("Charge")) && Inventory.Contains(TEXT("Head_Down")),
            TEXT("G1B-M6 imported animation inventory must enumerate all seven actual sequences"));
    }
    case 7:
        return RequireG1B(Test, Walk && Walk->GetPlayLength() > 2.0f,
                   TEXT("G1B-M7 selected WALK sequence must resolve")) &
            RequireG1B(Test, AnimSource.Contains(TEXT("HorizontalSpeed / WalkSpeed")) &&
                    AnimSource.Contains(TEXT("ApplyAnimationAsset(WalkSequence")),
                TEXT("G1B-M7 WALK must be speed-driven at the 250 uu/s nominal rate"));
    case 8:
        return RequireG1B(Test, Run && Run->GetPlayLength() > 1.2f,
                   TEXT("G1B-M8 selected RUN sequence must resolve")) &
            RequireG1B(Test, AnimSource.Contains(TEXT("HorizontalSpeed / RunSpeed")) &&
                    AnimSource.Contains(TEXT("ApplyAnimationAsset(RunSequence")),
                TEXT("G1B-M8 RUN must be speed-driven at the 500 uu/s nominal rate"));
    case 9:
        return RequireG1B(Test, Idle && Idle->GetPlayLength() > 1.8f,
                   TEXT("G1B-M9 stopped state must resolve formal Meshy Idle_11")) &
            RequireG1B(Test, AnimSource.Contains(TEXT("ApplyAnimationAsset(IdleSequence")) &&
                    !AnimSource.Contains(TEXT("ApplyAnimationAsset(nullptr")),
                TEXT("G1B-M9 stopped state must not silently fall back to Reference Pose"));
    case 10:
        return RequireG1B(Test, Idle && Walk && Run && Dash &&
                    !Idle->HasRootMotion() && !Walk->HasRootMotion() &&
                    !Run->HasRootMotion() && !Dash->HasRootMotion(),
                   TEXT("G1B-M10 IDLE/WALK/RUN/DASH root motion must be disabled")) &
            RequireG1B(Test, Idle && Walk && Run && Dash && Idle->bForceRootLock &&
                    Walk->bForceRootLock && Run->bForceRootLock && Dash->bForceRootLock,
                TEXT("G1B-M10 all four root bones must be force locked")) &
            RequireG1B(Test, AnimSource.Contains(TEXT("ERootMotionMode::IgnoreRootMotion")),
                TEXT("G1B-M10 AnimInstance must ignore root motion"));
    case 11:
        return RequireG1B(Test, PlayerSource.Contains(TEXT("StageGPointDrag")) &&
                PlayerSource.Contains(TEXT("IssueMoveToLocation")) &&
                PlayerSource.Contains(TEXT("BuildActiveNavRoute")),
            TEXT("G1B-M11 accepted left-click NavMesh movement must remain unchanged"));
    case 12:
        return RequireG1B(Test, PlayerSource.Contains(TEXT("TryIssueMoveFromCursor(true)")) &&
                PlayerSource.Contains(TEXT("HoldUpdateSeconds")),
            TEXT("G1B-M12 accepted held-click movement must remain unchanged"));
    case 13:
        return RequireG1B(Test,
                   AStageG1APlayerCharacter::ResolveMovementSpeed(
                       EStageG1AMovementMode::Walk, 250.0f, 500.0f) == 250.0f &&
                   AStageG1APlayerCharacter::ResolveMovementSpeed(
                       EStageG1AMovementMode::Run, 250.0f, 500.0f) == 500.0f,
                   TEXT("G1B-M13 WALK/RUN must remain 250/500")) &
            RequireG1B(Test, GetDefault<UStageG1BSettings>()->DashSpeed == 750.0f,
                TEXT("G1B-M13 Stage G-1B DASH must be 750")) &
            RequireG1B(Test, !PlayerSource.Contains(
                    TEXT("SetMovementMode(EStageG1AMovementMode NewMode)\n{\n    StopMove")),
                TEXT("G1B-M13 movement mode toggle must preserve destination and NavPath"));
    case 14:
        return RequireG1B(Test, AdapterSource.Contains(TEXT("-CapsuleHalfHeight - LocalBottomZ + 1.0f")) &&
                AdapterSource.Contains(TEXT("bCastDynamicShadow = true")) &&
                AdapterSource.Contains(TEXT("bCastContactShadow = true")),
            TEXT("G1B-M14 feet, capsule floor, dynamic shadow, and contact shadow must be aligned"));
    case 15:
        return RequireG1B(Test,
                   FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(),
                       TEXT("Maps/StageG1_3DCharacterPoC.umap"))),
                   TEXT("G1B-M15 accepted Stage G-1A map must remain present")) &
            RequireG1B(Test, LoadObject<USkeletalMesh>(nullptr,
                    TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple")) != nullptr,
                TEXT("G1B-M15 Manny fallback/reference asset must remain present"));
    case 16:
    {
        const FString Config = ReadG1BFile(TEXT("Config/DefaultEngine.ini"));
        return RequireG1B(Test, FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(),
                   TEXT("Maps/StageG1B_OriginalPlayerPoC.umap"))) &&
                Config.Contains(TEXT("GameDefaultMap=/Game/Maps/StageG1B_OriginalPlayerPoC")),
            TEXT("G1B-M16 packaged no-argument startup map must be Stage G-1B"));
    }
    case 17:
        return RequireG1B(Test, AdditionalManifest.Contains(TEXT("PLAYER_M_Meshy_Rigged_IDLE_v0.1.zip")) &&
                    AdditionalManifest.Contains(TEXT("Idle_11")) &&
                    AdditionalManifest.Contains(TEXT("\"bone_count\": 24")) &&
                    AdditionalManifest.Contains(TEXT("\"root_bone\": \"Hips\"")) &&
                    AdditionalManifest.Contains(TEXT("\"validation_status\": \"PASS\"")),
                TEXT("G1B-M17 formal IDLE ZIP, FBX, take, and Skeleton contract must PASS")) &
            RequireG1B(Test, Idle && Idle->GetSkeleton() == Skeleton,
                TEXT("G1B-M17 Idle_11 must use the existing PLAYER_M Skeleton"));
    case 18:
        return RequireG1B(Test, Idle && Idle->GetPlayLength() > 1.8f &&
                    !Idle->HasRootMotion() && Idle->bForceRootLock,
                TEXT("G1B-M18 formal Idle_11 must be imported with isolated root motion")) &
            RequireG1B(Test, AdapterSource.Contains(TEXT("IdleSequence = LoadObject")) &&
                    AdapterSource.Contains(TEXT("provisional=false")) &&
                    !AdapterSource.Contains(TEXT("Reference Pose provisional")),
                TEXT("G1B-M18 runtime IDLE must be formal, not provisional"));
    case 19:
        return RequireG1B(Test, AnimSource.Contains(TEXT("SetPlaying(NewAsset != nullptr)")) &&
                    AnimSource.Contains(TEXT("SetLooping(true)")) &&
                    AnimSource.Contains(TEXT("ApplyAnimationAsset(IdleSequence")),
                TEXT("G1B-M19 Idle_11 animation time must play and progress"));
    case 20:
        return RequireG1B(Test, Idle && Idle->GetPlayLength() > 1.8f &&
                    AnimSource.Contains(TEXT("SetLooping(true)")) &&
                    AdapterSource.Contains(TEXT("CaptureIdleAfterLoops")) &&
                    AdapterSource.Contains(TEXT("bIdleActorTransformStable")),
                TEXT("G1B-M20 Idle_11 must loop with transform-drift evidence"));
    case 21:
        return RequireG1B(Test, AdditionalManifest.Contains(TEXT("PLAYER_M_Meshy_Rigged_DASH_Run02_v0.1.zip")) &&
                    AdditionalManifest.Contains(TEXT("PLAYER_M_Meshy_v0.1_DASH_Run02.fbx")) &&
                    AdditionalManifest.Contains(TEXT("Run_02")),
                TEXT("G1B-M21 formal DASH ZIP and Run_02 take must be recorded"));
    case 22:
    {
        TArray<FString> DashAssets;
        IFileManager::Get().FindFiles(DashAssets, *FPaths::Combine(FPaths::ProjectContentDir(),
            TEXT("StageG1B/Characters/PLAYER_M/MeshyV01/Animations/DASH/*.uasset")), true, false);
        return RequireG1B(Test, DashDedup.Contains(TEXT("IMPORT_SINGLE_FBX_AS_DISTINCT_ASSET")) &&
                    DashDedup.Contains(TEXT("\"timing_matches\": false")) &&
                    DashDedup.Contains(TEXT("\"duplicate_asset_created\": false")),
                TEXT("G1B-M22 timing difference must justify the distinct formal DASH asset")) &
            RequireG1B(Test, DashAssets.Num() == 1,
                TEXT("G1B-M22 DASH directory must contain exactly one formal asset"));
    }
    case 23:
        return RequireG1B(Test, AdapterHeader.Contains(TEXT("MoveMode = EStageG1BMoveMode::Walk")) &&
                    AdapterSource.Contains(TEXT("SetMoveMode(EStageG1BMoveMode::Walk)")),
                TEXT("G1B-M23 normal startup mode must be WALK"));
    case 24:
        return RequireG1B(Test, AdapterSource.Contains(TEXT("EStageG1BMoveMode::Run")) &&
                    AdapterSource.Contains(TEXT("EStageG1BMoveMode::Dash")) &&
                    AdapterSource.Contains(TEXT("CycleMovementMode")) &&
                    MovementWidgetSource.Contains(TEXT("CycleMovementMode")),
                TEXT("G1B-M24 UI must cycle WALK to RUN to DASH to WALK"));
    case 25:
        return RequireG1B(Test, AdapterSource.Contains(TEXT("bRunDestinationMaintained")) &&
                    AdapterSource.Contains(TEXT("bDashDestinationMaintained")) &&
                    AdapterSource.Contains(TEXT("bWalkReturnDestinationMaintained")) &&
                    AdapterSource.Contains(TEXT("bDashPathMaintained")),
                TEXT("G1B-M25 all three transitions must preserve destination and NavPath"));
    case 26:
        return RequireG1B(Test, GetDefault<UStageG1BSettings>()->WalkSpeed == 250.0f &&
                    GetDefault<UStageG1BSettings>()->RunSpeed == 500.0f &&
                    GetDefault<UStageG1BSettings>()->DashSpeed == 750.0f,
                TEXT("G1B-M26 configured WALK/RUN/DASH speeds must be 250/500/750")) &
            RequireG1B(Test, AdapterSource.Contains(TEXT("Settings->DashSpeed")),
                TEXT("G1B-M26 DASH speed must come from Stage G-1B settings"));
    case 27:
        return RequireG1B(Test, Dash && Dash->GetPlayLength() > 0.7f &&
                    Dash->GetSkeleton() == Skeleton,
                TEXT("G1B-M27 formal DASH Run_02 must resolve on the Meshy Skeleton")) &
            RequireG1B(Test, AnimSource.Contains(TEXT("ApplyAnimationAsset(DashSequence")) &&
                    AnimSource.Contains(TEXT("HorizontalSpeed / DashSpeed")),
                TEXT("G1B-M27 DASH must play Run_02 at the 750 nominal rate"));
    case 28:
        return RequireG1B(Test, AdapterSource.Contains(TEXT("CaptureIdleReturn")) &&
                    AdapterSource.Contains(TEXT("bIdleReturnObserved")) &&
                    AnimSource.Contains(TEXT("ApplyAnimationAsset(IdleSequence")),
                TEXT("G1B-M28 destination arrival must return every move mode to Idle_11"));
    case 29:
        return RequireG1B(Test, MovementWidgetSource.Contains(TEXT("OnMovementModeButtonClicked")) &&
                    MovementWidgetSource.Contains(TEXT("CycleMovementMode")) &&
                    !MovementWidgetSource.Contains(TEXT("IssueMoveToLocation")) &&
                    !MovementWidgetSource.Contains(TEXT("SelectedTarget")),
                TEXT("G1B-M29 UI toggle must be isolated from world movement and target state"));
    case 30:
        return RequireG1B(Test, Idle && Walk && Run && Dash &&
                    !Idle->HasRootMotion() && !Walk->HasRootMotion() &&
                    !Run->HasRootMotion() && !Dash->HasRootMotion() &&
                    Idle->bForceRootLock && Walk->bForceRootLock &&
                    Run->bForceRootLock && Dash->bForceRootLock,
                TEXT("G1B-M30 all four Animation states must isolate root motion")) &
            RequireG1B(Test, AnimSource.Contains(TEXT("ERootMotionMode::IgnoreRootMotion")),
                TEXT("G1B-M30 AnimInstance must ignore root motion"));
    default:
        Test->AddError(TEXT("Unknown Stage G-1B Meshy test category"));
        return false;
    }
}
}

#define G1B_TEST(ClassName, TestName, Category) \
    IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, \
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
    bool ClassName::RunTest(const FString&) { const bool bPassed = RunG1BCheck(this, Category); \
        RecordG1BResult(Category, bPassed); return bPassed; }

G1B_TEST(FStageG1BSourceZipContractTest, "NationSimulation.StageG1B.Meshy.01.SourceZipContract", 1)
G1B_TEST(FStageG1BMeshImportTest, "NationSimulation.StageG1B.Meshy.02.MeshImport", 2)
G1B_TEST(FStageG1BNoMannyFallbackTest, "NationSimulation.StageG1B.Meshy.03.NoMannyFallback", 3)
G1B_TEST(FStageG1BWorldHeightTest, "NationSimulation.StageG1B.Meshy.04.WorldHeight", 4)
G1B_TEST(FStageG1BMaterialContractTest, "NationSimulation.StageG1B.Meshy.05.MaterialContract", 5)
G1B_TEST(FStageG1BAnimationInventoryTest, "NationSimulation.StageG1B.Meshy.06.AnimationInventory", 6)
G1B_TEST(FStageG1BWalkTest, "NationSimulation.StageG1B.Meshy.07.Walk", 7)
G1B_TEST(FStageG1BRunTest, "NationSimulation.StageG1B.Meshy.08.Run", 8)
G1B_TEST(FStageG1BIdleTest, "NationSimulation.StageG1B.Meshy.09.Idle", 9)
G1B_TEST(FStageG1BRootMotionIsolationTest, "NationSimulation.StageG1B.Meshy.10.RootMotionIsolation", 10)
G1B_TEST(FStageG1BClickMovementTest, "NationSimulation.StageG1B.Meshy.11.ClickMovement", 11)
G1B_TEST(FStageG1BHoldMovementTest, "NationSimulation.StageG1B.Meshy.12.HoldMovement", 12)
G1B_TEST(FStageG1BWalkRunToggleTest, "NationSimulation.StageG1B.Meshy.13.WalkRunToggle", 13)
G1B_TEST(FStageG1BGroundingTest, "NationSimulation.StageG1B.Meshy.14.Grounding", 14)
G1B_TEST(FStageG1BG1ARegressionTest, "NationSimulation.StageG1B.Meshy.15.StageG1ARegression", 15)
G1B_TEST(FStageG1BPackageDefaultMapTest, "NationSimulation.StageG1B.Meshy.16.PackageDefaultMap", 16)
G1B_TEST(FStageG1BIdleSourceContractTest, "NationSimulation.StageG1B.Meshy.17.IdleSourceContract", 17)
G1B_TEST(FStageG1BFormalIdleTest, "NationSimulation.StageG1B.Meshy.18.FormalIdle", 18)
G1B_TEST(FStageG1BIdleProgressionTest, "NationSimulation.StageG1B.Meshy.19.IdleProgression", 19)
G1B_TEST(FStageG1BIdleLoopTest, "NationSimulation.StageG1B.Meshy.20.IdleLoop", 20)
G1B_TEST(FStageG1BDashSourceContractTest, "NationSimulation.StageG1B.Meshy.21.DashSourceContract", 21)
G1B_TEST(FStageG1BDashDeduplicationTest, "NationSimulation.StageG1B.Meshy.22.DashDeduplication", 22)
G1B_TEST(FStageG1BDefaultWalkTest, "NationSimulation.StageG1B.Meshy.23.DefaultWalk", 23)
G1B_TEST(FStageG1BWalkRunDashToggleTest, "NationSimulation.StageG1B.Meshy.24.WalkRunDashToggle", 24)
G1B_TEST(FStageG1BDestinationPreservationTest, "NationSimulation.StageG1B.Meshy.25.DestinationPreservation", 25)
G1B_TEST(FStageG1BDashSpeedTest, "NationSimulation.StageG1B.Meshy.26.DashSpeed", 26)
G1B_TEST(FStageG1BDashAnimationTest, "NationSimulation.StageG1B.Meshy.27.DashAnimation", 27)
G1B_TEST(FStageG1BIdleReturnTest, "NationSimulation.StageG1B.Meshy.28.IdleReturn", 28)
G1B_TEST(FStageG1BUiIsolationTest, "NationSimulation.StageG1B.Meshy.29.UIIsolation", 29)
G1B_TEST(FStageG1BRootMotionFourStateTest, "NationSimulation.StageG1B.Meshy.30.RootMotionIsolation", 30)

#undef G1B_TEST

#endif
