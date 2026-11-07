#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Fishing/FishingScenario.h"
#include "Fishing/FishingStation.h"
#include "Fishing/FishingItem.h"
#include "Fishing/FishingInteractionComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "MotionControllerComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

namespace FishingTests
{
// Uses the saved level, ordinary world ticks and production interaction handlers.
class FLevelPlaythrough : public IAutomationLatentCommand
{
public:
    explicit FLevelPlaythrough(FAutomationTestBase* InTest) : Test(InTest) {}

    virtual ~FLevelPlaythrough() override
    {
        if (IsValid(Interaction)) Interaction->SetComponentTickEnabled(true);
    }

    virtual bool Update() override
    {
        UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
        if (FPlatformTime::Seconds() - Started > 240)
        {
            Test->AddError(FString::Printf(TEXT("Playthrough timed out at phase %d"), Phase));
            return true;
        }
        if (!World) return false;
        if (Phase == 0)
        {
            if (World->GetTimeSeconds() < .5f) return false;
            for (TActorIterator<AFishingScenario> It(World); It; ++It) Scenario = *It;
            Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
            if (!Require(Scenario && Pawn, TEXT("Level creates a scenario and player"))) return true;
            Interaction = Pawn->FindComponentByClass<UFishingInteractionComponent>();
            Camera = Pawn->FindComponentByClass<UCameraComponent>();
            if (!Require(Interaction && Camera, TEXT("XR pawn has fishing input and camera"))) return true;
            TArray<UMotionControllerComponent*> Controllers;
            Pawn->GetComponents(Controllers);
            for (UMotionControllerComponent* Controller : Controllers)
            {
                if (Controller->MotionSource == TEXT("LeftGrip")) Hands[0] = Controller;
                if (Controller->MotionSource == TEXT("RightGrip")) Hands[1] = Controller;
            }
            if (!Require(Hands[0] && Hands[1], TEXT("Both grip controller sources exist"))) return true;
            UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(Pawn->InputComponent);
            TSet<FName> BoundActions;
            if (Input) for (const auto& Binding : Input->GetActionEventBindings())
                if (Binding->IsBoundToObject(Interaction) && Binding->GetAction()) BoundActions.Add(Binding->GetAction()->GetFName());
            const TCHAR* RequiredActions[] = {TEXT("IA_Grab_Left_Pressed"), TEXT("IA_Grab_Left_Released"),
                TEXT("IA_Grab_Right_Pressed"), TEXT("IA_Grab_Right_Released"), TEXT("IA_Shoot_Left"), TEXT("IA_Shoot_Right")};
            for (const TCHAR* Action : RequiredActions)
                if (!Require(BoundActions.Contains(FName(Action)), TEXT("Production Enhanced Input action is bound"))) return true;
            // shortcut: synthetic poses bypass the tracking-loss guard; real HMD tracking remains a hardware check.
            Interaction->SetComponentTickEnabled(false);
            for (UMotionControllerComponent* Hand : Hands) Hand->SetComponentTickEnabled(false);
            Camera->SetRelativeLocation(FVector(25, -15, 165));
            if (!Require(Scenario->FishCount == 8 && Scenario->Stage == EFishingStage::Safety, TEXT("Saved level starts an eight-fish shift"))) return true;
            if (!Use(EFishingStationKind::StartOrResume) || !Use(EFishingStationKind::GoToProcessing)) return true;
            if (!Require(Scenario->Stage == EFishingStage::Safety, TEXT("Buttons cannot skip the safety drill"))) return true;
            if (!Use(EFishingStationKind::SafetyEquipment) || !Use(EFishingStationKind::EmergencyStop) || !Use(EFishingStationKind::StartOrResume)) return true;
            if (!Require(Scenario->Stage == EFishingStage::Deploying, TEXT("Safety buttons start deployment"))) return true;
            if (!HoldLever(1.f)) return true;
            Next(1, TEXT("Testing overload through the lever"));
        }
        else if (Phase == 1 && Scenario->bEmergencyStopped)
        {
            if (!Require(Scenario->Results.Overloads == 1, TEXT("Actual lever movement causes one overload stop"))) return true;
            Interaction->ReleaseGrip(false);
            if (!Use(EFishingStationKind::StartOrResume) || !HoldLever(.5f)) return true;
            Next(2, TEXT("Running deployment, trawling and hauling at default speed"));
        }
        else if (Phase == 2)
        {
            if (Scenario->Stage == EFishingStage::Hauling && !bHaulingStarted)
            {
                AFishingStation* Lever = Station(EFishingStationKind::WinchLever);
                if (!Require(Lever && !Lever->GetLeverHand(), TEXT("Trawling releases the lever before the next operation"))) return true;
                Interaction->ReleaseGrip(false);
                if (!HoldLever(.5f)) return true;
                bHaulingStarted = true;
                Test->AddInfo(TEXT("Grabbing the lever again after the hauling instruction"));
            }
            else if (Scenario->Stage == EFishingStage::AwaitSorting)
            {
                Interaction->ReleaseGrip(false);
                if (!Use(EFishingStationKind::GoToSorting)) return true;
                Next(3, TEXT("Moving to the sorting workplace"));
            }
        }
        else if (Phase == 3 && SincePhase() > .3f)
        {
            if (!CheckPlayer(Scenario->SortingPlayerPoint) || !Require(Scenario->Catch.Num() == 8, TEXT("Full batch appears on the saved map"))) return true;
            Next(4, TEXT("Sorting eight fish, including a wrong bin and a missed fish"));
        }
        else if (Phase == 4)
        {
            if (Scenario->Stage == EFishingStage::AwaitProcessing)
            {
                if (!Require(Scenario->Results.SortedFish == 8 && Scenario->Results.SortingMistakes == 1 && Scenario->Results.MissedFish == 1,
                    TEXT("All fish remain sortable after wrong-bin and conveyor recovery"))) return true;
                if (!Use(EFishingStationKind::GoToProcessing)) return true;
                Next(5, TEXT("Moving to processing"));
            }
            else for (AFishingItem* Fish : Scenario->Catch)
            {
                if (Fish->State != EFishingItemState::Conveyor && Fish->State != EFishingItemState::Loose) continue;
                if (Fish->BatchIndex == 1 && !Fish->bMissRecorded) continue;
                if (!GrabFish(Fish)) return true;
                const bool WrongBin = Fish->BatchIndex == 0 && !bWrongBinTested;
                AFishingStation* Bin = Station(EFishingStationKind::SortingBin, WrongBin ? EFishSpecies::B : Fish->Species);
                if (!Require(Bin != nullptr, TEXT("Sorting bin exists"))) return true;
                Move(0, Bin->GetActorLocation());
                Interaction->ReleaseGrip(false);
                if (!Require(Fish->State == (WrongBin ? EFishingItemState::Loose : EFishingItemState::Sorted), TEXT("Grip release routes fish through the correct receiver"))) return true;
                if (WrongBin)
                {
                    bWrongBinTested = true;
                    if (!Require(!Fish->Body->IsSimulatingPhysics(), TEXT("Wrong-bin recovery does not resume falling"))) return true;
                }
            }
        }
        else if (Phase == 5 && SincePhase() > .3f)
        {
            if (!CheckPlayer(Scenario->ProcessingPlayerPoint)) return true;
            FishOnBoard = SupplyFish();
            if (!GrabFish(FishOnBoard) || !GrabKnife()) return true;
            Move(0, FVector(160, -120, 140));
            Move(1, FVector(340, -100, 135));
            Interaction->ReleaseGrip(false);
            Interaction->ReleaseGrip(true);
            if (!Require(FishOnBoard->Body->IsSimulatingPhysics() && Scenario->Knife->Body->IsSimulatingPhysics(), TEXT("Dropped supply fish and knife enter physics"))) return true;
            Next(6, TEXT("Waiting for automatic recovery of dropped fish and knife"));
        }
        else if (Phase == 6 && SincePhase() > 5.3f)
        {
            if (!Require(Near(FishOnBoard, Scenario->ProcessingSupplyPoint) && Near(Scenario->Knife, Scenario->KnifeHome)
                && !FishOnBoard->Body->IsSimulatingPhysics() && !Scenario->Knife->Body->IsSimulatingPhysics(), TEXT("Real actor ticks return dropped objects to their supply points"))) return true;
            if (!BeginFish()) return true;
        }
        else if (Phase == 7 && SincePhase() > .15f)
        {
            if (!Require(FishOnBoard->NextCutPoint == 0, TEXT("Moving the real knife cannot cut an unbraced fish"))) return true;
            Move(0, FishOnBoard->GetActorLocation());
            Interaction->Grip(false);
            if (!Require(FishOnBoard->IsBraced(), TEXT("Grip braces the fish on the board"))) return true;
            Next(8, TEXT("Checking ordered cutting points"));
        }
        else if (Phase == 8 && SincePhase() > .15f)
        {
            if (!Require(FishOnBoard->NextCutPoint == 0, TEXT("Last checkpoint cannot start the cut"))) return true;
            Next(9, TEXT("Cutting using actual knife poses and ordinary world ticks"));
        }
        else if (Phase == 9)
        {
            if (FishOnBoard->State == EFishingItemState::Processed)
            {
                Interaction->ReleaseGrip(false);
                if (!GrabFish(FishOnBoard)) return true;
                if (!bProcessedDropTested)
                {
                    Move(0, FVector(160, -120, 140));
                    Interaction->ReleaseGrip(false);
                    bProcessedDropTested = true;
                    Next(10, TEXT("Recovering processed fish without losing progress"));
                }
                else if (!Deliver()) return true;
            }
            else
            {
                const TArray<FVector> Path = ExpectedCut();
                if (!Require(Path.IsValidIndex(FishOnBoard->NextCutPoint), TEXT("Cut progress remains within the expected procedure"))) return true;
                const FVector Target = FishOnBoard->GetActorTransform().TransformPosition(Path[FishOnBoard->NextCutPoint]);
                const FVector Tip = Scenario->Knife->BladeTip->GetComponentLocation();
                MoveKnifeTip(Tip + (Target - Tip).GetClampedToMaxSize(2.f));
            }
        }
        else if (Phase == 10 && SincePhase() > 5.3f)
        {
            if (!Require(FishOnBoard->State == EFishingItemState::Processed && Near(FishOnBoard, Scenario->ProcessingPoint), TEXT("Processed fish recovers with its completed cut"))) return true;
            if (!GrabFish(FishOnBoard) || !Deliver()) return true;
        }
        else if (Phase == 11)
        {
            const FFishingResults& R = Scenario->Results;
            if (!Require(Scenario->Stage == EFishingStage::Complete && R.SortedFish == 8 && R.ProcessedFish == 8 && R.DeliveredFish == 8
                && R.SortingMistakes == 1 && R.MissedFish == 1 && R.Overloads == 1, TEXT("Full shift completes with exact batch and error counts"))) return true;
            Interaction->ReleaseGrip(true);
            if (!Use(EFishingStationKind::Restart)) return true;
            Next(12, TEXT("Checking restart and return to deck"));
        }
        else if (Phase == 12 && SincePhase() > .3f)
        {
            if (!CheckPlayer(Scenario->DeckPlayerPoint)) return true;
            int32 RemainingItems = 0;
            for (TActorIterator<AFishingItem> It(World); It; ++It) ++RemainingItems;
            if (!Require(Scenario->Stage == EFishingStage::Safety && Scenario->Catch.IsEmpty() && !Scenario->Knife && RemainingItems == 0
                && Scenario->Results.DeliveredFish == 0 && Scenario->Results.SortingMistakes == 0 && Scenario->Results.Overloads == 0,
                TEXT("Restart clears old actors, results and scenario state"))) return true;
            Test->AddInfo(TEXT("Saved-level playthrough passed: 8/8 fish, both species, overload, wrong bin, miss, three physical drops and restart."));
            return true;
        }
        return false;
    }

private:
    FAutomationTestBase* Test;
    AFishingScenario* Scenario = nullptr;
    APawn* Pawn = nullptr;
    UCameraComponent* Camera = nullptr;
    UFishingInteractionComponent* Interaction = nullptr;
    UMotionControllerComponent* Hands[2] = {};
    AFishingItem* FishOnBoard = nullptr;
    int32 Phase = 0;
    double Started = FPlatformTime::Seconds();
    float PhaseStarted = 0;
    bool bWrongBinTested = false;
    bool bProcessedDropTested = false;
    bool bHaulingStarted = false;

