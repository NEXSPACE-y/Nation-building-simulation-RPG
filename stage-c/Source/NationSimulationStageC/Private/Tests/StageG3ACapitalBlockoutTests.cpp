#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Level.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"
#include "StageG2B/StageG2BGuardMActor.h"
#include "StageG3A/StageG3ACapitalEvidenceActor.h"
#include "StageG3A/StageG3AStandardCameraAutoFollowActor.h"

namespace
{
FString G3AProjectPath(const FString& Relative)
{
    return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), Relative));
}

FString ReadG3AFile(const FString& Relative)
{
    FString Text;
    FFileHelper::LoadFileToString(Text, *G3AProjectPath(Relative));
    return Text;
}

bool G3AFileMd5Equals(const FString& Relative, const FString& Expected)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *G3AProjectPath(Relative))) return false;
    FMD5 Md5;
    if (!Bytes.IsEmpty()) Md5.Update(Bytes.GetData(), Bytes.Num());
    uint8 Digest[16];
    Md5.Final(Digest);
    return BytesToHex(Digest, UE_ARRAY_COUNT(Digest)).Equals(Expected, ESearchCase::IgnoreCase);
}

bool RequireG3A(FAutomationTestBase* Test, bool bCondition, const FString& Message)
{
    if (!bCondition) Test->AddError(Message);
    return bCondition;
}

UWorld* CapitalWorld()
{
    return LoadObject<UWorld>(nullptr,
        TEXT("/Game/Maps/StageG3A_CapitalBlockout_PoC.StageG3A_CapitalBlockout_PoC"));
}

int32 CountCapitalActorsWithTag(const FName Tag)
{
    UWorld* World = CapitalWorld();
    int32 Count = 0;
    if (!World || !World->PersistentLevel) return Count;
    for (const AActor* Actor : World->PersistentLevel->Actors)
        if (Actor && Actor->ActorHasTag(TEXT("StageG3A")) && Actor->ActorHasTag(Tag)) ++Count;
    return Count;
}

int32 CountCapitalActorsOfClass(const UClass* Class)
{
    UWorld* World = CapitalWorld();
    int32 Count = 0;
    if (!World || !World->PersistentLevel || !Class) return Count;
    for (const AActor* Actor : World->PersistentLevel->Actors)
        if (Actor && Actor->IsA(Class)) ++Count;
    return Count;
}

bool NoG3AFileExceeds50Mb()
{
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files,
        *G3AProjectPath(TEXT("Content/StageG3A")), TEXT("*"), true, false);
    IFileManager::Get().FindFilesRecursive(Files,
        *G3AProjectPath(TEXT("SourceArt/StageG3A")), TEXT("*"), true, false);
    Files.Add(FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectDir(), TEXT("../Docs/Stage G-3A Capital Blockout 設計・実装指示書.pdf"))));
    constexpr int64 Limit = 50LL * 1024LL * 1024LL;
    if (Files.IsEmpty()) return false;
    for (const FString& File : Files)
    {
        const int64 Size = IFileManager::Get().FileSize(*File);
        if (Size < 0 || Size > Limit) return false;
    }
    return true;
}

