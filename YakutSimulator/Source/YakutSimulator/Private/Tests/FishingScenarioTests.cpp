#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Fishing/FishingScenario.h"
#include "Fishing/FishingItem.h"
#include "Fishing/FishingStation.h"
#include "Fishing/FishingInteractionComponent.h"
#include "Fishing/FishingLog.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "MotionControllerComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"

namespace FishingTests
{
struct FLogCapture : FOutputDevice
{
    FCriticalSection Lock;
    FString Messages;
    ELogVerbosity::Type PreviousVerbosity = LogFishing.GetVerbosity();
    FLogCapture() { LogFishing.SetVerbosity(ELogVerbosity::Verbose); GLog->AddOutputDevice(this); Clear(); }
    ~FLogCapture() { GLog->RemoveOutputDevice(this); LogFishing.SetVerbosity(PreviousVerbosity); }
    virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
    {
        if (Category == FName(TEXT("LogFishing"))) { FScopeLock Guard(&Lock); Messages += FString(Text) + TEXT("\n"); }
    }
    FString Read() { GLog->FlushThreadedLogs(); FScopeLock Guard(&Lock); return Messages; }
    void Clear() { GLog->FlushThreadedLogs(); FScopeLock Guard(&Lock); Messages.Empty(); }
};

struct FFixture
{
    UWorld* World;
    AFishingScenario* Scenario;

    FFixture()
    {
        World = UWorld::CreateWorld(EWorldType::Game, false);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Scenario = World->SpawnActor<AFishingScenario>();
        Scenario->bAttachVRInput = false;
        Scenario->FishCount = 2;
        Scenario->DeploySeconds = Scenario->HaulSeconds = 1;
        Scenario->TrawlSeconds = .1f;
        Scenario->RestartShift();
    }

    ~FFixture()
    {
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
    }

    bool ReachSorting()
    {
        if (!Scenario->EquipSafety()) return false;
        Scenario->EmergencyStop();
        if (!Scenario->StartOrResume()) return false;
        for (int32 I = 0; I < 500 && Scenario->Stage != EFishingStage::AwaitSorting; ++I)
        {
            Scenario->SetWinchInput(.5f);
            Scenario->Tick(.02f);
        }
        return Scenario->Stage == EFishingStage::AwaitSorting && Scenario->BeginSorting();
    }

    bool SortAll()
    {
        if (!ReachSorting()) return false;
        for (AFishingItem* Fish : Scenario->Catch)
        {
            Scenario->Tick(Scenario->FeedInterval + .1f);
            if (!Scenario->TrySortFish(Fish, Fish->Species)) return false;
        }
        return Scenario->Stage == EFishingStage::AwaitProcessing;
    }

