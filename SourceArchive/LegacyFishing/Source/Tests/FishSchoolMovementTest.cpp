#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "NiagaraComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FishSchool.h"
#include "PrototypeGameMode.h"
#include "Tests/AutomationEditorCommon.h"

class FFishSchoolMovementCommand : public IAutomationLatentCommand
{
public:
    explicit FFishSchoolMovementCommand(FAutomationTestBase* InTest)
        : Test(InTest)
        , Started(FPlatformTime::Seconds())
    {
    }

    virtual bool Update() override
    {
        if (!School)
        {
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::PIE && Context.World()->GetTimeSeconds() > 0.75f)
                {
                    return Initialize(Context.World());
                }
            }
            if (FPlatformTime::Seconds() - Started > 15)
            {
                Test->AddError(TEXT("The fish school movement PIE fixture did not start"));
                return true;
            }
            return false;
        }
        const float CurrentTime = World->GetTimeSeconds();
        const float DeltaSeconds = CurrentTime - PreviousTime;
        if (DeltaSeconds <= 0)
        {
            return false;
        }
        const FVector Position = School->GetActorLocation();
        const FVector2D LocalPosition(Position - Origin);
        MinimumClearance = FMath::Min(MinimumClearance, FMath::Sqrt(CoastBounds.ComputeSquaredDistanceToPoint(LocalPosition)));
        MinimumClearance = FMath::Min(MinimumClearance, FMath::Sqrt(LowerCoastBounds.ComputeSquaredDistanceToPoint(LocalPosition)));
        MinimumClearance = FMath::Min(MinimumClearance, FMath::Sqrt(LeftCoastBounds.ComputeSquaredDistanceToPoint(LocalPosition)));
        TravelDistance += FVector::Dist(Position, PreviousPosition);
        MaximumApproach = FMath::Max(MaximumApproach, Position.X - Origin.X);
        if (FVector::Distance(School->MovementVelocity, PreviousVelocity) > School->MovementAcceleration * DeltaSeconds + 1)
        {
            Test->AddError(TEXT("Fish school velocity changed faster than its configured acceleration"));
            return Finish();
        }
        if (FMath::Abs(Position.Z - Origin.Z) > 0.01 || FVector::DistSquared2D(Position, Origin) > FMath::Square(School->MovementRadius + 1))
        {
            Test->AddError(TEXT("The fish school left its configured movement plane or range"));
            return Finish();
        }
        if (MinimumClearance < School->Radius + School->ShoreClearance - 0.1f)
        {
            Test->AddError(TEXT("The fish school entered the protected coastal clearance"));
            return Finish();
        }
        PreviousTime = CurrentTime;
        PreviousPosition = Position;
        PreviousVelocity = School->MovementVelocity;
        if (CurrentTime - MovementStarted < 14)
        {
            return false;
        }
        Test->TestTrue(TEXT("A fish school travels autonomously through the saved fishing world"), TravelDistance > 1000);
        Test->TestTrue(TEXT("The school approached the coastal obstacle before changing course"), MaximumApproach > 400);
        Test->TestTrue(TEXT("The complete school radius and shore margin remain outside the polygon"), MinimumClearance >= School->Radius + School->ShoreClearance);
        Test->TestNotNull(TEXT("The saved school Blueprint uses a Niagara system"), School->FishVisuals->GetAsset());
        Test->AddInfo(FString::Format(TEXT("Fish school movement in desktop PIE: travel={0} cm, minimum coast distance={1} cm"), { TravelDistance, MinimumClearance }));
        return Finish();
    }

