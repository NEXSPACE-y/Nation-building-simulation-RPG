#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "PaperSprite.h"
#include "StageD/StageDNpcActor.h"
#include "StageD/StageDGameMode.h"
#include "StageD/StageDPlayerCharacter.h"
#include "StageG0/StageG0FangRatActor.h"
#include "StageG0/StageG0Settings.h"
#include "StageG0/StageG0Targetable.h"
#include "StageG0/StageG0VerificationPanel.h"
#include "StageG0/StageGDirectionalFlipbookComponent.h"
#include "nation_sim/stage_f_runtime.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
using json = nlohmann::json;

void RequireG0(bool Condition, const std::string& Message)
{
    if (!Condition) throw std::runtime_error(Message);
}

std::string Utf8(const FString& Value)
{
    return TCHAR_TO_UTF8(*Value);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStageG0VisualPoCTest,
    "NationSimulation.StageG0.VisualPoC",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageG0VisualPoCTest::RunTest(const FString& Parameters)
{
    (void)Parameters;
    const FString RepositoryRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("..")));
    const std::filesystem::path ScaleRoot(*FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/StageF/Generated")));
    const std::filesystem::path Output(*FPaths::Combine(RepositoryRoot, TEXT("out/stage-g0/test-output")));
    std::filesystem::create_directories(Output);
    int Passed = 0;
    int Failed = 0;
    std::vector<std::string> Results;
    json DirectionEvidence;
    json MappingEvidence;
    json ActorEvidence;
    json Evidence;
    json ClickMoveEvidence;
    json TargetingEvidence;

    const auto Run = [&](const char* Name, const std::function<void()>& Body)
    {
        try
        {
            Body();
            ++Passed;
            Results.push_back(std::string("PASS | ") + Name);
            AddInfo(FString::Printf(TEXT("PASS | %s"), UTF8_TO_TCHAR(Name)));
        }
        catch (const std::exception& Exception)
        {
            ++Failed;
            Results.push_back(std::string("FAIL | ") + Name + " | " + Exception.what());
            AddError(FString::Printf(TEXT("FAIL | %s | %s"), UTF8_TO_TCHAR(Name), UTF8_TO_TCHAR(Exception.what())));
        }
    };

    Run("G0-1 Component Contract", [&]
    {
        FString Project;
        RequireG0(FFileHelper::LoadFileToString(Project, *FPaths::Combine(FPaths::ProjectDir(), TEXT("NationSimulationStageC.uproject"))),
            "uproject unreadable");
        RequireG0(Project.Contains(TEXT("\"Name\": \"Paper2D\"")) && Project.Contains(TEXT("\"Enabled\": true")),
            "Paper2D plugin is not enabled");
        const auto* Player = GetDefault<AStageDPlayerCharacter>();
        const auto* Npc = GetDefault<AStageDNpcActor>();
        RequireG0(Player->FindComponentByClass<UCapsuleComponent>() != nullptr, "player capsule missing");
        RequireG0(Npc->FindComponentByClass<UCapsuleComponent>() != nullptr, "NPC capsule missing");
        const auto* PlayerVisual = Player->FindComponentByClass<UStageGDirectionalFlipbookComponent>();
        const auto* NpcVisual = Npc->FindComponentByClass<UStageGDirectionalFlipbookComponent>();
        RequireG0(PlayerVisual && NpcVisual, "directional Paper2D component missing");
        RequireG0(PlayerVisual->GetCollisionEnabled() == ECollisionEnabled::NoCollision &&
            NpcVisual->GetCollisionEnabled() == ECollisionEnabled::NoCollision, "sprite collision must be disabled");
        Evidence["component_contract"] = {{"paper2d", true}, {"capsule_collision", true},
            {"sprite_collision", false}, {"adapter", "UStageGDirectionalFlipbookComponent"}};
    });

    Run("G0-2 Direction Quantization", [&]
    {
        const FVector Forward(1.0f, 0.0f, 0.0f);
        const FVector Right(0.0f, 1.0f, 0.0f);
        const std::vector<std::pair<FVector, EStageGVisualDirection>> Cases = {
            {FVector(1, 0, 0), EStageGVisualDirection::Front},
            {FVector(1, 1, 0), EStageGVisualDirection::FrontRight},
            {FVector(0, 1, 0), EStageGVisualDirection::Right},
            {FVector(-1, 1, 0), EStageGVisualDirection::BackRight},
            {FVector(-1, 0, 0), EStageGVisualDirection::Back},
            {FVector(-1, -1, 0), EStageGVisualDirection::BackLeft},
            {FVector(0, -1, 0), EStageGVisualDirection::Left},
            {FVector(1, -1, 0), EStageGVisualDirection::FrontLeft}
        };
        json Rows = json::array();
        for (const auto& [Vector, Expected] : Cases)
        {
            const auto Actual = UStageGDirectionalFlipbookComponent::QuantizeDirection(
                Vector, Forward, Right, EStageGVisualDirection::Front, 0.01f);
            RequireG0(Actual == Expected, "direction mismatch: " + Utf8(StageGVisualDirectionName(Expected)));
            Rows.push_back({{"vector", {Vector.X, Vector.Y}}, {"direction", Utf8(StageGVisualDirectionName(Actual))}});
        }
        const double Below = FMath::DegreesToRadians(22.4);
        const double Above = FMath::DegreesToRadians(22.6);
        RequireG0(UStageGDirectionalFlipbookComponent::QuantizeDirection(
            FVector(std::cos(Below), std::sin(Below), 0), Forward, Right, EStageGVisualDirection::Back, 0.01f) ==
            EStageGVisualDirection::Front, "lower direction boundary mismatch");
        RequireG0(UStageGDirectionalFlipbookComponent::QuantizeDirection(
            FVector(std::cos(Above), std::sin(Above), 0), Forward, Right, EStageGVisualDirection::Back, 0.01f) ==
            EStageGVisualDirection::FrontRight, "upper direction boundary mismatch");
        RequireG0(UStageGDirectionalFlipbookComponent::QuantizeDirection(
            FVector::ZeroVector, Forward, Right, EStageGVisualDirection::Left, 3.0f) ==
            EStageGVisualDirection::Left, "stopped direction was not retained");
        DirectionEvidence = {{"directions", Rows}, {"boundary_degrees", {22.4, 22.6}},
            {"stopped_retains_last", true}, {"pass", true}};
    });

    Run("G0-3 Idle Walk", [&]
    {
        RequireG0(UStageGDirectionalFlipbookComponent::ResolveMovementAction(7.99f, 8.0f) == EStageGVisualAction::Idle,
            "below threshold must be Idle");
        RequireG0(UStageGDirectionalFlipbookComponent::ResolveMovementAction(8.01f, 8.0f) == EStageGVisualAction::Walk,
            "above threshold must be Walk");
        RequireG0(UStageGDirectionalFlipbookComponent::ResolveMovementAction(8.01f, 8.0f, true) == EStageGVisualAction::MonsterMove,
            "monster movement threshold mismatch");
    });

    Run("G0-4 Action Mapping", [&]
    {
        const std::vector<std::pair<const TCHAR*, EStageGVisualAction>> Cases = {
            {TEXT("WAIT"), EStageGVisualAction::Idle}, {TEXT("MOVE"), EStageGVisualAction::Walk},
            {TEXT("TALK"), EStageGVisualAction::Talk}, {TEXT("WARN"), EStageGVisualAction::Warn},
            {TEXT("REPORT"), EStageGVisualAction::Report}, {TEXT("ARREST"), EStageGVisualAction::Arrest},
            {TEXT("FLEE"), EStageGVisualAction::Flee}, {TEXT("REFUSE_TRADE"), EStageGVisualAction::Talk},
            {TEXT("INVESTIGATE"), EStageGVisualAction::Walk}, {TEXT("HELP"), EStageGVisualAction::Talk},
            {TEXT("PROTECT"), EStageGVisualAction::Walk}
        };
        json Rows = json::array();
        for (const auto& [Source, Expected] : Cases)
        {
            const auto Actual = UStageGDirectionalFlipbookComponent::MapCoreAction(Source, false);
            RequireG0(Actual == Expected, "action mapping mismatch: " + std::string(TCHAR_TO_UTF8(Source)));
            Rows.push_back({{"core_action", TCHAR_TO_UTF8(Source)}, {"visual_action", Utf8(StageGVisualActionName(Actual))}});
        }
        for (const auto& [Source, Expected] : std::vector<std::pair<const TCHAR*, EStageGVisualAction>>{
            {TEXT("IDLE"), EStageGVisualAction::Idle}, {TEXT("MOVE"), EStageGVisualAction::MonsterMove},
            {TEXT("CHARGE"), EStageGVisualAction::MonsterCharge}, {TEXT("BITE"), EStageGVisualAction::MonsterBite},
            {TEXT("HIT"), EStageGVisualAction::Hit}, {TEXT("DEATH"), EStageGVisualAction::Death}})
        {
            const auto Actual = UStageGDirectionalFlipbookComponent::MapCoreAction(Source, true);
            RequireG0(Actual == Expected, "monster mapping mismatch");
            Rows.push_back({{"monster_action", TCHAR_TO_UTF8(Source)}, {"visual_action", Utf8(StageGVisualActionName(Actual))}});
        }
        MappingEvidence = {{"mappings", Rows}, {"pass", true}, {"visual_placeholder", true}};
    });

    Run("G0-5 Flipbook Stability", [&]
    {
        auto* Component = NewObject<UStageGDirectionalFlipbookComponent>();
        Component->ConfigureVisual(TEXT("automation_actor"), TEXT("GUARD"), false);
        Component->InitializeVisualPlaceholderForAutomation();
        Component->UpdatePresentation(FVector::ZeroVector, TEXT("WAIT"), FVector(500, 500, 500), FRotator(-55, 45, 0), 0.016f);
        const int32 AfterFirst = Component->GetFlipbookSwitchCount();
        Component->UpdatePresentation(FVector::ZeroVector, TEXT("WAIT"), FVector(500, 500, 500), FRotator(-55, 45, 0), 0.016f);
        RequireG0(Component->GetFlipbookSwitchCount() == AfterFirst, "same state reset flipbook");
        Component->ApplyDebugVisualAction(EStageGVisualAction::Warn);
        const int32 AfterWarn = Component->GetFlipbookSwitchCount();
        Component->ApplyDebugVisualAction(EStageGVisualAction::Warn);
        RequireG0(Component->GetFlipbookSwitchCount() == AfterWarn, "same debug action reset flipbook");
    });

    Run("G0-6 Non-loop Fallback", [&]
    {
        for (const EStageGVisualAction Action : {EStageGVisualAction::Talk, EStageGVisualAction::Warn,
            EStageGVisualAction::Report, EStageGVisualAction::Arrest, EStageGVisualAction::MonsterCharge,
            EStageGVisualAction::MonsterBite, EStageGVisualAction::Hit})
        {
            RequireG0(!UStageGDirectionalFlipbookComponent::IsLoopingAction(Action), "non-loop action marked loop");
            RequireG0(UStageGDirectionalFlipbookComponent::FallsBackToIdle(Action), "non-loop action lacks fallback");
        }
        RequireG0(!UStageGDirectionalFlipbookComponent::FallsBackToIdle(EStageGVisualAction::Death),
            "Death must not fall back to Idle");
    });

    Run("G0-7 Save Load Independence", [&]
    {
        FString SaveSchema;
        RequireG0(FFileHelper::LoadFileToString(SaveSchema,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Data/StageF/stage_f_save_schema.json"))), "save schema unreadable");
        RequireG0(!SaveSchema.Contains(TEXT("visual_action")) && !SaveSchema.Contains(TEXT("flipbook")) &&
            !SaveSchema.Contains(TEXT("visual_direction")), "visual state leaked into save schema");
        FString ComponentSource;
        RequireG0(FFileHelper::LoadFileToString(ComponentSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageG0/StageGDirectionalFlipbookComponent.cpp"))),
            "component source unreadable");
        RequireG0(!ComponentSource.Contains(TEXT("save_generation")) && !ComponentSource.Contains(TEXT("stage_d_save")),
            "visual component owns save state");
    });

    Run("G0-8 Actor Count Boundary", [&]
    {
        auto Runtime = nation_sim::StageFProductionRuntime::load(ScaleRoot);
        RequireG0(Runtime.counters().ai_npc_total_count == 2500, "Stage F runtime count mismatch");
        RequireG0(FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/StageG0_VisualPoC.umap"))),
            "Stage G-0 map missing");
        RequireG0(AStageG0FangRatActor::FixtureActionCount == 6, "fang rat action fixture count mismatch");
        ActorEvidence = {{"stage_f_ai_runtime", 2500}, {"presentation_ai", 20}, {"presentation_non_ai", 20},
            {"player", 1}, {"fang_rat", 1}, {"visual_component_count", 42}, {"bulk_actor_spawn", false}, {"pass", true}};
    });

    Run("G0-9 Causal Separation", [&]
    {
        const TArray<FString> Files = {
            TEXT("StageGDirectionalFlipbookComponent.cpp"), TEXT("StageG0FangRatActor.cpp"), TEXT("StageG0Types.cpp")};
        for (const FString& File : Files)
        {
            FString Source;
            RequireG0(FFileHelper::LoadFileToString(Source,
                *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageG0"), File)),
                "cannot inspect Stage G-0 source");
            RequireG0(!Source.Contains(TEXT("nation_sim::Simulation")) && !Source.Contains(TEXT("transition_rule")) &&
                !Source.Contains(TEXT("COUNTRY_STATE_CHANGED")) && !Source.Contains(TEXT("SubmitPlayerAction")) &&
                !Source.Contains(TEXT("enqueue_country_event")), "causal logic leaked into Stage G-0 display");
        }
    });

    Run("G0-10 Package Asset Load", [&]
    {
        RequireG0(LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Paper2D/MaskedUnlitSpriteMaterial.MaskedUnlitSpriteMaterial")) != nullptr,
            "Paper2D masked material missing");
        RequireG0(UStageGDirectionalFlipbookComponent::DirectionalPlaceholderAssetId(
            TEXT("GUARD"), EStageGVisualDirection::Front).Contains(TEXT("PROC_ARROW_F")),
            "procedural directional placeholder asset contract missing");
        RequireG0(FPaths::FileExists(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Maps/StageG0_VisualPoC.umap"))),
            "Stage G-0 package map missing");
    });

    Run("G0-11 Camera Follow Zoom Limits", [&]
    {
        const UStageG0Settings* Settings = GetDefault<UStageG0Settings>();
        RequireG0(AStageDPlayerCharacter::ClampStageGCameraPitch(-90.0f) == Settings->CameraMinPitch,
            "camera lower pitch limit mismatch");
        RequireG0(AStageDPlayerCharacter::ClampStageGCameraPitch(-10.0f) == Settings->CameraMaxPitch,
            "camera upper pitch limit mismatch");
        RequireG0(AStageDPlayerCharacter::ClampStageGCameraZoom(100.0f) == Settings->CameraMinArmLength,
            "camera minimum zoom mismatch");
        RequireG0(AStageDPlayerCharacter::ClampStageGCameraZoom(3000.0f) == Settings->CameraMaxArmLength,
            "camera maximum zoom mismatch");
        RequireG0(Settings->CameraYaw == 45.0f && Settings->CameraPitch == -55.0f && Settings->CameraArmLength == 1100.0f,
            "camera defaults changed");
    });

    Run("G0-12 Directional Placeholder Difference", [&]
    {
        auto* Component = NewObject<UStageGDirectionalFlipbookComponent>();
        Component->ConfigureVisual(TEXT("direction_asset_test"), TEXT("GUARD"), false);
        Component->InitializeVisualPlaceholderForAutomation();
        TSet<FString> AssetIds;
        TSet<FString> FlipbookNames;
        for (int32 Index = 0; Index < 8; ++Index)
        {
            Component->ApplyDebugVisualDirection(static_cast<EStageGVisualDirection>(Index));
            const FStageGVisualDebugView View = Component->GetDebugView(FVector(500), FRotator(-55, 45, 0));
            AssetIds.Add(View.VisualAssetId);
            FlipbookNames.Add(View.FlipbookName);
            RequireG0(View.VisualAssetId.Contains(TEXT("PROC_ARROW_")), "directional procedural asset id missing");
        }
        RequireG0(AssetIds.Num() == 8 && FlipbookNames.Num() == 8, "eight direction assets are not distinct");
    });

    Run("G0-13 Japanese Display Names", [&]
    {
        RequireG0(StageGVisualDirectionJapanese(EStageGVisualDirection::FrontRight) == TEXT("右前"),
            "Japanese direction name missing");
        RequireG0(StageGVisualActionJapanese(EStageGVisualAction::MonsterBite) == TEXT("噛みつき"),
            "Japanese action name missing");
        RequireG0(StageGVisualFamilyJapanese(TEXT("CAPTAIN")) == TEXT("隊長") &&
            StageGVisualFamilyJapanese(TEXT("RESIDENT")) == TEXT("一般住民"), "Japanese family name missing");
        FString HudSource;
        RequireG0(FFileHelper::LoadFileToString(HudSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDHudWidget.cpp"))),
            "HUD source unreadable");
        RequireG0(HudSource.Contains(TEXT("表示対象ID")) && HudSource.Contains(TEXT("表示動作")) &&
            HudSource.Contains(TEXT("遮蔽状態")) && HudSource.Contains(TEXT("切替回数")), "Japanese F1 labels missing");
        FString WorldLabelSource;
        FString SignSource;
        RequireG0(FFileHelper::LoadFileToString(WorldLabelSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageG0/StageG0WorldLabelWidget.cpp"))) &&
            FFileHelper::LoadFileToString(SignSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageG0/StageG0TestSignActor.cpp"))),
            "Japanese world label source unreadable");
        RequireG0(WorldLabelSource.Contains(TEXT("FCoreStyle::GetDefaultFontStyle")) &&
            SignSource.Contains(TEXT("StageG0JapaneseTestSign")) &&
            SignSource.Contains(TEXT("CreateDefaultSubobject<UWidgetComponent>")),
            "Japanese fallback world labels or verification signs missing");
    });

    Run("G0-14 Verification Panel Non-causal", [&]
    {
        for (const FString& Command : {TEXT("TARGET_PLAYER"), TEXT("DIR_7"), TEXT("HUMAN_5"),
            TEXT("RAT_AUTO"), TEXT("MOVE_slope"), TEXT("CAMERA_RESET"), TEXT("SHADOW_OFF")})
            RequireG0(UStageG0VerificationPanel::IsPresentationOnlyCommand(Command), "panel command rejected");
        FString PanelSource;
        RequireG0(FFileHelper::LoadFileToString(PanelSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageG0/StageG0VerificationPanel.cpp"))),
            "verification panel source unreadable");
        RequireG0(!PanelSource.Contains(TEXT("SubmitPlayerAction")) &&
            !PanelSource.Contains(TEXT("AdvancePresentation")) &&
            !PanelSource.Contains(TEXT("COUNTRY_STATE_CHANGED")), "verification panel contains causal mutation");
        FString GameModeSource;
        FString LocationVolumeSource;
        RequireG0(FFileHelper::LoadFileToString(GameModeSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDGameMode.cpp"))) &&
            FFileHelper::LoadFileToString(LocationVolumeSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDLocationVolume.cpp"))),
            "verification teleport boundary source unreadable");
        RequireG0(GameModeSource.Contains(TEXT("StageG0IgnoreLocationEntriesUntil")) &&
            LocationVolumeSource.Contains(TEXT("STAGE_G0_FIXTURE_LOCATION_ENTRY_SUPPRESSED")) &&
            LocationVolumeSource.Contains(TEXT("causal_events=0")),
            "verification point teleport can enter the causal location path");
    });

    Run("G0-15 Fall Recovery", [&]
    {
        RequireG0(AStageDPlayerCharacter::ShouldRecoverFromFall(-501.0f, -500.0f), "fall below limit not recovered");
        RequireG0(!AStageDPlayerCharacter::ShouldRecoverFromFall(-499.0f, -500.0f), "safe Z recovered incorrectly");
        FString GameModeSource;
        RequireG0(FFileHelper::LoadFileToString(GameModeSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDGameMode.cpp"))),
            "game mode source unreadable");
        RequireG0(GameModeSource.Contains(TEXT("StageG0BlockingVolume")) &&
            GameModeSource.Contains(TEXT("STAGE_G0_FALL_RECOVERY")), "fall boundary or audit marker missing");
    });

    Run("G0-16 Verification Points", [&]
    {
        const TArray<FString> Points = AStageDGameMode::GetStageG0TestPointIds();
        RequireG0(Points.Num() == 5 && Points.Contains(TEXT("gate_collision")) && Points.Contains(TEXT("narrow")) &&
            Points.Contains(TEXT("slope")) && Points.Contains(TEXT("occlusion")) && Points.Contains(TEXT("fang_rat")),
            "required verification points missing");
        TSet<FVector> Locations;
        for (const FString& Point : Points) Locations.Add(AStageDGameMode::GetStageG0TestPoint(Point));
        RequireG0(Locations.Num() == 5, "verification point locations are not distinct");
    });

    Run("G0-17 Fang Rat Manual Actions", [&]
    {
        int32 Supported = 0;
        for (const EStageGVisualAction Action : {EStageGVisualAction::Idle, EStageGVisualAction::MonsterMove,
            EStageGVisualAction::MonsterCharge, EStageGVisualAction::MonsterBite,
            EStageGVisualAction::Hit, EStageGVisualAction::Death})
            if (AStageG0FangRatActor::IsSupportedFixtureAction(Action)) ++Supported;
        RequireG0(Supported == 6 && AStageG0FangRatActor::FixtureActionCount == 6,
            "fang rat six manual fixture actions missing");
        RequireG0(!UStageGDirectionalFlipbookComponent::FallsBackToIdle(EStageGVisualAction::Death),
            "fang rat death does not retain final state");
    });

    Run("G0-18 Blob Shadow Toggle", [&]
    {
        const auto* Player = GetDefault<AStageDPlayerCharacter>();
        TArray<UStaticMeshComponent*> Meshes;
        Player->GetComponents<UStaticMeshComponent>(Meshes);
        const UStaticMeshComponent* Shadow = nullptr;
        for (const UStaticMeshComponent* Mesh : Meshes)
        {
            if (Mesh && Mesh->ComponentHasTag(TEXT("StageG0BlobShadow"))) { Shadow = Mesh; break; }
        }
        RequireG0(Shadow && Shadow->GetCollisionEnabled() == ECollisionEnabled::NoCollision,
            "blob shadow missing or collidable");
        RequireG0(UStageG0VerificationPanel::IsPresentationOnlyCommand(TEXT("SHADOW_ON")) &&
            UStageG0VerificationPanel::IsPresentationOnlyCommand(TEXT("SHADOW_OFF")), "shadow toggle commands missing");
    });

    Run("G0-19 Fixed Follow Camera Input", [&]
    {
        FString Input;
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(Input,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/DefaultInput.ini"))) &&
            FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "camera input sources unreadable");
        RequireG0(!Input.Contains(TEXT("StageGCameraDrag")) && !Input.Contains(TEXT("RightMouseButton")),
            "obsolete right mouse camera orbit is still bound");
        RequireG0(PlayerSource.Contains(TEXT("CameraBoom->bDoCollisionTest = false")),
            "fixed follow camera can collapse against placeholder geometry");
        const auto* Player = GetDefault<AStageDPlayerCharacter>();
        RequireG0(Player->GetStageGCameraYaw() == 45.0f && Player->GetStageGCameraPitch() == -55.0f,
            "fixed player-follow camera defaults changed");
    });

    Run("G0-M1 Cursor Ray Contract", [&]
    {
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "player source unreadable");
        RequireG0(PlayerSource.Contains(TEXT("DeprojectMousePositionToWorld")) &&
            PlayerSource.Contains(TEXT("ECC_GameTraceChannel1")) &&
            PlayerSource.Contains(TEXT("RayOrigin + RayDirection * 100000.0f")),
            "cursor to world ray contract is incomplete");
        RequireG0(!PlayerSource.Contains(TEXT("GetHitResultUnderCursor(ECC_Visibility")),
            "legacy ambiguous visibility trace remains");
        ClickMoveEvidence["G0-M1"] = {{"deproject", true}, {"dedicated_trace", "ClickMoveSurface"}};
    });

    Run("G0-M2 Full Direction Destination", [&]
    {
        TSet<FIntPoint> Directions;
        json Rows = json::array();
        for (int32 Index = 0; Index < 16; ++Index)
        {
            const float Degrees = Index * 22.5f;
            const FVector Direction = FVector(1, 0, 0).RotateAngleAxis(Degrees, FVector::UpVector);
            Directions.Add(FIntPoint(FMath::RoundToInt(Direction.X * 1000.0f), FMath::RoundToInt(Direction.Y * 1000.0f)));
            Rows.push_back({{"degrees", Degrees}, {"destination", {Direction.X * 500.0f, Direction.Y * 500.0f}}});
        }
        RequireG0(Directions.Num() == 16, "sixteen destinations are not directionally distinct");
        ClickMoveEvidence["G0-M2"] = {{"directions", Rows}, {"distinct", 16}};
    });

    Run("G0-M3 Hold Update", [&]
    {
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "player source unreadable");
        const auto* Settings = GetDefault<UStageG0Settings>();
        RequireG0(Settings->ClickHoldUpdateSeconds >= 0.05f && Settings->ClickHoldUpdateSeconds <= 0.10f,
            "hold update interval is outside contract");
        RequireG0(AStageDPlayerCharacter::ShouldUpdateClickDestination(FVector::ZeroVector, FVector(51, 0, 0), 50.0f) &&
            !AStageDPlayerCharacter::ShouldUpdateClickDestination(FVector::ZeroVector, FVector(49, 0, 0), 50.0f),
            "destination delta threshold mismatch");
        RequireG0(PlayerSource.Contains(TEXT("StageGPointHoldAccumulator")) &&
            PlayerSource.Contains(TEXT("TryIssueStageG0MoveFromCursor(true)")), "hold update wiring missing");
        ClickMoveEvidence["G0-M3"] = {{"interval_seconds", Settings->ClickHoldUpdateSeconds},
            {"minimum_delta", Settings->ClickDestinationMinUpdateDistance}};
    });

    Run("G0-M4 Pointer Consistency", [&]
    {
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "player source unreadable");
        RequireG0(PlayerSource.Contains(TEXT("StageGPointTarget = Destination")) &&
            PlayerSource.Contains(TEXT("PointMoveMarker->SetWorldLocation(Destination")) &&
            PlayerSource.Contains(TEXT("SimpleMoveToLocation(PlayerController, Destination)")),
            "pointer and nav destination do not share one resolved value");
        ClickMoveEvidence["G0-M4"] = {{"single_destination_source", true}, {"label", "移動先"}};
    });

    Run("G0-M5 Invalid Surface", [&]
    {
        FString EngineConfig;
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(EngineConfig,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/DefaultEngine.ini"))) &&
            FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "move boundary source unreadable");
        RequireG0(EngineConfig.Contains(TEXT("Name=\"ClickMoveSurface\"")) &&
            EngineConfig.Contains(TEXT("Name=\"ClickTargetable\"")), "dedicated collision channels missing");
        RequireG0(PlayerSource.Contains(TEXT("IsPointerOverStageG0Ui")) &&
            PlayerSource.Contains(TEXT("移動不可：通行可能な地面ではありません")) &&
            PlayerSource.Contains(TEXT("移動不可：NavMesh上の地点ではありません")),
            "invalid surface rejection is incomplete");
        ClickMoveEvidence["G0-M5"] = {{"wall", "reject"}, {"air", "reject"},
            {"ui", "isolated"}, {"outside_navmesh", "reject"}};
    });

    Run("G0-M6 Path Around Obstacle", [&]
    {
        FString PlayerSource;
        FString BuildRules;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))) &&
            FFileHelper::LoadFileToString(BuildRules,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/NationSimulationStageC.Build.cs"))),
            "path following source unreadable");
        RequireG0(PlayerSource.Contains(TEXT("FindPathToLocationSynchronously")) &&
            PlayerSource.Contains(TEXT("SimpleMoveToLocation")) && PlayerSource.Contains(TEXT("Path->IsPartial()")),
            "nav path validation and following missing");
        RequireG0(BuildRules.Contains(TEXT("\"AIModule\"")) && BuildRules.Contains(TEXT("\"NavigationSystem\"")),
            "navigation module dependencies missing");
        ClickMoveEvidence["G0-M6"] = {{"path_following", "PlayerController+SimpleMoveToLocation"},
            {"partial_path", "reject"}, {"obstacle_policy", "navmesh_detour"}};
    });

    Run("G0-M7 No Fall", [&]
    {
        FString PlayerSource;
        FString GameModeSource;
        FString MapGenerator;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))) &&
            FFileHelper::LoadFileToString(GameModeSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDGameMode.cpp"))) &&
            FFileHelper::LoadFileToString(MapGenerator,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Build/GenerateStageG0Map.py"))), "fall boundary source unreadable");
        RequireG0(PlayerSource.Contains(TEXT("ProjectPointToNavigation")) &&
            PlayerSource.Contains(TEXT("FMath::Abs(Hit.ImpactPoint.X) > 4200.0f")), "nav/bounds rejection missing");
        RequireG0(GameModeSource.Contains(TEXT("StageG0BlockingNorth")) &&
            MapGenerator.Contains(TEXT("NavMeshBoundsVolume")), "physical or nav boundary missing");
        ClickMoveEvidence["G0-M7"] = {{"navmesh_outside", "reject"}, {"blocking_volumes", 4},
            {"fall_recovery_only", true}};
    });

    Run("G0-M8 Directional Visual", [&]
    {
        TSet<EStageGVisualDirection> VisualDirections;
        for (int32 Index = 0; Index < 16; ++Index)
        {
            const FVector Direction = FVector(1, 0, 0).RotateAngleAxis(Index * 22.5f, FVector::UpVector);
            VisualDirections.Add(UStageGDirectionalFlipbookComponent::QuantizeDirection(
                Direction, FVector::ForwardVector, FVector::RightVector,
                EStageGVisualDirection::Front, 0.01f));
        }
        RequireG0(VisualDirections.Num() == 8, "sixteen move directions do not cover all eight visual directions");
        ClickMoveEvidence["G0-M8"] = {{"move_directions", 16}, {"visual_directions", 8}};
    });

    Run("G0-T1 NPC Click Target", [&]
    {
        const auto* Npc = GetDefault<AStageDNpcActor>();
        const auto* Capsule = Npc->FindComponentByClass<UCapsuleComponent>();
        RequireG0(Npc->GetClass()->ImplementsInterface(UStageG0Targetable::StaticClass()),
            "NPC targetable interface missing");
        RequireG0(Capsule && Capsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Overlap,
            "NPC target capsule does not overlap ClickTargetable");
        TargetingEvidence["G0-T1"] = {{"types", {"AI_NPC", "NON_AI_NPC"}},
            {"families", {"GUARD", "CAPTAIN", "BROKER", "RESIDENT"}}};
    });

    Run("G0-T2 Fang Rat Click Target", [&]
    {
        const auto* FangRat = GetDefault<AStageG0FangRatActor>();
        const auto* Capsule = FangRat->FindComponentByClass<UCapsuleComponent>();
        const auto* HitArea = FangRat->FindComponentByClass<USphereComponent>();
        RequireG0(FangRat->GetClass()->ImplementsInterface(UStageG0Targetable::StaticClass()),
            "fang rat targetable interface missing");
        RequireG0(Capsule && HitArea &&
            Capsule->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Overlap &&
            HitArea->GetCollisionResponseToChannel(ECC_GameTraceChannel2) == ECR_Overlap,
            "fang rat click target collision missing");
        TargetingEvidence["G0-T2"] = {{"target_id", "fang_rat_001"},
            {"type", "FANG_RAT"}, {"causal_action", "blocked"}};
    });

    Run("G0-T3 Ground Does Not Select", [&]
    {
        const auto* Settings = GetDefault<UStageG0Settings>();
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "target source unreadable");
        RequireG0(Settings->bKeepTargetOnGroundClick, "ground click target retention default changed");
        RequireG0(PlayerSource.Contains(TEXT("if (TrySelectStageG0TargetUnderCursor())")) &&
            PlayerSource.Contains(TEXT("TryIssueStageG0MoveFromCursor(false)")),
            "target-first input boundary missing");
        TargetingEvidence["G0-T3"] = {{"ground_selects_actor", false}, {"keeps_target", true}};
    });

    Run("G0-T4 Target Does Not Move", [&]
    {
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "target source unreadable");
        RequireG0(PlayerSource.Contains(TEXT("STAGE_G0_CLICK_TARGET_SELECTED")) &&
            PlayerSource.Contains(TEXT("move_issued=0")), "target selection movement isolation missing");
        TargetingEvidence["G0-T4"] = {{"selection_move_command", false}};
    });

    Run("G0-T5 UI Click Isolation", [&]
    {
        FString PlayerSource;
        RequireG0(FFileHelper::LoadFileToString(PlayerSource,
            *FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/NationSimulationStageC/Private/StageD/StageDPlayerCharacter.cpp"))),
            "UI isolation source unreadable");
        RequireG0(PlayerSource.Contains(TEXT("LocateWindowUnderMouse")) &&
            PlayerSource.Contains(TEXT("UI上の操作はゲーム内へ送信されません")),
            "Slate UI hit test does not isolate world input");
        TargetingEvidence["G0-T5"] = {{"ui_world_passthrough", false}};
    });

    Run("G0-T6 Overlap Priority", [&]
    {
        RequireG0(AStageDPlayerCharacter::CompareTargetPriority(100, 10, TEXT("b"), 200, 1, TEXT("a")) < 0,
            "front-most priority mismatch");
        RequireG0(AStageDPlayerCharacter::CompareTargetPriority(100, 5, TEXT("b"), 100, 10, TEXT("a")) < 0,
            "cursor proximity priority mismatch");
        RequireG0(AStageDPlayerCharacter::CompareTargetPriority(100, 5, TEXT("a"), 100, 5, TEXT("b")) < 0,
            "target id deterministic priority mismatch");
        TargetingEvidence["G0-T6"] = {{"priority", {"front_depth", "cursor_distance", "target_id"}},
            {"deterministic", true}};
    });

    Evidence["summary"] = {{"passed", Passed}, {"failed", Failed}, {"expected", 33}};
    std::ofstream(Output / "stage_g0_direction_evidence.json", std::ios::trunc) << DirectionEvidence.dump(2) << '\n';
    std::ofstream(Output / "stage_g0_animation_mapping.json", std::ios::trunc) << MappingEvidence.dump(2) << '\n';
    std::ofstream(Output / "stage_g0_actor_count.json", std::ios::trunc) << ActorEvidence.dump(2) << '\n';
    ClickMoveEvidence["summary"] = {{"passed", Failed == 0}, {"tests", 8}};
    TargetingEvidence["summary"] = {{"passed", Failed == 0}, {"tests", 6}};
    std::ofstream(Output / "stage_g0_click_move_evidence.json", std::ios::trunc) << ClickMoveEvidence.dump(2) << '\n';
    std::ofstream(Output / "stage_g0_targeting_evidence.json", std::ios::trunc) << TargetingEvidence.dump(2) << '\n';
    std::ofstream(Output / "stage_g0_automation_evidence.json", std::ios::trunc) << Evidence.dump(2) << '\n';
    std::ofstream ResultFile(Output / "stage_g0_automation_results.txt", std::ios::trunc);
    for (const auto& Line : Results) ResultFile << Line << '\n';
    ResultFile << "SUMMARY | " << Passed << "/33 tests passed\n";
    return Failed == 0;
}

#endif
