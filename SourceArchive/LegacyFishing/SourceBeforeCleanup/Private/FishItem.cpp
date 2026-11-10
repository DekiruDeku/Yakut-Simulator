#include "FishItem.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ScenarioDirector.h"
#include "InteractionZoneComponent.h"
#include "KnifeItem.h"

AFishItem::AFishItem()
{
    ItemType = TEXT("Fish");
    Body->SetRelativeScale3D(FVector::OneVector);
    CuttingTarget = CreateDefaultSubobject<USphereComponent>(TEXT("CuttingTarget"));
    CuttingTarget->SetupAttachment(Body);
    CuttingTarget->SetSphereRadius(12);
    CuttingTarget->SetRelativeLocation(FVector(0, 0, 8));
    CuttingTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CuttingTarget->SetCollisionObjectType(ECC_WorldDynamic);
    CuttingTarget->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AFishItem::InitializeFish(UFishSpeciesDefinition* Definition, AScenarioDirector* FishingScenario)
{
    Species = Definition;
    Scenario = FishingScenario;
    Body->SetSimulatePhysics(false);
    Body->SetStaticMesh(Species->Mesh);
    Body->SetMaterial(0, Species->Material);
    Body->SetMassOverrideInKg(NAME_None, Species->WeightKilograms);
    CuttingTarget->SetRelativeLocation(Species->CuttingTargetOffset);
    DisplayName = Species->DisplayName;
    Body->SetSimulatePhysics(true);
}

bool AFishItem::CanBeGrabbed() const
{
    return Super::CanBeGrabbed() && ProductState != EFishProductState::Packed && ProductState != EFishProductState::Frozen;
}

bool AFishItem::TryGrab(USceneComponent* Hand)
{
    if (!Super::TryGrab(Hand))
    {
        return false;
    }
    bOnProcessingBench = false;
    CuttingKnife = nullptr;
    return true;
}

bool AFishItem::ApplyKnifeMovement(AKnifeItem* Knife, const FVector& BladePosition)
{
    if (!bOnProcessingBench || bIsHeld || !bSorted || ProductState != EFishProductState::Whole || Knife->Sharpness < 0.25f)
    {
        return false;
    }
    const FVector LocalPosition = GetActorTransform().InverseTransformPosition(BladePosition);
    if (CuttingKnife == Knife)
    {
        CuttingProgress += FMath::Min(FVector::Distance(LocalPosition, PreviousBladePosition), 5.0f) / 20;
    }
    CuttingKnife = Knife;
    PreviousBladePosition = LocalPosition;
    if (CuttingProgress < 1)
    {
        return false;
    }
    CuttingProgress = 1;
    ProductState = EFishProductState::Processed;
    bDefective = false;
    if (WasteItemClass)
    {
        FActorSpawnParameters Parameters;
        Parameters.Owner = Scenario;
        Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        GetWorld()->SpawnActor<AWorkItem>(WasteItemClass, GetActorLocation() + FVector(0, 20, 15), FRotator::ZeroRotator, Parameters);
    }
    Body->SetVectorParameterValueOnMaterials(TEXT("ProductTint"), FVector(0.9, 0.75, 0.6));
    if (Scenario)
    {
        Scenario->ReportResult(TEXT("ProcessFish"));
        Scenario->SetStatus(FText::FromString(TEXT("Рыба обработана.")));
    }
    return true;
}

AFishContainerItem::AFishContainerItem()
{
    ItemType = TEXT("FishContainer");
    DisplayName = FText::FromString(TEXT("Ящик для рыбы"));
    Body->SetRelativeScale3D(FVector::OneVector);
    Intake = CreateDefaultSubobject<UInteractionZoneComponent>(TEXT("Intake"));
    Intake->SetupAttachment(Body);
    Intake->SetBoxExtent(FVector(34, 24, 25));
    Intake->SetRelativeLocation(FVector(0, 0, 10));
    Intake->AcceptedItemType = TEXT("Fish");
    Intake->bSnapOnPlacement = true;
    Intake->bAcceptReleasedItemsOnOverlap = true;
}

void AFishContainerItem::BeginPlay()
{
    Super::BeginPlay();
    Intake->AcceptanceRule.BindDynamic(this, &AFishContainerItem::CanReceiveFish);
    Intake->OnItemAccepted.AddUniqueDynamic(this, &AFishContainerItem::ReceiveFish);
}

bool AFishContainerItem::CanReceiveFish(AWorkItem* Item)
{
    const AFishItem* Fish = Cast<AFishItem>(Item);
    return Fish && Fish->ProductState == EFishProductState::Processed && Contents.Num() < Capacity && !bFrozen && !bFreezing && !bIsHeld;
}

void AFishContainerItem::ReceiveFish(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    AFishItem* Fish = CastChecked<AFishItem>(Item);
    Fish->ProductState = EFishProductState::Packed;
    Fish->SetActorRelativeLocation(FVector((Contents.Num() % 2) * 24 - 12, (Contents.Num() / 2) * 12 - 12, 0));
    Contents.Add(Fish);
    if (Fish->Scenario)
    {
        Fish->Scenario->ReportResult(TEXT("PackFish"));
    }
}

bool AFishContainerItem::CanBeGrabbed() const
{
    return Super::CanBeGrabbed() && !bFreezing;
}

void AFishContainerItem::FreezeContents()
{
    if (bFrozen)
    {
        return;
    }
    bFrozen = true;
    bFreezing = false;
    for (AFishItem* Fish : Contents)
    {
        Fish->ProductState = EFishProductState::Frozen;
        if (Fish->Scenario)
        {
            Fish->Scenario->ReportResult(TEXT("FreezeFish"));
        }
    }
}
