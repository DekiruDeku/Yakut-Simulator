#include "Minigames/Fishing/TrawlerVessel.h"

#include "Components/BoxComponent.h"
#include "Base/PhysicalLever.h"
#include "Minigames/Fishing/VesselMovementComponent.h"

ATrawlerVessel::ATrawlerVessel()
{
    Hull = CreateDefaultSubobject<UBoxComponent>(TEXT("Hull"));
    SetRootComponent(Hull);
    Hull->SetBoxExtent(FVector(700, 400, 45));
    Hull->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Hull->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Ignore);
    Hull->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    Movement = CreateDefaultSubobject<UVesselMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Hull);
    TowPoint = CreateDefaultSubobject<USceneComponent>(TEXT("TowPoint"));
    TowPoint->SetupAttachment(Hull);
    TowPoint->SetRelativeLocation(FVector(-700, 0, 200));
    CatchDropPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CatchDropPoint"));
    CatchDropPoint->SetupAttachment(Hull);
    CatchDropPoint->SetRelativeLocation(FVector(-500, 130, 170));
}

void ATrawlerVessel::BeginPlay()
{
    Super::BeginPlay();
    TArray<AActor*> Equipment;
    GetAttachedActors(Equipment, true, true);
    for (AActor* Part : Equipment)
    {
        Hull->IgnoreActorWhenMoving(Part, true);
    }
    if (ThrottleLever)
    {
        ThrottleLever->OnValueChanged.AddUniqueDynamic(this, &ATrawlerVessel::SetThrottle);
        Movement->SetThrottle(ThrottleLever->Value);
    }
    if (SteeringLever)
    {
        SteeringLever->OnValueChanged.AddUniqueDynamic(this, &ATrawlerVessel::SetSteering);
        Movement->SetSteering(SteeringLever->Value);
    }
}

void ATrawlerVessel::SetThrottle(float Value)
{
    Movement->SetThrottle(Value);
}

void ATrawlerVessel::SetSteering(float Value)
{
    Movement->SetSteering(Value);
}

void ATrawlerVessel::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(ThrottleLever))
    {
        ThrottleLever->OnValueChanged.RemoveDynamic(this, &ATrawlerVessel::SetThrottle);
    }
    if (IsValid(SteeringLever))
    {
        SteeringLever->OnValueChanged.RemoveDynamic(this, &ATrawlerVessel::SetSteering);
    }
    Super::EndPlay(EndPlayReason);
}
