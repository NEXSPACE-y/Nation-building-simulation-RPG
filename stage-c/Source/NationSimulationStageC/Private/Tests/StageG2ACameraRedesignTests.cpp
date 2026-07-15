#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "StageG1B/StageG1BSettings.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"
#include "StageG2A/StageG2ACameraModeSettings.h"

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

FString ExtractFunction(const FString& Source, const FString& Signature, const FString& NextSignature)
{
    const int32 Start = Source.Find(Signature);
    if (Start == INDEX_NONE) return FString();
    const int32 End = NextSignature.IsEmpty() ? Source.Len() : Source.Find(NextSignature, ESearchCase::CaseSensitive,
        ESearchDir::FromStart, Start + Signature.Len());
    return End == INDEX_NONE ? Source.Mid(Start) : Source.Mid(Start, End - Start);
}

bool FileMd5Equals(const FString& Relative, const FString& Expected)
{
    TArray<uint8> Bytes;
    if (!FFileHelper::LoadFileToArray(Bytes, *ProjectPath(Relative))) return false;
    FMD5 Md5;
    if (!Bytes.IsEmpty()) Md5.Update(Bytes.GetData(), Bytes.Num());
    uint8 Digest[16];
    Md5.Final(Digest);
    const FString Actual = BytesToHex(Digest, UE_ARRAY_COUNT(Digest));
    return Actual.Equals(Expected, ESearchCase::IgnoreCase);
}

bool RequireG2A(FAutomationTestBase* Test, bool bCondition, const FString& Message)
{
    if (!bCondition) Test->AddError(Message);
    return bCondition;
}

