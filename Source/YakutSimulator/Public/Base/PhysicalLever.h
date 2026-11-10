#pragma once

#include "CoreMinimal.h"
#include "Base/WorkItem.h"
#include "PhysicalLever.generated.h"

class UPhysicsConstraintComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLeverValueChanged, float, Value);

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API APhysicalLever : public AWorkItem
{
    GENERATED_BODY()

public:
    APhysicalLever();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lever")
    TObjectPtr<USceneComponent> Mount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lever")
    TObjectPtr<UStaticMeshComponent> Foundation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lever")
    TObjectPtr<UStaticMeshComponent> Knob;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Lever")
    TObjectPtr<UPhysicsConstraintComponent> Hinge;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Lever", meta=(ClampMin="5", ClampMax="80"))
    float MaximumAngle = 40;

    UPROPERTY(BlueprintReadOnly, Category="Lever")
    float Value = 0;

    UPROPERTY(BlueprintAssignable, Category="Lever")
    FLeverValueChanged OnValueChanged;

    virtual bool TryGrab(USceneComponent* Hand) override;
    virtual void Release(const FVector& LinearVelocity, const FVector& AngularVelocity) override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
};
