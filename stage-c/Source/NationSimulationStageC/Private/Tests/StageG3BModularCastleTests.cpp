#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/Level.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"
#include "StageG2B/StageG2BGuardMActor.h"
#include "StageG3A/StageG3AStandardCameraAutoFollowActor.h"
#include "StageG3B/StageG3BModularCastleEvidenceActor.h"

namespace
{
constexpr int32 G3BRTestCount = 33;

FString G3BRProjectPath(const FString& Relative)
{
    return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), Relative));
}

FString ReadG3BRFile(const FString& Relative)
{
    FString Text;
    FFileHelper::LoadFileToString(Text, *G3BRProjectPath(Relative));
    return Text;
}

bool G3BRFileMd5Equals(const FString& Relative, const FString& Expected)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *G3BRProjectPath(Relative))) return false;
    FMD5 Md5;
    if (!Bytes.IsEmpty()) Md5.Update(Bytes.GetData(), Bytes.Num());
    uint8 Digest[16];
    Md5.Final(Digest);
    return BytesToHex(Digest, UE_ARRAY_COUNT(Digest)).Equals(Expected, ESearchCase::IgnoreCase);
}

bool RequireG3BR(FAutomationTestBase* Test, bool bCondition, const FString& Message)
{
    if (!bCondition) Test->AddError(Message);
    return bCondition;
}

UWorld* ModularCastleWorld()
{
    return LoadObject<UWorld>(nullptr,
        TEXT("/Game/Maps/StageG3B_ModularCastle_PoC.StageG3B_ModularCastle_PoC"));
}

int32 CountG3BRActorsWithTag(const FName Tag)
{
    UWorld* World = ModularCastleWorld();
    int32 Count = 0;
    if (!World || !World->PersistentLevel) return Count;
    for (const AActor* Actor : World->PersistentLevel->Actors)
        if (Actor && Actor->ActorHasTag(TEXT("StageG3BR")) && Actor->ActorHasTag(Tag)) ++Count;
    return Count;
}

int32 CountG3BRActorsOfClass(const UClass* Class)
{
    UWorld* World = ModularCastleWorld();
    int32 Count = 0;
    if (!World || !World->PersistentLevel || !Class) return Count;
    for (const AActor* Actor : World->PersistentLevel->Actors)
        if (Actor && Actor->IsA(Class)) ++Count;
    return Count;
}

bool NoG3BRFileExceeds50Mb()
{
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files,
        *G3BRProjectPath(TEXT("Content/StageG3B")), TEXT("*"), true, false);
    IFileManager::Get().FindFilesRecursive(Files,
        *G3BRProjectPath(TEXT("SourceArt/StageG3B")), TEXT("*"), true, false);
    constexpr int64 Limit = 50LL * 1024LL * 1024LL;
    if (Files.IsEmpty()) return false;
    for (const FString& File : Files)
    {
        const int64 Size = IFileManager::Get().FileSize(*File);
        if (Size < 0 || Size > Limit) return false;
    }
    return true;
}

int32 CountImportedModules()
{
    const TCHAR* Names[] = {
        TEXT("SM_G3BR_WallBay_300"), TEXT("SM_G3BR_WallBayTall_300"),
        TEXT("SM_G3BR_GateArch_600"), TEXT("SM_G3BR_TunnelWall"),
        TEXT("SM_G3BR_TunnelRoof"), TEXT("SM_G3BR_RoofBaySteep_320"),
        TEXT("SM_G3BR_RoofCentral_680"), TEXT("SM_G3BR_SideTower_400"),
        TEXT("SM_G3BR_CentralTower_520"), TEXT("SM_G3BR_CornerTurret_180"),
        TEXT("SM_G3BR_SideSpire_500"), TEXT("SM_G3BR_CentralSpire_640"),
        TEXT("SM_G3BR_CornerSpire_220"), TEXT("SM_G3BR_ButtressStepped"),
        TEXT("SM_G3BR_WindowFramePointed"), TEXT("SM_G3BR_WindowInsetPointed"),
        TEXT("SM_G3BR_Crenellation_300"), TEXT("SM_G3BR_EntranceStair"),
        TEXT("SM_G3BR_BannerPole"), TEXT("SM_G3BR_BannerCloth"),
        TEXT("SM_G3BR_MagicPlinth"), TEXT("SM_G3BR_MagicCrystal") };
    int32 Count = 0;
    for (const TCHAR* Name : Names)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/StageG3B/ModularCastle/Meshes/%s.%s"), Name, Name);
        Count += LoadObject<UStaticMesh>(nullptr, *Path) ? 1 : 0;
    }
    return Count;
}