    USceneComponent* MakeHand(const FVector& Position)
    {
        AActor* Owner = World->SpawnActor<AActor>();
        USceneComponent* Hand = NewObject<USceneComponent>(Owner);
        Owner->SetRootComponent(Hand);
        Owner->AddInstanceComponent(Hand);
        Hand->RegisterComponent();
        Hand->SetWorldLocation(Position);
        return Hand;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingSafetyTest, "Yakut.Fishing.SafetyAndWinch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingSafetyTest::RunTest(const FString& Parameters)
{
    FishingTests::FFixture F;
    TestFalse(TEXT("Cannot start without the safety drill"), F.Scenario->StartOrResume());
    F.Scenario->EmergencyStop();
    TestFalse(TEXT("Stop before equipment does not complete the drill"), F.Scenario->bStopTested);
    F.Scenario->EquipSafety();
    TestFalse(TEXT("Equipment alone is insufficient"), F.Scenario->StartOrResume());
    F.Scenario->EmergencyStop();
    TestTrue(TEXT("Completed drill allows deployment"), F.Scenario->StartOrResume());
    F.Scenario->SetWinchInput(-1);
    TestEqual(TEXT("Negative throttle is clamped"), F.Scenario->WinchInput, 0.f);
    F.Scenario->SetWinchInput(2);
    TestEqual(TEXT("Excessive throttle is clamped"), F.Scenario->WinchInput, 1.f);
    F.Scenario->EmergencyStop();
    F.Scenario->Tick(1);
    TestEqual(TEXT("Emergency stop prevents net movement"), F.Scenario->NetDepth, 0.f);
    TestFalse(TEXT("A stop button is not reported as an overload"), F.Scenario->bStoppedByOverload);
    TestTrue(TEXT("Stop message identifies the button"), F.Scenario->GetWinchStatusText().ToString().Contains(TEXT("НАЖАТА КНОПКА СТОП")));
    TestTrue(TEXT("Emergency stop can be reset"), F.Scenario->StartOrResume());
    for (int32 I = 0; I < 500 && F.Scenario->Stage != EFishingStage::AwaitSorting; ++I)
    {
        F.Scenario->SetWinchInput(.5f);
        F.Scenario->Tick(.02f);
    }
    TestEqual(TEXT("Deployment, trawling and hauling complete"), F.Scenario->Stage, EFishingStage::AwaitSorting);
    TestEqual(TEXT("Net is back aboard"), F.Scenario->NetDepth, 0.f);
    F.Scenario->RestartShift();
    F.Scenario->EquipSafety();
    F.Scenario->EmergencyStop();
    F.Scenario->StartOrResume();
    F.Scenario->SetWinchInput(1);
    F.Scenario->DeploySeconds = 8.f;
    F.Scenario->Tick(.6f);
    TestFalse(TEXT("High load gives the player time to react"), F.Scenario->bEmergencyStopped);
    TestTrue(TEXT("High load shows the corrective action and countdown"), F.Scenario->GetWinchStatusText().ToString().Contains(TEXT("РЫЧАГ НАЗАД")));
    for (int32 I = 0; I < 100 && !F.Scenario->bEmergencyStopped; ++I)
    {
        F.Scenario->SetWinchInput(1);
        F.Scenario->Tick(.02f);
    }
    TestTrue(TEXT("Sustained overload stops the mechanism"), F.Scenario->bEmergencyStopped);
    TestEqual(TEXT("Overload is counted once"), F.Scenario->Results.Overloads, 1);
    TestTrue(TEXT("Overload has its own stop reason"), F.Scenario->bStoppedByOverload);
    TestTrue(TEXT("Overload message is explicit"), F.Scenario->GetWinchStatusText().ToString().Contains(TEXT("ПЕРЕГРУЗКА")));
    F.Scenario->StartOrResume();
    TestFalse(TEXT("Reset clears the overload reason"), F.Scenario->bStoppedByOverload);
    TestEqual(TEXT("Reset starts in neutral"), F.Scenario->WinchInput, 0.f);
    TestTrue(TEXT("Reset gives a fresh control instruction"), F.Scenario->LastFeedback.ToString().Contains(TEXT("Лебёдка готова")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingLeverGrabTest, "Yakut.Fishing.LeverGrabAndFollow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingLeverGrabTest::RunTest(const FString& Parameters)
{
    FishingTests::FFixture F;
    F.Scenario->EquipSafety();
    F.Scenario->EmergencyStop();
    F.Scenario->StartOrResume();
    AFishingStation* Lever = F.World->SpawnActor<AFishingStation>();
    Lever->Scenario = F.Scenario;
    Lever->Kind = EFishingStationKind::WinchLever;
    Lever->SetActorLocationAndRotation(FVector(100, 200, 300), FRotator(0, 90, 0));
    Lever->OnConstruction(Lever->GetActorTransform());
    const FTransform Home(FRotator(10, 20, 30), FVector(2, 3, 4), FVector(.1f));
    Lever->Handle->SetRelativeTransform(Home);
    const FVector Scale = Lever->Handle->GetComponentScale();
    const FVector GrabLocal(12, 7, 15);
    USceneComponent* Hand = F.MakeHand(Lever->GetActorTransform().TransformPosition(GrabLocal));
    USceneComponent* OtherHand = F.MakeHand(Hand->GetComponentLocation());
    if (!TestTrue(TEXT("Lever accepts a grip"), Lever->BeginLever(Hand))) return false;
    TestTrue(TEXT("Visible handle attaches to the holding controller"), Lever->Handle->GetAttachParent() == Hand);
    TestTrue(TEXT("Handle immediately snaps to the hand"), Lever->Handle->GetComponentLocation().Equals(Hand->GetComponentLocation()));
    TestTrue(TEXT("Attachment keeps the handle size"), Lever->Handle->GetComponentScale().Equals(Scale));
    TestTrue(TEXT("Grab has a visible confirmation"), Lever->Label->Text.ToString().Contains(TEXT("РЫЧАГ В РУКЕ")));
    TestEqual(TEXT("Grab confirmation is green"), Lever->Label->TextRenderColor, FColor::Green);
    Lever->Tick(.016f);
    TestEqual(TEXT("Grabbing away from the station origin does not start the winch"), F.Scenario->WinchInput, 0.f);
    TestFalse(TEXT("Second hand cannot steal the lever"), Lever->BeginLever(OtherHand));
    const auto Move = [&](const FVector& Offset)
    {
        Hand->SetWorldLocationAndRotation(Lever->GetActorTransform().TransformPosition(GrabLocal + Offset), FRotator(15, 40, 20));
        Lever->Tick(.016f);
    };
    Move(FVector(1, 0, 0));
    TestEqual(TEXT("Small hand jitter remains in neutral"), F.Scenario->WinchInput, 0.f);
    Move(FVector(10, 5, -3));
    TestTrue(TEXT("Visible handle follows the controller in space"), Lever->Handle->GetComponentLocation().Equals(Hand->GetComponentLocation()));
    TestTrue(TEXT("Visible handle follows the controller rotation"), Lever->Handle->GetComponentQuat().Equals(Hand->GetComponentQuat()));
    TestTrue(TEXT("A forward jump accelerates gradually"), F.Scenario->WinchInput > 0.f && F.Scenario->WinchInput < .05f);
    Lever->Tick(1.f);
    TestEqual(TEXT("Ten centimeters along the rotated station axis settles at half speed"), F.Scenario->WinchInput, .5f);
    Lever->EndLever(OtherHand);
    TestTrue(TEXT("Other hand cannot release the handle"), Lever->Handle->GetAttachParent() == Hand);
    Move(FVector(0, 50, 30));
    TestEqual(TEXT("Sideways and vertical movement do not add throttle"), F.Scenario->WinchInput, 0.f);
    Move(FVector(-10, 0, 0));
    TestEqual(TEXT("Backward movement cannot reverse the winch"), F.Scenario->WinchInput, 0.f);
    Move(FVector(100, 0, 0));
    Lever->Tick(2.f);
    TestEqual(TEXT("Forward movement is capped at full speed"), F.Scenario->WinchInput, 1.f);
    Lever->EndLever(Hand);
    TestEqual(TEXT("Release stops the winch immediately"), F.Scenario->WinchInput, 0.f);
    TestTrue(TEXT("Release returns the handle to the station"), Lever->Handle->GetAttachParent() == Lever->Zone);
    TestTrue(TEXT("Release restores the authored handle transform"), Lever->Handle->GetRelativeTransform().Equals(Home));
    TestEqual(TEXT("Release clears the green confirmation"), Lever->Label->TextRenderColor, FColor::White);
    TestTrue(TEXT("Release restores the grip hint"), Lever->Label->Text.ToString().Contains(TEXT("ЗАЖМИТЕ GRIP")));
    if (!TestTrue(TEXT("Lever can be grabbed again"), Lever->BeginLever(Hand))) return false;
    Lever->Tick(.016f);
    TestEqual(TEXT("A new grip has its own neutral position"), F.Scenario->WinchInput, 0.f);
    Hand->SetWorldLocation(Hand->GetComponentLocation() + Lever->GetActorForwardVector() * 10);
    Lever->Tick(.016f);
    Lever->Tick(1.f);
    TestEqual(TEXT("A new grip controls throttle"), F.Scenario->WinchInput, .5f);
    Hand->DestroyComponent();
    Lever->Tick(.016f);
    TestEqual(TEXT("Losing the hand stops the winch"), F.Scenario->WinchInput, 0.f);
    TestTrue(TEXT("Losing the hand restores the handle"), Lever->Handle->GetAttachParent() == Lever->Zone && Lever->Handle->GetRelativeTransform().Equals(Home));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingButtonRecoveryTest, "Yakut.Fishing.ButtonTargetingAndStopRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingButtonRecoveryTest::RunTest(const FString& Parameters)
{
    FishingTests::FFixture F;
    FishingTests::FLogCapture Capture;
    F.Scenario->EquipSafety();
    F.Scenario->EmergencyStop();
    F.Scenario->StartOrResume();
    APawn* Pawn = F.World->SpawnActor<APawn>();
    USceneComponent* Root = NewObject<USceneComponent>(Pawn);
    Pawn->SetRootComponent(Root);
    Root->RegisterComponent();
    UMotionControllerComponent* Hands[2];
    for (int32 I = 0; I < 2; ++I)
    {
        Hands[I] = NewObject<UMotionControllerComponent>(Pawn);
        Hands[I]->MotionSource = I ? TEXT("RightGrip") : TEXT("LeftGrip");
        Hands[I]->SetupAttachment(Root);
        Pawn->AddInstanceComponent(Hands[I]);
        Hands[I]->RegisterComponent();
    }
    UFishingInteractionComponent* Interaction = NewObject<UFishingInteractionComponent>(Pawn);
    Interaction->Scenario = F.Scenario;
    Pawn->AddInstanceComponent(Interaction);
    Interaction->RegisterComponent();
    Interaction->TickComponent(.016f, LEVELTICK_All, nullptr);
    const auto MakeStation = [&](EFishingStationKind Kind, const FVector& Position)
    {
        AFishingStation* Station = F.World->SpawnActor<AFishingStation>();
        Station->Scenario = F.Scenario;
        Station->Kind = Kind;
        Station->SetActorLocationAndRotation(Position, FRotator(0, 90, 0));
        Station->OnConstruction(Station->GetActorTransform());
        return Station;
    };
    AFishingStation* Stop = MakeStation(EFishingStationKind::EmergencyStop, FVector(-740, -20, 410));
    AFishingStation* Start = MakeStation(EFishingStationKind::StartOrResume, FVector(-660, -20, 410));
    AFishingStation* Lever = MakeStation(EFishingStationKind::WinchLever, FVector(-700, 0, 410));
    // Replay the two controller poses that selected a neighbouring STOP in the user's log.
    Hands[0]->SetWorldLocation(FVector(-699.32, -4.07, 414.68));
    Hands[1]->SetWorldLocation(FVector(-702.07, 6.48, 389.67));
    Interaction->Grip(false);
    Interaction->Grip(true);
    TestTrue(TEXT("Grip near the lever selects its visible handle"), Lever->Handle->GetAttachParent() == Hands[0]);
    Interaction->Use(false);
    Interaction->Use(true);
    TestFalse(TEXT("Input from both hands near the lever cannot hit the distant STOP"), F.Scenario->bEmergencyStopped);
    TestEqual(TEXT("A simple grip remains at zero speed"), F.Scenario->WinchInput, 0.f);
    TestTrue(TEXT("Rejected distant button contact is explained"), Capture.Read().Contains(TEXT("HandTooFarFromButtonFace")));
    Hands[1]->SetWorldLocation(Stop->Mesh->GetComponentLocation());
    Interaction->Use(true);
    TestTrue(TEXT("Deliberate contact with the STOP button still works"), F.Scenario->bEmergencyStopped);
    TestFalse(TEXT("The button does not report a speed fault"), F.Scenario->bStoppedByOverload);
    TestEqual(TEXT("The button does not count an overload"), F.Scenario->Results.Overloads, 0);
    Lever->Tick(.016f);
    TestTrue(TEXT("A stop returns the held handle to its base"), Lever->Handle->GetAttachParent() == Lever->Zone);
    TestFalse(TEXT("Stopped machinery cannot be grabbed"), Lever->BeginLever(Hands[0]));
    TestTrue(TEXT("Stopped UI identifies the real cause"), Lever->Label->Text.ToString().Contains(TEXT("НАЖАТА КНОПКА СТОП")));
    TestEqual(TEXT("Stopped UI is red"), Lever->Label->TextRenderColor, FColor::Red);
    Start->Tick(.1f);
    TestEqual(TEXT("Restart button describes its current function"), Start->Label->Text.ToString(), FString(TEXT("СБРОС\nОСТАНОВКИ")));
    Interaction->TickComponent(.016f, LEVELTICK_All, nullptr);
    Hands[1]->SetWorldLocation(FVector(-702.07, 6.48, 389.67));
    Hands[0]->SetWorldLocation(Start->Mesh->GetComponentLocation());
    Interaction->Use(true);
    Interaction->Use(false);
    TestFalse(TEXT("Reset works even when both input events arrive"), F.Scenario->bEmergencyStopped);
    TestEqual(TEXT("Reset leaves the winch in neutral"), F.Scenario->WinchInput, 0.f);
    TestFalse(TEXT("Ready UI clears the old stop reason"), F.Scenario->GetWinchStatusText().ToString().Contains(TEXT("СТОП:")));
    TestTrue(TEXT("Pressing start again is harmless"), F.Scenario->StartOrResume());
    F.Scenario->RestartShift();
    TestFalse(TEXT("Lever remains locked during preparation"), Lever->BeginLever(Hands[0]));
    TestTrue(TEXT("Preparation shows exactly the next action"), F.Scenario->GetStatusText().ToString().Contains(TEXT("СЕЙЧАС: НАДЕТЬ ЗАЩИТУ")));
    F.Scenario->EquipSafety();
    TestTrue(TEXT("Equipping advances the instruction"), F.Scenario->GetStatusText().ToString().Contains(TEXT("СЕЙЧАС: СТОП")));
    F.Scenario->EmergencyStop();
    TestTrue(TEXT("Testing stop advances to the start instruction"), F.Scenario->GetStatusText().ToString().Contains(TEXT("СЕЙЧАС: НАЧАТЬ ЛОВЛЮ")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingSortingTest, "Yakut.Fishing.SortingRecoveryAndOwnership",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingSortingTest::RunTest(const FString& Parameters)
{
    FishingTests::FFixture F;
    if (!TestTrue(TEXT("Fixture reaches sorting"), F.ReachSorting())) return false;
    if (!TestEqual(TEXT("Batch contains both species"), F.Scenario->Catch.Num(), 2)) return false;
    AFishingItem* A = F.Scenario->Catch[0];
    TestFalse(TEXT("Queued fish cannot be submitted"), F.Scenario->TrySortFish(A, EFishSpecies::A));
    F.Scenario->Tick(.1f);
    TestFalse(TEXT("Wrong bin rejects fish"), F.Scenario->TrySortFish(A, EFishSpecies::B));
    TestEqual(TEXT("Rejected fish is recoverable"), A->State, EFishingItemState::Loose);
    F.Scenario->TrySortFish(A, EFishSpecies::B);
    TestEqual(TEXT("Repeated rejection does not inflate affected-fish count"), F.Scenario->Results.SortingMistakes, 1);
    TestTrue(TEXT("Recovered fish can be sorted correctly"), F.Scenario->TrySortFish(A, EFishSpecies::A));
    TestFalse(TEXT("Sorting the same fish twice is rejected"), F.Scenario->TrySortFish(A, EFishSpecies::A));
    AFishingItem* Foreign = F.World->SpawnActor<AFishingItem>();
    Foreign->Scenario = F.Scenario;
    Foreign->State = EFishingItemState::Loose;
    TestFalse(TEXT("Actor outside the batch cannot count towards completion"), F.Scenario->TrySortFish(Foreign, EFishSpecies::A));
    F.Scenario->Tick(F.Scenario->FeedInterval + .1f);
    AFishingItem* B = F.Scenario->Catch[1];
    F.Scenario->RecoverItem(B, true);
    F.Scenario->RecoverItem(B, true);
    TestEqual(TEXT("Missed fish is counted once"), F.Scenario->Results.MissedFish, 1);
    TestTrue(TEXT("Missed fish can still complete the batch"), F.Scenario->TrySortFish(B, EFishSpecies::B));
    TestEqual(TEXT("Processing unlocks after the entire batch"), F.Scenario->Stage, EFishingStage::AwaitProcessing);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingCompleteShiftTest, "Yakut.Fishing.CompleteShiftAndRestart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingCompleteShiftTest::RunTest(const FString& Parameters)
{
    FishingTests::FFixture F;
    if (!TestTrue(TEXT("Fixture sorts the batch"), F.SortAll())) return false;
    if (!TestTrue(TEXT("Processing begins"), F.Scenario->BeginProcessing())) return false;
    if (!TestNotNull(TEXT("Processing creates a knife"), F.Scenario->Knife.Get())) return false;
    USceneComponent* FishHand = F.MakeHand(FVector::ZeroVector);
    USceneComponent* KnifeHand = F.MakeHand(F.Scenario->Knife->GetActorLocation());
    TestTrue(TEXT("Knife can be grabbed"), F.Scenario->Knife->TryGrab(KnifeHand));
    for (AFishingItem* Fish : F.Scenario->Catch)
    {
        TestTrue(TEXT("Sorted supply fish loads onto the board"), F.Scenario->TryLoadFish(Fish));
        TestFalse(TEXT("Unprocessed fish cannot be delivered"), F.Scenario->TryDeliverFish(Fish));
        const TArray<FVector> Path = Fish->GetCutPoints();
        FVector Start = Fish->GetActorTransform().TransformPosition(Path[0]);
        F.Scenario->AdvanceKnifeStroke(Start, Start);
        TestEqual(TEXT("Cutting requires a second hand holding the fish"), Fish->NextCutPoint, 0);
        FishHand->SetWorldLocation(Fish->GetActorLocation());
        TestTrue(TEXT("Fish can be braced"), Fish->TryGrab(FishHand));
        const FVector Last = Fish->GetActorTransform().TransformPosition(Path.Last());
        F.Scenario->AdvanceKnifeStroke(Last, Last);
        TestEqual(TEXT("Last checkpoint cannot bypass the path"), Fish->NextCutPoint, 0);
        F.Scenario->AdvanceKnifeStroke(Start, Start);
        for (int32 I = 1; I < Path.Num(); ++I)
        {
            const FVector End = Fish->GetActorTransform().TransformPosition(Path[I]);
            F.Scenario->AdvanceKnifeStroke(Start, End);
            Start = End;
        }
        TestEqual(TEXT("Ordered path processes the fish"), Fish->State, EFishingItemState::Processed);
        Fish->Release();
        TestTrue(TEXT("Processed fish can be delivered"), F.Scenario->TryDeliverFish(Fish));
        TestFalse(TEXT("Same fish cannot be delivered twice"), F.Scenario->TryDeliverFish(Fish));
    }
    TestEqual(TEXT("Full shift reaches the results"), F.Scenario->Stage, EFishingStage::Complete);
    TestEqual(TEXT("Entire batch delivered"), F.Scenario->Results.DeliveredFish, 2);
    const TWeakObjectPtr<AFishingItem> OldFish(F.Scenario->Catch[0]);
    F.Scenario->RestartShift();
    TestEqual(TEXT("Restart restores safety stage"), F.Scenario->Stage, EFishingStage::Safety);
    TestEqual(TEXT("Restart clears results"), F.Scenario->Results.DeliveredFish, 0);
    TestEqual(TEXT("Restart clears the old batch"), F.Scenario->Catch.Num(), 0);
    TestFalse(TEXT("Old batch actor is destroyed"), OldFish.IsValid());
    F.Scenario->FishCount = -10;
    F.Scenario->RestartShift();
    TestEqual(TEXT("Invalid batch size has a usable minimum"), F.Scenario->Results.TotalFish, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingStationReleaseTest, "Yakut.Fishing.ReleaseThroughStations",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingStationReleaseTest::RunTest(const FString& Parameters)
{
    FishingTests::FFixture F;
    if (!TestTrue(TEXT("Fixture reaches sorting"), F.ReachSorting())) return false;
    if (!TestEqual(TEXT("Fixture has two fish"), F.Scenario->Catch.Num(), 2)) return false;
    auto MakeStation = [&F](EFishingStationKind Kind, const FVector& Position)
    {
        AFishingStation* Station = F.World->SpawnActor<AFishingStation>();
        Station->Scenario = F.Scenario;
        Station->Kind = Kind;
        Station->SetActorLocation(Position);
        return Station;
    };
    AFishingStation* BinA = MakeStation(EFishingStationKind::SortingBin, FVector(-100, -60, 95));
    AFishingStation* BinB = MakeStation(EFishingStationKind::SortingBin, FVector(-100, 60, 95));
    BinA->Species = EFishSpecies::A;
    BinB->Species = EFishSpecies::B;
    USceneComponent* Hand = F.MakeHand(FVector::ZeroVector);
    for (AFishingItem* Fish : F.Scenario->Catch)
    {
        F.Scenario->Tick(F.Scenario->FeedInterval + .1f);
        Hand->SetWorldLocation(Fish->GetActorLocation());
        TestTrue(TEXT("Fish can be taken off the conveyor"), Fish->TryGrab(Hand));
        Hand->SetWorldLocation(Fish->Species == EFishSpecies::A ? BinA->GetActorLocation() : BinB->GetActorLocation());
        Fish->Release();
        TestEqual(TEXT("Release over a bin routes the fish"), Fish->State, EFishingItemState::Sorted);
    }
    TestEqual(TEXT("Physical sorting unlocks processing"), F.Scenario->Stage, EFishingStage::AwaitProcessing);
    if (!TestTrue(TEXT("Processing starts for the sorted batch"), F.Scenario->BeginProcessing())) return false;
    if (!TestNotNull(TEXT("Processing creates a knife"), F.Scenario->Knife.Get())) return false;
    MakeStation(EFishingStationKind::ProcessingBoard, F.Scenario->ProcessingPoint->GetComponentLocation());
    MakeStation(EFishingStationKind::DeliveryBin, F.Scenario->FinishedPoint->GetComponentLocation());
    USceneComponent* KnifeHand = F.MakeHand(F.Scenario->Knife->GetActorLocation());
    TestTrue(TEXT("Knife is available at the processing station"), F.Scenario->Knife->TryGrab(KnifeHand));
    for (AFishingItem* Fish : F.Scenario->Catch)
    {
        Hand->SetWorldLocation(Fish->GetActorLocation());
        TestTrue(TEXT("Sorted fish can be taken from supply"), Fish->TryGrab(Hand));
        Hand->SetWorldLocation(F.Scenario->ProcessingPoint->GetComponentLocation());
        Fish->Release();
        TestEqual(TEXT("Release over the board loads the fish"), Fish->State, EFishingItemState::OnBoard);
        TestTrue(TEXT("Fish can be held down on the board"), Fish->TryGrab(Hand));
        const TArray<FVector> Path = Fish->GetCutPoints();
        FVector Previous = Fish->GetActorTransform().TransformPosition(Path[0]);
        for (const FVector& LocalPoint : Path)
        {
            const FVector Point = Fish->GetActorTransform().TransformPosition(LocalPoint);
            F.Scenario->AdvanceKnifeStroke(Previous, Point);
            Previous = Point;
        }
        TestEqual(TEXT("The held fish is processed"), Fish->State, EFishingItemState::Processed);
        Fish->Release();
        TestTrue(TEXT("Processed fish can be picked up"), Fish->TryGrab(Hand));
        Hand->SetWorldLocation(F.Scenario->FinishedPoint->GetComponentLocation());
        Fish->Release();
        TestEqual(TEXT("Release into finished catch completes delivery"), Fish->State, EFishingItemState::Delivered);
    }
    TestEqual(TEXT("Entire shift completes through station release interactions"), F.Scenario->Stage, EFishingStage::Complete);
    TestEqual(TEXT("Exactly the batch size was delivered"), F.Scenario->Results.DeliveredFish, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingDiagnosticsTest, "Yakut.Fishing.InteractionDiagnostics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingDiagnosticsTest::RunTest(const FString& Parameters)
{
    FishingTests::FFixture F;
    FishingTests::FLogCapture Capture;
    APawn* Pawn = F.World->SpawnActor<APawn>();
    USceneComponent* Root = NewObject<USceneComponent>(Pawn);
    Pawn->SetRootComponent(Root);
    Root->RegisterComponent();
    UMotionControllerComponent* Hand = NewObject<UMotionControllerComponent>(Pawn);
    Hand->MotionSource = TEXT("LeftGrip");
    Hand->SetupAttachment(Root);
    Pawn->AddInstanceComponent(Hand);
    Hand->RegisterComponent();
    Pawn->InputComponent = NewObject<UEnhancedInputComponent>(Pawn);
    Pawn->InputComponent->RegisterComponent();
    UFishingInteractionComponent* Interaction = NewObject<UFishingInteractionComponent>(Pawn);
    Interaction->Scenario = F.Scenario;
    Pawn->AddInstanceComponent(Interaction);
    Interaction->RegisterComponent();
    Interaction->TickComponent(.016f, LEVELTICK_All, nullptr);
    AFishingStation* Station = F.World->SpawnActor<AFishingStation>();
    Station->Kind = EFishingStationKind::SafetyEquipment;
    Station->Scenario = F.Scenario;
    Station->SetActorLocation(FVector(1000, 0, 0));
    Station->OnConstruction(Station->GetActorTransform());
    Interaction->Use(false);
    TestFalse(TEXT("A far button does not equip safety"), F.Scenario->bSafetyEquipped);
    TestTrue(TEXT("An unsuccessful interaction has a reason"), Capture.Read().Contains(TEXT("reason=NoTargetInHandReach")));
    TestTrue(TEXT("Nearest button distance is included"), Capture.Read().Contains(TEXT("[Target.NearestButton]")));
    TestTrue(TEXT("Equipment button accepts a valid safety action"), Station->Use());
    TestTrue(TEXT("Equipment button immediately confirms success"), Station->Label->Text.ToString().Contains(TEXT("ЗАЩИТА НАДЕТА")));
    TestTrue(TEXT("Successful equipment is logged"), Capture.Read().Contains(TEXT("[Safety.Equipped]")));
    TestTrue(TEXT("Station result is logged"), Capture.Read().Contains(TEXT("[Station.Result]")) && Capture.Read().Contains(TEXT("result=Accepted")));
    Interaction->LogDiagnostics();
    TestTrue(TEXT("Snapshot includes player input and tracking"), Capture.Read().Contains(TEXT("[Snapshot.Interaction]")) && Capture.Read().Contains(TEXT("[Snapshot.Hand]")));
    UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(Pawn->InputComponent);
    const UInputAction* UseAction = nullptr;
    for (const auto& Binding : Input->GetActionEventBindings())
    {
        if (!Binding->IsBoundToObject(Interaction) || !Binding->GetAction() || Binding->GetAction()->GetFName() != TEXT("IA_Shoot_Left")) continue;
        UseAction = Binding->GetAction();
    }
    if (!TestNotNull(TEXT("Use action is available for the input pipeline test"), UseAction)) return false;
    APlayerController* PC = F.World->SpawnActor<APlayerController>();
    UEnhancedPlayerInput* PlayerInput = NewObject<UEnhancedPlayerInput>(PC);
    PC->PlayerInput = PlayerInput;
    const TArray<UInputComponent*> InputStack{Input};
    const auto InputFrame = [&](float Value)
    {
        FInputActionValue InputValue(Value);
        InputValue.ConvertToType(UseAction->ValueType);
        PlayerInput->InjectInputForAction(UseAction, InputValue);
        PlayerInput->ProcessInputStack(InputStack, .016f, false);
    };
    const auto UseCount = [&Capture]()
    {
        TArray<FString> Lines;
        Capture.Read().ParseIntoArrayLines(Lines);
        return Lines.FilterByPredicate([](const FString& Line) { return Line.Contains(TEXT("[Interaction.Use]")); }).Num();
    };
    Capture.Clear();
    InputFrame(1.f);
    InputFrame(1.f);
    InputFrame(1.f);
    TestEqual(TEXT("A press and hold dispatches exactly one use request"), UseCount(), 1);
    TestTrue(TEXT("Enhanced Input press event is logged"), Capture.Read().Contains(TEXT("[Input.Action]")) && Capture.Read().Contains(TEXT("Started")));
    InputFrame(0.f);
    InputFrame(1.f);
    TestEqual(TEXT("A second press dispatches another use request"), UseCount(), 2);
    F.Scenario->EmergencyStop();
    F.Scenario->StartOrResume();
    TestFalse(TEXT("Equipment rejects use outside safety"), Station->Use());
    TestTrue(TEXT("Wrong-stage rejection is explicit"), Capture.Read().Contains(TEXT("[Safety.EquipRejected]")) && Capture.Read().Contains(TEXT("reason=WrongStage")));
    F.Scenario->RestartShift();
    Station->Tick(.2f);
    TestEqual(TEXT("Restart restores the equipment prompt"), Station->Label->Text.ToString(), FString(TEXT("НАДЕТЬ ЗАЩИТУ")));
    return true;
}

#endif
