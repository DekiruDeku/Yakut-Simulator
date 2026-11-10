#include "Base/PhysicalLever.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "UObject/ConstructorHelpers.h"

APhysicalLever::APhysicalLever()
{
    Mount = CreateDefaultSubobject<USceneComponent>(TEXT("Mount"));
    SetRootComponent(Mount);
    Foundation = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Foundation"));
    Foundation->SetupAttachment(Mount);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Foundation->SetStaticMesh(Cube.Object);
    Foundation->SetRelativeScale3D(FVector(0.35, 0.3, 0.12));
    Foundation->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Body->SetupAttachment(Mount);
    Body->SetRelativeLocation(FVector(0, 0, 38.5));
    Body->SetRelativeScale3D(FVector(0.08, 0.08, 0.65));
    Body->SetMassOverrideInKg(NAME_None, 1);
    Body->SetAngularDamping(5);
    GripPoint->SetRelativeLocation(FVector(0, 0, 50));
    Knob = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Knob"));
    Knob->SetupAttachment(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Knob->SetStaticMesh(Sphere.Object);
    Knob->SetRelativeLocation(FVector(0, 0, 50));
    Knob->SetRelativeScale3D(FVector(2, 2, 0.2461538));
    Knob->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Hinge = CreateDefaultSubobject<UPhysicsConstraintComponent>(TEXT("Hinge"));
    Hinge->SetupAttachment(Mount);
    Hinge->SetRelativeLocation(FVector(0, 0, 6));
    Hinge->SetDisableCollision(true);
    Hinge->SetLinearXLimit(ELinearConstraintMotion::LCM_Locked, 0);
    Hinge->SetLinearYLimit(ELinearConstraintMotion::LCM_Locked, 0);
    Hinge->SetLinearZLimit(ELinearConstraintMotion::LCM_Locked, 0);
    Hinge->SetAngularSwing1Limit(EAngularConstraintMotion::ACM_Locked, 0);
    Hinge->SetAngularSwing2Limit(EAngularConstraintMotion::ACM_Locked, 0);
    Hinge->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, MaximumAngle);
    Hinge->SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
    Hinge->SetAngularOrientationDrive(false, true);
    Hinge->SetAngularDriveParams(20000, 1000, 100000);
    bConstrainGripRotation = false;
    bIsAnchored = true;
    ItemType = TEXT("Lever");
    DisplayName = FText::FromString(TEXT("Рычаг"));
    UseIndicator->SetRelativeLocation(FVector::ZeroVector);
}

void APhysicalLever::BeginPlay()
{
    Super::BeginPlay();
    Hinge->SetAngularTwistLimit(EAngularConstraintMotion::ACM_Limited, MaximumAngle);
    Hinge->SetConstrainedComponents(Foundation, NAME_None, Body, NAME_None);
}

bool APhysicalLever::TryGrab(USceneComponent* Hand)
{
    if (!Super::TryGrab(Hand))
    {
        return false;
    }
    Hinge->SetAngularOrientationDrive(false, false);
    return true;
}

void APhysicalLever::Release(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
    if (!bIsHeld)
    {
        return;
    }
    const float RetainedAngle = FMath::Clamp(Hinge->GetCurrentTwist(), -MaximumAngle, MaximumAngle);
    Super::Release(LinearVelocity, AngularVelocity);
    Hinge->SetAngularOrientationTarget(FRotator(0, 0, -RetainedAngle));
    Hinge->SetAngularOrientationDrive(false, true);
}

void APhysicalLever::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float UpdatedValue = FMath::Clamp(Hinge->GetCurrentTwist() / MaximumAngle, -1.0f, 1.0f);
    if (!FMath::IsNearlyEqual(Value, UpdatedValue, 0.005f))
    {
        Value = UpdatedValue;
        OnValueChanged.Broadcast(Value);
    }
}
