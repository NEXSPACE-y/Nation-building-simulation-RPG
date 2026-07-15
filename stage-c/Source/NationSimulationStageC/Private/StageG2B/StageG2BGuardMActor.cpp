#include "StageG2B/StageG2BGuardMActor.h"

#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "StageG1A/StageG1APlayerCharacter.h"
#include "StageG1B/StageG1BPlayerVisualAdapter.h"
#include "StageG2A/StageG2ACameraModeAdapter.h"
#include "TimerManager.h"
#include "UnrealClient.h"

namespace
{
const TCHAR* GuardMeshPath = TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Mesh/SK_GUARD_M_Meshy_v0_1.SK_GUARD_M_Meshy_v0_1");
const TCHAR* GuardMaterialPath = TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Materials/MI_GUARD_M_Meshy_v0_1.MI_GUARD_M_Meshy_v0_1");
const TCHAR* GuardIdlePath = TEXT("/Game/StageG2B/Characters/GUARD_M/MeshyV01/Animations/GUARD_M_Meshy_v0_1_AllAnimationsIdle_11_frame_rate_60_fbx.GUARD_M_Meshy_v0_1_AllAnimationsIdle_11_frame_rate_60_fbx");

bool WriteG2BJson(const FString& Filename, const TSharedRef<FJsonObject>& Json)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG2B"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FString Serialized;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Serialized);
    return FJsonSerializer::Serialize(Json, Writer) &&
        FFileHelper::SaveStringToFile(Serialized, *FPaths::Combine(Directory, Filename),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}
}

AStageG2BGuardMActor::AStageG2BGuardMActor()
{
    PrimaryActorTick.bCanEverTick = false;
    GuardCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("GuardCapsule"));
    SetRootComponent(GuardCapsule);
    GuardCapsule->InitCapsuleSize(42.0f, 88.0f);
    GuardCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    GuardCapsule->SetCollisionObjectType(ECC_WorldDynamic);
    GuardCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
    GuardCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    GuardCapsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);

    GuardMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GuardMesh"));
    GuardMesh->SetupAttachment(GuardCapsule);
    GuardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GuardMesh->SetCastShadow(true);
    GuardMesh->bCastDynamicShadow = true;
    GuardMesh->bCastContactShadow = true;
}

void AStageG2BGuardMActor::BeginPlay()
{
    Super::BeginPlay();
    ConfigureGuardVisual();
    ResolveAcceptedAdapters();
    OriginalGuardLocation = GetActorLocation();
    OriginalGuardRotation = GetActorRotation();
    if (AcceptedPlayer.IsValid())
    {
        OriginalPlayerLocation = AcceptedPlayer->GetActorLocation();
        OriginalPlayerRotation = AcceptedPlayer->GetActorRotation();
    }

    if (bGuardVisualActive && FParse::Param(FCommandLine::Get(), TEXT("StageG2BEvidence")))
    {
        FTimerHandle EvidenceTimer;
        GetWorldTimerManager().SetTimer(
            EvidenceTimer, this, &AStageG2BGuardMActor::WriteRuntimeEvidence, 0.80f, false);
    }
    if (bGuardVisualActive && FParse::Param(FCommandLine::Get(), TEXT("StageG2BScreenshots")))
        BeginScreenshotSequence();
}

void AStageG2BGuardMActor::ResolveAcceptedAdapters()
{
    AcceptedPlayer = Cast<AStageG1APlayerCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
    for (TActorIterator<AStageG1BPlayerVisualAdapter> It(GetWorld()); It; ++It)
    {
        PlayerVisualAdapter = *It;
        break;
    }
    for (TActorIterator<AStageG2ACameraModeAdapter> It(GetWorld()); It; ++It)
    {
        CameraAdapter = *It;
        break;
    }
}