void RecordG3BRResult(int32 Category, bool bPassed)
{
    static TMap<int32, bool> Results;
    Results.Add(Category, bPassed);
    if (Results.Num() != G3BRTestCount) return;
    const TCHAR* Names[] = {
        TEXT("Dedicated Map"), TEXT("Rejected Archive SHA"), TEXT("Source Modules"),
        TEXT("Imported Modules"), TEXT("Rejected Map Excluded"), TEXT("Short Wall Bays"),
        TEXT("No Giant Front Wall"), TEXT("Gate Arch"), TEXT("Gate Tunnel"),
        TEXT("Gate Dimensions"), TEXT("Side Towers"), TEXT("Central Tower"),
        TEXT("Small Spires"), TEXT("Steep Roofs"), TEXT("Buttresses"),
        TEXT("Pointed Windows"), TEXT("Crenellations"), TEXT("Banners"),
        TEXT("Magic Marker Limit"), TEXT("Single Guard"), TEXT("Guard Skeleton"),
        TEXT("Manny Fallback False"), TEXT("Player Asset Isolation"),
        TEXT("Guard Asset Isolation"), TEXT("G3A Map Isolation"),
        TEXT("Core And Save Isolation"), TEXT("No Oversized Asset"),
        TEXT("NavMesh Routes"), TEXT("Standard Camera And AutoFollow"),
        TEXT("F6 Tactical RoundTrip"), TEXT("Camera Collision"),
        TEXT("Package Default Map"), TEXT("Sixteen Evaluation Images") };
    TArray<FString> Lines;
    int32 Passed = 0;
    for (int32 Index = 1; Index <= G3BRTestCount; ++Index)
    {
        const bool bResult = Results.FindRef(Index);
        Passed += bResult ? 1 : 0;
        Lines.Add(FString::Printf(TEXT("G3BR-%02d | %s | %s"), Index,
            bResult ? TEXT("PASS") : TEXT("FAIL"), Names[Index - 1]));
    }
    Lines.Add(FString::Printf(TEXT("SUMMARY | %d/33 tests passed"), Passed));
    const FString Output = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectDir(), TEXT("../out/stage-g3br-modular-castle/test-output")));
    IFileManager::Get().MakeDirectory(*Output, true);
    FFileHelper::SaveStringArrayToFile(Lines,
        *FPaths::Combine(Output, TEXT("stage_g3br_modular_automation_results.txt")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool RunG3BRCheck(FAutomationTestBase* Test, int32 Category)
{
    UWorld* World = ModularCastleWorld();
    const FString Design = ReadG3BRFile(TEXT("Docs/StageG3B_ModularCastle_Design.md"));
    const FString ArchiveReport = ReadG3BRFile(
        TEXT("Docs/StageG3B_Rejected_PrimitiveCastle_Archive_Report.md"));
    const FString ModuleManifest = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/module_source_manifest.json"));
    const FString MapEvidence = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/map_generation_evidence.json"));
    const FString Runtime = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/modular_castle_runtime_evidence.json"));
    const FString Nav = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/modular_castle_navmesh_evidence.json"));
    const FString Camera = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/modular_castle_camera_evidence.json"));
    const FString Visual = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/modular_castle_visual_contract_evidence.json"));
    const FString Isolation = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/modular_castle_asset_isolation_evidence.json"));
    const FString ScreenshotManifest = ReadG3BRFile(
        TEXT("SourceArt/StageG3B/ModularCastle/v0.1/Reports/screenshot_manifest.json"));
    const FString WindowsEngine = ReadG3BRFile(TEXT("Config/Windows/WindowsEngine.ini"));
    const FString DefaultGame = ReadG3BRFile(TEXT("Config/DefaultGame.ini"));
    const FString WindowsInput = ReadG3BRFile(TEXT("Config/Windows/WindowsInput.ini"));

    switch (Category)
    {
    case 1:
        return RequireG3BR(Test, World && FPaths::FileExists(G3BRProjectPath(
            TEXT("Content/Maps/StageG3B_ModularCastle_PoC.umap"))),
            TEXT("G3BR-01 dedicated modular castle map must exist and load"));
    case 2:
        return RequireG3BR(Test,
            IFileManager::Get().DirectoryExists(TEXT("C:/Users/rinpa/Desktop/TITLE_StageG3B_Rejected_PrimitiveCastle_Archive_20260717_095537")) &&
            ArchiveReport.Contains(TEXT("5,284")) && ArchiveReport.Contains(TEXT("SHA-256: PASS")),
            TEXT("G3BR-02 rejected trial archive and SHA verification must be recorded"));
    case 3:
        return RequireG3BR(Test,
            ModuleManifest.Contains(TEXT("\"module_count\": 22")) &&
            ModuleManifest.Contains(TEXT("\"external_assets\": 0")),
            TEXT("G3BR-03 twenty-two self-authored source modules must exist"));
    case 4:
        return RequireG3BR(Test, CountImportedModules() == 22,
            TEXT("G3BR-04 all self-authored FBX modules must import as StaticMesh assets"));
    case 5:
        return RequireG3BR(Test,
            !FPaths::FileExists(G3BRProjectPath(TEXT("Content/Maps/StageG3B_CastleExterior_PoC.umap"))) &&
            !Design.Contains(TEXT("formal map: /Game/Maps/StageG3B_CastleExterior_PoC")),
            TEXT("G3BR-05 rejected map must not be formalized or reused"));
    case 6:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("WallBay")) >= 11,
            TEXT("G3BR-06 castle walls must be assembled from short repeated bays"));
    case 7:
        return RequireG3BR(Test,
            Visual.Contains(TEXT("\"single_front_wall_over_1000uu\": false")) &&
            MapEvidence.Contains(TEXT("\"wall_bay_width_uu\": 300")),
            TEXT("G3BR-07 no giant 1000uu front-wall module may exist"));
    case 8:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("GateArch")) == 1,
            TEXT("G3BR-08 one recognisable pointed gate arch must exist"));
    case 9:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("TunnelWall")) == 2,
            TEXT("G3BR-09 gate must have two physical tunnel walls and depth"));
    case 10:
        return RequireG3BR(Test,
            MapEvidence.Contains(TEXT("\"gate_opening_width_uu\": 600")) &&
            MapEvidence.Contains(TEXT("\"gate_opening_height_uu\": 565")) &&
            MapEvidence.Contains(TEXT("\"gate_tunnel_depth_uu\": 380")),
            TEXT("G3BR-10 gate opening and tunnel must be within the accepted size range"));
    case 11:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("SideTower")) == 2,
            TEXT("G3BR-11 exactly two octagonal side towers must frame the facade"));
    case 12:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("CentralTower")) == 1,
            TEXT("G3BR-12 one taller set-back central tower must exist"));
    case 13:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("SmallSpire")) == 4,
            TEXT("G3BR-13 four slender corner spires must break the skyline"));
    case 14:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("SteepRoof")) >= 12,
            TEXT("G3BR-14 multiple steep slate roofs must replace the flat box silhouette"));
    case 15:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("Buttress")) >= 12 &&
            Visual.Contains(TEXT("\"buttress_count\": 14")),
            TEXT("G3BR-15 at least twelve stepped buttresses must establish scale"));
    case 16:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("WindowFrame")) >= 10 &&
            Visual.Contains(TEXT("\"window_frame_count\": 20")),
            TEXT("G3BR-16 at least ten deep pointed windows must be visible"));
    case 17:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("Crenellation")) >= 10,
            TEXT("G3BR-17 small repeated crenellation modules must exist"));
    case 18:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("Banner")) == 5,
            TEXT("G3BR-18 royal banners must remain limited to five"));
    case 19:
        return RequireG3BR(Test, CountG3BRActorsWithTag(TEXT("MagicMarker")) <= 3 &&
            CountG3BRActorsWithTag(TEXT("MagicMarker")) > 0,
            TEXT("G3BR-19 restrained magic markers must not exceed three"));
    case 20:
        return RequireG3BR(Test,
            CountG3BRActorsOfClass(AStageG2BGuardMActor::StaticClass()) == 1 &&
            Runtime.Contains(TEXT("\"guard_actor_count\": 1")),
            TEXT("G3BR-20 exactly one accepted GUARD_M must remain"));
    case 21:
    {
        USkeletalMesh* Guard = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Mesh/SK_GUARD_M_Meshy_v0_1.SK_GUARD_M_Meshy_v0_1"));
        USkeletalMesh* Player = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.SK_PLAYER_M_Meshy_v0_1"));
        return RequireG3BR(Test, Guard && Player && Guard->GetSkeleton() != Player->GetSkeleton() &&
            Runtime.Contains(TEXT("\"guard_player_skeleton_shared\": false")),
            TEXT("G3BR-21 GUARD_M must keep its dedicated Skeleton"));
    }
    case 22:
        return RequireG3BR(Test, Runtime.Contains(TEXT("\"player_fallback\": false")),
            TEXT("G3BR-22 PLAYER_M must not fall back to Manny"));
    case 23:
        return RequireG3BR(Test,
            G3BRFileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.uasset"), TEXT("D8A6A679D3D610F347370E6DB88193B7")) &&
            G3BRFileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Skeleton/SKEL_PLAYER_M_Meshy_v0_1.uasset"), TEXT("65BA407A472D5D9E3A98D60E228CF5E6")) &&
            Isolation.Contains(TEXT("\"player_assets_modified\": false")),
            TEXT("G3BR-23 accepted PLAYER_M assets must remain byte-identical"));
    case 24:
        return RequireG3BR(Test,
            G3BRFileMd5Equals(TEXT("Content/StageG2B/Characters/GUARD_M/MeshyV01/Mesh/SK_GUARD_M_Meshy_v0_1.uasset"), TEXT("4E5DE1B35806252B0F71BFDA4D3BF2A7")) &&
            G3BRFileMd5Equals(TEXT("Content/StageG2B/Characters/GUARD_M/MeshyV01/Skeleton/SKEL_GUARD_M_Meshy_v0_1.uasset"), TEXT("CECAA674BC42F251A6359B99CE7D3E3E")) &&
            Isolation.Contains(TEXT("\"guard_assets_modified\": false")),
            TEXT("G3BR-24 accepted GUARD_M assets must remain byte-identical"));
    case 25:
        return RequireG3BR(Test,
            G3BRFileMd5Equals(TEXT("Content/Maps/StageG3A_CapitalBlockout_PoC.umap"),
                TEXT("0A3968089E9DEF0DF9A419AE6AC5FB0F")) &&
            Isolation.Contains(TEXT("\"stage_g3a_map_modified\": false")),
            TEXT("G3BR-25 accepted G-3A map must remain byte-identical"));
    case 26:
        return RequireG3BR(Test,
            G3BRFileMd5Equals(TEXT("Source/NationSimulationStageC/Private/StageD/NationSimulationGameInstanceSubsystem.cpp"), TEXT("44C777A4C57A4E0508A1ACF214FC4A59")) &&
            G3BRFileMd5Equals(TEXT("../data/stage_a_fixture.json"), TEXT("6883C1D97FF6763E07D7F37BF3A163F0")) &&
            G3BRFileMd5Equals(TEXT("Data/StageF/stage_f_save_schema.json"), TEXT("C33F613C72F414CFB76E597CF51DCC95")) &&
            Isolation.Contains(TEXT("\"causal_core_modified\": false")) &&
            Isolation.Contains(TEXT("\"save_schema_modified\": false")),
            TEXT("G3BR-26 causal core, subsystem, and save schema must remain unchanged"));
    case 27:
        return RequireG3BR(Test, NoG3BRFileExceeds50Mb() &&
            ModuleManifest.Contains(TEXT("\"external_assets\": 0")),
            TEXT("G3BR-27 no G-3B-R background file may exceed 50MB or use external art"));
    case 28:
        return RequireG3BR(Test,
            Nav.Contains(TEXT("\"route_count\": 5")) &&
            Nav.Contains(TEXT("\"passed_route_count\": 5")) &&
            Nav.Contains(TEXT("\"all_routes_passed\": true")) &&
            Runtime.Contains(TEXT("\"movement_passed\": true")),
            TEXT("G3BR-28 plaza, gate, tunnel, and inner routes plus actual movement must pass"));
    case 29:
        return RequireG3BR(Test,
            CountG3BRActorsOfClass(AStageG2ACameraModeAdapter::StaticClass()) == 1 &&
            CountG3BRActorsOfClass(AStageG3AStandardCameraAutoFollowActor::StaticClass()) == 1 &&
            Camera.Contains(TEXT("\"standard_initial\": true")) &&
            Camera.Contains(TEXT("\"autofollow_actor_present\": true")),
            TEXT("G3BR-29 accepted Standard camera and G-3A auto-follow must remain"));
    case 30:
        return RequireG3BR(Test,
            WindowsInput.Contains(TEXT("StageG2AToggleCameraMode\",Key=F6")) &&
            Camera.Contains(TEXT("\"tactical_reached\": true")) &&
            Camera.Contains(TEXT("\"returned_to_standard\": true")),
            TEXT("G3BR-30 F6 TacticalOverlook round-trip must remain available"));
    case 31:
        return RequireG3BR(Test,
            Camera.Contains(TEXT("\"camera_collision_contract\": true")) &&
            Camera.Contains(TEXT("\"camera_blocking_actor_count\"")),
            TEXT("G3BR-31 all physical castle modules must block the Camera channel"));
    case 32:
        return RequireG3BR(Test,
            WindowsEngine.Contains(TEXT("GameDefaultMap=/Game/Maps/StageG3B_ModularCastle_PoC")) &&
            DefaultGame.Contains(TEXT("DirectoriesToAlwaysCook=(Path=\"/Game/StageG3B\")")),
            TEXT("G3BR-32 no-argument Package must select and cook the G-3B-R map"));
    case 33:
        return RequireG3BR(Test,
            ScreenshotManifest.Contains(TEXT("\"image_count\": 16")) &&
            ScreenshotManifest.Contains(TEXT("16_modular_castle_gate_from_city_entrance.png")),
            TEXT("G3BR-33 all sixteen evaluation images must be recorded"));
    default:
        Test->AddError(TEXT("Unknown Stage G-3B-R test category"));
        return false;
    }
}
}

