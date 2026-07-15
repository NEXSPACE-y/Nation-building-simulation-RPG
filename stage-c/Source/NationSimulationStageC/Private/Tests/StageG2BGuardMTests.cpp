#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Engine/Level.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "StageG1B/StageG1BSettings.h"
#include "StageG2B/StageG2BGuardMActor.h"

namespace
{
FString ProjectPath(const FString& Relative)
{
    return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), Relative));
}

FString ReadProjectFile(const FString& Relative)
{
    FString Text;
    FFileHelper::LoadFileToString(Text, *ProjectPath(Relative));
    return Text;
}

bool FileMd5Equals(const FString& Relative, const FString& Expected)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *ProjectPath(Relative))) return false;
    FMD5 Md5;
    if (!Bytes.IsEmpty()) Md5.Update(Bytes.GetData(), Bytes.Num());
    uint8 Digest[16];
    Md5.Final(Digest);
    return BytesToHex(Digest, UE_ARRAY_COUNT(Digest)).Equals(Expected, ESearchCase::IgnoreCase);
}

bool PngIs2048(const FString& Relative)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *ProjectPath(Relative)) || Bytes.Num() < 24) return false;
    const bool bSignature = Bytes[0] == 0x89 && Bytes[1] == 0x50 && Bytes[2] == 0x4E && Bytes[3] == 0x47;
    const uint32 Width = (static_cast<uint32>(Bytes[16]) << 24) |
        (static_cast<uint32>(Bytes[17]) << 16) | (static_cast<uint32>(Bytes[18]) << 8) | Bytes[19];
    const uint32 Height = (static_cast<uint32>(Bytes[20]) << 24) |
        (static_cast<uint32>(Bytes[21]) << 16) | (static_cast<uint32>(Bytes[22]) << 8) | Bytes[23];
    return bSignature && Width == 2048 && Height == 2048;
}

bool RequireG2B(FAutomationTestBase* Test, bool bCondition, const FString& Message)
{
    if (!bCondition) Test->AddError(Message);
    return bCondition;
}