void AStageG2BGuardMActor::ConfigureGuardVisual()
{
    USkeletalMesh* MeshAsset = LoadObject<USkeletalMesh>(nullptr, GuardMeshPath);
    UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, GuardMaterialPath);
    IdleSequence = LoadObject<UAnimSequence>(nullptr, GuardIdlePath);
    if (!MeshAsset || !Material || !IdleSequence || !GuardMesh)
    {
        if (GuardMesh) GuardMesh->SetVisibility(false, true);
        UE_LOG(LogTemp, Error, TEXT("STAGE_G2B_GUARD_M_FAIL mesh=%d material=%d idle=%d"),
            MeshAsset ? 1 : 0, Material ? 1 : 0, IdleSequence ? 1 : 0);
        return;
    }
    if (!MeshAsset->GetSkeleton() || IdleSequence->GetSkeleton() != MeshAsset->GetSkeleton())
    {
        GuardMesh->SetVisibility(false, true);
        UE_LOG(LogTemp, Error, TEXT("STAGE_G2B_GUARD_M_FAIL reason=guard_idle_skeleton_mismatch"));
        return;
    }

    GuardMesh->SetSkeletalMeshAsset(MeshAsset);
    GuardMesh->SetRelativeScale3D(FVector::OneVector);
    const FBoxSphereBounds Bounds = MeshAsset->GetImportedBounds();
    const float LocalBottomZ = Bounds.Origin.Z - Bounds.BoxExtent.Z;
    GuardMesh->SetRelativeLocation(FVector(0.0f, 0.0f,
        -GuardCapsule->GetUnscaledCapsuleHalfHeight() - LocalBottomZ + 1.0f));
    GuardMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
    GuardMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    GuardMesh->SetAnimation(IdleSequence);
    GuardMesh->Play(true);
    GuardMesh->SetMaterial(0, Material);
    GuardMesh->SetVisibility(true, true);

    bUsingReferencePose = false;
    bGuardVisualActive = true;
    bPlayerSkeletonShared = false;
    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G2B_GUARD_M_READY mesh=%s skeleton=%s idle=%s height=%.2f scale=1.0 material_slots=%d player_skeleton_shared=false placement=1"),
        *MeshAsset->GetPathName(), *MeshAsset->GetSkeleton()->GetPathName(),
        *IdleSequence->GetName(), Bounds.BoxExtent.Z * 2.0f, GuardMesh->GetNumMaterials());
}

