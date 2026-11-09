#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FishItem.h"
#include "FishSchool.h"
#include "FishingScenario.h"
#include "FishingSonar.h"
#include "FishingStations.h"
#include "FishingWinch.h"
#include "InteractionZoneComponent.h"
#include "KnifeItem.h"
#include "PhysicalLever.h"
#include "PrototypeGameMode.h"
#include "Trawl.h"
#include "TrawlerVessel.h"
#include "VesselMovementComponent.h"
#include "WorkOperationComponent.h"
#include "Tests/AutomationEditorCommon.h"

enum class EFishingTestPhase : uint8
{
    Initialize,
    DeployStarted,
    FaultStopped,
    RepairNeutral,
    Deploying,
    OutsideSchool,
    Catching,
    Hauling,
    Unloading,
    Sharpening,
    Processing,
    Freezing
};

class FFishingScenarioCommand : public IAutomationLatentCommand
{
public:
    explicit FFishingScenarioCommand(FAutomationTestBase* InTest)
        : Test(InTest)
        , Started(FPlatformTime::Seconds())
    {
    }

    virtual bool Update() override
    {
        if (Phase == EFishingTestPhase::Initialize)
        {
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::PIE && Context.World()->GetTimeSeconds() > 0.75f)
                {
                    return Initialize(Context.World());
                }
            }
            if (FPlatformTime::Seconds() - Started > 20)
            {
                Test->AddError(TEXT("The fishing scenario PIE fixture did not become ready"));
                return true;
            }
            return false;
        }
        if (Phase == EFishingTestPhase::Sharpening || Phase == EFishingTestPhase::Processing)
        {
            FVector Target;
            if (Phase == EFishingTestPhase::Sharpening)
            {
                Target = Scenario->SharpeningStone->Intake->GetComponentLocation();
            }
            else
            {
                Target = Fish[FishIndex]->CuttingTarget->GetComponentLocation() + FVector(0, 0, 3);
            }
            Target.X += FMath::Sin((World->GetTimeSeconds() - PhaseStarted) * 12) * 7;
            ToolHand->SetWorldLocation(Target - FVector(24, 0, 0));
        }
        if (World->GetTimeSeconds() - PhaseStarted < Duration)
        {
            return false;
        }
        switch (Phase)
        {
            case EFishingTestPhase::DeployStarted:
            {
                Test->TestTrue(TEXT("The physical winch lever pays out cable"), Scenario->Winch->ControlLever->Value > 0.2f && Scenario->Trawl->CableLength > 10);
                Scenario->Winch->SetFaulted(true);
                RetainedCableLength = Scenario->Trawl->CableLength;
                Advance(EFishingTestPhase::FaultStopped, 0.4f);
                break;
            }
            case EFishingTestPhase::FaultStopped:
            {
                Test->TestTrue(TEXT("A winch fault stops cable motion"), FMath::IsNearlyEqual(RetainedCableLength, Scenario->Trawl->CableLength, 1));
                ControlHand->SetWorldLocation(Scenario->Winch->ControlLever->GetActorTransform().TransformPosition(FVector(0, 0, 71)));
                Advance(EFishingTestPhase::RepairNeutral, 0.8f);
                break;
            }
            case EFishingTestPhase::RepairNeutral:
            {
                Scenario->Winch->ControlLever->BeginUse();
                Scenario->Winch->ControlLever->EndUse();
                Test->TestFalse(TEXT("Trigger in neutral repairs the winch"), Scenario->Winch->bFaulted);
                SetLeverTarget(Scenario->Winch->ControlLever, 55);
                Advance(EFishingTestPhase::Deploying, 3.0f);
                break;
            }
            case EFishingTestPhase::Deploying:
            {
                Test->TestTrue(TEXT("The deployed trawl enters towing state"), Scenario->Trawl->State == ETrawlState::Towing);
                Test->TestEqual(TEXT("A stationary vessel does not catch fish"), Scenario->Trawl->GetCatchCount(), 0);
                School->SetActorLocation(Scenario->Trawl->GetActorLocation() + FVector(20000, 0, 0));
                Scenario->Vessel->Movement->SetThrottle(0.6f);
                Advance(EFishingTestPhase::OutsideSchool, 1.2f);
                break;
            }
            case EFishingTestPhase::OutsideSchool:
            {
                Test->TestEqual(TEXT("Towing outside a school does not create catch"), Scenario->Trawl->GetCatchCount(), 0);
                School->SetActorLocation(Scenario->Trawl->GetActorLocation());
                SchoolFishBefore = School->RemainingFish;
                Advance(EFishingTestPhase::Catching, 4.0f);
                break;
            }
            case EFishingTestPhase::Catching:
            {
                Test->TestEqual(TEXT("The net catches fish from its spatially intersected school"), Scenario->Trawl->GetCatchCount(), Goal);
                Test->TestEqual(TEXT("School stock is conserved"), SchoolFishBefore - School->RemainingFish, Goal);
                Scenario->Vessel->Movement->SetThrottle(0);
                SetLeverTarget(Scenario->Winch->ControlLever, -55);
                Advance(EFishingTestPhase::Hauling, 5.0f);
                break;
            }
            case EFishingTestPhase::Hauling:
            {
                Test->TestTrue(TEXT("Reverse lever hauls the net back to the vessel"), Scenario->Trawl->State == ETrawlState::ReadyToUnload && Scenario->Trawl->CableLength < 1);
                Test->TestTrue(TEXT("The recovered net reaches the towing frame"), FVector::Distance(Scenario->Trawl->GetActorLocation(), Scenario->Vessel->TowPoint->GetComponentLocation()) < 60);
                Scenario->Winch->ControlLever->Release(FVector::ZeroVector, FVector::ZeroVector);
                ControlHand->SetWorldLocation(Scenario->Winch->UnloadLever->GripPoint->GetComponentLocation());
                Scenario->Winch->UnloadLever->TryGrab(ControlHand);
                SetLeverTarget(Scenario->Winch->UnloadLever, 55);
                Advance(EFishingTestPhase::Unloading, 1.0f);
                break;
            }
            case EFishingTestPhase::Unloading:
            {
                Scenario->Winch->UnloadLever->Release(FVector::ZeroVector, FVector::ZeroVector);
                Test->TestEqual(TEXT("Unload creates the actual caught fish"), Scenario->UnloadedFish, Goal);
                Test->TestEqual(TEXT("Unload empties net inventory"), Scenario->Trawl->GetCatchCount(), 0);
                Test->TestFalse(TEXT("The same catch cannot be unloaded twice"), Scenario->Trawl->UnloadCatch());
                for (TActorIterator<AFishItem> FishIterator(World); FishIterator; ++FishIterator)
                {
                    Fish.Add(*FishIterator);
                }
                if (!Test->TestEqual(TEXT("Spawned fish count matches conserved catch"), Fish.Num(), Goal))
                {
                    return true;
                }
                for (AFishItem* Product : Fish)
                {
                    AFishSortingConveyor* Correct = nullptr;
                    for (AFishSortingConveyor* Conveyor : Scenario->SortingConveyors)
                    {
                        if (Conveyor->AcceptedSpecies == Product->Species)
                        {
                            Correct = Conveyor;
                        }
                        else
                        {
                            DropItem(Product, Conveyor->Intake->GetComponentLocation());
                            Test->TestFalse(TEXT("Wrong-species sorting is rejected"), Product->bSorted);
                        }
                    }
                    if (!Correct)
                    {
                        Test->AddError(TEXT("A caught species has no configured sorting route"));
                        return true;
                    }
                    DropItem(Product, Correct->Intake->GetComponentLocation());
                    Test->TestTrue(TEXT("The correct sorting route accepts fish"), Product->bSorted);
                    Test->TestFalse(TEXT("A sorted fish cannot be counted twice"), Correct->Intake->TryAcceptItem(Product));
                }
                Test->TestEqual(TEXT("Sorted fish are counted once"), Scenario->SortedFish, Goal);
                const FVector BladeTarget = Scenario->SharpeningStone->Intake->GetComponentLocation();
                Scenario->Knife->Body->SetWorldLocationAndRotation(BladeTarget - FVector(24, 0, 0), FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
                ToolHand->SetWorldLocation(Scenario->Knife->GripPoint->GetComponentLocation());
                Scenario->Knife->TryGrab(ToolHand);
                Scenario->Knife->BeginUse();
                Advance(EFishingTestPhase::Sharpening, 3.0f);
                break;
            }
            case EFishingTestPhase::Sharpening:
            {
                Test->TestTrue(TEXT("Knife strokes over the stone prepare the blade"), Scenario->Knife->Sharpness >= 0.95f && Scenario->bKnifePrepared);
                PlaceNextFish();
                break;
            }
            case EFishingTestPhase::Processing:
            {
                AFishItem* Product = Fish[FishIndex];
                if (!Test->TestTrue(FString::Format(TEXT("Knife motion processes fish {0}; progress={1}"), { FishIndex, Product->CuttingProgress }), Product->ProductState == EFishProductState::Processed))
                {
                    return true;
                }
                DropItem(Product, Scenario->PackingContainer->Intake->GetComponentLocation() + FVector(0, 0, 20));
                Test->TestTrue(TEXT("Processed fish enter the packing container"), Product->ProductState == EFishProductState::Packed);
                Test->TestFalse(TEXT("Packed fish cannot be grabbed separately"), Product->CanBeGrabbed());
                ++FishIndex;
                if (FishIndex < Fish.Num())
                {
                    PlaceNextFish();
                    break;
                }
                Scenario->Knife->EndUse();
                Scenario->Knife->Release(FVector::ZeroVector, FVector::ZeroVector);
                Test->TestEqual(TEXT("Packing inventory has no duplicates"), Scenario->PackingContainer->Contents.Num(), Goal);
                DropItem(Scenario->PackingContainer, Scenario->Freezer->Intake->GetComponentLocation() + FVector(0, 0, 15));
                Test->TestTrue(TEXT("Freezing locks the admitted container"), Scenario->PackingContainer->bFreezing && !Scenario->PackingContainer->CanBeGrabbed());
                Advance(EFishingTestPhase::Freezing, 1.0f);
                break;
            }
            case EFishingTestPhase::Freezing:
            {
                Test->TestTrue(TEXT("The freezer completes its timed cycle"), Scenario->PackingContainer->bFrozen);
                Test->TestEqual(TEXT("Frozen product count matches the original catch"), Scenario->FrozenFish, Goal);
                Test->TestFalse(TEXT("Frozen containers cannot be submitted twice"), Scenario->Freezer->Intake->TryAcceptItem(Scenario->PackingContainer));
                Test->TestEqual(TEXT("Every configured fishing operation is complete"), Scenario->GetCompletedStepCount(), Scenario->Steps.Num());
                Test->AddInfo(TEXT("Saved fishing map: physical winch payout, fault recovery, spatial catch conservation, haul, release lever, species sorting, knife sharpening and cutting, packing and timed freezing exercised in desktop PIE"));
                return true;
            }
            default:
            {
                break;
            }
        }
        return false;
    }