void RecordG3AResult(int32 Category, bool bPassed)
{
    static TMap<int32, bool> Results;
    Results.Add(Category, bPassed);
    if (Results.Num() != 29) return;
    const TCHAR* Names[] = {
        TEXT("Capital Map"), TEXT("Reference Record"), TEXT("Outer Wall"),
        TEXT("South Gate"), TEXT("Main Street"), TEXT("Central Plaza"),
        TEXT("Royal Castle"), TEXT("Market District"), TEXT("Residential District"),
        TEXT("Noble District"), TEXT("Workshop District"), TEXT("Outskirts"),
        TEXT("Player Start"), TEXT("Single Guard"), TEXT("Dedicated Guard Skeleton"),
        TEXT("NavMesh Major Routes"), TEXT("South Gate Passage"), TEXT("Central Plaza Route"),
        TEXT("Castle Approach Route"), TEXT("Standard Camera"), TEXT("F6 Tactical Camera"),
        TEXT("Camera Collision"), TEXT("Player Fallback False"), TEXT("Player Asset Isolation"),
        TEXT("Guard Asset Isolation"), TEXT("Core And Save Isolation"),
        TEXT("No Oversized Background Asset"), TEXT("Package Default Map"),
        TEXT("Standard Camera Auto Follow") };
    TArray<FString> Lines;
    int32 Passed = 0;
    for (int32 Index = 1; Index <= 29; ++Index)
    {
        const bool bResult = Results.FindRef(Index);
        Passed += bResult ? 1 : 0;
        Lines.Add(FString::Printf(TEXT("G3A-%02d | %s | %s"), Index,
            bResult ? TEXT("PASS") : TEXT("FAIL"), Names[Index - 1]));
    }
    Lines.Add(FString::Printf(TEXT("SUMMARY | %d/29 tests passed"), Passed));
    const FString Output = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectDir(), TEXT("../out/stage-g3a-capital-blockout/test-output")));
    IFileManager::Get().MakeDirectory(*Output, true);
    FFileHelper::SaveStringArrayToFile(Lines,
        *FPaths::Combine(Output, TEXT("stage_g3a_capital_automation_results.txt")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool RunG3ACheck(FAutomationTestBase* Test, int32 Category)
{
    UWorld* World = CapitalWorld();
    const FString Design = ReadG3AFile(TEXT("Docs/StageG3A_CapitalBlockout_Design.md"));
    const FString MapEvidence = ReadG3AFile(
        TEXT("SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/map_generation_evidence.json"));
    const FString RuntimeEvidence = ReadG3AFile(
        TEXT("SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/capital_runtime_evidence.json"));
    const FString NavEvidence = ReadG3AFile(
        TEXT("SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/navmesh_route_evidence.json"));
    const FString CameraEvidence = ReadG3AFile(
        TEXT("SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/camera_collision_evidence.json"));
    const FString IsolationEvidence = ReadG3AFile(
        TEXT("SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/asset_isolation_evidence.json"));
    const FString AutoFollowEvidence = ReadG3AFile(
        TEXT("SourceArt/StageG3A/CapitalBlockout/v0.1/Reports/camera_autofollow_evidence.json"));
    const FString WindowsEngine = ReadG3AFile(TEXT("Config/Windows/WindowsEngine.ini"));
    const FString DefaultGame = ReadG3AFile(TEXT("Config/DefaultGame.ini"));
    const FString WindowsInput = ReadG3AFile(TEXT("Config/Windows/WindowsInput.ini"));

    switch (Category)
    {
    case 1:
        return RequireG3A(Test, World && FPaths::FileExists(G3AProjectPath(
            TEXT("Content/Maps/StageG3A_CapitalBlockout_PoC.umap"))),
            TEXT("G3A-01 dedicated capital blockout map must exist and load"));
    case 2:
        return RequireG3A(Test,
            Design.Contains(TEXT("ChatGPT Image 2026年7月11日 21_50_21.png")) &&
            MapEvidence.Contains(TEXT("21_50_21.png")) &&
            MapEvidence.Contains(TEXT("2.5D character and fang-rat rendering specifications")),
            TEXT("G3A-02 approved reference path, usage, and exclusions must be recorded"));
    case 3:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("OuterWall")) >= 5,
            TEXT("G3A-03 capital outer wall must contain all four sides and the divided south wall"));
    case 4:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("SouthGate")) >= 6,
            TEXT("G3A-04 south gate must include the opening, towers, wall, and guard marker"));
    case 5:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("MainStreet")) >= 3,
            TEXT("G3A-05 main street and cross-street surfaces must exist"));
    case 6:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("CentralPlaza")) >= 4,
            TEXT("G3A-06 central plaza, center marker, and limited magic markers must exist"));
    case 7:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("Castle")) >= 5,
            TEXT("G3A-07 royal castle block and towers must exist"));
    case 8:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("Market")) >= 8,
            TEXT("G3A-08 market district surface, stalls, and commercial blocks must exist"));
    case 9:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("Residential")) >= 8,
            TEXT("G3A-09 residential blocks and a readable side-street must exist"));
    case 10:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("Noble")) >= 5,
            TEXT("G3A-10 noble district and estate blocks must exist"));
    case 11:
        return RequireG3A(Test, CountCapitalActorsWithTag(TEXT("Workshop")) >= 6,
            TEXT("G3A-11 workshop buildings, warehouses, and chimneys must exist"));
    case 12:
        return RequireG3A(Test,
            CountCapitalActorsWithTag(TEXT("Outskirts")) >= 10 &&
            CountCapitalActorsWithTag(TEXT("Farmland")) >= 3 &&
            CountCapitalActorsWithTag(TEXT("ForestMarker")) >= 5,
            TEXT("G3A-12 outskirts, farmland, and sparse forest markers must exist"));
    case 13:
        return RequireG3A(Test, CountCapitalActorsOfClass(APlayerStart::StaticClass()) == 1 &&
            CountCapitalActorsWithTag(TEXT("PlayerStart")) == 1,
            TEXT("G3A-13 one capital-entry PlayerStart must exist"));
    case 14:
        return RequireG3A(Test, CountCapitalActorsOfClass(AStageG2BGuardMActor::StaticClass()) == 1 &&
            RuntimeEvidence.Contains(TEXT("\"guard_actor_count\": 1")),
            TEXT("G3A-14 exactly one accepted GUARD_M must be present"));
    case 15:
    {
        USkeletalMesh* GuardMesh = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Mesh/SK_GUARD_M_Meshy_v0_1.SK_GUARD_M_Meshy_v0_1"));
        USkeletalMesh* PlayerMesh = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.SK_PLAYER_M_Meshy_v0_1"));
        return RequireG3A(Test, GuardMesh && PlayerMesh &&
            GuardMesh->GetSkeleton() != PlayerMesh->GetSkeleton() &&
            IsolationEvidence.Contains(TEXT("\"guard_player_skeleton_shared\": false")),
            TEXT("G3A-15 GUARD_M must keep its dedicated Skeleton"));
    }
    case 16:
        return RequireG3A(Test,
            NavEvidence.Contains(TEXT("\"route_count\": 10")) &&
            NavEvidence.Contains(TEXT("\"passed_route_count\": 10")) &&
            NavEvidence.Contains(TEXT("\"all_routes_passed\": true")) &&
            NavEvidence.Contains(TEXT("\"click_move_surface_passed\": true")) &&
            NavEvidence.Contains(TEXT("\"actual_player_movement_passed\": true")),
            TEXT("G3A-16 all ten routes, the cursor ground trace, and actual Player movement must pass"));
    case 17:
        return RequireG3A(Test,
            NavEvidence.Contains(TEXT("player_start_to_south_gate")) &&
            NavEvidence.Contains(TEXT("south_gate_to_main_street")) &&
            RuntimeEvidence.Contains(TEXT("\"click_trace_passed_count\": 6")) &&
            RuntimeEvidence.Contains(TEXT("\"player_move_command_issued\": true")) &&
            CountCapitalActorsWithTag(TEXT("SouthGate")) > 0,
            TEXT("G3A-17 the real click-move surface and south-gate movement command must pass"));
    case 18:
        return RequireG3A(Test,
            NavEvidence.Contains(TEXT("main_street_to_central_plaza")) &&
            RuntimeEvidence.Contains(TEXT("\"south_gate_plaza_castle_traversal_passed\": true")) &&
            CountCapitalActorsWithTag(TEXT("CentralPlaza")) > 0,
            TEXT("G3A-18 Player must reach the central plaza through NavMesh"));
    case 19:
        return RequireG3A(Test,
            NavEvidence.Contains(TEXT("central_plaza_to_castle_approach")) &&
            CountCapitalActorsWithTag(TEXT("CastleApproach")) > 0,
            TEXT("G3A-19 Player must reach the castle approach through NavMesh"));
    case 20:
        return RequireG3A(Test,
            CountCapitalActorsOfClass(AStageG2ACameraModeAdapter::StaticClass()) == 1 &&
            G3AFileMd5Equals(TEXT("Content/Maps/StageG2A_CameraRedesignPoC.umap"),
                TEXT("A75793F1EC4461827BACED1146AC1F11")) &&
            CameraEvidence.Contains(TEXT("\"standard_initial\": true")),
            TEXT("G3A-20 accepted StandardCharacterCamera must remain present and unchanged"));
    case 21:
        return RequireG3A(Test,
            WindowsInput.Contains(TEXT("StageG2AToggleCameraMode\",Key=F6")) &&
            CameraEvidence.Contains(TEXT("\"tactical_reached\": true")) &&
            CameraEvidence.Contains(TEXT("\"returned_to_standard\": true")),
            TEXT("G3A-21 F6 TacticalOverlook round-trip must remain available"));
    case 22:
        return RequireG3A(Test,
            CameraEvidence.Contains(TEXT("\"camera_collision_contract\": true")) &&
            CameraEvidence.Contains(TEXT("\"camera_blocking_actor_count\"")),
            TEXT("G3A-22 wall and building fixtures must block the Camera channel"));
    case 23:
        return RequireG3A(Test,
            RuntimeEvidence.Contains(TEXT("\"player_fallback\": false")),
            TEXT("G3A-23 PLAYER_M must never silently fall back to Manny"));
    case 24:
        return RequireG3A(Test,
            G3AFileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.uasset"), TEXT("D8A6A679D3D610F347370E6DB88193B7")) &&
            G3AFileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Skeleton/SKEL_PLAYER_M_Meshy_v0_1.uasset"), TEXT("65BA407A472D5D9E3A98D60E228CF5E6")) &&
            G3AFileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/IDLE/A_PLAYER_M_Meshy_v0_1_IDLE_Idle11.uasset"), TEXT("A3AFAF726450416E4357C79FD66AFB79")),
            TEXT("G3A-24 accepted PLAYER_M Mesh, Skeleton, and animation must remain byte-identical"));
    case 25:
        return RequireG3A(Test,
            G3AFileMd5Equals(TEXT("Content/StageG2B/Characters/GUARD_M/MeshyV01/Mesh/SK_GUARD_M_Meshy_v0_1.uasset"), TEXT("4E5DE1B35806252B0F71BFDA4D3BF2A7")) &&
            G3AFileMd5Equals(TEXT("Content/StageG2B/Characters/GUARD_M/MeshyV01/Skeleton/SKEL_GUARD_M_Meshy_v0_1.uasset"), TEXT("CECAA674BC42F251A6359B99CE7D3E3E")) &&
            G3AFileMd5Equals(TEXT("Content/StageG2B/Characters/GUARD_M/MeshyV01/Animations/GUARD_M_Meshy_v0_1_AllAnimationsIdle_11_frame_rate_60_fbx.uasset"), TEXT("A38BF83CCBDF8821A7086F1D9919693D")),
            TEXT("G3A-25 accepted GUARD_M Mesh, Skeleton, and Idle_11 must remain byte-identical"));
    case 26:
        return RequireG3A(Test,
            G3AFileMd5Equals(TEXT("Source/NationSimulationStageC/Private/StageD/NationSimulationGameInstanceSubsystem.cpp"), TEXT("44C777A4C57A4E0508A1ACF214FC4A59")) &&
            G3AFileMd5Equals(TEXT("../data/stage_a_fixture.json"), TEXT("6883C1D97FF6763E07D7F37BF3A163F0")) &&
            G3AFileMd5Equals(TEXT("Data/StageE/stage_e_state_definitions.json"), TEXT("0DD4238AB25CB35518FF1D1DB854317C")) &&
            G3AFileMd5Equals(TEXT("Data/StageF/stage_f_save_schema.json"), TEXT("C33F613C72F414CFB76E597CF51DCC95")),
            TEXT("G3A-26 GameInstanceSubsystem, causal fixture, Stage E, and save schema must remain unchanged"));
    case 27:
        return RequireG3A(Test, NoG3AFileExceeds50Mb() &&
            MapEvidence.Contains(TEXT("\"external_assets\": 0")) &&
            MapEvidence.Contains(TEXT("\"engine_primitives_only\": true")),
            TEXT("G3A-27 no new G-3A background asset may exceed 50MB or use external art"));
    case 28:
        return RequireG3A(Test,
            WindowsEngine.Contains(TEXT("GameDefaultMap=/Game/Maps/StageG3A_CapitalBlockout_PoC")) &&
            DefaultGame.Contains(TEXT("DirectoriesToAlwaysCook=(Path=\"/Game/StageG3A\")")) &&
            FPaths::FileExists(G3AProjectPath(TEXT("Content/Maps/StageG3A_CapitalBlockout_PoC.umap"))),
            TEXT("G3A-28 no-argument Package must select and cook the G-3A capital map"));
    case 29:
    {
        const float WrappedYaw = AStageG3AStandardCameraAutoFollowActor::InterpolateYaw(
            350.0f, 10.0f, 0.1f, 5.0f);
        return RequireG3A(Test,
            CountCapitalActorsOfClass(AStageG3AStandardCameraAutoFollowActor::StaticClass()) == 1 &&
            DefaultGame.Contains(TEXT("YawInterpSpeed=5.0")) &&
            DefaultGame.Contains(TEXT("ManualOverrideSeconds=1.25")) &&
            FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::NormalizeYaw(WrappedYaw), 0.0f, 0.01f) &&
            !AStageG3AStandardCameraAutoFollowActor::ShouldBlockForUiInput(true, false) &&
            AStageG3AStandardCameraAutoFollowActor::ShouldBlockForUiInput(true, true) &&
            AutoFollowEvidence.Contains(TEXT("\"standard_autofollow_observed\": true")) &&
            AutoFollowEvidence.Contains(TEXT("\"manual_drag_priority_observed\": true")) &&
            AutoFollowEvidence.Contains(TEXT("\"manual_release_hold_observed\": true")) &&
            AutoFollowEvidence.Contains(TEXT("\"standard_autofollow_resumed_after_drag\": true")) &&
            AutoFollowEvidence.Contains(TEXT("\"tactical_autofollow_disabled\": true")) &&
            AutoFollowEvidence.Contains(TEXT("\"return_standard_autofollow_observed\": true")),
            TEXT("G3A-29 Standard camera must auto-follow with manual override and Tactical exclusion"));
    }
    default:
        Test->AddError(TEXT("Unknown Stage G-3A test category"));
        return false;
    }
}
}

