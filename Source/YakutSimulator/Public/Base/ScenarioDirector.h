#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Base/WorkTypes.h"
#include "ScenarioDirector.generated.h"

class AWorkItem;
class UWorkOperationComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FScenarioChanged);

UCLASS()
class YAKUTSIMULATOR_API AScenarioDirector : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario")
    FText ScenarioTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scenario")
    TArray<FScenarioStepDefinition> Steps;

    UPROPERTY(BlueprintReadOnly, Category="Scenario")
    FText LastAction;

    UPROPERTY(BlueprintAssignable, Category="Scenario")
    FScenarioChanged OnScenarioChanged;

    UFUNCTION(BlueprintCallable, Category="Scenario")
    void RegisterItem(AWorkItem* Item);

    UFUNCTION(BlueprintCallable, Category="Scenario")
    void ReportResult(FName StepIdentifier, int32 Amount = 1);

    UFUNCTION(BlueprintPure, Category="Scenario")
    int32 GetRecordedResult(FName StepIdentifier) const;

    UFUNCTION(BlueprintCallable, Category="Scenario")
    void SetStatus(const FText& Message);

    UFUNCTION(BlueprintCallable, Category="Scenario")
    void RestartScenario();

    UFUNCTION(BlueprintPure, Category="Scenario")
    bool ValidateSteps(FText& Error) const;

    UFUNCTION(BlueprintPure, Category="Scenario")
    int32 GetCompletedStepCount() const;

    UFUNCTION(BlueprintPure, Category="Scenario")
    FText GetCurrentInstruction() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void AddOperationStep(FName Identifier, const FText& Instruction, UWorkOperationComponent* Operation, const TArray<FName>& Dependencies = {});

private:
    UFUNCTION()
    void ProcessItemAction(AWorkItem* Item, EWorkAction Action);

    UFUNCTION()
    void ProcessOperationChanged(UWorkOperationComponent* Operation);

    void RefreshAvailability();
    void RegisterSpawnedActor(AActor* Actor);

    bool bRefreshing = false;
    FDelegateHandle ActorSpawnSubscription;

    UPROPERTY(Transient)
    TMap<FName, int32> RecordedResults;
};
