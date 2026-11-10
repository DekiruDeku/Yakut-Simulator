#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameMapsSettings.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InteractionZoneComponent.h"
#include "MotionControllerComponent.h"
#include "PlacementStation.h"
#include "PrototypePlayerController.h"
#include "PrototypeGameMode.h"
#include "ScenarioDirector.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "VirtualRealityPawn.h"
#include "WorkItem.h"
#include "WorkOperationComponent.h"
#include "Tests/AutomationEditorCommon.h"

enum class EItemInteractionPhase : uint8
{
    UseWithoutGrab,
    ReleaseTrigger,
    GrabRight,
    UseRight,
    HoldRightUse,
    GrabLeft,
    UseLeft,
    ReleaseLeft,
    MoveHeldItem,
    ThrowRight,
    VerifyOperations,
    VerifyScenarioFlow,
    VerifyPlacement,
    Complete
};

class FItemInteractionCommand : public IAutomationLatentCommand
{
public:
    FItemInteractionCommand(FAutomationTestBase* InTest)
        : Test(InTest)
    {
        InitializationStarted = FPlatformTime::Seconds();
    }

    void Initialize(UWorld* InWorld)
    {
        World = InWorld;
        Player = Cast<APrototypePlayerController>(World->GetFirstPlayerController());
        PlayerPawn = Cast<AVirtualRealityPawn>(Player->GetPawn());
        Inputs = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer());
        ExistingDirector = World->GetAuthGameMode<APrototypeGameMode>()->FindScenarioDirector();
        InitialLocation = PlayerPawn->GetActorLocation();
        PrimaryItem = World->SpawnActor<AWorkItem>();
        SecondaryItem = World->SpawnActor<AWorkItem>();
        PrimaryItem->Body->SetSimulatePhysics(false);
        SecondaryItem->Body->SetSimulatePhysics(false);
        PrimaryItem->SetActorLocation(PlayerPawn->RightHand->GetComponentLocation());
        SecondaryItem->SetActorLocation(PlayerPawn->LeftHand->GetComponentLocation());
        PhaseStarted = FPlatformTime::Seconds();
    }

    virtual bool Update() override
    {
        if (!World)
        {
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::PIE)
                {
                    APrototypePlayerController* Candidate = Cast<APrototypePlayerController>(Context.World()->GetFirstPlayerController());
                    if (Candidate && Cast<AVirtualRealityPawn>(Candidate->GetPawn()) && Context.World()->GetTimeSeconds() > 0.25f)
                    {
                        Initialize(Context.World());
                        return false;
                    }
                }
            }
            if (FPlatformTime::Seconds() - InitializationStarted > 15)
            {
                Test->AddError(TEXT("The PIE interaction fixture did not become ready"));
                return true;
            }
            return false;
        }
        if (!IsValid(World) || !IsValid(PlayerPawn))
        {
            Test->AddError(TEXT("The PIE world was destroyed during the interaction test"));
            return true;
        }
        if (Phase == EItemInteractionPhase::GrabRight
            || Phase == EItemInteractionPhase::UseRight
            || Phase == EItemInteractionPhase::HoldRightUse
            || Phase == EItemInteractionPhase::GrabLeft
            || Phase == EItemInteractionPhase::UseLeft
            || Phase == EItemInteractionPhase::ReleaseLeft
            || Phase == EItemInteractionPhase::MoveHeldItem)
        {
            Inject(PlayerPawn->GrabRightAction, FInputActionValue(1.0f));
        }
        if (Phase == EItemInteractionPhase::GrabLeft
            || Phase == EItemInteractionPhase::UseLeft)
        {
            Inject(PlayerPawn->GrabLeftAction, FInputActionValue(1.0f));
        }
        if (Phase == EItemInteractionPhase::UseWithoutGrab
            || Phase == EItemInteractionPhase::UseRight
            || Phase == EItemInteractionPhase::HoldRightUse)
        {
            Inject(PlayerPawn->UseRightAction, FInputActionValue(1.0f));
        }
        if (Phase == EItemInteractionPhase::UseLeft)
        {
            Inject(PlayerPawn->UseLeftAction, FInputActionValue(1.0f));
        }
        if (Phase == EItemInteractionPhase::MoveHeldItem
            || Phase == EItemInteractionPhase::ThrowRight)
        {
            Inject(PlayerPawn->DesktopMoveAction, FInputActionValue(FVector2D(0, 1)));
        }
        if (FPlatformTime::Seconds() - PhaseStarted < 0.15)
        {
            return false;
        }

        switch (Phase)
        {
            case EItemInteractionPhase::UseWithoutGrab:
            {
                Test->TestEqual(TEXT("Trigger without a held item does not use it"), PrimaryItem->UseCount, 0);
                Phase = EItemInteractionPhase::ReleaseTrigger;
                break;
            }
            case EItemInteractionPhase::GrabRight:
            {
                Test->TestTrue(TEXT("Right Grip picks up the nearest item"), PlayerPawn->RightItem == PrimaryItem && PrimaryItem->bIsHeld);
                Test->TestTrue(TEXT("A held item continues simulating physics"), PrimaryItem->Body->IsSimulatingPhysics());
                Test->TestNull(TEXT("A held item is not attached to its hand"), PrimaryItem->GetAttachParentActor());
                Test->TestFalse(TEXT("The other hand cannot take an already held item"), PrimaryItem->TryGrab(PlayerPawn->LeftHand));
                Phase = EItemInteractionPhase::UseRight;
                break;
            }
            case EItemInteractionPhase::UseRight:
            {
                Test->TestEqual(TEXT("Trigger starts use once"), PrimaryItem->UseCount, 1);
                Test->TestTrue(TEXT("Use produces visible feedback"), PrimaryItem->UseIndicator->IsVisible());
                Phase = EItemInteractionPhase::HoldRightUse;
                break;
            }
            case EItemInteractionPhase::HoldRightUse:
            {
                Test->TestEqual(TEXT("Holding Trigger does not repeatedly restart use"), PrimaryItem->UseCount, 1);
                Phase = EItemInteractionPhase::GrabLeft;
                break;
            }
            case EItemInteractionPhase::GrabLeft:
            {
                Test->TestTrue(TEXT("Both hands hold independent items"), PlayerPawn->LeftItem == SecondaryItem && PlayerPawn->RightItem == PrimaryItem);
                Test->TestFalse(TEXT("Releasing Trigger ends use"), PrimaryItem->bIsUsing);
                Phase = EItemInteractionPhase::UseLeft;
                break;
            }
            case EItemInteractionPhase::UseLeft:
            {
                Test->TestEqual(TEXT("Left Trigger uses the left item"), SecondaryItem->UseCount, 1);
                Phase = EItemInteractionPhase::ReleaseLeft;
                break;
            }
            case EItemInteractionPhase::ReleaseLeft:
            {
                Test->TestTrue(TEXT("Releasing left Grip frees the left hand"), PlayerPawn->LeftItem == nullptr);
                Test->TestTrue(TEXT("Released items return to physics"), SecondaryItem->Body->IsSimulatingPhysics());
                Phase = EItemInteractionPhase::MoveHeldItem;
                break;
            }
            case EItemInteractionPhase::MoveHeldItem:
            {
                Test->TestTrue(TEXT("Movement Input Action moves the pawn"), FVector::Distance(InitialLocation, PlayerPawn->GetActorLocation()) > 5);
                Phase = EItemInteractionPhase::ThrowRight;
                break;
            }
            case EItemInteractionPhase::ThrowRight:
            {
                Test->TestTrue(TEXT("Releasing right Grip frees the right hand"), PlayerPawn->RightItem == nullptr);
                Test->TestTrue(TEXT("A thrown item simulates physics"), PrimaryItem->Body->IsSimulatingPhysics());
                Test->TestTrue(TEXT("Throwing transfers horizontal hand velocity"), PrimaryItem->Body->GetPhysicsLinearVelocity().Size2D() > 20);
                Phase = EItemInteractionPhase::VerifyOperations;
                break;
            }
            case EItemInteractionPhase::VerifyOperations:
            {
                TestOperation();
                Phase = EItemInteractionPhase::VerifyScenarioFlow;
                break;
            }
            case EItemInteractionPhase::VerifyScenarioFlow:
            {
                AScenarioDirector* Director = World->SpawnActorDeferred<AScenarioDirector>(AScenarioDirector::StaticClass(), FTransform::Identity);
                UWorkOperationComponent* FirstOperation = NewObject<UWorkOperationComponent>(Director);
                UWorkOperationComponent* SecondOperation = NewObject<UWorkOperationComponent>(Director);
                FScenarioStepDefinition FirstStep;
                FirstStep.StepIdentifier = TEXT("First");
                FirstStep.Operation = FirstOperation;
                FScenarioStepDefinition SecondStep;
                SecondStep.StepIdentifier = TEXT("Second");
                SecondStep.Operation = SecondOperation;
                SecondStep.RequiredSteps.Add(FirstStep.StepIdentifier);
                Director->Steps = { SecondStep, FirstStep };
                FText ConfigurationError;
                Test->TestTrue(TEXT("Dependencies do not require array ordering"), Director->ValidateSteps(ConfigurationError));
                UGameplayStatics::FinishSpawningActor(Director, FTransform::Identity);

                AWorkItem* LateItem = World->SpawnActor<AWorkItem>();
                LateItem->TryGrab(PlayerPawn->RightHand);
                LateItem->BeginUse();
                Test->TestTrue(TEXT("An item spawned after BeginPlay is registered automatically"), FirstOperation->State == EWorkOperationState::Completed);
                Test->TestTrue(TEXT("A newly unlocked step cannot reuse the same event"), SecondOperation->State == EWorkOperationState::Ready && SecondOperation->CurrentCount == 0);
                LateItem->EndUse();
                LateItem->BeginUse();
                Test->TestEqual(TEXT("Reversed dependency order completes correctly"), Director->GetCompletedStepCount(), 2);

                APrototypeGameMode* GameMode = World->GetAuthGameMode<APrototypeGameMode>();
                if (ExistingDirector)
                {
                    Test->TestNull(TEXT("Multiple configured directors are rejected"), GameMode->FindScenarioDirector());
                }
                else
                {
                    Test->TestTrue(TEXT("The configured director is selected explicitly"), GameMode->FindScenarioDirector() == Director);
                }

                SecondStep.RequiredSteps = { TEXT("Missing") };
                Director->Steps = { SecondStep, FirstStep };
                Test->TestFalse(TEXT("Missing dependencies are rejected"), Director->ValidateSteps(ConfigurationError));
                SecondStep.RequiredSteps = { FirstStep.StepIdentifier };
                FirstStep.RequiredSteps = { SecondStep.StepIdentifier };
                Director->Steps = { FirstStep, SecondStep };
                Test->TestFalse(TEXT("Dependency cycles are rejected"), Director->ValidateSteps(ConfigurationError));
                FirstStep.RequiredSteps.Reset();
                SecondStep.StepIdentifier = FirstStep.StepIdentifier;
                Director->Steps = { FirstStep, SecondStep };
                Test->TestFalse(TEXT("Duplicate step identifiers are rejected"), Director->ValidateSteps(ConfigurationError));
                SecondStep.StepIdentifier = TEXT("Second");
                SecondStep.Operation = FirstOperation;
                Director->Steps = { FirstStep, SecondStep };
                Test->TestFalse(TEXT("Sharing an operation between steps is rejected"), Director->ValidateSteps(ConfigurationError));
                LateItem->Destroy();
                Director->Destroy();
                Phase = EItemInteractionPhase::VerifyPlacement;
                break;
            }
            case EItemInteractionPhase::VerifyPlacement:
            {
                APlacementStation* FarStation = World->SpawnActor<APlacementStation>(FVector(2000, 0, 200), FRotator::ZeroRotator);
                APlacementStation* NearStation = World->SpawnActor<APlacementStation>(FVector(2015, 0, 200), FRotator::ZeroRotator);
                const FTransform PreviousHandTransform = PlayerPawn->RightHand->GetComponentTransform();
                PlayerPawn->RightHand->SetWorldLocation(NearStation->AcceptanceZone->GetComponentLocation());
                PrimaryItem->Body->SetWorldLocation(NearStation->AcceptanceZone->GetComponentLocation(), false, nullptr, ETeleportType::TeleportPhysics);
                PrimaryItem->TryGrab(PlayerPawn->RightHand);
                PrimaryItem->Body->UpdateOverlaps();
                PrimaryItem->Release(FVector::ZeroVector, FVector::ZeroVector);
                Test->TestTrue(TEXT("Placement selects the closest valid zone rather than the first overlap"), PrimaryItem->GetAttachParentActor() == NearStation);
                PlayerPawn->RightHand->SetWorldTransform(PreviousHandTransform);
                PrimaryItem->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                FarStation->Destroy();
                NearStation->Destroy();
                Phase = EItemInteractionPhase::Complete;
                break;
            }
            case EItemInteractionPhase::Complete:
            {
                PlayerPawn->ReleaseAllItems();
                PrimaryItem->Destroy();
                SecondaryItem->Destroy();
                PlayerPawn->SetActorLocation(InitialLocation);
                Test->AddInfo(TEXT("Input Actions, two-hand pickup, use lifecycle, release, throw and operation lifecycle were exercised in PIE"));
                return true;
            }
            case EItemInteractionPhase::ReleaseTrigger:
            {
                Phase = EItemInteractionPhase::GrabRight;
                break;
            }
        }
        PhaseStarted = FPlatformTime::Seconds();
        return false;
    }

