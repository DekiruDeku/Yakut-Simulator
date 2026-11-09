#include "Trawl.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FishItem.h"
#include "FishSchool.h"
#include "FishingScenario.h"
#include "Kismet/KismetSystemLibrary.h"
#include "TrawlerVessel.h"
#include "VesselMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

ATrawl::ATrawl()
{
    PrimaryActorTick.bCanEverTick = true;
    Net = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Net"));
    SetRootComponent(Net);
    Net->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PortCable = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortCable"));
    PortCable->SetupAttachment(Net);
    StarboardCable = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardCable"));
    StarboardCable->SetupAttachment(Net);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    PortCable->SetStaticMesh(Cylinder.Object);
    StarboardCable->SetStaticMesh(Cylinder.Object);
    PortCable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StarboardCable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ATrawl::BeginPlay()
{
    Super::BeginPlay();
    if (Vessel)
    {
        AddTickPrerequisiteComponent(Vessel->Movement);
        SetActorLocation(Vessel->TowPoint->GetComponentLocation());
    }
}

int32 ATrawl::GetCatchCount() const
{
    int32 Total = 0;
    for (const FFishCatchEntry& Entry : Catch)
    {
        Total += Entry.Count;
    }
    return Total;
}

void ATrawl::MoveCable(float Distance)
{
    if (!Vessel || FMath::IsNearlyZero(Distance))
    {
        return;
    }
    if (State == ETrawlState::ReadyToUnload && Distance > 0)
    {
        return;
    }
    CableLength = FMath::Clamp(CableLength + Distance, 0.0f, MaximumCableLength);
    if (Distance > 0)
    {
        State = ETrawlState::Deploying;
        if (CableLength >= MinimumFishingLength)
        {
            State = ETrawlState::Towing;
            if (Scenario)
            {
                Scenario->bTrawlDeployed = true;
            }
        }
    }
    else
    {
        State = ETrawlState::Hauling;
        if (CableLength <= KINDA_SMALL_NUMBER)
        {
            State = ETrawlState::Stowed;
            if (GetCatchCount() > 0)
            {
                State = ETrawlState::ReadyToUnload;
                if (Scenario)
                {
                    Scenario->bTrawlRecovered = true;
                }
            }
        }
    }
    OnTrawlChanged.Broadcast();
}

void ATrawl::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!Vessel)
    {
        return;
    }
    const FVector PreviousPosition = GetActorLocation();
    const FVector TowPosition = Vessel->TowPoint->GetComponentLocation();
    FVector Destination = TowPosition - Vessel->GetActorForwardVector() * CableLength * 0.9f;
    Destination.Z = FMath::Lerp(TowPosition.Z, WaterHeight - FishingDepth, FMath::Clamp(CableLength / MinimumFishingLength, 0.0f, 1.0f));
    SetActorLocation(FMath::VInterpConstantTo(PreviousPosition, Destination, DeltaSeconds, 600));
    SetActorRotation(Vessel->GetActorRotation());
    UpdateCable(PortCable, TowPosition - Vessel->GetActorRightVector() * 120, GetActorLocation() - GetActorRightVector() * 90);
    UpdateCable(StarboardCable, TowPosition + Vessel->GetActorRightVector() * 120, GetActorLocation() + GetActorRightVector() * 90);
    if (State == ETrawlState::Towing && Vessel->Movement->Velocity.Size2D() > 40 && GetCatchCount() < Capacity)
    {
        CatchAccumulator += DeltaSeconds;
        if (CatchAccumulator >= 1)
        {
            CatchAccumulator -= 1;
            CollectFish(PreviousPosition);
        }
    }
    else
    {
        CatchAccumulator = 0;
    }
}

void ATrawl::UpdateCable(UStaticMeshComponent* Cable, const FVector& Start, const FVector& End)
{
    Cable->SetWorldLocation((Start + End) * 0.5f);
    Cable->SetWorldRotation(FRotationMatrix::MakeFromZ(End - Start).Rotator());
    Cable->SetWorldScale3D(FVector(0.022f, 0.022f, FVector::Distance(Start, End) / 100));
}

void ATrawl::CollectFish(const FVector& PreviousPosition)
{
    for (TActorIterator<AFishSchool> SchoolIterator(GetWorld()); SchoolIterator; ++SchoolIterator)
    {
        AFishSchool* School = *SchoolIterator;
        if (!School->Species || FVector::DistSquared(School->GetActorLocation(), FMath::ClosestPointOnSegment(School->GetActorLocation(), PreviousPosition, GetActorLocation())) > FMath::Square(School->Radius))
        {
            continue;
        }
        if (School->ConsumeFish(1) == 0)
        {
            continue;
        }
        FFishCatchEntry* Entry = Catch.FindByPredicate([School](const FFishCatchEntry& Candidate)
        {
            return Candidate.Species == School->Species;
        });
        if (!Entry)
        {
            FFishCatchEntry NewEntry;
            NewEntry.Species = School->Species;
            Entry = &Catch.Add_GetRef(NewEntry);
        }
        ++Entry->Count;
        if (Scenario)
        {
            Scenario->bFishCaught = true;
        }
        OnTrawlChanged.Broadcast();
        break;
    }
}

bool ATrawl::UnloadCatch()
{
    if (!Vessel || State != ETrawlState::ReadyToUnload || GetCatchCount() == 0)
    {
        return false;
    }
    if (!FishItemClass)
    {
        UKismetSystemLibrary::PrintString(this, TEXT("The trawl fish item class is not configured"), true, true, FLinearColor::Red);
        return false;
    }
    if (Vessel->Movement->Velocity.Size2D() > 20 || FVector::Distance(GetActorLocation(), Vessel->TowPoint->GetComponentLocation()) > 60)
    {
        if (Scenario)
        {
            Scenario->SetStatus(FText::FromString(TEXT("Остановите судно и дождитесь подъёма сети.")));
        }
        return false;
    }
    int32 SpawnedCount = 0;
    FActorSpawnParameters Parameters;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (FFishCatchEntry& Entry : Catch)
    {
        while (Entry.Count > 0)
        {
            const FVector Offset((SpawnedCount % 3) * 55 - 55, (SpawnedCount / 3) * 40, 0);
            AFishItem* Fish = GetWorld()->SpawnActor<AFishItem>(FishItemClass, Vessel->CatchDropPoint->GetComponentLocation() + Vessel->GetActorRotation().RotateVector(Offset), Vessel->GetActorRotation(), Parameters);
            if (!Fish)
            {
                return false;
            }
            Fish->InitializeFish(Entry.Species, Scenario);
            Fish->Body->SetPhysicsLinearVelocity(Vessel->Movement->Velocity);
            --Entry.Count;
            ++SpawnedCount;
            if (Scenario)
            {
                ++Scenario->UnloadedFish;
            }
        }
    }
    Catch.Reset();
    State = ETrawlState::Stowed;
    OnTrawlChanged.Broadcast();
    return true;
}