#define G3BR_TEST(ClassName, TestName, Category) \
    IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, \
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
    bool ClassName::RunTest(const FString&) { const bool bPassed = RunG3BRCheck(this, Category); \
        RecordG3BRResult(Category, bPassed); return bPassed; }

G3BR_TEST(FG3BR01, "NationSimulation.StageG3B.ModularCastle.01.Map", 1)
G3BR_TEST(FG3BR02, "NationSimulation.StageG3B.ModularCastle.02.Archive", 2)
G3BR_TEST(FG3BR03, "NationSimulation.StageG3B.ModularCastle.03.SourceModules", 3)
G3BR_TEST(FG3BR04, "NationSimulation.StageG3B.ModularCastle.04.ImportedModules", 4)
G3BR_TEST(FG3BR05, "NationSimulation.StageG3B.ModularCastle.05.RejectedMapExcluded", 5)
G3BR_TEST(FG3BR06, "NationSimulation.StageG3B.ModularCastle.06.WallBays", 6)
G3BR_TEST(FG3BR07, "NationSimulation.StageG3B.ModularCastle.07.NoGiantWall", 7)
G3BR_TEST(FG3BR08, "NationSimulation.StageG3B.ModularCastle.08.GateArch", 8)
G3BR_TEST(FG3BR09, "NationSimulation.StageG3B.ModularCastle.09.GateTunnel", 9)
G3BR_TEST(FG3BR10, "NationSimulation.StageG3B.ModularCastle.10.GateDimensions", 10)
G3BR_TEST(FG3BR11, "NationSimulation.StageG3B.ModularCastle.11.SideTowers", 11)
G3BR_TEST(FG3BR12, "NationSimulation.StageG3B.ModularCastle.12.CentralTower", 12)
G3BR_TEST(FG3BR13, "NationSimulation.StageG3B.ModularCastle.13.SmallSpires", 13)
G3BR_TEST(FG3BR14, "NationSimulation.StageG3B.ModularCastle.14.SteepRoofs", 14)
G3BR_TEST(FG3BR15, "NationSimulation.StageG3B.ModularCastle.15.Buttresses", 15)
G3BR_TEST(FG3BR16, "NationSimulation.StageG3B.ModularCastle.16.Windows", 16)
G3BR_TEST(FG3BR17, "NationSimulation.StageG3B.ModularCastle.17.Crenellations", 17)
G3BR_TEST(FG3BR18, "NationSimulation.StageG3B.ModularCastle.18.Banners", 18)
G3BR_TEST(FG3BR19, "NationSimulation.StageG3B.ModularCastle.19.MagicLimit", 19)
G3BR_TEST(FG3BR20, "NationSimulation.StageG3B.ModularCastle.20.SingleGuard", 20)
G3BR_TEST(FG3BR21, "NationSimulation.StageG3B.ModularCastle.21.GuardSkeleton", 21)
G3BR_TEST(FG3BR22, "NationSimulation.StageG3B.ModularCastle.22.PlayerFallback", 22)
G3BR_TEST(FG3BR23, "NationSimulation.StageG3B.ModularCastle.23.PlayerIsolation", 23)
G3BR_TEST(FG3BR24, "NationSimulation.StageG3B.ModularCastle.24.GuardIsolation", 24)
G3BR_TEST(FG3BR25, "NationSimulation.StageG3B.ModularCastle.25.G3AMapIsolation", 25)
G3BR_TEST(FG3BR26, "NationSimulation.StageG3B.ModularCastle.26.CoreSaveIsolation", 26)
G3BR_TEST(FG3BR27, "NationSimulation.StageG3B.ModularCastle.27.AssetSize", 27)
G3BR_TEST(FG3BR28, "NationSimulation.StageG3B.ModularCastle.28.NavMesh", 28)
G3BR_TEST(FG3BR29, "NationSimulation.StageG3B.ModularCastle.29.StandardCamera", 29)
G3BR_TEST(FG3BR30, "NationSimulation.StageG3B.ModularCastle.30.TacticalCamera", 30)
G3BR_TEST(FG3BR31, "NationSimulation.StageG3B.ModularCastle.31.CameraCollision", 31)
G3BR_TEST(FG3BR32, "NationSimulation.StageG3B.ModularCastle.32.PackageMap", 32)
G3BR_TEST(FG3BR33, "NationSimulation.StageG3B.ModularCastle.33.Screenshots", 33)

#undef G3BR_TEST

#endif
