#include "Minigames/Fishing/VesselMovementComponent.h"

UVesselMovementComponent::UVesselMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UVesselMovementComponent::SetThrottle(float Value)
{
    Throttle = FMath::Clamp(Value, -1.0f, 1.0f);
}

void UVesselMovementComponent::SetSteering(float Value)
{
    Steering = FMath::Clamp(Value, -1.0f, 1.0f);
}

void UVesselMovementComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, TickFunction);
    if (!bMovementEnabled)
    {
        CurrentSpeed = 0;
        Velocity = FVector::ZeroVector;
        UpdateComponentVelocity();
        return;
    }
    if (!UpdatedComponent || ShouldSkipUpdate(DeltaSeconds))
    {
        return;
    }
    CurrentSpeed = FMath::FInterpConstantTo(CurrentSpeed, Throttle * MaximumSpeed, DeltaSeconds, Acceleration);
    const FVector PreviousLocation = UpdatedComponent->GetComponentLocation();
    FRotator Rotation = UpdatedComponent->GetComponentRotation();
    Rotation.Pitch = 0;
    Rotation.Roll = 0;
    Rotation.Yaw += Steering * TurnRate * FMath::Clamp(CurrentSpeed / FMath::Max(MaximumSpeed, 1.0f), -1.0f, 1.0f) * DeltaSeconds;
    MovementCollision = FHitResult();
    MoveUpdatedComponent(Rotation.Vector() * CurrentSpeed * DeltaSeconds, Rotation.Quaternion(), true, &MovementCollision);
    if (MovementCollision.bBlockingHit)
    {
        CurrentSpeed = 0;
    }
    Velocity = (UpdatedComponent->GetComponentLocation() - PreviousLocation) / DeltaSeconds;
    UpdateComponentVelocity();
}
