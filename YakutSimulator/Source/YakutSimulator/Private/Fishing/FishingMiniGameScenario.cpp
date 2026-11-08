#include "Fishing/FishingMiniGameScenario.h"
#include "Fishing/FishingMechanics.h"
#include "Fishing/FishingFish.h"
#include "Fishing/FishingLog.h"
#include "Common/GrabbableItem.h"
#include "Common/MiniGamePanel.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"

AFishingMiniGameScenario::AFishingMiniGameScenario()
{
    Winch = CreateDefaultSubobject<UFishingWinchComponent>(TEXT("Winch"));
    Conveyor = CreateDefaultSubobject<UFishingConveyorComponent>(TEXT("Conveyor"));
    Processing = CreateDefaultSubobject<UFishingProcessingComponent>(TEXT("Processing"));
    ConveyorPath = CreateDefaultSubobject<USplineComponent>(TEXT("ConveyorPath"));
    ConveyorPath->SetupAttachment(RootComponent);
    ConveyorPath->ClearSplinePoints();
    const auto Anchor = [this](const TCHAR* Name) { auto* Point = CreateDefaultSubobject<USceneComponent>(Name); Point->SetupAttachment(RootComponent); return Point; };
    RecoveryPoint = Anchor(TEXT("RecoveryPoint"));
    ProcessingPoint = Anchor(TEXT("ProcessingPoint"));
    ProcessingSupplyPoint = Anchor(TEXT("ProcessingSupplyPoint"));
    KnifeHome = Anchor(TEXT("KnifeHome"));
    FinishedPoint = Anchor(TEXT("FinishedPoint"));
    DeckPlayerPoint = Anchor(TEXT("DeckPlayerPoint"));
    SortingPlayerPoint = Anchor(TEXT("SortingPlayerPoint"));
    ProcessingPlayerPoint = Anchor(TEXT("ProcessingPlayerPoint"));
    NetPosition = Anchor(TEXT("NetPosition"));
    CutGuide = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CutGuide"));
    CutGuide->SetupAttachment(RootComponent);
    CutGuide->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
UFishingDefinition* AFishingMiniGameScenario::FishingDefinition() const { return Cast<UFishingDefinition>(ActiveDefinition()); }
void AFishingMiniGameScenario::BeginPlay()
{
    Winch->OnFinished.AddUniqueDynamic(this, &AFishingMiniGameScenario::OnMechanicFinished);
    Winch->OnStopped.AddUniqueDynamic(this, &AFishingMiniGameScenario::OnWinchStopped);
    Conveyor->OnFinished.AddUniqueDynamic(this, &AFishingMiniGameScenario::OnMechanicFinished);
    Processing->OnFinished.AddUniqueDynamic(this, &AFishingMiniGameScenario::OnMechanicFinished);
    Super::BeginPlay();
}
bool AFishingMiniGameScenario::ValidateScene(TArray<FText>& Errors) const
{
    Super::ValidateScene(Errors);
    const auto* Config = Cast<UFishingDefinition>(Definition);
    if (!Config) Errors.Add(NSLOCTEXT("Fishing.Validation", "Definition", "Assign a fishing definition."));
    if (!WinchStation || !WinchStation->Lever->GetHandle() || !ConveyorPath || ConveyorPath->GetSplineLength() <= 0 ||
        !ProcessingStation || !DeliveryStation || !CutGuide->GetStaticMesh())
        Errors.Add(NSLOCTEXT("Fishing.Validation", "Bindings", "Assign the winch, receiver stations, conveyor spline and cutting guide mesh."));
    if (Config) for (UFishSpeciesDefinition* Species : Config->SpeciesCycle)
        if (!SortingBins.ContainsByPredicate([&](const auto& Bin) { return Bin.Station && Bin.Species == Species; }))
            Errors.Add(FText::Format(NSLOCTEXT("Fishing.Validation", "Bin", "Missing sorting bin for {0}"), Species ? Species->DisplayName : FText::GetEmpty()));
    return Errors.IsEmpty();
}
void AFishingMiniGameScenario::OnRunReset_Implementation()
{
    Processing->Clear();
    Conveyor->Clear();
    Operation = EFishingOperation::None;
    bEquipped = bStopTested = false;
    Overloads = 0;
    if (auto* Config = FishingDefinition())
    {
        Winch->Lever = WinchStation ? WinchStation->Lever.Get() : nullptr;
        Winch->Net = NetPosition;
        Winch->Reset(Config->Winch);
    }
    Conveyor->Path = ConveyorPath;
    Conveyor->Recovery = RecoveryPoint;
    Processing->Board = ProcessingPoint;
    Processing->Supply = ProcessingSupplyPoint;
    Processing->KnifeHome = KnifeHome;
    Processing->Finished = FinishedPoint;
    Processing->Guide = CutGuide;
}
void AFishingMiniGameScenario::OnObjectiveEntered_Implementation(FName Id)
{
    const auto* Config = FishingDefinition();
    const auto* Objective = Config ? Config->FindObjective(Id) : nullptr;
    Operation = Objective ? Config->Operations.FindRef(Objective->Operation) : EFishingOperation::None;
    UE_LOG(LogFishing, Display, TEXT("[Fishing.Operation] run=%s objective=%s operation=%s"), *RunId.ToString(), *Id.ToString(), *UEnum::GetValueAsString(Operation));
    if (Operation == EFishingOperation::Deploy || Operation == EFishingOperation::Trawl || Operation == EFishingOperation::Haul) Winch->Start(Operation);
    else if (Operation == EFishingOperation::Sorting) { if (!Conveyor->Start(FishingDefinition())) PublishFeedback(TEXT("SpawnFailed"), EMiniGameFeedback::Error); }
    else if (Operation == EFishingOperation::Processing) { if (!Processing->Start(FishingDefinition(), Conveyor)) PublishFeedback(TEXT("SpawnFailed"), EMiniGameFeedback::Error); }
}
void AFishingMiniGameScenario::OnObjectiveExited_Implementation(FName)
{
    Winch->Deactivate();
    Conveyor->Deactivate();
    Processing->SetComponentTickEnabled(false);
}
bool AFishingMiniGameScenario::IsActionAllowed(FName Action) const
{
    if (!Super::IsActionAllowed(Action)) return false;
    const auto* Config = FishingDefinition();
    if (!Config) return false;
    if (Action == Config->WinchAction) return (Operation == EFishingOperation::Deploy || Operation == EFishingOperation::Haul) && Winch->StopCause == EWinchStopCause::None;
    return true;
}
bool AFishingMiniGameScenario::ExecuteAction_Implementation(FName Action, AActor* Source)
{
    auto* Config = FishingDefinition();
    if (!Config || !IsActionAllowed(Action)) return false;
    if (Action == Config->RestartAction) return Super::ExecuteAction_Implementation(Action, Source);
    if (Action == Config->EquipAction)
    {
        bEquipped = true;
        PublishFeedback(TEXT("Equipped"), EMiniGameFeedback::Success);
        CompleteObjective();
    }
    else if (Action == Config->StopAction)
    {
        if (Operation == EFishingOperation::Safety)
        {
            bStopTested = bEquipped || !Config->bRequireEquipment;
            if (!bStopTested) return false;
            PublishFeedback(TEXT("StopTested"), EMiniGameFeedback::Success);
            CompleteObjective();
        }
        else Winch->Stop(EWinchStopCause::Button);
    }
    else if (Action == Config->StartAction || Action == Config->ResumeAction)
    {
        if (Operation == EFishingOperation::Safety)
        {
            if (Config->bRequireEquipment && !bEquipped) { PublishFeedback(TEXT("NeedEquipment"), EMiniGameFeedback::Warning); return false; }
            if (Config->bRequireStopTest && !bStopTested) { PublishFeedback(TEXT("NeedStop"), EMiniGameFeedback::Warning); return false; }
            CompleteObjective();
        }
        else if (Winch->StopCause != EWinchStopCause::None) { Winch->Resume(); PublishFeedback(TEXT("WinchReady")); }
        else PublishFeedback(TEXT("AlreadyRunning"));
    }
    else if (Action == Config->SortingAction || Action == Config->ProcessingAction) CompleteObjective();
    else return Super::ExecuteAction_Implementation(Action, Source);
    NotifyPresentation();
    return true;
}
void AFishingMiniGameScenario::OnMechanicFinished() { CompleteObjective(); }
void AFishingMiniGameScenario::OnWinchStopped()
{
    const bool Overload = Winch->StopCause == EWinchStopCause::Overload;
    if (Overload) ++Overloads;
    PublishFeedback(Overload ? TEXT("OverloadStopped") : TEXT("ButtonStopped"), EMiniGameFeedback::Error);
}
bool AFishingMiniGameScenario::TryReceive_Implementation(UItemReceiverComponent* Receiver, AGrabbableItem* Item)
{
    auto* Fish = Cast<AFishingFish>(Item);
    if (!Receiver || !Fish || Fish->Scenario != this || Fish->SourceRun != RunId || !Conveyor->Fish.Contains(Fish)) return false;
    if (Operation == EFishingOperation::Sorting)
        for (const auto& Bin : SortingBins) if (Bin.Station && Bin.Station->Receiver == Receiver) return Conveyor->Sort(Fish, Bin.Species);
    if (Operation == EFishingOperation::Processing)
    {
        if (ProcessingStation && Receiver == ProcessingStation->Receiver) return Processing->Load(Fish);
        if (DeliveryStation && Receiver == DeliveryStation->Receiver) return Processing->Deliver(Fish);
    }
    return false;
}
void AFishingMiniGameScenario::RecoverItem_Implementation(AGrabbableItem* Item)
{
    if (!Item || Item->Scenario != this || Item->SourceRun != RunId || Item->IsHeld()) return;
    if (Item == Processing->Knife) { Item->ReturnHome(); return; }
    auto* Fish = Cast<AFishingFish>(Item);
    if (!Fish || !Conveyor->Fish.Contains(Fish)) return;
    if (Operation == EFishingOperation::Sorting) Conveyor->Recover(Fish, false);
    else if (Operation == EFishingOperation::Processing) Processing->Recover(Fish);
}
float AFishingMiniGameScenario::Score() const
{
    const auto* Config = FishingDefinition();
    if (!Config) return 0;
    return FMath::Max(0.f, Processing->Delivered * Config->Scoring.DeliveredPoints - Conveyor->Mistakes * Config->Scoring.SortingPenalty -
        Conveyor->Missed * Config->Scoring.MissPenalty - Overloads * Config->Scoring.OverloadPenalty);
}
FMiniGamePresentation AFishingMiniGameScenario::GetPresentation_Implementation(FName Panel, AActor* Source) const
{
    auto Result = Super::GetPresentation_Implementation(Panel, Source);
    const auto* Config = FishingDefinition();
    if (!Config) return Result;
    if (const auto* Item = Cast<AGrabbableItem>(Source))
    {
        Result = {};
        Result.Title = Item->DisplayName;
        const FText Hand = Config->Text(Item->Grab->HoldingContext.bRightHand ? TEXT("RightHand") : TEXT("LeftHand"));
        if (Item->IsHeld())
        {
            Result.Body = FText::Format(Config->Text(TEXT("ItemHeld")), Hand);
            Result.Feedback = EMiniGameFeedback::Success;
        }
        if (const auto* Fish = Cast<AFishingFish>(Item))
        {
            if (Fish->Species) Result.Title = Fish->Species->DisplayName;
            if (Fish->State == EFishWorkState::OnBoard)
            {
                Result.Body = Fish->IsBraced() ? FText::Format(Config->Text(TEXT("FishBraced")), Hand) :
                    Config->Text(Fish->IsHeld() ? TEXT("ProcessingBraceTooFar") : TEXT("ProcessingNeedFishGrip"));
                Result.Feedback = Fish->IsBraced() ? EMiniGameFeedback::Success : EMiniGameFeedback::Warning;
            }
            if (Fish->State == EFishWorkState::OnBoard || Fish->State == EFishWorkState::Processed)
                Result.Metrics = {{Config->Text(TEXT("CurrentFishCut")), float(Fish->NextCutPoint), float(FMath::Max(1, Fish->GetCutPoints().Num()))}};
            if (Fish->State == EFishWorkState::Processed)
            {
                Result.Body = Config->Text(TEXT("ProcessingComplete"));
                Result.Feedback = EMiniGameFeedback::Success;
            }
        }
        return Result;
    }
    const auto* Station = Cast<AMiniGameStation>(Source);
    if (Station && Station->Button->bEnabled)
    {
        FName Key = Station->Button->ActionId;
        const bool Allowed = IsActionAllowed(Key);
        if (Key == Config->EquipAction && bEquipped) { Key = TEXT("Equipped"); Result.Feedback = EMiniGameFeedback::Success; }
        else
        {
            Result.Feedback = Allowed ? EMiniGameFeedback::Warning : EMiniGameFeedback::Disabled;
            if (Key == Config->StartAction || Key == Config->ResumeAction) Key = Winch->StopCause != EWinchStopCause::None ? FName(TEXT("Reset")) : Operation == EFishingOperation::Safety ? FName(TEXT("StartLabel")) : FName(TEXT("ReadyLabel"));
        }
        Result.Title = Config->Text(Key);
        Result.Body = Allowed ? Config->Text(TEXT("PressHint")) : FText::GetEmpty();
        if (Station->Button->ActionId == Config->StopAction) Result.Feedback = EMiniGameFeedback::Error;
        return Result;
    }
    if (Station && Station->Receiver->bEnabled)
    {
        if (Operation == EFishingOperation::Processing && Station == ProcessingStation && IsValid(Processing->Knife)) return Processing->GetPresentation();
        for (const auto& Bin : SortingBins) if (Bin.Station == Station && Bin.Species) { Result.Title = Bin.Species->DisplayName; Result.Body = Config->Text(TEXT("SortBin")); return Result; }
        Result.Title = Config->Text(Station->Receiver->Id);
        Result.Body = FText::GetEmpty();
        return Result;
    }
    if (Panel == Config->WinchPanel)
    {
        if (Operation != EFishingOperation::Deploy && Operation != EFishingOperation::Haul) return FMiniGamePresentation{};
        Result.Title = Config->Text(Winch->Lever && Winch->Lever->GetHoldingHand() ? TEXT("LeverHeld") : TEXT("Lever"));
        Result.Body = FText::GetEmpty();
        if (Operation == EFishingOperation::Deploy || Operation == EFishingOperation::Haul)
        {
            if (Winch->StopCause != EWinchStopCause::None)
            {
                Result.Title = Config->Text(Winch->StopCause == EWinchStopCause::Overload ? TEXT("OverloadStopped") : TEXT("ButtonStopped"));
                Result.Body = Config->Text(TEXT("ResetHint"));
                Result.Feedback = EMiniGameFeedback::Error;
            }
            else
            {
                Result.Feedback = Winch->Lever && Winch->Lever->GetHoldingHand() ? EMiniGameFeedback::Success : EMiniGameFeedback::Normal;
                if (Config->Winch.bEnableOverloads && Winch->Load > Config->Winch.WarningLoad)
                {
                    Result.Feedback = EMiniGameFeedback::Warning;
                    Result.Body = Winch->Load > Config->Winch.OverloadLoad
                        ? FText::Format(Config->Text(TEXT("OverloadWarning")), FText::AsNumber(FMath::Max(0.f, Config->Winch.OverloadSeconds - Winch->OverloadTime)))
                        : Config->Text(TEXT("LoadWarning"));
                }
            }
            Result.Metrics = {{Config->Text(TEXT("Speed")), Winch->Input, 1}, {Config->Text(TEXT("Load")), Winch->Load, 1}};
        }
        else Result.Feedback = EMiniGameFeedback::Disabled;
        return Result;
    }
    if (Operation == EFishingOperation::Deploy || Operation == EFishingOperation::Haul)
        Result.Metrics = {{Config->Text(Operation == EFishingOperation::Deploy ? TEXT("Deployed") : TEXT("Hauled")), Operation == EFishingOperation::Deploy ? Winch->Depth : 1.f - Winch->Depth, 1}};
    if (Operation == EFishingOperation::Sorting) Result.Metrics = {{Config->Text(TEXT("Sorted")), float(Conveyor->Sorted), float(Config->Catch.FishCount)}};
    if (Operation == EFishingOperation::Processing && IsValid(Processing->Knife)) return Processing->GetPresentation();
    if (Status == EMiniGameStatus::Completed)
    {
        FFormatNamedArguments Args;
        Args.Add(TEXT("Sorted"), FText::AsNumber(Conveyor->Sorted));
        Args.Add(TEXT("Processed"), FText::AsNumber(Processing->Processed));
        Args.Add(TEXT("Delivered"), FText::AsNumber(Processing->Delivered));
        Args.Add(TEXT("Mistakes"), FText::AsNumber(Conveyor->Mistakes));
        Args.Add(TEXT("Missed"), FText::AsNumber(Conveyor->Missed));
        Args.Add(TEXT("Overloads"), FText::AsNumber(Overloads));
        Args.Add(TEXT("Seconds"), FText::AsNumber(FMath::RoundToInt(ElapsedSeconds)));
        Args.Add(TEXT("Score"), FText::AsNumber(Score()));
        Result.Body = FText::Format(Config->Text(TEXT("Results")), Args);
        Result.Feedback = EMiniGameFeedback::Success;
    }
    return Result;
}
