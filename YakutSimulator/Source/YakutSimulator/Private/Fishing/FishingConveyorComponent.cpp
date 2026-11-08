#include "Fishing/FishingMechanics.h"
#include "Fishing/FishingFish.h"
#include "Fishing/FishingLog.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"

UFishingConveyorComponent::UFishingConveyorComponent() { PrimaryComponentTick.bCanEverTick = true; }
void UFishingConveyorComponent::Clear()
{
    bActive = false;
    for (AFishingFish* Item : Fish) if (IsValid(Item)) Item->Destroy();
    Fish.Empty();
    Sorted = Mistakes = Missed = NextFish = 0;
    FeedSeconds = 0;
    Configuration = nullptr;
}
bool UFishingConveyorComponent::Start(UFishingDefinition* Definition)
{
    auto* Scenario = Cast<AMiniGameScenario>(GetOwner());
    if (!Scenario || !Definition || !Path || Definition->SpeciesCycle.IsEmpty()) return false;
    Configuration = Definition;
    if (!Fish.IsEmpty()) { bActive = true; return true; }
    for (int32 Index = 0; Index < Definition->Catch.FishCount; ++Index)
    {
        UFishSpeciesDefinition* Species = Definition->SpeciesCycle[Index % Definition->SpeciesCycle.Num()];
        if (!Species || !Species->ActorClass) { Clear(); return false; }
        AFishingFish* Item = GetWorld()->SpawnActorDeferred<AFishingFish>(Species->ActorClass, GetOwner()->GetActorTransform(), GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Item) { Clear(); return false; }
        Item->Species = Species;
        Item->Scenario = Scenario;
        Item->SourceRun = Scenario->RunId;
        Item->BatchIndex = Index;
        Item->DisplayName = Species->DisplayName;
        Item->Home = Recovery;
        Item->FinishSpawning(GetOwner()->GetActorTransform());
        Item->Place(GetOwner()->GetActorTransform(), EFishWorkState::Queued);
        Fish.Add(Item);
    }
    bActive = true;
    FeedSeconds = Definition->Catch.FeedInterval;
    return true;
}
void UFishingConveyorComponent::Deactivate() { bActive = false; }
void UFishingConveyorComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    if (!bActive || !Configuration || !Path || !FMath::IsFinite(Delta) || Delta <= 0) return;
    FeedSeconds += Delta;
    if (NextFish < Fish.Num() && FeedSeconds >= Configuration->Catch.FeedInterval)
    {
        AFishingFish* Item = Fish[NextFish++];
        Item->ConveyorDistance = 0;
        Item->Place(Path->GetTransformAtDistanceAlongSpline(0, ESplineCoordinateSpace::World), EFishWorkState::Conveyor);
        FeedSeconds = 0;
        UE_LOG(LogFishing, Log, TEXT("[Conveyor.Feed] actor=%s species=%s"), *Item->GetName(), *GetNameSafe(Item->Species));
    }
    for (AFishingFish* Item : Fish)
    {
        if (!IsValid(Item) || Item->IsHeld() || Item->State != EFishWorkState::Conveyor) continue;
        Item->ConveyorDistance += Configuration->Catch.ConveyorSpeed * Delta;
        if (Item->ConveyorDistance >= Path->GetSplineLength()) Recover(Item, true);
        else
        {
            FTransform Pose = Path->GetTransformAtDistanceAlongSpline(Item->ConveyorDistance, ESplineCoordinateSpace::World);
            Pose.SetScale3D(Item->GetActorScale3D());
            Item->SetActorTransform(Pose);
        }
    }
}
bool UFishingConveyorComponent::Sort(AFishingFish* Item, UFishSpeciesDefinition* Species)
{
    if (!bActive || !Configuration || !IsValid(Item) || !Fish.Contains(Item) || Item->bSorted || Item->IsHeld() ||
        (Item->State != EFishWorkState::Conveyor && Item->State != EFishWorkState::Loose)) return false;
    auto* Scenario = Cast<AMiniGameScenario>(GetOwner());
    if (Item->Species != Species)
    {
        if (!Item->bSortingMistakeRecorded) { ++Mistakes; Item->bSortingMistakeRecorded = true; }
        Recover(Item, false);
        if (Scenario) Scenario->PublishFeedback(TEXT("WrongBin"), EMiniGameFeedback::Warning);
        return true;
    }
    Item->bSorted = true;
    Item->Place(Item->GetActorTransform(), EFishWorkState::Sorted);
    ++Sorted;
    if (Scenario) Scenario->PublishFeedback(TEXT("SortedAccepted"), EMiniGameFeedback::Success);
    UE_LOG(LogFishing, Log, TEXT("[Sort.Accepted] fish=%s sorted=%d total=%d"), *Item->GetName(), Sorted, Fish.Num());
    if (Sorted == Fish.Num()) { bActive = false; OnFinished.Broadcast(); }
    return true;
}
void UFishingConveyorComponent::Recover(AFishingFish* Item, bool Miss)
{
    if (!IsValid(Item) || !Configuration || !Recovery || !Fish.Contains(Item) || Item->IsHeld() || Item->bSorted ||
        Item->State == EFishWorkState::Queued || Item->State == EFishWorkState::Delivered) return;
    if (Miss && !Item->bMissRecorded) { ++Missed; Item->bMissRecorded = true; }
    FTransform Pose = Recovery->GetComponentTransform();
    const int32 Columns = FMath::Max(1, Configuration->Catch.RecoveryColumns);
    const auto Spacing = Configuration->Catch.RecoverySpacing;
    Pose.AddToTranslation(Recovery->GetComponentRotation().RotateVector(FVector(Item->BatchIndex % Columns * Spacing.X, Item->BatchIndex / Columns * Spacing.Y, 0)));
    Item->Place(Pose, EFishWorkState::Loose);
    UE_LOG(LogFishing, Log, TEXT("[Recovery.Fish] actor=%s missed=%d"), *Item->GetName(), Miss);
}
