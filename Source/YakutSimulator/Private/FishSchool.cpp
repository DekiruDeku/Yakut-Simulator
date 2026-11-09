#include "FishSchool.h"

#include "NiagaraComponent.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Kismet/KismetSystemLibrary.h"

#if WITH_EDITOR
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraMeshRendererProperties.h"
#endif

AFishSchool::AFishSchool()
{
    PrimaryActorTick.bCanEverTick = true;
    FishVisuals = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FishVisuals"));
    FishVisuals->SetMobility(EComponentMobility::Movable);
    SetRootComponent(FishVisuals);
    FishVisuals->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFishSchool::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ConfigureVisuals();
}

void AFishSchool::ConfigureVisuals()
{
    if (!Species || !Species->Mesh)
    {
        FishVisuals->SetVisibility(false);
        return;
    }
    const float VisualRadius = FMath::Max(0.0f, Radius - Species->Mesh->GetBounds().SphereRadius);
    FishVisuals->SetVariableStaticMesh(TEXT("User.FishMesh"), Species->Mesh);
    FishVisuals->SetVariableMaterial(TEXT("User.FishMaterial"), Species->Material);
    FishVisuals->SetVariableInt(TEXT("User.FishCount"), FMath::Clamp(RemainingFish, 0, MaximumVisualFish));
    FishVisuals->SetVariableFloat(TEXT("User.SchoolRadius"), VisualRadius);
    FishVisuals->SetVariableFloat(TEXT("User.VerticalSpread"), VerticalSpread);
    FishVisuals->SetVariableFloat(TEXT("User.SwimAmplitude"), FMath::Min(SwimAmplitude, VisualRadius * 0.1f));
    FishVisuals->SetVariableFloat(TEXT("User.SwimFrequency"), SwimFrequency);
    FishVisuals->SetVariableVec3(TEXT("User.SchoolDirection"), VisualHeading.Vector());
    FishVisuals->SetSystemFixedBounds(FBox(FVector(-Radius, -Radius, -VerticalSpread - 100), FVector(Radius, Radius, VerticalSpread + 100)));
    FishVisuals->SetVisibility(RemainingFish > 0);
}

#if WITH_EDITOR
void AFishSchool::ConfigureVisualSystemBindings()
{
    UNiagaraSystem* System = FishVisuals->GetAsset();
    if (!System)
    {
        return;
    }
    System->Modify();
    for (const FNiagaraEmitterHandle& Emitter : System->GetEmitterHandles())
    {
        FVersionedNiagaraEmitterData* EmitterData = Emitter.GetInstance().GetEmitterData();
        if (!EmitterData)
        {
            continue;
        }
        for (UNiagaraRendererProperties* Renderer : EmitterData->GetRenderers())
        {
            UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer);
            if (!MeshRenderer)
            {
                continue;
            }
            MeshRenderer->Modify();
            for (FNiagaraMeshRendererMeshProperties& Mesh : MeshRenderer->Meshes)
            {
                Mesh.MeshParameterBinding.ResolvedParameter = FNiagaraVariableBase(FNiagaraTypeDefinition(UStaticMesh::StaticClass()), TEXT("User.FishMesh"));
                Mesh.MeshParameterBinding.AliasedParameter = Mesh.MeshParameterBinding.ResolvedParameter;
            }
            MeshRenderer->bOverrideMaterials = true;
            MeshRenderer->OverrideMaterials.SetNum(1);
            MeshRenderer->OverrideMaterials[0].UserParamBinding.Parameter = FNiagaraVariable(FNiagaraTypeDefinition(UMaterialInterface::StaticClass()), TEXT("User.FishMaterial"));
            MeshRenderer->PostEditChange();
        }
    }
    System->PostEditChange();
    System->MarkPackageDirty();
}
#endif

void AFishSchool::BeginPlay()
{
    Super::BeginPlay();
    MovementOrigin = GetActorLocation();
    MovementDestination = MovementOrigin;
    MovementRandom.Initialize(MovementSeed);
    RefreshCoastalBoundaries();
    if (IsCoastalPathBlocked(MovementOrigin, MovementOrigin))
    {
        UKismetSystemLibrary::PrintString(this, TEXT("The fish school starts inside a coastal exclusion area; place it in open water"), true, true, FLinearColor::Yellow);
    }
    ConfigureVisuals();
}