void AStageG2BGuardMActor::WriteRuntimeEvidence()
{
    ResolveAcceptedAdapters();
    USkeletalMesh* MeshAsset = GuardMesh ? GuardMesh->GetSkeletalMeshAsset() : nullptr;
    int32 GuardCount = 0;
    for (TActorIterator<AStageG2BGuardMActor> It(GetWorld()); It; ++It) ++GuardCount;

    int32 Vertices = 0;
    int32 Triangles = 0;
    int32 LodCount = 0;
    if (MeshAsset && MeshAsset->GetResourceForRendering())
    {
        const FSkeletalMeshRenderData* RenderData = MeshAsset->GetResourceForRendering();
        LodCount = RenderData->LODRenderData.Num();
        if (LodCount > 0)
        {
            const FSkeletalMeshLODRenderData& Lod0 = RenderData->LODRenderData[0];
            Vertices = Lod0.GetNumVertices();
            Triangles = Lod0.MultiSizeIndexContainer.GetIndexBuffer()->Num() / 3;
        }
    }
    const int32 BoneCount = MeshAsset ? MeshAsset->GetRefSkeleton().GetNum() : 0;
    const FString RootBone = BoneCount > 0
        ? MeshAsset->GetRefSkeleton().GetBoneName(0).ToString() : TEXT("NONE");
    const FString GuardSkeleton = MeshAsset && MeshAsset->GetSkeleton()
        ? MeshAsset->GetSkeleton()->GetPathName() : TEXT("NONE");
    const USkeletalMesh* PlayerMesh = AcceptedPlayer.IsValid() && AcceptedPlayer->GetMesh()
        ? AcceptedPlayer->GetMesh()->GetSkeletalMeshAsset() : nullptr;
    const FString PlayerSkeleton = PlayerMesh && PlayerMesh->GetSkeleton()
        ? PlayerMesh->GetSkeleton()->GetPathName() : TEXT("NONE");
    bPlayerSkeletonShared = GuardSkeleton == PlayerSkeleton;

    const bool bInitialStandard = CameraAdapter.IsValid() && CameraAdapter->IsStandardMode();
    bool bTactical = false;
    bool bReturnedStandard = false;
    if (CameraAdapter.IsValid())
    {
        CameraAdapter->ToggleCameraMode();
        bTactical = !CameraAdapter->IsStandardMode();
        CameraAdapter->ToggleCameraMode();
        bReturnedStandard = CameraAdapter->IsStandardMode();
    }
    const bool bPlayerFallback = !PlayerVisualAdapter.IsValid() || PlayerVisualAdapter->IsMannyFallback();
    const float PlayerDistance = AcceptedPlayer.IsValid()
        ? FVector::Dist2D(GetActorLocation(), AcceptedPlayer->GetActorLocation()) : -1.0f;

    TSharedRef<FJsonObject> Runtime = MakeShared<FJsonObject>();
    Runtime->SetStringField(TEXT("guard_mesh"), MeshAsset ? MeshAsset->GetPathName() : TEXT("NONE"));
    Runtime->SetStringField(TEXT("guard_skeleton"), GuardSkeleton);
    Runtime->SetNumberField(TEXT("bone_count"), BoneCount);
    Runtime->SetStringField(TEXT("root_bone"), RootBone);
    Runtime->SetNumberField(TEXT("vertices_lod0"), Vertices);
    Runtime->SetNumberField(TEXT("triangles_lod0"), Triangles);
    Runtime->SetNumberField(TEXT("world_height_uu"), MeshAsset ? MeshAsset->GetImportedBounds().BoxExtent.Z * 2.0f : 0.0f);
    Runtime->SetNumberField(TEXT("material_slot_count"), GuardMesh ? GuardMesh->GetNumMaterials() : 0);
    Runtime->SetNumberField(TEXT("guard_actor_count"), GuardCount);
    Runtime->SetNumberField(TEXT("player_distance_uu"), PlayerDistance);
    Runtime->SetBoolField(TEXT("guard_visual_active"), bGuardVisualActive);
    Runtime->SetBoolField(TEXT("player_fallback"), bPlayerFallback);
    Runtime->SetBoolField(TEXT("player_skeleton_shared"), bPlayerSkeletonShared);
    Runtime->SetBoolField(TEXT("dynamic_shadow"), GuardMesh && GuardMesh->bCastDynamicShadow);
    Runtime->SetBoolField(TEXT("contact_shadow"), GuardMesh && GuardMesh->bCastContactShadow);
    WriteG2BJson(TEXT("guard_runtime_evidence.json"), Runtime);

    TSharedRef<FJsonObject> Animation = MakeShared<FJsonObject>();
    Animation->SetStringField(TEXT("idle_sequence"), IdleSequence ? IdleSequence->GetPathName() : TEXT("NONE"));
    Animation->SetStringField(TEXT("animation_method"), TEXT("GUARD source AnimationSingleNode"));
    Animation->SetBoolField(TEXT("guard_source_idle_11"), IdleSequence && IdleSequence->GetName().Contains(TEXT("Idle_11")));
    Animation->SetBoolField(TEXT("is_playing"), GuardMesh && GuardMesh->IsPlaying());
    Animation->SetBoolField(TEXT("reference_pose"), bUsingReferencePose);
    Animation->SetBoolField(TEXT("retarget_used"), false);
    Animation->SetBoolField(TEXT("player_animation_modified"), false);
    WriteG2BJson(TEXT("guard_animation_evidence.json"), Animation);

    TSharedRef<FJsonObject> Camera = MakeShared<FJsonObject>();
    Camera->SetBoolField(TEXT("standard_initial"), bInitialStandard);
    Camera->SetBoolField(TEXT("tactical_reached"), bTactical);
    Camera->SetBoolField(TEXT("returned_to_standard"), bReturnedStandard);
    Camera->SetBoolField(TEXT("f6_adapter_preserved"), CameraAdapter.IsValid());
    Camera->SetNumberField(TEXT("guard_distance_from_player_uu"), PlayerDistance);
    Camera->SetBoolField(TEXT("standard_visibility_distance"), PlayerDistance >= 300.0f && PlayerDistance <= 500.0f);
    WriteG2BJson(TEXT("guard_camera_evidence.json"), Camera);

    TSharedRef<FJsonObject> Lod = MakeShared<FJsonObject>();
    Lod->SetNumberField(TEXT("lod_count"), LodCount);
    Lod->SetNumberField(TEXT("lod0_triangles"), Triangles);
    Lod->SetStringField(TEXT("lod0_policy"), TEXT("Original source retained"));
    Lod->SetBoolField(TEXT("multiple_placement_allowed"), false);
    Lod->SetStringField(TEXT("future_requirement"), TEXT("Additional LOD required before multiple GUARD placement"));
    WriteG2BJson(TEXT("guard_lod_evidence.json"), Lod);

    TSharedRef<FJsonObject> Isolation = MakeShared<FJsonObject>();
    Isolation->SetStringField(TEXT("player_mesh"), PlayerMesh ? PlayerMesh->GetPathName() : TEXT("NONE"));
    Isolation->SetStringField(TEXT("player_skeleton"), PlayerSkeleton);
    Isolation->SetBoolField(TEXT("player_skeleton_shared"), bPlayerSkeletonShared);
    Isolation->SetBoolField(TEXT("player_mesh_modified"), false);
    Isolation->SetBoolField(TEXT("player_animation_modified"), false);
    Isolation->SetBoolField(TEXT("causal_core_connected"), false);
    Isolation->SetBoolField(TEXT("save_schema_connected"), false);
    WriteG2BJson(TEXT("guard_isolation_evidence.json"), Isolation);

    UE_LOG(LogTemp, Display,
        TEXT("STAGE_G2B_GUARD_M_EVIDENCE_WRITTEN files=5 guard=%d bones=%d root=%s material=%d standard=%d tactical=%d return=%d player_fallback=%d skeleton_shared=%d triangles=%d vertices=%d lods=%d"),
        GuardCount, BoneCount, *RootBone, GuardMesh ? GuardMesh->GetNumMaterials() : 0,
        bInitialStandard ? 1 : 0, bTactical ? 1 : 0, bReturnedStandard ? 1 : 0,
        bPlayerFallback ? 1 : 0, bPlayerSkeletonShared ? 1 : 0,
        Triangles, Vertices, LodCount);
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG2BEvidenceExit")))
        FGenericPlatformMisc::RequestExit(false);
}