    bool Require(bool Condition, const TCHAR* Message)
    {
        if (!Condition) Test->AddError(FString::Printf(TEXT("Phase %d: %s"), Phase, Message));
        return Condition;
    }

    void Next(int32 NewPhase, const TCHAR* Message)
    {
        Phase = NewPhase;
        PhaseStarted = Scenario->GetWorld()->GetTimeSeconds();
        Test->AddInfo(Message);
    }

    float SincePhase() const { return Scenario->GetWorld()->GetTimeSeconds() - PhaseStarted; }

    AFishingStation* Station(EFishingStationKind Kind, EFishSpecies Species = EFishSpecies::A) const
    {
        for (TActorIterator<AFishingStation> It(Scenario->GetWorld()); It; ++It)
            if (It->Scenario == Scenario && It->Kind == Kind && (Kind != EFishingStationKind::SortingBin || It->Species == Species)) return *It;
        return nullptr;
    }

    void Move(int32 Hand, const FVector& Position)
    {
        Hands[Hand]->SetWorldLocationAndRotation(Position, FRotator::ZeroRotator);
    }

    bool Use(EFishingStationKind Kind)
    {
        AFishingStation* Button = Station(Kind);
        if (!Require(Button != nullptr, TEXT("Required level button exists"))) return false;
        Move(0, Button->Mesh->GetComponentLocation());
        Interaction->Use(false);
        return true;
    }

