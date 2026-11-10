#pragma once

#include "CoreMinimal.h"
#include "Base/WorkstationScenario.h"
#include "FishCuttingScenario.generated.h"

class AFishItem;
class AFishProcessingBench;
class AFishReceivingBin;
class AKnifeItem;
class UFishSpeciesDefinition;
class UWorkOperationComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishCuttingScenario : public AWorkstationScenario
{
    GENERATED_BODY()

public:
    AFishCuttingScenario();

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<AFishProcessingBench> CuttingBench;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<AKnifeItem> CuttingKnife;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<AFishReceivingBin> ProductBin;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Production")
    TSubclassOf<AFishItem> FishItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production")
    TArray<TObjectPtr<UFishSpeciesDefinition>> FishSpecies;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Production", meta=(ClampMin="1"))
    int32 BatchFishCount = 6;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> StartOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> CutOperation;

    virtual FText GetReadout() const override;
    virtual void ConfirmCommand() override;
    virtual void RestartWorkstation() override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    int32 SpawnedFish = 0;
    bool bBatchStarted = false;
    void FeedFish();
};