private:
    FAutomationTestBase* Test;
    UWorld* World = nullptr;
    AFishSchool* School = nullptr;
    AActor* Coast = nullptr;
    AActor* SecondaryCoast = nullptr;
    FVector Origin = FVector(100000, 100000, -400);
    FBox2D CoastBounds = FBox2D(FVector2D(1000, -500), FVector2D(2000, 500));
    FBox2D LowerCoastBounds = FBox2D(FVector2D(-3000, -3000), FVector2D(-1000, -2500));
    FBox2D LeftCoastBounds = FBox2D(FVector2D(-3000, -3000), FVector2D(-2500, -1000));
    FVector PreviousPosition = FVector::ZeroVector;
    FVector PreviousVelocity = FVector::ZeroVector;
    double Started;
    float PreviousTime = 0;
    float MovementStarted = 0;
    float MinimumClearance = MAX_flt;
    float TravelDistance = 0;
    float MaximumApproach = 0;

    bool Initialize(UWorld* InWorld)
    {
        World = InWorld;
        AFishSchool* TemplateSchool = nullptr;
        for (TActorIterator<AFishSchool> SchoolIterator(World); SchoolIterator; ++SchoolIterator)
        {
            TemplateSchool = *SchoolIterator;
            SchoolIterator->bMovementEnabled = false;
        }
        if (!TemplateSchool)
        {
            Test->AddError(TEXT("The saved map contains no configured fish school Blueprint"));
            return true;
        }
        Coast = World->SpawnActor<AActor>();
        USplineComponent* Spline = NewObject<USplineComponent>(Coast, TEXT("BoundarySpline"));
        Coast->SetRootComponent(Spline);
        Spline->RegisterComponent();
        Spline->SetWorldLocation(Origin);
        Coast->Tags.Add(TemplateSchool->CoastalZoneTag);
        Spline->ClearSplinePoints(false);
        for (const FVector& Point : { FVector(1000, -500, 0), FVector(2000, -500, 0), FVector(2000, 500, 0), FVector(1000, 500, 0) })
        {
            Spline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
        }
        for (int32 PointIndex = 0; PointIndex < Spline->GetNumberOfSplinePoints(); ++PointIndex)
        {
            Spline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
        }
        Spline->SetClosedLoop(true, true);
        SecondaryCoast = World->SpawnActor<AActor>();
        USplineComponent* ConcaveSpline = NewObject<USplineComponent>(SecondaryCoast, TEXT("ConcaveBoundarySpline"));
        SecondaryCoast->SetRootComponent(ConcaveSpline);
        ConcaveSpline->RegisterComponent();
        ConcaveSpline->SetWorldLocation(Origin);
        ConcaveSpline->ComponentTags.Add(TemplateSchool->CoastalZoneTag);
        ConcaveSpline->ClearSplinePoints(false);
        for (const FVector& Point : { FVector(-3000, -3000, 0), FVector(-1000, -3000, 0), FVector(-1000, -2500, 0), FVector(-2500, -2500, 0), FVector(-2500, -1000, 0), FVector(-3000, -1000, 0) })
        {
            ConcaveSpline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
        }
        for (int32 PointIndex = 0; PointIndex < ConcaveSpline->GetNumberOfSplinePoints(); ++PointIndex)
        {
            ConcaveSpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
        }
        ConcaveSpline->SetClosedLoop(true, true);
        const FTransform Transform(FRotator::ZeroRotator, Origin);
        School = World->SpawnActorDeferred<AFishSchool>(TemplateSchool->GetClass(), Transform);
        School->Species = TemplateSchool->Species;
        School->Radius = 200;
        School->MovementRadius = 4000;
        School->MovementSpeed = 200;
        School->MovementAcceleration = 100;
        School->ShoreClearance = 100;
        School->MovementSeed = 71;
        School->bMovementEnabled = true;
        School->FinishSpawning(Transform);
        Test->TestFalse(TEXT("A component-tagged concave coastline is detected"), School->SetMovementDestination(Origin + FVector(-2800, -2800, 0)));
        Test->TestTrue(TEXT("Open water inside the bounds of a concave coast remains accessible"), School->SetMovementDestination(Origin + FVector(-1500, -1500, 0)));
        Test->TestFalse(TEXT("A route across a coastal polygon is rejected even when its destination is in open water"), School->SetMovementDestination(Origin + FVector(3000, 0, 0)));
        Test->TestFalse(TEXT("A destination inside the coast is rejected"), School->SetMovementDestination(Origin + FVector(1500, 0, 0)));
        Test->TestTrue(TEXT("A safe destination near the coast is accepted"), School->SetMovementDestination(Origin + FVector(600, 0, 0)));
        PreviousTime = World->GetTimeSeconds();
        MovementStarted = PreviousTime;
        PreviousPosition = School->GetActorLocation();
        return false;
    }

    bool Finish()
    {
        School->Destroy();
        Coast->Destroy();
        SecondaryCoast->Destroy();
        return true;
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishSchoolMovementTest, "YakutSimulator.Runtime.FishSchoolMovement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFishSchoolMovementTest::RunTest(const FString& Parameters)
{
    ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(TEXT("/Game/Prototype/Maps/L_Fishing")));
    ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
    ADD_LATENT_AUTOMATION_COMMAND(FFishSchoolMovementCommand(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}

#endif
