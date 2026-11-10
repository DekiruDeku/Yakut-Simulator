#include "FishingStations.h"

#include "Components/StaticMeshComponent.h"
#include "FishItem.h"
#include "ScenarioDirector.h"
#include "InteractionZoneComponent.h"
#include "PhysicalLever.h"
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
    return Fish->Species == AcceptedSpecies;
}

void AFishSortingConveyor::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    AFishItem* Fish = CastChecked<AFishItem>(Item);
    Fish->bSorted = true;
    BeltFish.AddUnique(Fish);
    if (Fish->Scenario)
    {
        Fish->Scenario->ReportResult(TEXT("SortFish"));
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
    return Fish && Fish->bSorted && Fish->ProductState == EFishProductState::Whole
        && (!IsValid(CurrentFish) || CurrentFish->bIsHeld || !CurrentFish->bOnProcessingBench);
}

void AFishProcessingBench::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    CurrentFish = CastChecked<AFishItem>(Item);
    CurrentFish->bOnProcessingBench = true;
}

AFishFreezer::AFishFreezer()
{
    PrimaryActorTick.bCanEverTick = true;
    Intake->SetBoxExtent(FVector(70, 55, 45));
    Intake->SetRelativeLocation(FVector(0, 0, 20));
}

bool AFishFreezer::CanReceiveItem(AWorkItem* Item)
{
    const AFishContainerItem* Container = Cast<AFishContainerItem>(Item);
    return Container && !Container->Contents.IsEmpty() && !Container->bFrozen && !Container->bFreezing
        && (!IsValid(CurrentContainer) || !CurrentContainer->bFreezing);
}

void AFishFreezer::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    CurrentContainer = CastChecked<AFishContainerItem>(Item);
    CurrentContainer->bFreezing = true;
    RemainingSeconds = FreezingSeconds;
}

void AFishFreezer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsValid(CurrentContainer) || !CurrentContainer->bFreezing)
    {
        return;
    }
    RemainingSeconds -= DeltaSeconds;
    if (RemainingSeconds <= 0)
    {
        CurrentContainer->FreezeContents();
        if (Scenario)
        {
            Scenario->SetStatus(FText::FromString(TEXT("Продукция заморожена.")));
        }
    }
}

bool AKnifeSharpeningStation::CanReceiveItem(AWorkItem* Item)
{
    return false;
}