#define G3A_CAPITAL_TEST(ClassName, TestName, Category) \
    IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, \
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
    bool ClassName::RunTest(const FString&) { const bool bPassed = RunG3ACheck(this, Category); \
        RecordG3AResult(Category, bPassed); return bPassed; }

G3A_CAPITAL_TEST(FG3A01, "NationSimulation.StageG3A.Capital.01.Map", 1)
G3A_CAPITAL_TEST(FG3A02, "NationSimulation.StageG3A.Capital.02.Reference", 2)
G3A_CAPITAL_TEST(FG3A03, "NationSimulation.StageG3A.Capital.03.OuterWall", 3)
G3A_CAPITAL_TEST(FG3A04, "NationSimulation.StageG3A.Capital.04.SouthGate", 4)
G3A_CAPITAL_TEST(FG3A05, "NationSimulation.StageG3A.Capital.05.MainStreet", 5)
G3A_CAPITAL_TEST(FG3A06, "NationSimulation.StageG3A.Capital.06.CentralPlaza", 6)
G3A_CAPITAL_TEST(FG3A07, "NationSimulation.StageG3A.Capital.07.Castle", 7)
G3A_CAPITAL_TEST(FG3A08, "NationSimulation.StageG3A.Capital.08.Market", 8)
G3A_CAPITAL_TEST(FG3A09, "NationSimulation.StageG3A.Capital.09.Residential", 9)
G3A_CAPITAL_TEST(FG3A10, "NationSimulation.StageG3A.Capital.10.Noble", 10)
G3A_CAPITAL_TEST(FG3A11, "NationSimulation.StageG3A.Capital.11.Workshop", 11)
G3A_CAPITAL_TEST(FG3A12, "NationSimulation.StageG3A.Capital.12.Outskirts", 12)
G3A_CAPITAL_TEST(FG3A13, "NationSimulation.StageG3A.Capital.13.PlayerStart", 13)
G3A_CAPITAL_TEST(FG3A14, "NationSimulation.StageG3A.Capital.14.SingleGuard", 14)
G3A_CAPITAL_TEST(FG3A15, "NationSimulation.StageG3A.Capital.15.GuardSkeleton", 15)
G3A_CAPITAL_TEST(FG3A16, "NationSimulation.StageG3A.Capital.16.NavRoutes", 16)
G3A_CAPITAL_TEST(FG3A17, "NationSimulation.StageG3A.Capital.17.GatePassage", 17)
G3A_CAPITAL_TEST(FG3A18, "NationSimulation.StageG3A.Capital.18.PlazaRoute", 18)
G3A_CAPITAL_TEST(FG3A19, "NationSimulation.StageG3A.Capital.19.CastleRoute", 19)
G3A_CAPITAL_TEST(FG3A20, "NationSimulation.StageG3A.Capital.20.StandardCamera", 20)
G3A_CAPITAL_TEST(FG3A21, "NationSimulation.StageG3A.Capital.21.TacticalF6", 21)
G3A_CAPITAL_TEST(FG3A22, "NationSimulation.StageG3A.Capital.22.CameraCollision", 22)
G3A_CAPITAL_TEST(FG3A23, "NationSimulation.StageG3A.Capital.23.PlayerFallback", 23)
G3A_CAPITAL_TEST(FG3A24, "NationSimulation.StageG3A.Capital.24.PlayerIsolation", 24)
G3A_CAPITAL_TEST(FG3A25, "NationSimulation.StageG3A.Capital.25.GuardIsolation", 25)
G3A_CAPITAL_TEST(FG3A26, "NationSimulation.StageG3A.Capital.26.CoreSaveIsolation", 26)
G3A_CAPITAL_TEST(FG3A27, "NationSimulation.StageG3A.Capital.27.AssetSize", 27)
G3A_CAPITAL_TEST(FG3A28, "NationSimulation.StageG3A.Capital.28.PackageMap", 28)
G3A_CAPITAL_TEST(FG3A29, "NationSimulation.StageG3A.Capital.29.StandardCameraAutoFollow", 29)

#undef G3A_CAPITAL_TEST

#endif
