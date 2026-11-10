#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "VesselMovementComponent.generated.h"

UCLASS(Blueprintable, ClassGroup=(Movement), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UVesselMovementComponent : public UMovementComponent
{
    GENERATED_BODY()

public:
    UVesselMovementComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vessel")
    bool bMovementEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vessel", meta=(ClampMin="0"))
    float MaximumSpeed = 160;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vessel", meta=(ClampMin="1"))
    float Acceleration = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vessel", meta=(ClampMin="0"))
    float TurnRate = 5;

    UPROPERTY(BlueprintReadOnly, Category="Vessel")
    float CurrentSpeed = 0;

    UPROPERTY(BlueprintReadOnly, Category="Vessel")
    float Throttle = 0;

    UPROPERTY(BlueprintReadOnly, Category="Vessel")
    float Steering = 0;

    UPROPERTY(BlueprintReadOnly, Category="Vessel")
    FHitResult MovementCollision;

    UFUNCTION(BlueprintCallable, Category="Vessel")
    void SetThrottle(float Value);

    UFUNCTION(BlueprintCallable, Category="Vessel")
    void SetSteering(float Value);

    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
};