void AFishSchool::RefreshCoastalBoundaries()
{
    CoastalSplines.Reset();
    CoastalBoundaries.Reset();
    for (TActorIterator<AActor> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
    {
        TArray<USplineComponent*> Splines;
        ActorIterator->GetComponents(Splines);
        for (USplineComponent* Spline : Splines)
        {
            if (!ActorIterator->ActorHasTag(CoastalZoneTag) && !Spline->ComponentHasTag(CoastalZoneTag))
            {
                continue;
            }
            if (!Spline->IsClosedLoop() || Spline->GetNumberOfSplinePoints() < 3)
            {
                UKismetSystemLibrary::PrintString(this, TEXT("A Shore spline must be closed and contain at least three points"), true, true, FLinearColor::Yellow);
                continue;
            }
            const float Length = Spline->GetSplineLength();
            if (Length < 1)
            {
                continue;
            }
            const int32 SampleCount = FMath::Clamp(FMath::CeilToInt(Length / FMath::Max(25.0f, BoundarySamplingDistance)), 3, 2048);
            FFishSchoolCoastalBoundary Boundary;
            Boundary.SamplingPadding = Length / SampleCount * 0.5f;
            for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
            {
                FVector Position = Spline->GetLocationAtDistanceAlongSpline(Length * SampleIndex / SampleCount, ESplineCoordinateSpace::World);
                Position.Z = 0;
                Boundary.Points.Add(Position);
                Boundary.Bounds += FVector2D(Position);
            }
            CoastalSplines.Add(Spline);
            CoastalBoundaries.Add(MoveTemp(Boundary));
        }
    }
    bHasMovementDestination = false;
    DestinationRetrySeconds = 0;
}

bool AFishSchool::IsInsideBoundary(const FVector& Position, const FFishSchoolCoastalBoundary& Boundary)
{
    bool bInside = false;
    FVector Previous = Boundary.Points.Last();
    for (const FVector& Current : Boundary.Points)
    {
        if ((Current.Y > Position.Y) != (Previous.Y > Position.Y))
        {
            if (Position.X < (Previous.X - Current.X) * (Position.Y - Current.Y) / (Previous.Y - Current.Y) + Current.X)
            {
                bInside = !bInside;
            }
        }
        Previous = Current;
    }
    return bInside;
}

bool AFishSchool::IsCoastalPathBlocked(const FVector& Start, const FVector& End) const
{
    const FVector FlatStart(Start.X, Start.Y, 0);
    const FVector FlatEnd(End.X, End.Y, 0);
    FBox2D RouteBounds(ForceInit);
    RouteBounds += FVector2D(FlatStart);
    RouteBounds += FVector2D(FlatEnd);
    for (const FFishSchoolCoastalBoundary& Boundary : CoastalBoundaries)
    {
        const float Clearance = FMath::Max(0.0f, Radius) + FMath::Max(0.0f, ShoreClearance) + Boundary.SamplingPadding;
        if (!Boundary.Bounds.ExpandBy(Clearance).Intersect(RouteBounds))
        {
            continue;
        }
        if (IsInsideBoundary(FlatStart, Boundary) || IsInsideBoundary(FlatEnd, Boundary))
        {
            return true;
        }
        FVector Previous = Boundary.Points.Last();
        for (const FVector& Current : Boundary.Points)
        {
            FVector ClosestOnRoute;
            FVector ClosestOnBoundary;
            FMath::SegmentDistToSegmentSafe(FlatStart, FlatEnd, Previous, Current, ClosestOnRoute, ClosestOnBoundary);
            if (FVector::DistSquared(ClosestOnRoute, ClosestOnBoundary) <= FMath::Square(Clearance))
            {
                return true;
            }
            Previous = Current;
        }
    }
    return false;
}

bool AFishSchool::SetMovementDestination(const FVector& Destination)
{
    FVector PlanarDestination = Destination;
    PlanarDestination.Z = MovementOrigin.Z;
    if (FVector::DistSquared2D(PlanarDestination, MovementOrigin) > FMath::Square(MovementRadius)
        || IsCoastalPathBlocked(GetActorLocation(), PlanarDestination))
    {
        return false;
    }
    MovementDestination = PlanarDestination;
    bHasMovementDestination = true;
    bChooseDestinationAfterStopping = false;
    return true;
}

void AFishSchool::ChooseMovementDestination()
{
    bHasMovementDestination = false;
    DestinationRetrySeconds = 1;
    for (int32 Attempt = 0; Attempt < 32; ++Attempt)
    {
        const FVector Offset = FVector(MovementRandom.FRandRange(0, MovementRadius * 0.85f), 0, 0).RotateAngleAxis(MovementRandom.FRandRange(0, 360), FVector::UpVector);
        const FVector Candidate = MovementOrigin + Offset;
        if (FVector::DistSquared2D(Candidate, GetActorLocation()) < FMath::Square(200.0f))
        {
            continue;
        }
        if (SetMovementDestination(Candidate))
        {
            return;
        }
    }
}

void AFishSchool::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bMovementEnabled || RemainingFish <= 0)
    {
        MovementVelocity = FVector::ZeroVector;
        bAvoidingCoast = false;
        return;
    }
    DestinationRetrySeconds -= DeltaSeconds;
    const FVector Position = GetActorLocation();
    if (bChooseDestinationAfterStopping && MovementVelocity.SizeSquared() < 4)
    {
        bChooseDestinationAfterStopping = false;
        bHasMovementDestination = false;
        DestinationRetrySeconds = 0;
    }
    if (!bChooseDestinationAfterStopping && (!bHasMovementDestination || FVector::DistSquared2D(Position, MovementDestination) < FMath::Square(100.0f)) && DestinationRetrySeconds <= 0)
    {
        ChooseMovementDestination();
    }
    FVector DesiredVelocity = FVector::ZeroVector;
    bAvoidingCoast = false;
    if (bHasMovementDestination)
    {
        const FVector Direction = MovementDestination - Position;
        DesiredVelocity = Direction.GetSafeNormal2D() * FMath::Min(MovementSpeed, FMath::Sqrt(2 * MovementAcceleration * Direction.Size2D()));
        if (IsCoastalPathBlocked(Position, MovementDestination))
        {
            bChooseDestinationAfterStopping = true;
            bAvoidingCoast = true;
        }
    }
    const FVector BrakingEnd = Position + MovementVelocity.GetSafeNormal2D() * (MovementVelocity.SizeSquared2D() / (2 * FMath::Max(1.0f, MovementAcceleration)) + MovementSpeed * DeltaSeconds * 2 + 2);
    if (IsCoastalPathBlocked(Position, BrakingEnd))
    {
        bChooseDestinationAfterStopping = true;
        bAvoidingCoast = true;
    }
    if (bChooseDestinationAfterStopping)
    {
        DesiredVelocity = FVector::ZeroVector;
    }
    MovementVelocity = FMath::VInterpConstantTo(MovementVelocity, DesiredVelocity, DeltaSeconds, MovementAcceleration);
    const FVector NextPosition = Position + MovementVelocity * DeltaSeconds;
    if (IsCoastalPathBlocked(Position, NextPosition) || FVector::DistSquared2D(NextPosition, MovementOrigin) > FMath::Square(MovementRadius))
    {
        MovementVelocity = FVector::ZeroVector;
        bHasMovementDestination = false;
        DestinationRetrySeconds = 0;
        return;
    }
    SetActorLocation(NextPosition);
    if (!MovementVelocity.IsNearlyZero())
    {
        const FRotator DesiredRotation = GetActorTransform().InverseTransformVectorNoScale(MovementVelocity).Rotation();
        VisualHeading = FMath::RInterpConstantTo(VisualHeading, DesiredRotation, DeltaSeconds, TurnRate);
        FishVisuals->SetVariableVec3(TEXT("User.SchoolDirection"), VisualHeading.Vector());
    }
}

int32 AFishSchool::ConsumeFish(int32 RequestedCount)
{
    const int32 Taken = FMath::Clamp(RequestedCount, 0, RemainingFish);
    RemainingFish -= Taken;
    FishVisuals->SetVariableInt(TEXT("User.FishCount"), FMath::Clamp(RemainingFish, 0, MaximumVisualFish));
    if (RemainingFish == 0)
    {
        FishVisuals->SetVisibility(false);
    }
    return Taken;
}