USkeletalMesh* GuardMesh()
{
    return LoadObject<USkeletalMesh>(nullptr,
        TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Mesh/SK_GUARD_M_Meshy_v0_1.SK_GUARD_M_Meshy_v0_1"));
}

USkeleton* GuardSkeleton()
{
    return LoadObject<USkeleton>(nullptr,
        TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Skeleton/SKEL_GUARD_M_Meshy_v0_1.SKEL_GUARD_M_Meshy_v0_1"));
}

void RecordG2BResult(int32 Category, bool bPassed)
{
    static TMap<int32, bool> Results;
    Results.Add(Category, bPassed);
    if (Results.Num() != 18) return;
    const TCHAR* Names[] = {
        TEXT("Source ZIP Contract"), TEXT("Guard Mesh"), TEXT("Dedicated Skeleton"),
        TEXT("Bone Count"), TEXT("Root Bone"), TEXT("Material Slot"),
        TEXT("Texture Contract"), TEXT("Single Guard Placement"), TEXT("No Player Skeleton Sharing"),
        TEXT("Player Fallback False"), TEXT("Player Asset Isolation"), TEXT("Standard Camera Distance"),
        TEXT("Tactical Camera Preservation"), TEXT("F6 Toggle Preservation"), TEXT("Left Click Movement"),
        TEXT("Walk Run Dash Preservation"), TEXT("Destination NavPath Preservation"), TEXT("Package Default Map") };
    TArray<FString> Lines;
    int32 Passed = 0;
    for (int32 Index = 1; Index <= 18; ++Index)
    {
        const bool bResult = Results.FindRef(Index);
        Passed += bResult ? 1 : 0;
        Lines.Add(FString::Printf(TEXT("G2B-%02d | %s | %s"), Index,
            bResult ? TEXT("PASS") : TEXT("FAIL"), Names[Index - 1]));
    }
    Lines.Add(FString::Printf(TEXT("SUMMARY | %d/18 tests passed"), Passed));
    const FString Output = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectDir(), TEXT("../out/stage-g2b-guardm/test-output")));
    IFileManager::Get().MakeDirectory(*Output, true);
    FFileHelper::SaveStringArrayToFile(Lines,
        *FPaths::Combine(Output, TEXT("stage_g2b_guardm_automation_results.txt")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool RunG2BCheck(FAutomationTestBase* Test, int32 Category)
{
    USkeletalMesh* Mesh = GuardMesh();
    USkeleton* Skeleton = GuardSkeleton();
    const FString SourceManifest = ReadProjectFile(
        TEXT("SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Working/Manifest/GUARD_M_Meshy_v0.1_SourceManifest.json"));
    const FString ActorSource = ReadProjectFile(
        TEXT("Source/NationSimulationStageC/Private/StageG2B/StageG2BGuardMActor.cpp"));
    const FString WindowsInput = ReadProjectFile(TEXT("Config/Windows/WindowsInput.ini"));
    const FString DefaultInput = ReadProjectFile(TEXT("Config/DefaultInput.ini"));
    const FString PlayerSource = ReadProjectFile(
        TEXT("Source/NationSimulationStageC/Private/StageG1A/StageG1APlayerCharacter.cpp"));
    const FString WindowsEngine = ReadProjectFile(TEXT("Config/Windows/WindowsEngine.ini"));
    const FString DefaultGame = ReadProjectFile(TEXT("Config/DefaultGame.ini"));

    switch (Category)
    {
    case 1:
        return RequireG2B(Test,
            SourceManifest.Contains(TEXT("AC7DEB2B0238970E413B8784FF46941860F08C29C51F03F88A3FFC88BDDFD6A8")) &&
            SourceManifest.Contains(TEXT("\"internal_file_count\": 6")) &&
            SourceManifest.Contains(TEXT("\"validation_status\": \"PASS\"")) &&
            SourceManifest.Contains(TEXT("\"clean_pack_used\": false")),
            TEXT("G2B-01 approved GUARD_M ZIP SHA, six members, and clean-pack exclusion must be recorded"));
    case 2:
        return RequireG2B(Test, Mesh != nullptr &&
            Mesh->GetPathName().Contains(TEXT("/Game/StageG2B/Characters/GUARD_M/")),
            TEXT("G2B-02 GUARD_M SkeletalMesh must exist only under StageG2B"));
    case 3:
        return RequireG2B(Test, Mesh && Skeleton && Mesh->GetSkeleton() == Skeleton &&
            Skeleton->GetPathName().Contains(TEXT("SKEL_GUARD_M_Meshy_v0_1")),
            TEXT("G2B-03 GUARD_M must use its dedicated Skeleton"));
    case 4:
        return RequireG2B(Test, Mesh && Mesh->GetRefSkeleton().GetNum() == 24,
            TEXT("G2B-04 GUARD_M reference skeleton must contain 24 bones"));
    case 5:
        return RequireG2B(Test, Mesh && Mesh->GetRefSkeleton().GetNum() > 0 &&
            Mesh->GetRefSkeleton().GetBoneName(0) == TEXT("Hips"),
            TEXT("G2B-05 GUARD_M root bone must be Hips"));
    case 6:
        return RequireG2B(Test, Mesh && Mesh->GetMaterials().Num() == 1 &&
            Mesh->GetMaterials()[0].MaterialInterface &&
            Mesh->GetMaterials()[0].MaterialInterface->GetPathName().Contains(TEXT("MI_GUARD_M_Meshy_v0_1")),
            TEXT("G2B-06 GUARD_M must have one dedicated material slot"));
    case 7:
    {
        const TCHAR* TexturePaths[] = {
            TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Textures/T_GUARD_M_BaseColor_2K.T_GUARD_M_BaseColor_2K"),
            TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Textures/T_GUARD_M_Normal_2K.T_GUARD_M_Normal_2K"),
            TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Textures/T_GUARD_M_Metallic_2K.T_GUARD_M_Metallic_2K"),
            TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Textures/T_GUARD_M_Roughness_2K.T_GUARD_M_Roughness_2K") };
        const TCHAR* SourcePaths[] = {
            TEXT("SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Working/Textures/T_GUARD_M_BaseColor_2K.png"),
            TEXT("SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Working/Textures/T_GUARD_M_Normal_2K.png"),
            TEXT("SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Working/Textures/T_GUARD_M_Metallic_2K.png"),
            TEXT("SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Working/Textures/T_GUARD_M_Roughness_2K.png") };
        bool bValid = true;
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(TexturePaths); ++Index)
        {
            const UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TexturePaths[Index]);
            bValid &= Texture != nullptr && PngIs2048(SourcePaths[Index]);
        }
        return RequireG2B(Test, bValid, TEXT("G2B-07 all four dedicated GUARD_M textures must be 2048x2048"));
    }
    case 8:
    {
        UWorld* GuardWorld = LoadObject<UWorld>(nullptr,
            TEXT("/Game/Maps/StageG2B_GuardM_PoC.StageG2B_GuardM_PoC"));
        UClass* GuardClass = LoadClass<AStageG2BGuardMActor>(nullptr,
            TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Blueprints/BP_StageG2B_GUARD_M.BP_StageG2B_GUARD_M_C"));
        int32 GuardCount = 0;
        if (GuardWorld && GuardWorld->PersistentLevel && GuardClass)
        {
            for (const AActor* Actor : GuardWorld->PersistentLevel->Actors)
                if (Actor && Actor->IsA(GuardClass)) ++GuardCount;
        }
        return RequireG2B(Test, GuardWorld && GuardClass && GuardCount == 1,
            TEXT("G2B-08 dedicated PoC map must contain exactly one GUARD_M actor"));
    }
    case 9:
    {
        USkeletalMesh* PlayerMesh = LoadObject<USkeletalMesh>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.SK_PLAYER_M_Meshy_v0_1"));
        return RequireG2B(Test, Mesh && PlayerMesh && Mesh->GetSkeleton() != PlayerMesh->GetSkeleton() &&
            ActorSource.Contains(TEXT("bPlayerSkeletonShared = false")),
            TEXT("G2B-09 GUARD_M must never share the PLAYER_M Skeleton asset"));
    }
    case 10:
        return RequireG2B(Test,
            ActorSource.Contains(TEXT("IsMannyFallback")) &&
            ActorSource.Contains(TEXT("player_fallback")) &&
            !ActorSource.Contains(TEXT("SKM_Manny")),
            TEXT("G2B-10 runtime must verify PLAYER_M fallback=false without Manny substitution"));
    case 11:
        return RequireG2B(Test,
            FileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Mesh/SK_PLAYER_M_Meshy_v0_1.uasset"), TEXT("D8A6A679D3D610F347370E6DB88193B7")) &&
            FileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Skeleton/SKEL_PLAYER_M_Meshy_v0_1.uasset"), TEXT("65BA407A472D5D9E3A98D60E228CF5E6")) &&
            FileMd5Equals(TEXT("Content/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/IDLE/A_PLAYER_M_Meshy_v0_1_IDLE_Idle11.uasset"), TEXT("A3AFAF726450416E4357C79FD66AFB79")) &&
            FileMd5Equals(TEXT("../data/stage_a_fixture.json"), TEXT("6883C1D97FF6763E07D7F37BF3A163F0")) &&
            FileMd5Equals(TEXT("Data/StageE/stage_e_state_definitions.json"), TEXT("0DD4238AB25CB35518FF1D1DB854317C")) &&
            FileMd5Equals(TEXT("Data/StageF/stage_f_save_schema.json"), TEXT("C33F613C72F414CFB76E597CF51DCC95")),
            TEXT("G2B-11 accepted PLAYER_M, causal fixture, Stage E, and save schema must remain byte-identical"));
    case 12:
    {
        const FString MapEvidence = ReadProjectFile(
            TEXT("SourceArt/StageG2B/GUARD_M/Meshy/v0.1/Working/Reports/map_generation_evidence.json"));
        return RequireG2B(Test, MapEvidence.Contains(TEXT("\"distance_from_player_start_uu\": 425.0")) &&
            MapEvidence.Contains(TEXT("\"guard_actor_count\": 1")),
            TEXT("G2B-12 GUARD_M must be one actor at StandardCamera visibility distance"));
    }
    case 13:
        return RequireG2B(Test,
            FileMd5Equals(TEXT("Content/Maps/StageG2A_CameraRedesignPoC.umap"), TEXT("A75793F1EC4461827BACED1146AC1F11")) &&
            FileMd5Equals(TEXT("Source/NationSimulationStageC/Public/StageG2A/StageG2ACameraModeAdapter.h"), TEXT("E3CA544E22F86DF2ADBD90CDB1E4368C")) &&
            FileMd5Equals(TEXT("Source/NationSimulationStageC/Private/StageG2A/StageG2ACameraModeAdapter.cpp"), TEXT("C46751F14204B4247011270C3DBDAA5B")),
            TEXT("G2B-13 accepted G2A map and Standard/Tactical camera implementation must remain byte-identical"));
    case 14:
        return RequireG2B(Test,
            WindowsInput.Contains(TEXT("StageG2AToggleCameraMode\",Key=F6")) &&
            ActorSource.Contains(TEXT("ToggleCameraMode")),
            TEXT("G2B-14 F6 Standard/Tactical round trip must remain available"));
    case 15:
        return RequireG2B(Test,
            DefaultInput.Contains(TEXT("StageGPointDrag\",Key=LeftMouseButton")) &&
            PlayerSource.Contains(TEXT("BindAction(TEXT(\"StageGPointDrag\"), IE_Pressed")) &&
            PlayerSource.Contains(TEXT("BindAction(TEXT(\"StageGPointDrag\"), IE_Released")) &&
            PlayerSource.Contains(TEXT("TryIssueMoveFromCursor(true)")),
            TEXT("G2B-15 accepted left-click and left-hold movement mappings must remain unchanged"));
    case 16:
    {
        const UStageG1BSettings* Settings = GetDefault<UStageG1BSettings>();
        UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr,
            TEXT("/Game/StageG1B/Characters/PLAYER_M/MeshyV01/Animations/IDLE/A_PLAYER_M_Meshy_v0_1_IDLE_Idle11.A_PLAYER_M_Meshy_v0_1_IDLE_Idle11"));
        return RequireG2B(Test,
            FMath::IsNearlyEqual(Settings->WalkSpeed, 250.0f) &&
            FMath::IsNearlyEqual(Settings->RunSpeed, 500.0f) &&
            FMath::IsNearlyEqual(Settings->DashSpeed, 750.0f) && Idle,
            TEXT("G2B-16 PLAYER_M Walk/Run/Dash speeds and accepted Idle_11 must remain available"));
    }
    case 17:
        return RequireG2B(Test,
            FileMd5Equals(TEXT("Source/NationSimulationStageC/Public/StageG1B/StageG1BPlayerVisualAdapter.h"), TEXT("78307DE677D69AEFC7A53DD78D0A56C3")) &&
            FileMd5Equals(TEXT("Source/NationSimulationStageC/Private/StageG1B/StageG1BPlayerVisualAdapter.cpp"), TEXT("2E8BA7026F52D06B369E2ADF70D73DA7")) &&
            !ActorSource.Contains(TEXT("IssueMoveToLocation")) &&
            !ActorSource.Contains(TEXT("GameInstanceSubsystem")),
            TEXT("G2B-17 GUARD visual actor must not own destination, NavPath, or causal state"));
    case 18:
        return RequireG2B(Test,
            FPaths::FileExists(ProjectPath(TEXT("Content/Maps/StageG2B_GuardM_PoC.umap"))) &&
            WindowsEngine.Contains(TEXT("GameDefaultMap=/Game/Maps/StageG2B_GuardM_PoC")) &&
            DefaultGame.Contains(TEXT("DirectoriesToAlwaysCook=(Path=\"/Game/StageG2B\")")),
            TEXT("G2B-18 no-argument Package must select and cook the dedicated G2B map"));
    default:
        Test->AddError(TEXT("Unknown Stage G-2B test category"));
        return false;
    }
}
}

#define G2B_GUARD_TEST(ClassName, TestName, Category) \
    IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, \
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
    bool ClassName::RunTest(const FString&) { const bool bPassed = RunG2BCheck(this, Category); \
        RecordG2BResult(Category, bPassed); return bPassed; }

G2B_GUARD_TEST(FG2B01, "NationSimulation.StageG2B.GuardM.01.SourceZipContract", 1)
G2B_GUARD_TEST(FG2B02, "NationSimulation.StageG2B.GuardM.02.GuardMesh", 2)
G2B_GUARD_TEST(FG2B03, "NationSimulation.StageG2B.GuardM.03.DedicatedSkeleton", 3)
G2B_GUARD_TEST(FG2B04, "NationSimulation.StageG2B.GuardM.04.BoneCount", 4)
G2B_GUARD_TEST(FG2B05, "NationSimulation.StageG2B.GuardM.05.RootBone", 5)
G2B_GUARD_TEST(FG2B06, "NationSimulation.StageG2B.GuardM.06.MaterialSlot", 6)
G2B_GUARD_TEST(FG2B07, "NationSimulation.StageG2B.GuardM.07.TextureContract", 7)
G2B_GUARD_TEST(FG2B08, "NationSimulation.StageG2B.GuardM.08.SinglePlacement", 8)
G2B_GUARD_TEST(FG2B09, "NationSimulation.StageG2B.GuardM.09.NoPlayerSkeletonSharing", 9)
G2B_GUARD_TEST(FG2B10, "NationSimulation.StageG2B.GuardM.10.PlayerFallbackFalse", 10)
G2B_GUARD_TEST(FG2B11, "NationSimulation.StageG2B.GuardM.11.PlayerAssetIsolation", 11)
G2B_GUARD_TEST(FG2B12, "NationSimulation.StageG2B.GuardM.12.StandardCameraDistance", 12)
G2B_GUARD_TEST(FG2B13, "NationSimulation.StageG2B.GuardM.13.TacticalCameraPreservation", 13)
G2B_GUARD_TEST(FG2B14, "NationSimulation.StageG2B.GuardM.14.F6Toggle", 14)
G2B_GUARD_TEST(FG2B15, "NationSimulation.StageG2B.GuardM.15.LeftClickMovement", 15)
G2B_GUARD_TEST(FG2B16, "NationSimulation.StageG2B.GuardM.16.WalkRunDash", 16)
G2B_GUARD_TEST(FG2B17, "NationSimulation.StageG2B.GuardM.17.DestinationNavPath", 17)
G2B_GUARD_TEST(FG2B18, "NationSimulation.StageG2B.GuardM.18.PackageDefaultMap", 18)

#undef G2B_GUARD_TEST

#endif