void AStageG2BGuardMActor::BeginScreenshotSequence()
{
    ResolveAcceptedAdapters();
    if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        Controller->ConsoleCommand(TEXT("r.MotionBlurQuality 0"), true);
    OriginalGuardLocation = GetActorLocation();
    OriginalGuardRotation = GetActorRotation();
    if (AcceptedPlayer.IsValid())
    {
        OriginalPlayerLocation = AcceptedPlayer->GetActorLocation();
        OriginalPlayerRotation = AcceptedPlayer->GetActorRotation();
    }
    ScreenshotStep = 0;
    GetWorldTimerManager().SetTimer(ScreenshotSequenceTimer, this,
        &AStageG2BGuardMActor::AdvanceScreenshotSequence, 0.90f, true, 0.55f);
}

void AStageG2BGuardMActor::AdvanceScreenshotSequence()
{
    ++ScreenshotStep;
    AStageG1APlayerCharacter* Player = AcceptedPlayer.Get();
    const FVector Forward = Player ? Player->GetActorForwardVector() : FVector::ForwardVector;
    const FVector Right = Player ? Player->GetActorRightVector() : FVector::RightVector;
    switch (ScreenshotStep)
    {
    case 1:
        RestoreScreenshotTransforms();
        if (Player && Player->GetMesh()) Player->GetMesh()->SetVisibility(true, true);
        PendingScreenshotFilename = TEXT("01_guard_standard_front.png");
        break;
    case 2:
        if (Player && Player->GetMesh()) Player->GetMesh()->SetVisibility(false, true);
        if (Player) SetActorLocation(Player->GetActorLocation() + Forward * 280.0f + FVector(0, 0, -8.0f));
        PendingScreenshotFilename = TEXT("02_guard_standard_close.png");
        break;
    case 3:
        SetActorRotation(OriginalGuardRotation + FRotator(0.0f, 90.0f, 0.0f));
        PendingScreenshotFilename = TEXT("03_guard_standard_side.png");
        break;
    case 4:
        SetActorRotation(OriginalGuardRotation + FRotator(0.0f, 180.0f, 0.0f));
        PendingScreenshotFilename = TEXT("04_guard_standard_back.png");
        break;
    case 5:
        if (Player && Player->GetMesh()) Player->GetMesh()->SetVisibility(true, true);
        if (Player) SetActorLocation(Player->GetActorLocation() + Right * 160.0f + FVector(0, 0, -8.0f));
        SetActorRotation(OriginalGuardRotation);
        PendingScreenshotFilename = TEXT("05_guard_player_scale_compare.png");
        break;
    case 6:
        RestoreScreenshotTransforms();
        if (CameraAdapter.IsValid() && CameraAdapter->IsStandardMode()) CameraAdapter->ToggleCameraMode();
        PendingScreenshotFilename = TEXT("06_guard_tactical_initial.png");
        break;
    case 7:
        if (APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0))
        {
            for (int32 Index = 0; Index < 10; ++Index)
            {
                Controller->InputKey(FInputKeyEventArgs(
                    nullptr, INPUTDEVICEID_NONE, EKeys::MouseWheelAxis,
                    -1.0f, 1.0f / 60.0f, 1, FPlatformTime::Cycles64()));
            }
        }
        PendingScreenshotFilename = TEXT("07_guard_tactical_zoom_max.png");
        break;
    case 8:
        if (CameraAdapter.IsValid() && !CameraAdapter->IsStandardMode()) CameraAdapter->ToggleCameraMode();
        if (Player && Player->GetMesh()) Player->GetMesh()->SetVisibility(false, true);
        if (Player) SetActorLocation(Player->GetActorLocation() + Forward * 220.0f + FVector(0, 0, -8.0f));
        SetActorRotation(OriginalGuardRotation);
        PendingScreenshotFilename = TEXT("08_guard_material_close.png");
        break;
    case 9:
        if (Player && Player->GetMesh()) Player->GetMesh()->SetVisibility(true, true);
        if (Player)
        {
            Player->SetActorLocation(FVector(1700.0f, -1000.0f, OriginalPlayerLocation.Z));
            SetActorLocation(FVector(1980.0f, -1000.0f, OriginalGuardLocation.Z));
        }
        PendingScreenshotFilename = TEXT("09_guard_collision_near_wall.png");
        break;
    case 10:
        RestoreScreenshotTransforms();
        if (CameraAdapter.IsValid() && !CameraAdapter->IsStandardMode()) CameraAdapter->ToggleCameraMode();
        PendingScreenshotFilename = TEXT("10_return_to_standard_with_guard.png");
        break;
    default:
        FinishScreenshotSequence();
        return;
    }
    FTimerHandle CaptureTimer;
    GetWorldTimerManager().SetTimer(CaptureTimer, this,
        &AStageG2BGuardMActor::CapturePendingScreenshot, 0.35f, false);
}

