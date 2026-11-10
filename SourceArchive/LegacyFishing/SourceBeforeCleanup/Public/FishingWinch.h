#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkTypes.h"
#include "FishingWinch.generated.h"

class APhysicalLever;
class AWorkItem;
class ATrawl;
class UStaticMeshComponent;

UCLASS()
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

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Winch")
    TObjectPtr<APhysicalLever> UnloadLever;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Winch")
    TObjectPtr<ATrawl> Trawl;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Winch")
    float CableSpeed = 250;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Independent Winch")
    bool bIndependentControl = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Independent Winch")
    float MaximumCableLength = 3000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Independent Winch")
    float MotorAcceleration = 180;

    UPROPERTY(BlueprintReadOnly, Category="Independent Winch")
    float CableLength = 0;

    UPROPERTY(BlueprintReadOnly, Category="Independent Winch")
    float DrumSpeed = 0;

    UPROPERTY(BlueprintReadOnly, Category="Independent Winch")
    float Tension = 0;

    UPROPERTY(BlueprintReadWrite, Category="Independent Winch")
    float ExternalLoad = 0;

    UPROPERTY(BlueprintReadWrite, Category="Independent Winch")
    float TensionSetting = 0.5f;

    UPROPERTY(BlueprintReadWrite, Category="Independent Winch")
    bool bMotorEnabled = false;

    UPROPERTY(BlueprintReadWrite, Category="Independent Winch")
    bool bEmergencyStopped = false;

    UPROPERTY(BlueprintReadOnly, Category="Winch")
    bool bFaulted = false;

    UFUNCTION(BlueprintCallable, Category="Winch")
    void SetFaulted(bool bFault);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UFUNCTION()
    void TryUnload(float Value);

    UFUNCTION()
    void TryRepair(AWorkItem* Item, EWorkAction Action);
};
