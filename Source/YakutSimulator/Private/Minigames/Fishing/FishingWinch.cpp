#include "Minigames/Fishing/FishingWinch.h"

#include "Components/StaticMeshComponent.h"
#include "Base/PhysicalLever.h"

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

void AFishingWinch::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    float TargetSpeed = 0;
    if (bMotorEnabled && !bEmergencyStopped && !bFaulted && ControlLever && FMath::Abs(ControlLever->Value) >= 0.15f)
    {
        TargetSpeed = ControlLever->Value * CableSpeed;
    }
    if (bEmergencyStopped)
    {
        DrumSpeed = 0;
    }
    else
    {
        DrumSpeed = FMath::FInterpConstantTo(DrumSpeed, TargetSpeed, DeltaSeconds, MotorAcceleration);
    }
    const float PreviousLength = CableLength;
    CableLength = FMath::Clamp(CableLength + DrumSpeed * DeltaSeconds, 0.0f, MaximumCableLength);
    DrumSpeed = (CableLength - PreviousLength) / FMath::Max(DeltaSeconds, SMALL_NUMBER);
    Drum->AddLocalRotation(FRotator((CableLength - PreviousLength) * 0.8f, 0, 0));
    Tension = FMath::FInterpTo(Tension, FMath::Max(0.0f, ExternalLoad + FMath::Abs(DrumSpeed) * 0.035f - (TensionSetting - 0.5f) * 24), DeltaSeconds, 2);
}
