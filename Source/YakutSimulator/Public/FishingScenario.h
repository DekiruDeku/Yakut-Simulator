#pragma once

#include "CoreMinimal.h"
#include "ScenarioDirector.h"
#include "FishingScenario.generated.h"

class AFishingSonar;
class AFishingWinch;
class ATrawl;
class ATrawlerVessel;
class UWidgetComponent;
class AFishContainerItem;
class AKnifeItem;
class AFishProcessingBench;
class AKnifeSharpeningStation;
class AFishFreezer;
class AFishSortingConveyor;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishingScenario : public AScenarioDirector
{
    GENERATED_BODY()

public:
    AFishingScenario();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<UWidgetComponent> ConsoleDisplay;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<AFishingSonar> Sonar;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<ATrawl> Trawl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<AFishingWinch> Winch;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Fishing")
    TObjectPtr<ATrawlerVessel> Vessel;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Processing")
    TObjectPtr<AKnifeItem> Knife;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Processing")
    TObjectPtr<AFishProcessingBench> ProcessingBench;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Processing")
    TObjectPtr<AKnifeSharpeningStation> SharpeningStone;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Processing")
    TObjectPtr<AFishContainerItem> PackingContainer;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Processing")
    TObjectPtr<AFishFreezer> Freezer;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Processing")
    TArray<TObjectPtr<AFishSortingConveyor>> SortingConveyors;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> SearchOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> DeployOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> CatchOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> RecoverOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> UnloadOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> SortOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> PrepareKnifeOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> ProcessOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> PackOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> FreezeOperation;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    bool bTrawlDeployed = false;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    bool bFishCaught = false;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    bool bTrawlRecovered = false;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    int32 UnloadedFish = 0;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    int32 SortedFish = 0;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    bool bKnifePrepared = false;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    int32 ProcessedFish = 0;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    int32 PackedFish = 0;

    UPROPERTY(BlueprintReadOnly, Category="Results")
    int32 FrozenFish = 0;

    UFUNCTION(BlueprintPure, Category="Fishing")
    FText GetConsoleReadout() const;

    UFUNCTION(BlueprintCallable, Category="Fishing")
    void SetStatus(const FText& Message);

    UFUNCTION(BlueprintImplementableEvent, Category="Fishing")
    void InitializeConsole(APlayerController* Player);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    float ReadoutAccumulator = 0;
    bool UpdateOperation(UWorkOperationComponent* Operation, int32 Total);
    void AddFishingStep(FName Identifier, const FText& Instruction, UWorkOperationComponent* Operation, FName Dependency = NAME_None);
};