void RecordG2AResult(int32 Category, bool bPassed)
{
    static TMap<int32, bool> Results;
    Results.Add(Category, bPassed);
    if (Results.Num() != 18) return;
    const TCHAR* Names[] = {
        TEXT("G1B Baseline Isolation"), TEXT("Initial Standard Mode"), TEXT("Non Overhead Standard"),
        TEXT("F6 Toggle Contract"), TEXT("Standard Clamps"), TEXT("Tactical Clamps"),
        TEXT("Mode Parameter Isolation"), TEXT("Yaw 360 And Gain"), TEXT("Pitch Input"),
        TEXT("Wheel Zoom"), TEXT("Destination NavPath Isolation"), TEXT("Locomotion Preservation"),
        TEXT("Camera Collision"), TEXT("Player Follow And Lag"), TEXT("UI Input Isolation"),
        TEXT("Package Default Map"), TEXT("Twelve Screenshot Contract"), TEXT("Rejected G2A Not Adopted") };
    TArray<FString> Lines;
    int32 Passed = 0;
    for (int32 Index = 1; Index <= 18; ++Index)
    {
        const bool bResult = Results.FindRef(Index);
        Passed += bResult ? 1 : 0;
        Lines.Add(FString::Printf(TEXT("G2A-R%d | %s | %s"), Index,
            bResult ? TEXT("PASS") : TEXT("FAIL"), Names[Index - 1]));
    }
    Lines.Add(FString::Printf(TEXT("SUMMARY | %d/18 tests passed"), Passed));
    const FString Output = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectDir(), TEXT("../out/stage-g2a-redesign/test-output")));
    IFileManager::Get().MakeDirectory(*Output, true);
    FFileHelper::SaveStringArrayToFile(Lines,
        *FPaths::Combine(Output, TEXT("stage_g2a_redesign_automation_results.txt")),
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

bool RunG2ACheck(FAutomationTestBase* Test, int32 Category)
{
    const UStageG2ACameraModeSettings* S = GetDefault<UStageG2ACameraModeSettings>();
    const UStageG1BSettings* G1B = GetDefault<UStageG1BSettings>();
    const FString Adapter = ReadProjectFile(TEXT("Source/NationSimulationStageC/Private/StageG2A/StageG2ACameraModeAdapter.cpp"));
    const FString Header = ReadProjectFile(TEXT("Source/NationSimulationStageC/Public/StageG2A/StageG2ACameraModeAdapter.h"));
    const FString SettingsHeader = ReadProjectFile(TEXT("Source/NationSimulationStageC/Public/StageG2A/StageG2ACameraModeSettings.h"));
    const FString Input = ReadProjectFile(TEXT("Config/DefaultInput.ini"));
    const FString WindowsInput = ReadProjectFile(TEXT("Config/Windows/WindowsInput.ini"));
    const FString WindowsEngine = ReadProjectFile(TEXT("Config/Windows/WindowsEngine.ini"));
    const FString ToggleHandler = ExtractFunction(Adapter,
        TEXT("void AStageG2ACameraModeAdapter::ToggleCameraMode()"),
        TEXT("void AStageG2ACameraModeAdapter::ApplyMode("));
    const FString OrbitHandler = ExtractFunction(Adapter,
        TEXT("void AStageG2ACameraModeAdapter::ApplyOrbitDeltaDegrees("),
        TEXT("void AStageG2ACameraModeAdapter::ApplyZoomSteps("));
    const FString ZoomHandler = ExtractFunction(Adapter,
        TEXT("void AStageG2ACameraModeAdapter::ApplyZoomSteps("),
        TEXT("void AStageG2ACameraModeAdapter::BeginRuntimeEvidence("));

    switch (Category)
    {
    case 1:
        return RequireG2A(Test,
                FileMd5Equals(TEXT("Source/NationSimulationStageC/Public/StageG1A/StageG1APlayerCharacter.h"),
                    TEXT("5D2AF5563158A89AF0D67B3B04675CC4")) &&
                FileMd5Equals(TEXT("Source/NationSimulationStageC/Private/StageG1A/StageG1APlayerCharacter.cpp"),
                    TEXT("B30B1CAF4B1845E5B846E3D90D98AEEF")) &&
                FileMd5Equals(TEXT("Source/NationSimulationStageC/Public/StageG1B/StageG1BPlayerVisualAdapter.h"),
                    TEXT("78307DE677D69AEFC7A53DD78D0A56C3")) &&
                FileMd5Equals(TEXT("Source/NationSimulationStageC/Private/StageG1B/StageG1BPlayerVisualAdapter.cpp"),
                    TEXT("2E8BA7026F52D06B369E2ADF70D73DA7")) &&
                FileMd5Equals(TEXT("../data/stage_a_fixture.json"), TEXT("6883C1D97FF6763E07D7F37BF3A163F0")) &&
                FileMd5Equals(TEXT("../data/stage_parity_contract.json"), TEXT("C82F6DE4BC11B24A26E5CE489622D741")) &&
                FileMd5Equals(TEXT("Data/StageE/stage_e_state_definitions.json"), TEXT("0DD4238AB25CB35518FF1D1DB854317C")) &&
                FileMd5Equals(TEXT("Data/StageF/stage_f_save_schema.json"), TEXT("C33F613C72F414CFB76E597CF51DCC95")) &&
                FileMd5Equals(TEXT("../include/nation_sim/stage_f_runtime.hpp"), TEXT("D8258251F551BF8E441902A3F617CA6B")) &&
                FileMd5Equals(TEXT("../src/stage_f_runtime.cpp"), TEXT("8E2FF056CD6715502048F81198F2365B")),
            TEXT("G2A-R1 accepted G1A/G1B, shared contracts, Stage E/F, and save schema must remain byte-identical"));
    case 2:
        return RequireG2A(Test,
                FMath::IsNearlyEqual(S->StandardDefaultDistance, 520.0f) &&
                FMath::IsNearlyEqual(S->StandardDefaultPitch, -15.0f) &&
                FMath::IsNearlyEqual(S->StandardFov, 75.0f) &&
                Header.Contains(TEXT("ActiveMode = EStageG2ACameraMode::StandardCharacterCamera")),
            TEXT("G2A-R2 launch mode must be StandardCharacterCamera at 520uu, pitch -15, FOV 75"));
    case 3:
        return RequireG2A(Test, S->StandardDefaultPitch > -35.0f &&
                S->StandardDefaultDistance < 900.0f &&
                SettingsHeader.Contains(TEXT("Third")) == false &&
                Adapter.Contains(TEXT("initial_mode=%s standard_distance")),
            TEXT("G2A-R3 standard camera must be close, behind-character, and non-overhead"));
    case 4:
        return RequireG2A(Test,
                WindowsInput.Contains(TEXT("StageG2AToggleCameraMode\",Key=F6")) &&
                Adapter.Contains(TEXT("StageG2AToggleCameraMode")) &&
                ToggleHandler.Contains(TEXT("TacticalOverlookCamera")) &&
                ToggleHandler.Contains(TEXT("StandardCharacterCamera")),
            TEXT("G2A-R4 F6 must exclusively toggle the two camera modes"));
    case 5:
        return RequireG2A(Test,
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampPitch(-100.0f,
                    S->StandardMinPitch, S->StandardMaxPitch), -30.0f) &&
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampPitch(30.0f,
                    S->StandardMinPitch, S->StandardMaxPitch), 8.0f) &&
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampArmLength(10.0f,
                    S->StandardMinDistance, S->StandardMaxDistance), 320.0f) &&
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampArmLength(2000.0f,
                    S->StandardMinDistance, S->StandardMaxDistance), 850.0f) &&
                FMath::IsNearlyEqual(S->StandardCenterZ, 115.0f),
            TEXT("G2A-R5 Standard pitch, zoom, FOV, and center must match the redesign"));
    case 6:
        return RequireG2A(Test,
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampPitch(-100.0f,
                    S->TacticalMinPitch, S->TacticalMaxPitch), -65.0f) &&
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampPitch(0.0f,
                    S->TacticalMinPitch, S->TacticalMaxPitch), -45.0f) &&
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampArmLength(10.0f,
                    S->TacticalMinDistance, S->TacticalMaxDistance), 800.0f) &&
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::ClampArmLength(3000.0f,
                    S->TacticalMinDistance, S->TacticalMaxDistance), 2000.0f) &&
                FMath::IsNearlyEqual(S->TacticalDefaultDistance, 1200.0f) &&
                FMath::IsNearlyEqual(S->TacticalFov, 60.0f) && FMath::IsNearlyEqual(S->TacticalCenterZ, 120.0f),
            TEXT("G2A-R6 Tactical pitch, zoom, FOV, and center must match the helper-view design"));
    case 7:
        return RequireG2A(Test,
                Header.Contains(TEXT("FModeState StandardState")) && Header.Contains(TEXT("FModeState TacticalState")) &&
                Adapter.Contains(TEXT("SaveActiveModeState")) && Adapter.Contains(TEXT("ApplyModeParameters")) &&
                Adapter.Contains(TEXT("parameter_leakage"), ESearchCase::CaseSensitive),
            TEXT("G2A-R7 each mode must retain isolated camera state and parameter sets"));
    case 8:
    {
        const float Accumulated = AStageG2ACameraModeAdapter::AccumulateYaw(35.0f, 720.0f);
        return RequireG2A(Test, FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::NormalizeYaw(Accumulated), 35.0f) &&
                FMath::IsNearlyEqual(S->StandardYawSensitivity, 0.50f) &&
                FMath::IsNearlyEqual(S->TacticalYawSensitivity, 1.25f) &&
                FMath::IsNearlyEqual(S->TacticalYawSensitivity / 0.50f, 2.5f) &&
                FMath::IsNearlyEqual(AStageG2ACameraModeAdapter::YawDegreesFromMouseDelta(100.0f,
                    S->TacticalYawSensitivity), 125.0f),
            TEXT("G2A-R8 both modes must support continuous yaw and Tactical must be old observed gain x2.5"));
    }
    case 9:
        return RequireG2A(Test,
                FMath::IsNearlyEqual(S->StandardPitchSensitivity, 0.15f) &&
                FMath::IsNearlyEqual(S->TacticalPitchSensitivity, 0.18f) &&
                FMath::IsNearlyEqual(S->TacticalPitchSensitivity / 0.15f, 1.2f) &&
                Adapter.Contains(TEXT("GetRawKeyValue(EKeys::MouseY)")) &&
                Adapter.Contains(TEXT("PitchDegreesFromMouseDelta(RawDeltaY")),
            TEXT("G2A-R9 raw MouseY pitch must work in both modes and Tactical gain must be x1.2"));
    case 10:
        return RequireG2A(Test, ZoomHandler.Contains(TEXT("TargetArmLength")) &&
                ZoomHandler.Contains(TEXT("GetActiveMinDistance")) &&
                ZoomHandler.Contains(TEXT("GetActiveMaxDistance")) &&
                !ZoomHandler.Contains(TEXT("SetFieldOfView")),
            TEXT("G2A-R10 wheel must clamp SpringArm distance in both modes without FOV zoom"));
    case 11:
        return RequireG2A(Test,
                ToggleHandler.Contains(TEXT("Destination")) && ToggleHandler.Contains(TEXT("GetActivePathPointCount")) &&
                !ToggleHandler.Contains(TEXT("IssueMoveToLocation")) &&
                !OrbitHandler.Contains(TEXT("IssueMoveToLocation")) &&
                !ZoomHandler.Contains(TEXT("IssueMoveToLocation")) &&
                !ToggleHandler.Contains(TEXT("GameInstanceSubsystem")),
            TEXT("G2A-R11 camera operations must preserve destination/NavPath and avoid causal state"));
    case 12:
        return RequireG2A(Test, FMath::IsNearlyEqual(G1B->WalkSpeed, 250.0f) &&
                FMath::IsNearlyEqual(G1B->RunSpeed, 500.0f) && FMath::IsNearlyEqual(G1B->DashSpeed, 750.0f) &&
                !ToggleHandler.Contains(TEXT("SetMoveMode")) && !OrbitHandler.Contains(TEXT("SetMoveMode")) &&
                !ZoomHandler.Contains(TEXT("SetMoveMode")) && Adapter.Contains(TEXT("movement_mode_maintained")),
            TEXT("G2A-R12 WALK/RUN/DASH and Idle_11 ownership must remain in accepted G1B"));
    case 13:
        return RequireG2A(Test, Adapter.Contains(TEXT("bDoCollisionTest = true")) &&
                Adapter.Contains(TEXT("ProbeChannel = ECC_Camera")) &&
                FMath::IsNearlyEqual(S->StandardProbeSize, 12.0f) &&
                FMath::IsNearlyEqual(S->StandardCollisionMinDistance, 180.0f) &&
                FMath::IsNearlyEqual(S->StandardCollisionRecoverySeconds, 0.20f) &&
                FMath::IsNearlyEqual(S->TacticalProbeSize, 16.0f) &&
                FMath::IsNearlyEqual(S->TacticalCollisionMinDistance, 400.0f) &&
                FMath::IsNearlyEqual(S->TacticalCollisionRecoverySeconds, 0.30f) &&
                Adapter.Contains(TEXT("distance_recovered_after_clear")),
            TEXT("G2A-R13 both modes must enable Camera collision with their fixed probe/recovery contracts"));
    case 14:
        return RequireG2A(Test, Adapter.Contains(TEXT("Player->GetCameraBoom()")) &&
                Adapter.Contains(TEXT("TargetOffset = FVector")) &&
                Adapter.Contains(TEXT("bEnableCameraLag = true")) &&
                FMath::IsNearlyEqual(S->StandardCameraLagSpeed, 14.0f) &&
                FMath::IsNearlyEqual(S->TacticalCameraLagSpeed, 12.0f),
            TEXT("G2A-R14 accepted Player SpringArm must provide follow with weak mode-specific lag"));
    case 15:
        return RequireG2A(Test, Adapter.Contains(TEXT("IsPointerOverUi")) &&
                Adapter.Contains(TEXT("bLastCameraInputBlockedByUi")) &&
                Input.Contains(TEXT("StageGPointDrag\",Key=LeftMouseButton")) &&
                WindowsInput.Contains(TEXT("StageG2ACameraOrbit\",Key=RightMouseButton")),
            TEXT("G2A-R15 right-drag camera input must be UI-guarded and separate from accepted left-click move"));
    case 16:
    {
        UClass* AdapterClass = LoadClass<AStageG2ACameraModeAdapter>(nullptr,
            TEXT("/Game/StageG2A/CameraRedesign/Blueprints/BP_StageG2A_CameraModeAdapter.BP_StageG2A_CameraModeAdapter_C"));
        return RequireG2A(Test,
                FPaths::FileExists(ProjectPath(TEXT("Content/Maps/StageG2A_CameraRedesignPoC.umap"))) &&
                WindowsEngine.Contains(TEXT("GameDefaultMap=/Game/Maps/StageG2A_CameraRedesignPoC")) &&
                AdapterClass && AdapterClass->IsChildOf(AStageG2ACameraModeAdapter::StaticClass()),
            TEXT("G2A-R16 packaged no-argument map and redesigned Camera Adapter asset must exist"));
    }
    case 17:
    {
        const TCHAR* Names[] = { TEXT("01_standard_initial.png"), TEXT("02_standard_min_zoom.png"),
            TEXT("03_standard_max_zoom.png"), TEXT("04_standard_pitch_min.png"),
            TEXT("05_standard_pitch_max.png"), TEXT("06_tactical_initial.png"),
            TEXT("07_tactical_min_zoom.png"), TEXT("08_tactical_max_zoom.png"),
            TEXT("09_tactical_yaw_90.png"), TEXT("10_tactical_yaw_180.png"),
            TEXT("11_collision_near_wall.png"), TEXT("12_return_to_standard.png") };
        bool bAllPresent = true;
        for (const TCHAR* Name : Names) bAllPresent &= Adapter.Contains(Name);
        return RequireG2A(Test, bAllPresent, TEXT("G2A-R17 all twelve fixed evaluation screenshot names must be implemented"));
    }
    case 18:
        return RequireG2A(Test,
                !FPaths::FileExists(ProjectPath(TEXT("Content/Maps/StageG2A_RestrictedFreeCameraPoC.umap"))) &&
                !WindowsEngine.Contains(TEXT("StageG2A_RestrictedFreeCameraPoC")) &&
                !SettingsHeader.Contains(TEXT("float DefaultPitch = -55.0f")) &&
                SettingsHeader.Contains(TEXT("StandardDefaultPitch = -15.0f")) &&
                SettingsHeader.Contains(TEXT("TacticalDefaultPitch = -55.0f")),
            TEXT("G2A-R18 rejected overlook-as-standard G2A must not be adopted or baseline-fixed"));
    default:
        Test->AddError(TEXT("Unknown Stage G-2A redesign test category"));
        return false;
    }
}
}

