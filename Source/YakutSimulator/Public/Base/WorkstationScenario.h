#pragma once

#include "CoreMinimal.h"
#include "Base/ScenarioDirector.h"
#include "WorkstationScenario.generated.h"

class APhysicalLever;
class UWidgetComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AWorkstationScenario : public AScenarioDirector
{
    GENERATED_BODY()

public:
    AWorkstationScenario();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Workstation")
    TObjectPtr<UWidgetComponent> ConsoleDisplay;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Workstation")
    TObjectPtr<APhysicalLever> CommandControl;

    UFUNCTION(BlueprintPure, Category="Workstation")
    virtual FText GetReadout() const;

    UFUNCTION(BlueprintCallable, Category="Workstation")
    virtual void ConfirmCommand();

    UFUNCTION(BlueprintCallable, Category="Workstation")
    virtual void RestartWorkstation();

    UFUNCTION(BlueprintImplementableEvent, Category="Workstation")
    void InitializeConsole(APlayerController* Player);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    float DisplayAccumulator = 0;

    UFUNCTION()
    void ReceiveCommand(AWorkItem* Item, EWorkAction Action);
};
