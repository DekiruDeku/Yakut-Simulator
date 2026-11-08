#include "Fishing/FishingMechanics.h"
#include "Fishing/FishingFish.h"
#include "Fishing/FishingLog.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

UFishingProcessingComponent::UFishingProcessingComponent() { PrimaryComponentTick.bCanEverTick = true; }
void UFishingProcessingComponent::Clear()
{
    bActive = bPreviousTip = false;
    if (IsValid(Knife)) Knife->Destroy();
    Knife = nullptr;
    BoardFish = nullptr;
    Catch = nullptr;
    Configuration = nullptr;
    Processed = Delivered = 0;
    StrokeIssue = LastStatus = NAME_None;
    StrokeWarningTime = 0;
    LastCutPoint = INDEX_NONE;
    if (Guide) Guide->SetVisibility(false);
}
bool UFishingProcessingComponent::Start(UFishingDefinition* Definition, UFishingConveyorComponent* Conveyor)
{
    if (!Definition || !Conveyor || !Definition->KnifeClass || !KnifeHome || !Board || !Supply || !Finished) return false;
    Configuration = Definition;
    Catch = Conveyor;
    bActive = true;
    SetComponentTickEnabled(true);
    if (IsValid(Knife)) return true;
    auto* Scenario = Cast<AMiniGameScenario>(GetOwner());
    if (!Scenario) return false;
    Knife = GetWorld()->SpawnActorDeferred<AGrabbableItem>(Definition->KnifeClass, KnifeHome->GetComponentTransform(), GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Knife) { bActive = false; return false; }
    Knife->Scenario = Scenario;
    Knife->SourceRun = Scenario->RunId;
    Knife->Home = KnifeHome;
    Knife->FinishSpawning(KnifeHome->GetComponentTransform());
    Knife->Park(KnifeHome->GetComponentTransform());
    PresentNext();
    return true;
}
void UFishingProcessingComponent::PresentNext()
{
    if (!Catch) return;
    for (AFishingFish* Item : Catch->Fish) if (IsValid(Item) && Item->State == EFishWorkState::Sorted)
    {
        Item->Home = Supply;
        Item->Place(Supply->GetComponentTransform(), EFishWorkState::Loose);
        return;
    }
}
bool UFishingProcessingComponent::Load(AFishingFish* Item)
{
    if (!bActive || !Catch || !IsValid(Item) || !Catch->Fish.Contains(Item) || BoardFish || !Item->bSorted || Item->IsHeld() || Item->State != EFishWorkState::Loose) return false;
    BoardFish = Item;
    Item->NextCutPoint = 0;
    Item->Home = Board;
    Item->Place(Board->GetComponentTransform(), EFishWorkState::OnBoard);
    bPreviousTip = false;
    StrokeWarningTime = 0;
    if (auto* Scenario = Cast<AMiniGameScenario>(GetOwner())) Scenario->PublishFeedback(TEXT("HoldFish"));
    return true;
}
bool UFishingProcessingComponent::Deliver(AFishingFish* Item)
{
    if (!bActive || !Catch || !Item || Item != BoardFish || !Catch->Fish.Contains(Item) || Item->IsHeld() || Item->State != EFishWorkState::Processed) return false;
    Item->Place(Finished->GetComponentTransform(), EFishWorkState::Delivered);
    BoardFish = nullptr;
    ++Delivered;
    if (auto* Scenario = Cast<AMiniGameScenario>(GetOwner())) Scenario->PublishFeedback(TEXT("DeliveredAccepted"), EMiniGameFeedback::Success);
    if (Delivered == Catch->Fish.Num())
    {
        bActive = false;
        if (Knife) Knife->ReturnHome();
        OnFinished.Broadcast();
    }
    else PresentNext();
    return true;
}
void UFishingProcessingComponent::Recover(AFishingFish* Item)
{
    if (!bActive || !Catch || !IsValid(Item) || !Catch->Fish.Contains(Item) || Item->IsHeld() || Item->State == EFishWorkState::Delivered || Item->State == EFishWorkState::Sorted || Item->State == EFishWorkState::Queued) return;
    Item->Place(Item == BoardFish ? Board->GetComponentTransform() : Supply->GetComponentTransform(),
        Item->State == EFishWorkState::Processed ? EFishWorkState::Processed : Item == BoardFish ? EFishWorkState::OnBoard : EFishWorkState::Loose);
}
void UFishingProcessingComponent::AdvanceStroke(const FVector& From, const FVector& To)
{
    if (!GetCutBlockReason().IsNone()) return;
    if (From.ContainsNaN() || To.ContainsNaN() || FVector::Distance(From, To) > Configuration->Processing.MaxKnifeStep)
    {
        StrokeIssue = From.ContainsNaN() || To.ContainsNaN() ? TEXT("ProcessingInvalidTip") : TEXT("ProcessingMoveTooFast");
        StrokeWarningTime = Configuration->Processing.StrokeWarningSeconds;
        UE_LOG(LogFishing, Log, TEXT("[Cut.StrokeRejected] fish=%s reason=%s step=%.1f maximum=%.1f"),
            *BoardFish->GetName(), *StrokeIssue.ToString(), FVector::Distance(From, To), Configuration->Processing.MaxKnifeStep);
        return;
    }
    const auto Points = BoardFish->GetCutPoints();
    const FVector Segment = To - From;
    float PreviousT = -1;
    while (Points.IsValidIndex(BoardFish->NextCutPoint))
    {
        const FVector Target = BoardFish->GetActorTransform().TransformPosition(Points[BoardFish->NextCutPoint]);
        const float T = Segment.IsNearlyZero() ? 0 : FMath::Clamp(FVector::DotProduct(Target - From, Segment) / Segment.SizeSquared(), 0.f, 1.f);
        if (T < PreviousT || FVector::DistSquared(From + Segment * T, Target) > FMath::Square(Configuration->Processing.CutTolerance)) break;
        PreviousT = T;
        ++BoardFish->NextCutPoint;
        StrokeWarningTime = 0;
        UE_LOG(LogFishing, Verbose, TEXT("[Cut.Point] fish=%s count=%d total=%d"), *BoardFish->GetName(), BoardFish->NextCutPoint, Points.Num());
    }
    if (BoardFish->NextCutPoint == Points.Num() && !Points.IsEmpty())
    {
        BoardFish->State = EFishWorkState::Processed;
        BoardFish->OnWorkStateChanged(BoardFish->State);
        ++Processed;
        if (auto* Scenario = Cast<AMiniGameScenario>(GetOwner())) Scenario->PublishFeedback(TEXT("ProcessingComplete"), EMiniGameFeedback::Success);
    }
}
FName UFishingProcessingComponent::GetCutBlockReason() const
{
    if (!bActive || !Configuration) return TEXT("ProcessingInactive");
    if (!IsValid(BoardFish)) return TEXT("ProcessingNeedBoard");
    if (BoardFish->State == EFishWorkState::Processed) return TEXT("ProcessingReadyToDeliver");
    if (BoardFish->State != EFishWorkState::OnBoard) return TEXT("ProcessingNeedBoard");
    if (BoardFish->GetCutPoints().Num() < 2) return TEXT("ProcessingInvalidPath");
    if (Configuration->Processing.bRequireSecondHand)
    {
        if (!BoardFish->IsHeld()) return TEXT("ProcessingNeedFishGrip");
        if (!BoardFish->IsBraced()) return TEXT("ProcessingBraceTooFar");
    }
    if (!IsValid(Knife) || !Knife->IsHeld()) return TEXT("ProcessingNeedKnifeGrip");
    if (Configuration->Processing.bRequireSecondHand && Knife->Grab->GetHoldingHand() == BoardFish->Grab->GetHoldingHand()) return TEXT("ProcessingDifferentHands");
    return NAME_None;
}
FName UFishingProcessingComponent::GetCutStatus() const
{
    const FName Blocked = GetCutBlockReason();
    return !Blocked.IsNone() ? Blocked : StrokeWarningTime > 0 ? StrokeIssue : FName(TEXT("ProcessingAim"));
}
FMiniGamePresentation UFishingProcessingComponent::GetPresentation() const
{
    FMiniGamePresentation Result;
    if (!Configuration || !bActive) return Result;
    const FName State = GetCutStatus();
    Result.Title = Configuration->Text(State);
    Result.Feedback = State == TEXT("ProcessingAim") || State == TEXT("ProcessingReadyToDeliver") ? EMiniGameFeedback::Success :
        State == TEXT("ProcessingInvalidPath") || State == TEXT("ProcessingInvalidTip") ? EMiniGameFeedback::Error : EMiniGameFeedback::Warning;
    const FText FishHand = Configuration->Text(!IsValid(BoardFish) ? TEXT("NotOnBoard") : !BoardFish->IsHeld() ? TEXT("NotHeld") :
        BoardFish->Grab->HoldingContext.bRightHand ? TEXT("RightHand") : TEXT("LeftHand"));
    const FText KnifeHand = Configuration->Text(!IsValid(Knife) || !Knife->IsHeld() ? TEXT("NotHeld") :
        Knife->Grab->HoldingContext.bRightHand ? TEXT("RightHand") : TEXT("LeftHand"));
    Result.Body = FText::Format(Configuration->Text(TEXT("ProcessingHands")), FishHand, KnifeHand);
    const auto Points = IsValid(BoardFish) ? BoardFish->GetCutPoints() : TArray<FVector>{};
    const int32 Next = IsValid(BoardFish) ? BoardFish->NextCutPoint : 0;
    if (State == TEXT("ProcessingReadyToDeliver")) Result.Body = Configuration->Text(TEXT("ProcessingComplete"));
    else if (IsValid(Knife) && Knife->IsHeld() && Points.IsValidIndex(Next))
    {
        const float Distance = FVector::Distance(Knife->ToolTip->GetComponentLocation(), BoardFish->GetActorTransform().TransformPosition(Points[Next]));
        FNumberFormattingOptions Number;
        Number.SetMaximumFractionalDigits(1);
        const FText Target = FText::Format(Configuration->Text(TEXT("ProcessingTarget")), Next + 1, Points.Num(),
            FText::AsNumber(Distance, &Number), FText::AsNumber(Configuration->Processing.CutTolerance, &Number));
        Result.Body = FText::Format(Configuration->Text(TEXT("ProcessingDetails")), Result.Body, Target);
    }
    Result.Metrics = {{Configuration->Text(TEXT("CurrentFishCut")), float(Next), float(FMath::Max(1, Points.Num()))},
        {Configuration->Text(TEXT("Delivered")), float(Delivered), float(Configuration->Catch.FishCount)}};
    return Result;
}
void UFishingProcessingComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    if (!bActive || !Configuration || !FMath::IsFinite(Delta) || Delta < 0) return;
    StrokeWarningTime = FMath::Max(0.f, StrokeWarningTime - Delta);
    const bool Ready = IsValid(BoardFish) && BoardFish->State == EFishWorkState::OnBoard;
    if (Guide)
    {
        Guide->SetVisibility(Ready);
        if (Ready)
        {
            const auto Points = BoardFish->GetCutPoints();
            if (Points.IsValidIndex(BoardFish->NextCutPoint)) Guide->SetWorldLocation(BoardFish->GetActorTransform().TransformPosition(Points[BoardFish->NextCutPoint]));
        }
    }
    if (!GetCutBlockReason().IsNone()) bPreviousTip = false;
    else
    {
        const FVector Tip = Knife->ToolTip->GetComponentLocation();
        AdvanceStroke(bPreviousTip ? PreviousTip : Tip, Tip);
        PreviousTip = Tip;
        bPreviousTip = !Tip.ContainsNaN();
    }
    const FName State = GetCutStatus();
    const int32 Point = IsValid(BoardFish) ? BoardFish->NextCutPoint : INDEX_NONE;
    if (State != LastStatus || Point != LastCutPoint)
    {
        LastStatus = State;
        LastCutPoint = Point;
        UE_LOG(LogFishing, Display, TEXT("[Cut.ReadinessChanged] status=%s fish=%s held=%d braced=%d knifeHeld=%d nextPoint=%d"),
            *State.ToString(), *GetNameSafe(BoardFish), IsValid(BoardFish) && BoardFish->IsHeld(), IsValid(BoardFish) && BoardFish->IsBraced(), IsValid(Knife) && Knife->IsHeld(), Point);
        if (auto* Scenario = Cast<AMiniGameScenario>(GetOwner())) Scenario->NotifyPresentation();
    }
}