#define G2A_REDESIGN_TEST(ClassName, TestName, Category) \
    IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClassName, TestName, \
        EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter) \
    bool ClassName::RunTest(const FString&) { const bool bPassed = RunG2ACheck(this, Category); \
        RecordG2AResult(Category, bPassed); return bPassed; }

G2A_REDESIGN_TEST(FG2AR01, "NationSimulation.StageG2A.CameraRedesign.01.BaselineIsolation", 1)
G2A_REDESIGN_TEST(FG2AR02, "NationSimulation.StageG2A.CameraRedesign.02.InitialStandardMode", 2)
G2A_REDESIGN_TEST(FG2AR03, "NationSimulation.StageG2A.CameraRedesign.03.NonOverheadStandard", 3)
G2A_REDESIGN_TEST(FG2AR04, "NationSimulation.StageG2A.CameraRedesign.04.F6Toggle", 4)
G2A_REDESIGN_TEST(FG2AR05, "NationSimulation.StageG2A.CameraRedesign.05.StandardClamps", 5)
G2A_REDESIGN_TEST(FG2AR06, "NationSimulation.StageG2A.CameraRedesign.06.TacticalClamps", 6)
G2A_REDESIGN_TEST(FG2AR07, "NationSimulation.StageG2A.CameraRedesign.07.ParameterIsolation", 7)
G2A_REDESIGN_TEST(FG2AR08, "NationSimulation.StageG2A.CameraRedesign.08.Yaw360AndGain", 8)
G2A_REDESIGN_TEST(FG2AR09, "NationSimulation.StageG2A.CameraRedesign.09.PitchInput", 9)
G2A_REDESIGN_TEST(FG2AR10, "NationSimulation.StageG2A.CameraRedesign.10.WheelZoom", 10)
G2A_REDESIGN_TEST(FG2AR11, "NationSimulation.StageG2A.CameraRedesign.11.DestinationNavPath", 11)
G2A_REDESIGN_TEST(FG2AR12, "NationSimulation.StageG2A.CameraRedesign.12.LocomotionPreservation", 12)
G2A_REDESIGN_TEST(FG2AR13, "NationSimulation.StageG2A.CameraRedesign.13.CameraCollision", 13)
G2A_REDESIGN_TEST(FG2AR14, "NationSimulation.StageG2A.CameraRedesign.14.PlayerFollow", 14)
G2A_REDESIGN_TEST(FG2AR15, "NationSimulation.StageG2A.CameraRedesign.15.UIIsolation", 15)
G2A_REDESIGN_TEST(FG2AR16, "NationSimulation.StageG2A.CameraRedesign.16.PackageDefaultMap", 16)
G2A_REDESIGN_TEST(FG2AR17, "NationSimulation.StageG2A.CameraRedesign.17.ScreenshotContract", 17)
G2A_REDESIGN_TEST(FG2AR18, "NationSimulation.StageG2A.CameraRedesign.18.RejectedG2ANotAdopted", 18)

#undef G2A_REDESIGN_TEST

#endif
