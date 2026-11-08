#include "Fishing/FishingFish.h"
#include "Fishing/FishingMiniGameScenario.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
AFishingFish::AFishingFish()
{
    CutPath = CreateDefaultSubobject<USplineComponent>(TEXT("CutPath"));
    CutPath->SetupAttachment(RootComponent);
    CutPath->ClearSplinePoints();
    BraceIndicator = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BraceIndicator"));
    BraceIndicator->SetupAttachment(RootComponent);
    BraceIndicator->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BraceIndicator->SetVisibility(false);
}
TArray<FVector> AFishingFish::GetCutPoints() const
{
    TArray<FVector> Points;
    for (int32 I = 0; I < CutPath->GetNumberOfSplinePoints(); ++I) Points.Add(GetActorTransform().InverseTransformPosition(CutPath->GetLocationAtSplinePoint(I, ESplineCoordinateSpace::World)));
    return Points;
}
void AFishingFish::Place(const FTransform& Transform, EFishWorkState NewState)
{
    Park(Transform, NewState == EFishWorkState::Queued || NewState == EFishWorkState::Sorted || NewState == EFishWorkState::Delivered);
    State = NewState;
    Grab->bAttachOnGrab = State != EFishWorkState::OnBoard;
    OnWorkStateChanged(State);
}
bool AFishingFish::CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const
{
    if (!Super::CanInteract_Implementation(Context, Action, Reason)) return false;
    const auto* Fishing = Cast<AFishingMiniGameScenario>(Scenario);
    if (!Fishing) return false;
    const bool Allowed = Fishing->Operation == EFishingOperation::Sorting
        ? !bSorted && (State == EFishWorkState::Conveyor || State == EFishWorkState::Loose)
        : Fishing->Operation == EFishingOperation::Processing && (State == EFishWorkState::Loose || State == EFishWorkState::OnBoard || State == EFishWorkState::Processed);
    if (!Allowed) Reason = Fishing->FishingDefinition()->Text(TEXT("ActionUnavailable"));
    return Allowed;
}
bool AFishingFish::IsBraced() const
{
    const auto* Fishing = Cast<AFishingMiniGameScenario>(Scenario);
    const auto* Config = Fishing ? Fishing->FishingDefinition() : nullptr;
    return Config && State == EFishWorkState::OnBoard && !Grab->bAttachOnGrab && IsHeld() &&
        FVector::Distance(Grab->GetHoldingHand()->GetComponentLocation(), GetActorLocation()) <= Config->Processing.BraceDistance;
}
void AFishingFish::Tick(float Delta)
{
    Super::Tick(Delta);
    BraceIndicator->SetVisibility(IsBraced());
}
void AFishingFish::InteractionEnded_Implementation(const FInteractionContext& Context)
{
    Super::InteractionEnded_Implementation(Context);
    if (State == EFishWorkState::Conveyor) State = EFishWorkState::Loose;
    if (State == EFishWorkState::Processed) Grab->bAttachOnGrab = true;
}
bool AFishingFish::ValidateItem(TArray<FText>& Errors) const
{
    Super::ValidateItem(Errors);
    if (!CutPath || CutPath->GetNumberOfSplinePoints() < 2) Errors.Add(NSLOCTEXT("Fishing.Validation", "CutPath", "Author at least two cut checkpoints in the fish Blueprint."));
    return Errors.IsEmpty();
}
