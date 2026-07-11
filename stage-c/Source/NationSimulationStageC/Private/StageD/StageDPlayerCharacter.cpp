#include "StageD/StageDPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDGameMode.h"
#include "StageD/StageDHudWidget.h"
#include "StageD/StageDNpcActor.h"
#include "UObject/ConstructorHelpers.h"

AStageDPlayerCharacter::AStageDPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
    GetCharacterMovement()->MaxWalkSpeed = 600.0f;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(GetRootComponent());
    CameraBoom->TargetArmLength = 420.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    PlaceholderBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderBody"));
    PlaceholderBody->SetupAttachment(GetRootComponent());
    PlaceholderBody->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
    PlaceholderBody->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.95f));
    PlaceholderBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CapsuleMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CapsuleMesh.Succeeded()) PlaceholderBody->SetStaticMesh(CapsuleMesh.Object);
}

void AStageDPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AStageDPlayerCharacter::MoveForward);
    PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AStageDPlayerCharacter::MoveRight);
    PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AStageDPlayerCharacter::Turn);
    PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AStageDPlayerCharacter::LookUp);
    PlayerInputComponent->BindAction(TEXT("Talk"), IE_Pressed, this, &AStageDPlayerCharacter::Talk);
    PlayerInputComponent->BindAction(TEXT("Help"), IE_Pressed, this, &AStageDPlayerCharacter::Help);
    PlayerInputComponent->BindAction(TEXT("Harm"), IE_Pressed, this, &AStageDPlayerCharacter::Harm);
    PlayerInputComponent->BindAction(TEXT("Trade"), IE_Pressed, this, &AStageDPlayerCharacter::Trade);
    PlayerInputComponent->BindAction(TEXT("Steal"), IE_Pressed, this, &AStageDPlayerCharacter::Steal);
    PlayerInputComponent->BindAction(TEXT("Wait"), IE_Pressed, this, &AStageDPlayerCharacter::WaitAction);
    PlayerInputComponent->BindAction(TEXT("MoveLocation"), IE_Pressed, this, &AStageDPlayerCharacter::MoveToNextLocation);
    PlayerInputComponent->BindAction(TEXT("CycleTarget"), IE_Pressed, this, &AStageDPlayerCharacter::CycleTarget);
    PlayerInputComponent->BindAction(TEXT("SaveSimulation"), IE_Pressed, this, &AStageDPlayerCharacter::SaveSimulation);
    PlayerInputComponent->BindAction(TEXT("LoadSimulation"), IE_Pressed, this, &AStageDPlayerCharacter::LoadSimulation);
    PlayerInputComponent->BindAction(TEXT("ToggleDebug"), IE_Pressed, this, &AStageDPlayerCharacter::ToggleDebug);
}

void AStageDPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TargetScanAccumulator += DeltaSeconds;
    if (TargetScanAccumulator >= 0.15f)
    {
        TargetScanAccumulator = 0.0f;
        ScanTarget();
    }
}

void AStageDPlayerCharacter::MoveForward(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator Rotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(Rotation).GetUnitAxis(EAxis::X), Value);
    }
}

void AStageDPlayerCharacter::MoveRight(float Value)
{
    if (Controller && Value != 0.0f)
    {
        const FRotator Rotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
        AddMovementInput(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y), Value);
    }
}

void AStageDPlayerCharacter::Turn(float Value) { AddControllerYawInput(Value); }
void AStageDPlayerCharacter::LookUp(float Value) { AddControllerPitchInput(Value); }
void AStageDPlayerCharacter::Talk() { ExecuteCoreAction(TEXT("TALK")); }
void AStageDPlayerCharacter::Help() { ExecuteCoreAction(TEXT("HELP")); }
void AStageDPlayerCharacter::Harm() { ExecuteCoreAction(TEXT("HARM")); }
void AStageDPlayerCharacter::Trade() { ExecuteCoreAction(TEXT("TRADE")); }
void AStageDPlayerCharacter::Steal() { ExecuteCoreAction(TEXT("STEAL")); }
void AStageDPlayerCharacter::WaitAction() { ExecuteCoreAction(TEXT("WAIT")); }

void AStageDPlayerCharacter::ExecuteCoreAction(const FString& Action)
{
    if (!GetGameInstance()) return;
    if (auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>())
    {
        const FString Target = Action == TEXT("WAIT") ? TEXT("") : Subsystem->GetWorldView().TargetNpcId;
        Subsystem->SubmitPlayerAction(Action, Target, TEXT(""));
    }
}

