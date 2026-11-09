#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrawlerVessel.generated.h"

class APhysicalLever;
class UBoxComponent;
class UVesselMovementComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API ATrawlerVessel : public AActor
{
    GENERATED_BODY()

public:
    ATrawlerVessel();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vessel")
    TObjectPtr<UBoxComponent> Hull;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Vessel")
    TObjectPtr<UVesselMovementComponent> Movement;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<USceneComponent> TowPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<USceneComponent> CatchDropPoint;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Controls")
    TObjectPtr<APhysicalLever> ThrottleLever;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Controls")
    TObjectPtr<APhysicalLever> SteeringLever;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UFUNCTION()
    void SetThrottle(float Value);

    UFUNCTION()
    void SetSteering(float Value);
};
