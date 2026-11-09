#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "MotionControllerComponent.h"
#include "PhysicalLever.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PrototypeGameMode.h"
#include "TrawlerVessel.h"
#include "VesselMovementComponent.h"
#include "VirtualRealityPawn.h"
#include "Tests/AutomationEditorCommon.h"

enum class EPhysicalDeckPhase : uint8
{
    Initialize,
    GripSettled,
    Obstructed,
    LeverMoved,
    LeverReleased,
    CargoSettled,
    VesselMoving,
    VesselTurning,
    CargoFalling
};

class FPhysicalDeckCommand : public IAutomationLatentCommand
{
public:
    explicit FPhysicalDeckCommand(FAutomationTestBase* InTest)
        : Test(InTest)
        , PhaseStarted(FPlatformTime::Seconds())
    {
    }

    virtual bool Update() override
    {
        if (Phase == EPhysicalDeckPhase::Initialize)
        {
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::PIE && Context.World()->GetTimeSeconds() > 0.5f)
                {
                    return Initialize(Context.World());
                }
            }
            if (FPlatformTime::Seconds() - PhaseStarted > 15)
            {
                Test->AddError(TEXT("The physical deck PIE fixture did not start"));
                return true;
            }
            return false;
        }
        if (FPlatformTime::Seconds() - PhaseStarted < PhaseDuration)
        {
            return false;
        }
        switch (Phase)
        {
            case EPhysicalDeckPhase::GripSettled:
            {
                Test->TestTrue(TEXT("A held item keeps a dynamic rigid body"), Item->Body->IsSimulatingPhysics());
                Test->TestNull(TEXT("Physical grip does not parent the item"), Item->GetAttachParentActor());
                Test->TestTrue(TEXT("The physical grip reaches the hand"), FVector::Distance(Item->GripPoint->GetComponentLocation(), Hand->GetComponentLocation()) < 8);
                Hand->SetWorldLocation(FVector(70, 0, 260));
                Advance(EPhysicalDeckPhase::Obstructed, 0.7);
                break;
            }
            case EPhysicalDeckPhase::Obstructed:
            {
                Test->TestTrue(TEXT("A held item is blocked by the wall"), Item->Body->GetComponentLocation().X < -30);
                Item->Release(FVector(0, 200, 0), FVector(0, 0, 2));
                Test->TestTrue(TEXT("Release assigns throw velocity"), Item->Body->GetPhysicsLinearVelocity().Equals(FVector(0, 200, 0), 1));
                Item->Destroy();
                Wall->Destroy();
                Lever = World->SpawnActor<APhysicalLever>(FVector(-200, -150, 130), FRotator::ZeroRotator);
                Hand->SetWorldLocation(Lever->GripPoint->GetComponentLocation());
                Test->TestTrue(TEXT("A mounted lever accepts the same physical grip"), Lever->TryGrab(Hand));
                Hand->SetWorldLocation(Lever->GetActorTransform().TransformPosition(FVector(0, -80, 45)));
                Advance(EPhysicalDeckPhase::LeverMoved, 1);
                break;
            }
            case EPhysicalDeckPhase::LeverMoved:
            {
                Test->TestTrue(TEXT("Hand movement changes the lever value"), FMath::Abs(Lever->Value) > 0.2f);
                Test->TestTrue(TEXT("The hinge limits the lever angle"), FMath::Abs(Lever->Hinge->GetCurrentTwist()) < Lever->MaximumAngle + 5);
                Test->TestTrue(TEXT("The lever pivot remains anchored"), FVector::Distance(Lever->Body->GetComponentTransform().TransformPosition(FVector(0, 0, -50)), Lever->Hinge->GetComponentLocation()) < 3);
                LeverValue = Lever->Value;
                Lever->Release(FVector(1000), FVector(100));
                Advance(EPhysicalDeckPhase::LeverReleased, 0.6);
                break;
            }
            case EPhysicalDeckPhase::LeverReleased:
            {
                Test->TestTrue(TEXT("A released lever remains mounted and retains its position"), FMath::Abs(Lever->Value - LeverValue) < 0.15f);
                Lever->Destroy();
                Cargo = World->SpawnActor<AWorkItem>(Vessel->GetActorLocation() + FVector(-450, 160, 80), FRotator::ZeroRotator);
                Cargo->SetActorScale3D(FVector(0.25));
                Advance(EPhysicalDeckPhase::CargoSettled, 1.2);
                break;
            }
            case EPhysicalDeckPhase::CargoSettled:
            {
                CargoStart = Vessel->GetActorTransform().InverseTransformPosition(Cargo->GetActorLocation());
                PawnStart = Vessel->GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation());
                VesselStart = Vessel->GetActorLocation();
                HeldCargo = World->SpawnActor<AWorkItem>(Pawn->RightHand->GetComponentLocation(), FRotator::ZeroRotator);
                Test->TestTrue(TEXT("The player can grab cargo on the deck"), Pawn->TryGrabItem(false));
                Test->TestTrue(TEXT("The player grabbed the intended cargo"), Pawn->RightItem == HeldCargo);
                Vessel->Movement->SetThrottle(0.75);
                Advance(EPhysicalDeckPhase::VesselMoving, 5);
                break;
            }
            case EPhysicalDeckPhase::VesselMoving:
            {
                Test->TestTrue(TEXT("The vessel moved in the saved fishing map"), FVector::Distance(VesselStart, Vessel->GetActorLocation()) > 100);
                Test->TestTrue(TEXT("Loose cargo is carried by deck friction"), FVector::Distance(CargoStart, Vessel->GetActorTransform().InverseTransformPosition(Cargo->GetActorLocation())) < 25);
                Test->TestTrue(TEXT("The player follows the moving deck"), FVector::Distance(PawnStart, Vessel->GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation())) < 3);
                Test->TestTrue(TEXT("Loose cargo has no parent and remains physical"), !Cargo->GetAttachParentActor() && Cargo->Body->IsSimulatingPhysics());
                Test->TestTrue(TEXT("Held cargo follows the hand on a moving deck"), FVector::Distance(HeldCargo->GripPoint->GetComponentLocation(), Pawn->RightHand->GetComponentLocation()) < 15);
                Pawn->ReleaseItem(false);
                Test->TestTrue(TEXT("A stationary hand releases cargo with vessel velocity"), FVector::DotProduct(HeldCargo->Body->GetPhysicsLinearVelocity(), Vessel->GetActorForwardVector()) > Vessel->Movement->CurrentSpeed * 0.7f);
                Vessel->Movement->SetSteering(0.5);
                InitialYaw = Vessel->GetActorRotation().Yaw;
                Advance(EPhysicalDeckPhase::VesselTurning, 1.5);
                break;
            }
            case EPhysicalDeckPhase::VesselTurning:
            {
                Test->TestTrue(TEXT("Steering changes vessel heading"), FMath::Abs(Vessel->GetActorRotation().Yaw - InitialYaw) > 1);
                Test->TestTrue(TEXT("The vessel has no pitch, roll or vertical movement"), FMath::Abs(Vessel->GetActorRotation().Pitch) < 0.01 && FMath::Abs(Vessel->GetActorRotation().Roll) < 0.01 && FMath::Abs(Vessel->GetActorLocation().Z - VesselStart.Z) < 0.01);
                Test->TestTrue(TEXT("The player follows deck rotation"), FVector::Distance(PawnStart, Vessel->GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation())) < 5);
                Cargo->Body->SetWorldLocation(Vessel->GetActorLocation() + FVector(0, 650, 80), false, nullptr, ETeleportType::TeleportPhysics);
                CargoHeight = Cargo->GetActorLocation().Z;
                Advance(EPhysicalDeckPhase::CargoFalling, 0.8);
                break;
            }
            case EPhysicalDeckPhase::CargoFalling:
            {
                Test->TestTrue(TEXT("Cargo outside the deck falls freely"), Cargo->GetActorLocation().Z < CargoHeight - 80);
                Test->AddInfo(TEXT("Physical grip collision, throw, mounted hinge, moving deck, loose cargo, player transport and inherited release velocity were exercised in desktop PIE"));
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
    AVirtualRealityPawn* Pawn = nullptr;
    ATrawlerVessel* Vessel = nullptr;
    AActor* Wall = nullptr;
    USceneComponent* Hand = nullptr;
    AWorkItem* Item = nullptr;
    APhysicalLever* Lever = nullptr;
    AWorkItem* Cargo = nullptr;
    AWorkItem* HeldCargo = nullptr;
    FVector CargoStart;
    FVector PawnStart;
    FVector VesselStart;
    float LeverValue = 0;
    float InitialYaw = 0;
    float CargoHeight = 0;
    double PhaseStarted;
    double PhaseDuration = 0;
    EPhysicalDeckPhase Phase = EPhysicalDeckPhase::Initialize;

    void Advance(EPhysicalDeckPhase NextPhase, double Duration)
    {
        Phase = NextPhase;
        PhaseDuration = Duration;
        PhaseStarted = FPlatformTime::Seconds();
    }

    bool Initialize(UWorld* InWorld)
    {
        World = InWorld;
        Pawn = Cast<AVirtualRealityPawn>(World->GetFirstPlayerController()->GetPawn());
        for (TActorIterator<ATrawlerVessel> VesselIterator(World); VesselIterator; ++VesselIterator)
        {
            Vessel = *VesselIterator;
        }
        if (!Pawn || !Vessel)
        {
            Test->AddError(TEXT("The fishing map must contain a VR pawn and a trawler vessel"));
            return true;
        }
        Test->TestNotNull(TEXT("The vessel has a configured throttle lever"), Vessel->ThrottleLever.Get());
        Test->TestNotNull(TEXT("The vessel has a configured steering lever"), Vessel->SteeringLever.Get());
        AActor* HandActor = World->SpawnActor<AActor>();
        Hand = NewObject<USceneComponent>(HandActor);
        HandActor->SetRootComponent(Hand);
        Hand->RegisterComponent();
        Hand->SetWorldLocation(FVector(-100, 0, 260));
        Item = World->SpawnActor<AWorkItem>(Hand->GetComponentLocation(), FRotator::ZeroRotator);
        Test->TestTrue(TEXT("A free item accepts the physical grip"), Item->TryGrab(Hand));
        Wall = World->SpawnActor<AActor>();
        UBoxComponent* Obstacle = NewObject<UBoxComponent>(Wall);
        Wall->SetRootComponent(Obstacle);
        Obstacle->SetBoxExtent(FVector(5, 70, 70));
        Obstacle->SetCollisionProfileName(TEXT("BlockAll"));
        Obstacle->RegisterComponent();
        Obstacle->SetWorldLocation(FVector(-20, 0, 260));
        Advance(EPhysicalDeckPhase::GripSettled, 0.5);
        return false;
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicalDeckTest, "YakutSimulator.Runtime.PhysicalDeck", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPhysicalDeckTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Prototype/Maps/L_Fishing")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FPhysicalDeckCommand(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(GetDefault<APrototypeGameMode>()->MainMenuMap.ToSoftObjectPath().GetLongPackageName()));
    return true;
}

#endif