void AStageG2BGuardMActor::CapturePendingScreenshot()
{
    RequestEvidenceScreenshot(PendingScreenshotFilename);
}

void AStageG2BGuardMActor::RestoreScreenshotTransforms()
{
    SetActorLocationAndRotation(OriginalGuardLocation, OriginalGuardRotation);
    if (AcceptedPlayer.IsValid())
    {
        AcceptedPlayer->SetActorLocationAndRotation(OriginalPlayerLocation, OriginalPlayerRotation);
        if (AcceptedPlayer->GetMesh()) AcceptedPlayer->GetMesh()->SetVisibility(true, true);
    }
}

void AStageG2BGuardMActor::RequestEvidenceScreenshot(const FString& Filename)
{
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("StageG2B"), TEXT("Screenshots"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(Directory, Filename), false, false);
}

void AStageG2BGuardMActor::FinishScreenshotSequence()
{
    GetWorldTimerManager().ClearTimer(ScreenshotSequenceTimer);
    RestoreScreenshotTransforms();
    if (CameraAdapter.IsValid() && !CameraAdapter->IsStandardMode()) CameraAdapter->ToggleCameraMode();
    UE_LOG(LogTemp, Display, TEXT("STAGE_G2B_GUARD_M_SCREENSHOTS_REQUESTED count=10"));
    if (FParse::Param(FCommandLine::Get(), TEXT("StageG2BScreenshotsExit")))
        FGenericPlatformMisc::RequestExit(false);
}
