#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishingWinch.generated.h"

class APhysicalLever;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishingWinch : public AActor
{
    GENERATED_BODY()

public:
    AFishingWinch();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Winch")
    TObjectPtr<UStaticMeshComponent> Foundation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Winch")
    TObjectPtr<UStaticMeshComponent> Drum;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Winch")
    TObjectPtr<APhysicalLever> ControlLever;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Winch")
    float CableSpeed = 150;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Winch")
    float MaximumCableLength = 3000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Winch")
    float MotorAcceleration = 180;

    UPROPERTY(BlueprintReadOnly, Category="Winch")
    float CableLength = 0;

    UPROPERTY(BlueprintReadOnly, Category="Winch")
    float DrumSpeed = 0;

    UPROPERTY(BlueprintReadOnly, Category="Winch")
    float Tension = 0;

    UPROPERTY(BlueprintReadWrite, Category="Winch")
    float ExternalLoad = 0;

    UPROPERTY(BlueprintReadWrite, Category="Winch")
    float TensionSetting = 0.5f;

    UPROPERTY(BlueprintReadWrite, Category="Winch")
    bool bMotorEnabled = false;

    UPROPERTY(BlueprintReadWrite, Category="Winch")
    bool bEmergencyStopped = false;

    UPROPERTY(BlueprintReadWrite, Category="Winch")
    bool bFaulted = false;

protected:
    virtual void Tick(float DeltaSeconds) override;
};
