#include "Base/WorkItem.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Base/InteractionZoneComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "UObject/ConstructorHelpers.h"

AWorkItem::AWorkItem()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    SetRootComponent(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(DefaultShape.Object);
    Body->SetRelativeScale3D(FVector(0.12));
    Body->SetCollisionProfileName(TEXT("PhysicsActor"));
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    Body->SetGenerateOverlapEvents(true);
    Body->SetSimulatePhysics(true);
    Body->SetUseCCD(true);
    Body->SetLinearDamping(0.1f);
    Body->SetAngularDamping(0.2f);

    GripPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GripPoint"));
    GripPoint->SetupAttachment(Body);
    PhysicalGrip = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("PhysicalGrip"));
    PhysicalGrip->SetLinearStiffness(6000);
    PhysicalGrip->SetLinearDamping(400);
    PhysicalGrip->SetAngularStiffness(3000);
    PhysicalGrip->SetAngularDamping(500);
    PhysicalGrip->bInterpolateTarget = false;
    PhysicalGrip->PrimaryComponentTick.AddPrerequisite(this, PrimaryActorTick);
    UseIndicator = CreateDefaultSubobject<UPointLightComponent>(TEXT("UseIndicator"));
    UseIndicator->SetupAttachment(Body);
    UseIndicator->SetRelativeLocation(FVector(0, 0, 80));
    UseIndicator->SetLightColor(FLinearColor(0.15f, 0.7f, 1.0f));
    UseIndicator->SetIntensityUnits(ELightUnits::Lumens);
    UseIndicator->SetIntensity(30);
    UseIndicator->SetAttenuationRadius(120);
    UseIndicator->SetVisibility(false);
    DisplayName = FText::FromString(TEXT("Предмет"));
}

bool AWorkItem::TryGrab(USceneComponent* Hand)
{
    if (!CanBeGrabbed() || !Hand)
    {
        return false;
    }
    EndUse();
    Body->SetSimulatePhysics(true);
    Body->WakeAllRigidBodies();
    if (bConstrainGripRotation)
    {
        PhysicalGrip->GrabComponentAtLocationWithRotation(Body, NAME_None, GripPoint->GetComponentLocation(), GripPoint->GetComponentRotation());
    }
    else
    {
        PhysicalGrip->GrabComponentAtLocation(Body, NAME_None, GripPoint->GetComponentLocation());
    }
    if (!PhysicalGrip->GetGrabbedComponent())
    {
        return false;
    }
    HoldingHand = Hand;
    AddTickPrerequisiteActor(Hand->GetOwner());
    UpdateGripTarget();
    bIsHeld = true;
    OnItemAction.Broadcast(this, EWorkAction::Grabbed);
    return true;
}

bool AWorkItem::CanBeGrabbed() const
{
    return !bIsHeld;
}

void AWorkItem::Release(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
    if (!bIsHeld)
    {
        return;
    }
    EndUse();
    PhysicalGrip->ReleaseComponent();
    if (IsValid(HoldingHand))
    {
        RemoveTickPrerequisiteActor(HoldingHand->GetOwner());
    }
    HoldingHand = nullptr;
    bIsHeld = false;
    Body->SetSimulatePhysics(true);
    if (!bIsAnchored)
    {
        Body->SetPhysicsLinearVelocity(LinearVelocity.GetClampedToMaxSize(1800));
        Body->SetPhysicsAngularVelocityInRadians(AngularVelocity.GetClampedToMaxSize(30));
    }
    OnItemAction.Broadcast(this, EWorkAction::Released);

    if (bIsAnchored)
    {
        return;
    }

    TArray<UPrimitiveComponent*> OverlappingComponents;
    Body->GetOverlappingComponents(OverlappingComponents);
    UInteractionZoneComponent* ClosestZone = nullptr;
    float ClosestDistance = MAX_flt;
    for (UPrimitiveComponent* OverlappingComponent : OverlappingComponents)
    {
        if (UInteractionZoneComponent* Zone = Cast<UInteractionZoneComponent>(OverlappingComponent))
        {
            if (Zone->CanAcceptItem(this))
            {
                const float Distance = FVector::DistSquared(GetActorLocation(), Zone->GetComponentLocation());
                if (Distance < ClosestDistance)
                {
                    ClosestZone = Zone;
                    ClosestDistance = Distance;
                }
            }
        }
    }
    if (ClosestZone)
    {
        ClosestZone->TryAcceptItem(this);
    }
}

void AWorkItem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bIsHeld)
    {
        return;
    }
    if (!IsValid(HoldingHand))
    {
        Release(FVector::ZeroVector, FVector::ZeroVector);
        return;
    }
    UpdateGripTarget();
}

void AWorkItem::UpdateGripTarget()
{
    FVector Target = HoldingHand->GetComponentLocation();
    if (bConstrainGripRotation)
    {
        const FQuat BodyRotation = HoldingHand->GetComponentQuat() * GripPoint->GetComponentQuat().Inverse() * Body->GetComponentQuat();
        const FVector GripOffset = BodyRotation.RotateVector(Body->GetComponentQuat().Inverse().RotateVector(GripPoint->GetComponentLocation() - Body->GetComponentLocation()));
        const FBoxSphereBounds Shape = Body->CalcBounds(FTransform(FQuat::Identity, FVector::ZeroVector, Body->GetComponentScale()));
        const FVector DesiredBoundsOffset = BodyRotation.RotateVector(Shape.Origin);
        FCollisionQueryParams Query;
        Query.AddIgnoredActor(this);
        Query.AddIgnoredActor(HoldingHand->GetOwner());
        FHitResult Obstacle;
        if (GetWorld()->SweepSingleByChannel(Obstacle, Body->Bounds.Origin, Target - GripOffset + DesiredBoundsOffset, Body->GetComponentQuat(), Body->GetCollisionObjectType(), FCollisionShape::MakeBox(Shape.BoxExtent), Query)
            && !Obstacle.bStartPenetrating)
        {
            Target = Obstacle.Location + Obstacle.Normal * 0.5f - DesiredBoundsOffset + GripOffset;
        }
    }
    PhysicalGrip->SetTargetLocationAndRotation(Target, HoldingHand->GetComponentRotation());
}

void AWorkItem::BeginUse()
{
    if (!bIsHeld || bIsUsing)
    {
        return;
    }
    bIsUsing = true;
    ++UseCount;
    ReceiveUseStarted();
    OnItemAction.Broadcast(this, EWorkAction::Used);
}

void AWorkItem::EndUse()
{
    if (!bIsUsing)
    {
        return;
    }
    bIsUsing = false;
    ReceiveUseEnded();
}

void AWorkItem::ReceiveUseStarted_Implementation()
{
    UseIndicator->SetVisibility(true);
}

void AWorkItem::ReceiveUseEnded_Implementation()
{
    UseIndicator->SetVisibility(false);
}

void AWorkItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndUse();
    PhysicalGrip->ReleaseComponent();
    HoldingHand = nullptr;
    bIsHeld = false;
    Super::EndPlay(EndPlayReason);
}
