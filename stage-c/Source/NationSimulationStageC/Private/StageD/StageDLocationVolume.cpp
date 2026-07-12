#include "StageD/StageDLocationVolume.h"

#include "Components/BoxComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "StageD/NationSimulationGameInstanceSubsystem.h"
#include "StageD/StageDGameMode.h"
#include "StageD/StageDPlayerCharacter.h"

AStageDLocationVolume::AStageDLocationVolume()
{
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("LocationTrigger"));
    SetRootComponent(Trigger);
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->SetCanEverAffectNavigation(false);
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AStageDLocationVolume::OnLocationEntered);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LocationLabel"));
    Label->SetupAttachment(Trigger);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(38.0f);
    Label->SetTextRenderColor(FColor(255, 210, 80));
}

void AStageDLocationVolume::InitializeLocation(const FString& InLocationId, const FVector& HalfExtent)
{
    LocationId = InLocationId;
    Trigger->SetBoxExtent(HalfExtent);
    Label->SetText(FText::FromString(LocationId));
}

void AStageDLocationVolume::OnLocationEntered(UPrimitiveComponent*, AActor* OtherActor,
    UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (!Cast<AStageDPlayerCharacter>(OtherActor) || !GetGameInstance()) return;
    if (const AStageDGameMode* Mode = Cast<AStageDGameMode>(GetWorld()->GetAuthGameMode());
        Mode && Mode->ShouldIgnoreStageG0LocationEntry())
    {
        UE_LOG(LogTemp, Display, TEXT("STAGE_G0_FIXTURE_LOCATION_ENTRY_SUPPRESSED location=%s causal_events=0"), *LocationId);
        return;
    }
    if (auto* Subsystem = GetGameInstance()->GetSubsystem<UNationSimulationGameInstanceSubsystem>())
    {
        Subsystem->EnterLocation(LocationId);
    }
}
