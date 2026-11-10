#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkTypes.h"
#include "FishingStations.generated.h"

class AWorkItem;
class AFishItem;
class AFishContainerItem;
class AScenarioDirector;
class AKnifeItem;
class APhysicalLever;
class UFishSpeciesDefinition;
class UInteractionZoneComponent;
class UStaticMeshComponent;

UCLASS(Abstract)
class YAKUTSIMULATOR_API AFishingStation : public AActor
{
    GENERATED_BODY()

public:
    AFishingStation();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station")
    TObjectPtr<UStaticMeshComponent> Surface;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station")
    TObjectPtr<UInteractionZoneComponent> Intake;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Station")
    TObjectPtr<AScenarioDirector> Scenario;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    virtual bool CanReceiveItem(AWorkItem* Item);

    UFUNCTION()
    virtual void ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone);
};

UCLASS()
class YAKUTSIMULATOR_API AFishSortingConveyor : public AFishingStation
{
    GENERATED_BODY()

public:
    AFishSortingConveyor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sorting")
    TObjectPtr<UFishSpeciesDefinition> AcceptedSpecies;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Sorting")
    TObjectPtr<APhysicalLever> PowerLever;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sorting")
    float BeltSpeed = 35;

    UPROPERTY(BlueprintReadOnly, Category="Sorting")
    bool bFaulted = false;

    UFUNCTION(BlueprintCallable, Category="Sorting")
    void SetFaulted(bool bFault);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool CanReceiveItem(AWorkItem* Item) override;
    virtual void ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone) override;

private:
    UPROPERTY()
    TArray<TObjectPtr<AFishItem>> BeltFish;

    UFUNCTION()
    void TryRepair(AWorkItem* Item, EWorkAction Action);
};

UCLASS()
class YAKUTSIMULATOR_API AFishProcessingBench : public AFishingStation
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="Processing")
    TObjectPtr<AFishItem> CurrentFish;

protected:
    virtual bool CanReceiveItem(AWorkItem* Item) override;
    virtual void ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone) override;
};

UCLASS()
class YAKUTSIMULATOR_API AFishFreezer : public AFishingStation
{
    GENERATED_BODY()

public:
    AFishFreezer();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Freezer")
    float FreezingSeconds = 3;

    UPROPERTY(BlueprintReadOnly, Category="Freezer")
    TObjectPtr<AFishContainerItem> CurrentContainer;

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual bool CanReceiveItem(AWorkItem* Item) override;
    virtual void ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone) override;

private:
    float RemainingSeconds = 0;
};

UCLASS()
class YAKUTSIMULATOR_API AKnifeSharpeningStation : public AFishingStation
{
    GENERATED_BODY()

protected:
    virtual bool CanReceiveItem(AWorkItem* Item) override;
};