    bool HoldLever(float Throttle)
    {
        AFishingStation* Lever = Station(EFishingStationKind::WinchLever);
        if (!Require(Lever != nullptr, TEXT("Winch station exists"))) return false;
        Move(0, Lever->GetActorLocation());
        Interaction->Grip(false);
        if (!Require(Lever->Handle->GetAttachParent() == Hands[0], TEXT("Grip attaches the visible lever handle to the controller"))) return false;
        Move(0, Lever->GetActorTransform().TransformPosition(FVector(Lever->LeverTravel * Throttle, 0, 0)));
        return Require(Lever->Handle->GetComponentLocation().Equals(Hands[0]->GetComponentLocation()), TEXT("Held handle follows the controller pose"));
    }

    bool GrabFish(AFishingItem* Fish)
    {
        if (!Require(IsValid(Fish), TEXT("A playable fish exists"))) return false;
        Move(0, Fish->GetActorLocation());
        Interaction->Grip(false);
        return Require(Fish->GetHoldingHand() == Hands[0], TEXT("Spatial grip selects the intended fish"));
    }

    bool GrabKnife()
    {
        if (!Require(IsValid(Scenario->Knife), TEXT("Knife exists"))) return false;
        if (Scenario->Knife->GetHoldingHand() == Hands[1]) return true;
        Move(1, Scenario->Knife->GetActorLocation());
        Interaction->Grip(true);
        return Require(Scenario->Knife->GetHoldingHand() == Hands[1], TEXT("Right grip selects the knife"));
    }

