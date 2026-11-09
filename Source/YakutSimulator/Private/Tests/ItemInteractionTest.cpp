#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InteractionZoneComponent.h"
#include "MotionControllerComponent.h"
#include "PlacementStation.h"
#include "PrototypePlayerController.h"
#include "VirtualRealityPawn.h"
#include "WorkItem.h"
#include "WorkOperationComponent.h"
#include "Tests/AutomationEditorCommon.h"

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
                    if (Candidate && Cast<AVirtualRealityPawn>(Candidate->GetPawn()) && !Candidate->bMenuOpen && Context.World()->GetTimeSeconds() > 0.25f)
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
        if (Phase >= 2 && Phase <= 11)
        {
            Inject(TEXT("IA_GrabRight"), FInputActionValue(1.0f));
        }
        if (Phase >= 5 && Phase <= 9)
        {
            Inject(TEXT("IA_GrabLeft"), FInputActionValue(1.0f));
        }
        if (Phase == 0 || Phase == 3 || Phase == 4 || Phase == 8)
        {
            Inject(TEXT("IA_UseRight"), FInputActionValue(1.0f));
        }
        if (Phase == 6)
        {
            Inject(TEXT("IA_UseLeft"), FInputActionValue(1.0f));
        }
        if (Phase == 7 || Phase == 9)
        {
            Inject(TEXT("IA_ToggleMenu"), FInputActionValue(true));
        }
        if (Phase == 11 || Phase == 12)
        {
            Inject(TEXT("IA_DesktopMove"), FInputActionValue(FVector2D(0, 1)));
        }
        if (FPlatformTime::Seconds() - PhaseStarted < 0.15)
        {
            return false;
        }

        switch (Phase)
        {
            case 0:
            {
                Test->TestEqual(TEXT("Trigger without a held item does not use it"), PrimaryItem->UseCount, 0);
                break;
            }
            case 2:
            {
                Test->TestTrue(TEXT("Right Grip picks up the nearest item"), PlayerPawn->RightItem == PrimaryItem && PrimaryItem->bIsHeld);
                Test->TestFalse(TEXT("The other hand cannot take an already held item"), PrimaryItem->TryGrab(PlayerPawn->LeftHand));
                break;
            }
            case 3:
            {
                Test->TestEqual(TEXT("Trigger starts use once"), PrimaryItem->UseCount, 1);
                Test->TestTrue(TEXT("Use produces visible feedback"), PrimaryItem->UseIndicator->IsVisible());
                break;
            }
            case 4:
            {
                Test->TestEqual(TEXT("Holding Trigger does not repeatedly restart use"), PrimaryItem->UseCount, 1);
                break;
            }
            case 5:
            {
                Test->TestTrue(TEXT("Both hands hold independent items"), PlayerPawn->LeftItem == SecondaryItem && PlayerPawn->RightItem == PrimaryItem);
                Test->TestFalse(TEXT("Releasing Trigger ends use"), PrimaryItem->bIsUsing);
                break;
            }
            case 6:
            {
                Test->TestEqual(TEXT("Left Trigger uses the left item"), SecondaryItem->UseCount, 1);
                break;
            }
            case 7:
            {
                Test->TestTrue(TEXT("The menu opens through its Input Action"), Player->bMenuOpen);
                Test->TestTrue(TEXT("Opening the menu preserves held items"), PrimaryItem->bIsHeld && SecondaryItem->bIsHeld);
                Test->TestFalse(TEXT("Opening the menu ends tool use"), SecondaryItem->bIsUsing);
                break;
            }
            case 8:
            {
                Test->TestEqual(TEXT("A menu click does not activate the held tool"), PrimaryItem->UseCount, 1);
                break;
            }
            case 9:
            {
                Test->TestFalse(TEXT("The menu closes through its Input Action"), Player->bMenuOpen);
                break;
            }
            case 10:
            {
                Test->TestTrue(TEXT("Releasing left Grip frees the left hand"), PlayerPawn->LeftItem == nullptr);
                Test->TestTrue(TEXT("Released items return to physics"), SecondaryItem->Body->IsSimulatingPhysics());
                break;
            }
            case 11:
            {
                Test->TestTrue(TEXT("Movement Input Action moves the pawn"), FVector::Distance(InitialLocation, PlayerPawn->GetActorLocation()) > 5);
                break;
            }
            case 12:
            {
                Test->TestTrue(TEXT("Releasing right Grip frees the right hand"), PlayerPawn->RightItem == nullptr);
                Test->TestTrue(TEXT("A thrown item simulates physics"), PrimaryItem->Body->IsSimulatingPhysics());
                Test->TestTrue(TEXT("Throwing transfers horizontal hand velocity"), PrimaryItem->Body->GetPhysicsLinearVelocity().Size2D() > 20);
                break;
            }
            case 13:
            {
                TestOperation();
                PlayerPawn->ReleaseAllItems();
                PrimaryItem->Destroy();
                SecondaryItem->Destroy();
                PlayerPawn->SetActorLocation(InitialLocation);
                Test->AddInfo(TEXT("Input Actions, two-hand pickup, use lifecycle, menu isolation, release, throw and operation lifecycle were exercised in PIE"));
                return true;
            }
            default:
            {
                break;
            }
        }
        ++Phase;
        PhaseStarted = FPlatformTime::Seconds();
        return false;
    }

private:
    FAutomationTestBase* Test;
    UWorld* World = nullptr;
    APrototypePlayerController* Player = nullptr;
    AVirtualRealityPawn* PlayerPawn = nullptr;
    UEnhancedInputLocalPlayerSubsystem* Inputs = nullptr;
    AWorkItem* PrimaryItem = nullptr;
    AWorkItem* SecondaryItem = nullptr;
    FVector InitialLocation;
    int32 Phase = 0;
    double PhaseStarted = 0;
    double InitializationStarted = 0;

    void Inject(const FString& Name, FInputActionValue Value)
    {
        UInputAction* Action = LoadObject<UInputAction>(nullptr, *FString::Format(TEXT("/Game/Prototype/Input/{0}.{0}"), { Name }));
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
    BeautifiedNames.Add(TEXT("Fishing"));
    TestCommands.Add(TEXT("/Game/Prototype/Maps/L_Fishing"));
    BeautifiedNames.Add(TEXT("Clinic"));
    TestCommands.Add(TEXT("/Game/Prototype/Maps/L_Clinic"));
}

bool FItemInteractionTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(Parameters));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FItemInteractionCommand(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Prototype/Maps/L_MainMenu")));
    return true;
}

#endif
