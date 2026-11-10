#include "Minigames/Fishing/FishingStations.h"

#include "Components/StaticMeshComponent.h"
#include "Minigames/Fishing/FishItem.h"
#include "Base/ScenarioDirector.h"
#include "Base/InteractionZoneComponent.h"
#include "Base/PhysicalLever.h"
#include "UObject/ConstructorHelpers.h"

AFishingStation::AFishingStation()
{
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Surface"));
    Surface->SetupAttachment(Root);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Surface->SetStaticMesh(Cube.Object);
    Surface->SetRelativeScale3D(FVector(1.2, 0.8, 0.06));
    Surface->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Intake = CreateDefaultSubobject<UInteractionZoneComponent>(TEXT("Intake"));
    Intake->SetupAttachment(Root);
    Intake->SetRelativeLocation(FVector(0, 0, 14));
    Intake->SetBoxExtent(FVector(55, 35, 20));
    Intake->bAcceptReleasedItemsOnOverlap = true;
}

void AFishingStation::BeginPlay()
{
    Super::BeginPlay();
    Intake->AcceptanceRule.BindDynamic(this, &AFishingStation::CanReceiveItem);
    Intake->OnItemAccepted.AddUniqueDynamic(this, &AFishingStation::ReceiveItem);
}

bool AFishingStation::CanReceiveItem(AWorkItem* Item)
{
    return false;
}

void AFishingStation::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
}

AFishSortingConveyor::AFishSortingConveyor()
{
    PrimaryActorTick.bCanEverTick = true;
    Intake->bSnapOnPlacement = false;
}

void AFishSortingConveyor::BeginPlay()
{
    Super::BeginPlay();
    if (PowerLever)
    {
        PowerLever->OnItemAction.AddUniqueDynamic(this, &AFishSortingConveyor::TryRepair);
    }
}

bool AFishSortingConveyor::CanReceiveItem(AWorkItem* Item)
{
    const AFishItem* Fish = Cast<AFishItem>(Item);
    if (!Fish || Fish->bSorted || Fish->ProductState != EFishProductState::Whole || bFaulted)
    {
        return false;
    }
    if (Fish->Species != AcceptedSpecies)
    {
        if (Scenario)
        {
            Scenario->SetStatus(FText::FromString(TEXT("Этот вид рыбы нужно положить в другой лоток.")));
        }
        return false;
    }
    if (bRequireWashed && !Fish->bWashed)
    {
        if (Scenario)
        {
            Scenario->SetStatus(FText::FromString(TEXT("Сначала промойте рыбу в приёмном лотке.")));
        }
        return false;
    }
    return true;
}

void AFishSortingConveyor::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    AFishItem* Fish = CastChecked<AFishItem>(Item);
    Fish->bSorted = true;
    BeltFish.AddUnique(Fish);
    if (Fish->Scenario)
    {
        Fish->Scenario->ReportResult(TEXT("SortFish"));
        Fish->Scenario->SetStatus(FText::FromString(TEXT("Рыба помещена в лоток своего вида.")));
    }
}

void AFishSortingConveyor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFaulted || (PowerLever && PowerLever->Value < 0.2f))
    {
        return;
    }
    for (int32 FishIndex = BeltFish.Num() - 1; FishIndex >= 0; --FishIndex)
    {
        AFishItem* Fish = BeltFish[FishIndex];
        if (!IsValid(Fish) || Fish->bIsHeld || FVector::Distance(Fish->GetActorLocation(), GetActorLocation()) > 160)
        {
            BeltFish.RemoveAtSwap(FishIndex);
            continue;
        }
        if (Fish->Body->IsSimulatingPhysics())
        {
            const FVector DesiredVelocity = Surface->GetComponentVelocity() + GetActorForwardVector() * BeltSpeed;
            Fish->Body->AddForce((DesiredVelocity - Fish->Body->GetPhysicsLinearVelocity()).GetClampedToMaxSize(120) * Fish->Body->GetMass() * 8);
        }
    }
}

void AFishSortingConveyor::SetFaulted(bool bFault)
{
    bFaulted = bFault;
    if (bFault && Scenario)
    {
        Scenario->SetStatus(FText::FromString(TEXT("Сбой конвейера. Верните рычаг в нейтраль и нажмите Trigger.")));
    }
}

void AFishSortingConveyor::TryRepair(AWorkItem* Item, EWorkAction Action)
{
    if (Action == EWorkAction::Used && PowerLever && FMath::Abs(PowerLever->Value) < 0.15f)
    {
        bFaulted = false;
    }
}

bool AFishProcessingBench::CanReceiveItem(AWorkItem* Item)
{
    const AFishItem* Fish = Cast<AFishItem>(Item);
    return Fish && Fish->ProductState == EFishProductState::Whole
        && (!IsValid(CurrentFish) || CurrentFish->bIsHeld || !CurrentFish->bOnProcessingBench);
}

void AFishProcessingBench::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    CurrentFish = CastChecked<AFishItem>(Item);
    CurrentFish->bOnProcessingBench = true;
}