private:
    FAutomationTestBase* Test;
    UWorld* World = nullptr;
    APrototypePlayerController* Player = nullptr;
    AScenarioDirector* ExistingDirector = nullptr;
    AVirtualRealityPawn* PlayerPawn = nullptr;
    UEnhancedInputLocalPlayerSubsystem* Inputs = nullptr;
    AWorkItem* PrimaryItem = nullptr;
    AWorkItem* SecondaryItem = nullptr;
    FVector InitialLocation;
    EItemInteractionPhase Phase = EItemInteractionPhase::UseWithoutGrab;
    double PhaseStarted = 0;
    double InitializationStarted = 0;

    void Inject(UInputAction* Action, FInputActionValue Value)
    {
        Inputs->InjectInputForAction(Action, Value, {}, {});
    }

    void TestOperation()
    {
        UWorkOperationComponent* Operation = NewObject<UWorkOperationComponent>(PlayerPawn);
        Operation->RequiredAction = EWorkAction::Used;
        Operation->ObserveAction(PrimaryItem, EWorkAction::Used);
        Test->TestEqual(TEXT("A locked operation ignores actions"), Operation->CurrentCount, 0);
        Operation->SetAvailable(true);
        Operation->TryBeginOperation(PrimaryItem);
        Operation->CancelOperation();
        Test->TestTrue(TEXT("An interrupted operation returns to Ready"), Operation->State == EWorkOperationState::Ready);
        Operation->ObserveAction(PrimaryItem, EWorkAction::Used);
        Operation->ObserveAction(PrimaryItem, EWorkAction::Used);
        Test->TestTrue(TEXT("A matching action completes an available operation"), Operation->State == EWorkOperationState::Completed);
        Test->TestEqual(TEXT("A completed operation cannot be counted twice"), Operation->CurrentCount, 1);
    }
};

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FItemInteractionTest, "YakutSimulator.Runtime.ItemInteraction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

void FItemInteractionTest::GetTests(TArray<FString>& BeautifiedNames, TArray<FString>& TestCommands) const
{
    const UClass* GameModeClass = LoadClass<APrototypeGameMode>(nullptr, *UGameMapsSettings::GetGlobalDefaultGameMode());
    const APrototypeGameMode* GameModeDefaults = GameModeClass->GetDefaultObject<APrototypeGameMode>();
    for (const FScenarioMapEntry& Entry : GameModeDefaults->ScenarioMaps)
    {
        FString MapTestName = Entry.Map.ToSoftObjectPath().GetAssetName();
        MapTestName.RemoveFromStart(TEXT("L_"));
        BeautifiedNames.Add(MapTestName);
        TestCommands.Add(Entry.Map.ToSoftObjectPath().GetLongPackageName());
    }
}

bool FItemInteractionTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(Parameters));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FItemInteractionCommand(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}

#endif