    AFishingItem* SupplyFish() const
    {
        for (AFishingItem* Fish : Scenario->Catch)
            if (Fish->bSorted && Fish->State == EFishingItemState::Loose) return Fish;
        return nullptr;
    }

    bool BeginFish()
    {
        FishOnBoard = SupplyFish();
        if (!GrabFish(FishOnBoard)) return false;
        Move(0, Scenario->ProcessingPoint->GetComponentLocation());
        Interaction->ReleaseGrip(false);
        if (!Require(FishOnBoard->State == EFishingItemState::OnBoard, TEXT("Release loads the actual processing receiver")) || !GrabKnife()) return false;
        MoveKnifeTip(FishOnBoard->GetActorTransform().TransformPosition(ExpectedCut().Last()));
        Next(7, TEXT("Checking a fish before bracing"));
        return true;
    }

    TArray<FVector> ExpectedCut() const
    {
        return FishOnBoard->Species == EFishSpecies::A
            ? TArray<FVector>{FVector(-12, 0, 5), FVector(0, 0, 5), FVector(12, 0, 5)}
            : TArray<FVector>{FVector(-8, -7, 5), FVector(-8, 7, 5), FVector(8, 7, 5), FVector(8, -7, 5)};
    }

    void MoveKnifeTip(const FVector& Target)
    {
        const FVector Offset = Scenario->Knife->BladeTip->GetComponentLocation() - Hands[1]->GetComponentLocation();
        Move(1, Target - Offset);
    }

    bool Deliver()
    {
        AFishingStation* Bin = Station(EFishingStationKind::DeliveryBin);
        if (!Require(Bin != nullptr, TEXT("Finished catch receiver exists"))) return false;
        Move(0, Bin->GetActorLocation());
        Interaction->ReleaseGrip(false);
        if (!Require(FishOnBoard->State == EFishingItemState::Delivered, TEXT("Processed fish is delivered through grip release"))) return false;
        if (Scenario->Stage == EFishingStage::Complete) Next(11, TEXT("Verifying results for all eight fish"));
        else return BeginFish();
        return true;
    }

    bool Near(AActor* Item, USceneComponent* Point) const
    {
        return FVector::Dist(Item->GetActorLocation(), Point->GetComponentLocation()) < 1.f;
    }

    bool CheckPlayer(USceneComponent* Point)
    {
        return Require(FVector::DistXY(Camera->GetComponentLocation(), Point->GetComponentLocation()) < 1.f
            && FMath::Abs(Pawn->GetActorLocation().Z - Point->GetComponentLocation().Z) < 1.f,
            TEXT("Workplace transition corrects room-scale head offset and floor height"));
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingLevelPlaythroughTest, "Yakut.Fishing.SavedLevelPlaythrough",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFishingLevelPlaythroughTest::RunTest(const FString& Parameters)
{
    if (!GEditor || GEditor->PlayWorld)
    {
        AddError(TEXT("Run this test from the editor with Play stopped."));
        return false;
    }
    UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
    if (!EditorWorld || EditorWorld->GetPackage()->IsDirty())
    {
        AddError(TEXT("Save the current level before running its playthrough test."));
        return false;
    }
    const FString OriginalMap = EditorWorld->GetPackage()->GetName();
    const FString FishingMap(TEXT("/Game/Fishing/Maps/L_FishingPrototype"));
    FAutomationEditorCommonUtils::LoadMap(FishingMap);
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FishingTests::FLevelPlaythrough(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    if (OriginalMap != FishingMap) ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(OriginalMap));
    return true;
}

#endif