private:
    FAutomationTestBase* Test;
    UWorld* World = nullptr;
    AFishingScenario* Scenario = nullptr;
    AFishSchool* School = nullptr;
    USceneComponent* ControlHand = nullptr;
    USceneComponent* ToolHand = nullptr;
    TArray<AFishItem*> Fish;
    int32 FishIndex = 0;
    int32 Goal = 0;
    int32 SchoolFishBefore = 0;
    float RetainedCableLength = 0;
    double Started;
    float PhaseStarted = 0;
    float Duration = 0;
    EFishingTestPhase Phase = EFishingTestPhase::Initialize;

    USceneComponent* CreateHand()
    {
        AActor* Actor = World->SpawnActor<AActor>();
        USceneComponent* Hand = NewObject<USceneComponent>(Actor);
        Actor->SetRootComponent(Hand);
        Hand->RegisterComponent();
        Actor->AttachToActor(Scenario->Vessel, FAttachmentTransformRules::KeepWorldTransform);
        return Hand;
    }

    void Advance(EFishingTestPhase NextPhase, float Seconds)
    {
        Phase = NextPhase;
        PhaseStarted = World->GetTimeSeconds();
        Duration = Seconds;
    }

    void SetLeverTarget(APhysicalLever* Lever, float LateralOffset)
    {
        ControlHand->SetWorldLocation(Lever->GetActorTransform().TransformPosition(FVector(0, LateralOffset, 50)));
    }

    void DropItem(AWorkItem* Item, const FVector& Destination)
    {
        ControlHand->SetWorldLocation(Item->GripPoint->GetComponentLocation());
        Item->TryGrab(ControlHand);
        Item->Body->SetWorldLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);
        ControlHand->SetWorldLocation(Destination);
        Item->Body->UpdateOverlaps();
        Item->Release(FVector::ZeroVector, FVector::ZeroVector);
    }

    void PlaceNextFish()
    {
        DropItem(Fish[FishIndex], Scenario->ProcessingBench->Intake->GetComponentLocation());
        Test->TestTrue(TEXT("Sorted fish attach to the processing bench"), Fish[FishIndex]->bOnProcessingBench);
        Advance(EFishingTestPhase::Processing, 2.0f);
    }

    bool Initialize(UWorld* InWorld)
    {
        World = InWorld;
        Scenario = Cast<AFishingScenario>(World->GetAuthGameMode<APrototypeGameMode>()->FindScenarioDirector());
        if (!Scenario || !Scenario->Sonar || !Scenario->Winch || !Scenario->Trawl || !Scenario->Knife || !Scenario->ProcessingBench || !Scenario->SharpeningStone || !Scenario->PackingContainer || !Scenario->Freezer)
        {
            Test->AddError(TEXT("The fishing map is missing explicitly configured scenario equipment"));
            return true;
        }
        Test->TestNotNull(TEXT("The fishing console creates its UMG widget"), Scenario->ConsoleDisplay->GetUserWidgetObject());
        Scenario->Sonar->Scan();
        School = Scenario->Sonar->SelectedSchool;
        if (!School)
        {
            Test->AddError(TEXT("The sonar did not discover a configured school"));
            return true;
        }
        School->bMovementEnabled = false;
        Goal = Scenario->SortOperation->RequiredCount;
        Scenario->Trawl->Capacity = Goal;
        Scenario->Winch->CableSpeed = 2000;
        Scenario->Vessel->Movement->MaximumSpeed = 100;
        Scenario->Vessel->Movement->Acceleration = 400;
        Scenario->Freezer->FreezingSeconds = 0.5f;
        ControlHand = CreateHand();
        ToolHand = CreateHand();
        ControlHand->SetWorldLocation(Scenario->Winch->ControlLever->GripPoint->GetComponentLocation());
        Test->TestTrue(TEXT("The installed winch lever can be physically grabbed"), Scenario->Winch->ControlLever->TryGrab(ControlHand));
        SetLeverTarget(Scenario->Winch->ControlLever, 55);
        Advance(EFishingTestPhase::DeployStarted, 0.8f);
        return false;
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingScenarioTest, "YakutSimulator.Runtime.FishingScenario", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFishingScenarioTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Prototype/Maps/L_Fishing")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FFishingScenarioCommand(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(GetDefault<APrototypeGameMode>()->MainMenuMap.ToSoftObjectPath().GetLongPackageName()));
    return true;
}

#endif
