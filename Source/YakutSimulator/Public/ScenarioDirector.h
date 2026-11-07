#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkTypes.h"
#include "ScenarioDirector.generated.h"

class AWorkItem;
class UWorkOperationComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FScenarioChanged);

UCLASS()
class YAKUTSIMULATOR_API AScenarioDirector : public AActor
{
    GENERATED_BODY()

public:
    AScenarioDirector();

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

    UFUNCTION(BlueprintPure, Category="Scenario")
    int32 GetCompletedStepCount() const;

    UFUNCTION(BlueprintPure, Category="Scenario")
    FText GetCurrentInstruction() const;

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void ProcessItemAction(AWorkItem* Item, EWorkAction Action);

    UFUNCTION()
    void ProcessOperationChanged(UWorkOperationComponent* Operation);

    void RefreshAvailability();

    bool bRefreshing = false;
};
