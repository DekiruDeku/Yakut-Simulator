#pragma once

#include "CoreMinimal.h"
#include "Base/WorkstationScenario.h"
#include "FishSortingScenario.generated.h"

class AFishItem;
class AFishSortingConveyor;
class UFishSpeciesDefinition;
class UBoxComponent;
class UStaticMeshComponent;
class UWorkOperationComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishSortingScenario : public AWorkstationScenario
{
    GENERATED_BODY()

public:
    AFishSortingScenario();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conveyor")
    TObjectPtr<UBoxComponent> ConveyorZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conveyor")
    TObjectPtr<USceneComponent> FeedPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conveyor")
    TObjectPtr<UStaticMeshComponent> ConveyorSurface;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Washing")
    TObjectPtr<UStaticMeshComponent> WaterStream;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> ConveyorControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> FeedControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> WaterControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TArray<TObjectPtr<AFishSortingConveyor>> SortingTrays;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production")
    TSubclassOf<AFishItem> FishItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production")
    TArray<TObjectPtr<UFishSpeciesDefinition>> FishSpecies;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="1"))
    int32 BatchFishCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="0"))
    float MaximumBeltSpeed = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="1"))
    float FeedInterval = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Washing", meta=(ClampMin="0.1"))
    float WashingSeconds = 1.5f;

    UPROPERTY(BlueprintReadOnly, Category="Production")
    bool bProductionStarted = false;

    UPROPERTY(BlueprintReadOnly, Category="Production")
    float CurrentBeltSpeed = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> StartOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> WashOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> SortOperation;

    virtual FText GetReadout() const override;
    virtual void ConfirmCommand() override;
    virtual void RestartWorkstation() override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    int32 SpawnedFish = 0;
    float FeedAccumulator = 0;
    void FeedFish();
};