void AStageDPlayerCharacter::MoveToNextLocation()
{
    if (!GetGameInstance()) return;
    auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;
    static const TArray<FString> Locations = {TEXT("capital"), TEXT("market"), TEXT("tavern"), TEXT("residential"), TEXT("gate")};
    const int32 Current = Locations.IndexOfByKey(Subsystem->GetWorldView().CurrentLocationId);
    const FString& Next = Locations[(Current + 1 + Locations.Num()) % Locations.Num()];
    bManualTargetSelection = false;
    if (Subsystem->EnterLocation(Next)) SetActorLocation(AStageDGameMode::GetLocationCenter(Next) + FVector(0.0f, 0.0f, 140.0f));
}

void AStageDPlayerCharacter::CycleTarget()
{
    if (!GetGameInstance()) return;
    auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;
    TArray<AStageDNpcActor*> Npcs;
    for (TActorIterator<AStageDNpcActor> It(GetWorld()); It; ++It)
    {
        if (IsValid(*It) && FVector::Distance(GetActorLocation(), It->GetActorLocation()) <= Subsystem->GetInteractionRange())
        {
            Npcs.Add(*It);
        }
    }
    Npcs.Sort([](const AStageDNpcActor& A, const AStageDNpcActor& B) { return A.GetNpcId() < B.GetNpcId(); });
    if (Npcs.IsEmpty())
    {
        bManualTargetSelection = false;
        Subsystem->ClearTargetNpc(TEXT("No active NPC is within interaction range"));
        return;
    }
    const FString Current = Subsystem->GetWorldView().TargetNpcId;
    int32 Index = Npcs.IndexOfByPredicate([&](const AStageDNpcActor* Npc) { return Npc->GetNpcId() == Current; });
    Index = (Index + 1 + Npcs.Num()) % Npcs.Num();
    Subsystem->SetTargetNpc(Npcs[Index]->GetNpcId());
    bManualTargetSelection = true;
    TargetLostSeconds = 0.0f;
    UE_LOG(LogTemp, Display, TEXT("STAGE_D_TARGET_SELECTED npc=%s method=TAB"), *Npcs[Index]->GetNpcId());
}

void AStageDPlayerCharacter::ScanTarget()
{
    if (!FollowCamera || !GetGameInstance()) return;
    auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>();
    if (!Subsystem) return;

    // A target explicitly chosen with Tab remains selected while the core still
    // considers it valid. Range/location/despawn invalidation remains owned by
    // the subsystem; looking away must not silently cancel manual selection.
    if (bManualTargetSelection)
    {
        if (!Subsystem->GetWorldView().TargetNpcId.IsEmpty())
        {
            TargetLostSeconds = 0.0f;
            return;
        }
        bManualTargetSelection = false;
    }

    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * 1800.0f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(StageDTargetScan), false, this);
    bool bSawValidNpc = false;
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
    {
        if (const auto* Npc = Cast<AStageDNpcActor>(Hit.GetActor()))
        {
            if (FVector::Distance(GetActorLocation(), Npc->GetActorLocation()) <= Subsystem->GetInteractionRange())
            {
                Subsystem->SetTargetNpc(Npc->GetNpcId());
                TargetLostSeconds = 0.0f;
                bSawValidNpc = true;
            }
        }
    }
    if (!bSawValidNpc)
    {
        TargetLostSeconds += 0.15f;
        if (TargetLostSeconds >= 1.0f)
        {
            if (!Subsystem->GetWorldView().TargetNpcId.IsEmpty())
            {
                Subsystem->ClearTargetNpc(TEXT("Target NPC was outside the view/range timeout"));
            }
            TargetLostSeconds = 0.0f;
        }
    }
}

void AStageDPlayerCharacter::SaveSimulation()
{
    if (GetGameInstance()) GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>()->SaveMidChain();
}

void AStageDPlayerCharacter::LoadSimulation()
{
    if (GetGameInstance()) GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>()->ReloadSave();
}

void AStageDPlayerCharacter::ToggleDebug()
{
    if (const auto* GameMode = GetWorld()->GetAuthGameMode<AStageDGameMode>())
    {
        if (GameMode->GetStageDHud()) GameMode->GetStageDHud()->ToggleDebug();
    }
}
