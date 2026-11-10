#include "Minigames/Fishing/FishItem.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Base/ScenarioDirector.h"
#include "Minigames/Fishing/KnifeItem.h"

AFishItem::AFishItem()
{
    ItemType = TEXT("Fish");
    Body->SetRelativeScale3D(FVector::OneVector);
    CuttingTarget = CreateDefaultSubobject<USphereComponent>(TEXT("CuttingTarget"));
    CuttingTarget->SetupAttachment(Body);
    CuttingTarget->SetSphereRadius(30);
    CuttingTarget->SetRelativeLocation(FVector(0, 0, 8));
    CuttingTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CuttingTarget->SetCollisionObjectType(ECC_WorldDynamic);
    CuttingTarget->SetCollisionResponseToAllChannels(ECR_Ignore);
    CutGuide = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CutGuide"));
    CutGuide->SetupAttachment(Body);
    CutStartMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CutStartMarker"));
    CutStartMarker->SetupAttachment(Body);
    CutEndMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CutEndMarker"));
    CutEndMarker->SetupAttachment(Body);
    for (UStaticMeshComponent* Visual : { CutGuide.Get(), CutStartMarker.Get(), CutEndMarker.Get() })
    {
        Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Visual->SetVisibility(false);
    }
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
    float ContactRadius = 12;
    for (const FFishCutDefinition& Cut : Species->CuttingSteps)
    {
        ContactRadius = FMath::Max(ContactRadius, FMath::Max(FVector::Distance(Cut.Start, Species->CuttingTargetOffset), FVector::Distance(Cut.End, Species->CuttingTargetOffset)) + Cut.PositionTolerance);
    }
    CuttingTarget->SetSphereRadius(ContactRadius);
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
    ResetKnifeContact(CuttingKnife);
    return true;
}

void AFishItem::ResetKnifeContact(AKnifeItem* Knife)
{
    if (CuttingKnife == Knife)
    {
        CuttingKnife = nullptr;
        bCutStrokeStarted = false;
        CurrentCutProgress = 0;
        if (Species && !Species->CuttingSteps.IsEmpty())
        {
            CuttingProgress = static_cast<float>(CurrentCutIndex) / Species->CuttingSteps.Num();
        }
    }
}

FText AFishItem::GetCutInstruction() const
{
    if (ProductState == EFishProductState::Processed)
    {
        return FText::FromString(TEXT("Разрезы выполнены. Уберите рыбу с доски."));
    }
    if (!Species || !Species->CuttingSteps.IsValidIndex(CurrentCutIndex))
    {
        return FText::GetEmpty();
    }
    return Species->CuttingSteps[CurrentCutIndex].Instruction;
}

void AFishItem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const bool bShowGuide = bOnProcessingBench && ProductState == EFishProductState::Whole && Species && Species->CuttingSteps.IsValidIndex(CurrentCutIndex);
    for (UStaticMeshComponent* Visual : { CutGuide.Get(), CutStartMarker.Get(), CutEndMarker.Get() })
    {
        Visual->SetVisibility(bShowGuide);
    }
    if (!bShowGuide)
    {
        return;
    }
    const FFishCutDefinition& Cut = Species->CuttingSteps[CurrentCutIndex];
    const FVector VisualOffset(0, 0, 1);
    CutGuide->SetRelativeLocation((Cut.Start + Cut.End) * 0.5f + VisualOffset);
    CutGuide->SetRelativeRotation(FRotationMatrix::MakeFromX(Cut.End - Cut.Start).Rotator());
    CutGuide->SetRelativeScale3D(FVector(FVector::Distance(Cut.Start, Cut.End) / 100, 0.012, 0.012));
    CutStartMarker->SetRelativeLocation(Cut.Start + VisualOffset);
    CutStartMarker->SetRelativeScale3D(FVector(0.035));
    CutEndMarker->SetRelativeLocation(Cut.End + VisualOffset);
    CutEndMarker->SetRelativeScale3D(FVector(0.035));
}

bool AFishItem::ApplyKnifeMovement(AKnifeItem* Knife, const FVector& BladePosition)
{
    if (!bOnProcessingBench || bIsHeld || ProductState != EFishProductState::Whole || !Species || !Species->CuttingSteps.IsValidIndex(CurrentCutIndex) || Knife->Sharpness < 0.25f)
    {
        ResetKnifeContact(Knife);
        return false;
    }
    const FVector LocalPosition = GetActorTransform().InverseTransformPosition(BladePosition);
    if (CuttingKnife != Knife)
    {
        CuttingKnife = Knife;
        PreviousBladePosition = LocalPosition;
        bCutStrokeStarted = false;
        CurrentCutProgress = 0;
        return false;
    }
    const float Travel = FVector::Distance(LocalPosition, PreviousBladePosition);
    PreviousBladePosition = LocalPosition;
    if (Travel > Species->MaximumBladeTravelPerSample)
    {
        ResetKnifeContact(Knife);
        return false;
    }
    const FFishCutDefinition& Cut = Species->CuttingSteps[CurrentCutIndex];
    if (!bCutStrokeStarted)
    {
        if (FVector::Distance(LocalPosition, Cut.Start) > Cut.PositionTolerance)
        {
            return false;
        }
        bCutStrokeStarted = true;
    }
    const FVector CutDirection = Cut.End - Cut.Start;
    const float LengthSquared = CutDirection.SizeSquared();
    if (LengthSquared < 1)
    {
        return false;
    }
    const float Projection = FVector::DotProduct(LocalPosition - Cut.Start, CutDirection) / LengthSquared;
    if (FVector::Distance(LocalPosition, FMath::ClosestPointOnSegment(LocalPosition, Cut.Start, Cut.End)) > Cut.PositionTolerance || Projection < -0.05f)
    {
        ResetKnifeContact(Knife);
        if (Scenario)
        {
            Scenario->SetStatus(FText::FromString(TEXT("Нож вышел за линию разреза. Начните текущий разрез заново от зелёной метки.")));
        }
        return false;
    }
    CurrentCutProgress = FMath::Clamp(Projection, 0.0f, 1.0f);
    CuttingProgress = (CurrentCutIndex + CurrentCutProgress) / Species->CuttingSteps.Num();
    if (CurrentCutProgress < 0.95f || FVector::Distance(LocalPosition, Cut.End) > Cut.PositionTolerance)
    {
        return false;
    }
    ++CurrentCutIndex;
    ResetKnifeContact(Knife);
    CuttingProgress = static_cast<float>(CurrentCutIndex) / Species->CuttingSteps.Num();
    if (CurrentCutIndex < Species->CuttingSteps.Num())
    {
        if (Scenario)
        {
            Scenario->SetStatus(GetCutInstruction());
        }
        return false;
    }
    ProductState = EFishProductState::Processed;
    CurrentCutProgress = 1;
    if (Species->ProcessedMesh)
    {
        Body->SetStaticMesh(Species->ProcessedMesh);
    }
    Body->SetVectorParameterValueOnMaterials(TEXT("ProductTint"), FVector(0.9, 0.75, 0.6));
    if (Scenario)
    {
        Scenario->ReportResult(TEXT("ProcessFish"));
        Scenario->SetStatus(FText::FromString(TEXT("Все разрезы этого вида выполнены. Уберите рыбу с доски для следующей подачи.")));
    }
    return true;
}
