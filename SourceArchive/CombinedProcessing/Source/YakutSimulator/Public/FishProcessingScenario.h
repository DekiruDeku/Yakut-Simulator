#pragma once

#include "CoreMinimal.h"
#include "WorkstationScenario.h"
#include "FishingStations.h"
#include "FishProcessingScenario.generated.h"

class AFishItem;
class UFishSpeciesDefinition;
class UBoxComponent;
class UWorkOperationComponent;
class AKnifeItem;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishReceivingBin : public AFishingStation
{
    GENERATED_BODY()

public:
    AFishReceivingBin();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Receiving")
    bool bReceiveWaste = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Receiving")
    TObjectPtr<UFishSpeciesDefinition> AcceptedSpecies;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Receiving", meta=(ClampMin="1"))
    int32 Capacity = 3;

    UPROPERTY(BlueprintReadOnly, Category="Receiving")
    TArray<TObjectPtr<AWorkItem>> Contents;

    UFUNCTION(BlueprintPure, Category="Receiving")
    bool IsFull() const;

    UFUNCTION(BlueprintCallable, Category="Receiving")
    void ReplaceContainer();

protected:
    virtual bool CanReceiveItem(AWorkItem* Item) override;
    virtual void ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone) override;
};

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishProcessingScenario : public AWorkstationScenario
{
    GENERATED_BODY()

public:
    AFishProcessingScenario();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conveyor")
    TObjectPtr<UBoxComponent> ConveyorZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conveyor")
    TObjectPtr<USceneComponent> FeedPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conveyor")
    TObjectPtr<UStaticMeshComponent> ConveyorSurface;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Conveyor")
    TObjectPtr<UStaticMeshComponent> WaterStream;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> ConveyorControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> FeedControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> WaterControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<AFishProcessingBench> ProcessingBench;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<AKnifeItem> ProcessingKnife;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TArray<TObjectPtr<AFishReceivingBin>> ReceivingBins;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production")
    TSubclassOf<AFishItem> FishItemClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production")
    TSubclassOf<AWorkItem> DebrisItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production")
    TArray<TObjectPtr<UFishSpeciesDefinition>> FishSpecies;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="1"))
    int32 BatchFishCount = 6;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="0"))
    int32 BatchDebrisCount = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production")
    float MaximumBeltSpeed = 24;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="1"))
    float FeedInterval = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="0.1"))
    float WashingSeconds = 2;

    UPROPERTY(BlueprintReadOnly, Category="Production")
    bool bProductionStarted = false;

    UPROPERTY(BlueprintReadOnly, Category="Production")
    float CurrentBeltSpeed = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> StartOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> SortOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> DebrisOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> ProcessOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> WashOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> ProductOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> WasteOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> ContainerOperation;

    virtual FText GetReadout() const override;
    virtual void ConfirmCommand() override;
    virtual void RestartWorkstation() override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    int32 SpawnedFish = 0;
    int32 SpawnedDebris = 0;
    float FeedAccumulator = 0;
    bool bNextFeedIsDebris = false;
    void FeedItem();
};
