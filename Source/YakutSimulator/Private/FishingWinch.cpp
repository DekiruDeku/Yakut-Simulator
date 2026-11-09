#include "FishingWinch.h"

#include "Components/StaticMeshComponent.h"
#include "FishingScenario.h"
#include "PhysicalLever.h"
#include "Trawl.h"

AFishingWinch::AFishingWinch()
{
    PrimaryActorTick.bCanEverTick = true;
    Foundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Foundation"));
    SetRootComponent(Foundation);
    Foundation->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Drum = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Drum"));
    Drum->SetupAttachment(Foundation);
    Drum->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFishingWinch::BeginPlay()
{
    Super::BeginPlay();
    if (Trawl)
    {
        Trawl->AddTickPrerequisiteActor(this);
    }
    if (ControlLever)
    {
        ControlLever->OnItemAction.AddUniqueDynamic(this, &AFishingWinch::TryRepair);
    }
    if (UnloadLever)
    {
        UnloadLever->OnValueChanged.AddUniqueDynamic(this, &AFishingWinch::TryUnload);
    }
}

void AFishingWinch::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!ControlLever || !Trawl || bFaulted || FMath::Abs(ControlLever->Value) < 0.15f)
    {
        return;
    }
    const float PreviousLength = Trawl->CableLength;
    Trawl->MoveCable(ControlLever->Value * CableSpeed * DeltaSeconds);
    Drum->AddLocalRotation(FRotator((Trawl->CableLength - PreviousLength) * 0.8f, 0, 0));
}

void AFishingWinch::TryUnload(float Value)
{
    if (Value > 0.6f && !bFaulted && Trawl)
    {
        Trawl->UnloadCatch();
    }
}

void AFishingWinch::SetFaulted(bool bFault)
{
    bFaulted = bFault;
    if (bFaulted && Trawl && Trawl->Scenario)
    {
        Trawl->Scenario->SetStatus(FText::FromString(TEXT("Сбой лебёдки. Верните рычаг в нейтраль и нажмите Trigger, удерживая рукоятку.")));
    }
}

void AFishingWinch::TryRepair(AWorkItem* Item, EWorkAction Action)
{
    if (bFaulted && Action == EWorkAction::Used && ControlLever && FMath::Abs(ControlLever->Value) < 0.15f)
    {
        bFaulted = false;
        if (Trawl && Trawl->Scenario)
        {
            Trawl->Scenario->SetStatus(FText::FromString(TEXT("Лебёдка готова к работе.")));
        }
    }
}

void AFishingWinch::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(ControlLever))
    {
        ControlLever->OnItemAction.RemoveDynamic(this, &AFishingWinch::TryRepair);
    }
    if (IsValid(UnloadLever))
    {
        UnloadLever->OnValueChanged.RemoveDynamic(this, &AFishingWinch::TryUnload);
    }
    Super::EndPlay(EndPlayReason);
}
