#pragma once

#include "CoreMinimal.h"
#include "Base/WorkstationScenario.h"
#include "TrawlWinchScenario.generated.h"

class AFishingWinch;
class UWorkOperationComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class EWinchCycleStage : uint8
{
    Preparation,
    PayingOut,
    WorkingDepth,
    Towing,
    Hauling,
    Securing,
    Completed
};

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API ATrawlWinchScenario : public AWorkstationScenario
{
    GENERATED_BODY()

public:
    ATrawlWinchScenario();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<UStaticMeshComponent> TrawlVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<UStaticMeshComponent> PortCableVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<UStaticMeshComponent> StarboardCableVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<USceneComponent> PortFairlead;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<USceneComponent> StarboardFairlead;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<AFishingWinch> PortWinch;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<AFishingWinch> StarboardWinch;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> PortTensionControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> StarboardTensionControl;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Equipment")
    TObjectPtr<APhysicalLever> EmergencyControl;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle")
    float DeploymentLength = 700;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle")
    float WorkingLength = 1800;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle")
    float LengthTolerance = 80;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle")
    float MaximumLengthDifference = 150;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle")
    float MaximumTension = 85;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle")
    float MaximumTensionDifference = 28;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle")
    float TowingSeconds = 25;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle", meta=(ClampMin="0"))
    float LoadSurge = 30;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cycle", meta=(ClampMin="1"))
    float LoadVariationPeriod = 18;

    UPROPERTY(BlueprintReadOnly, Category="Cycle")
    EWinchCycleStage Stage = EWinchCycleStage::Preparation;

    UPROPERTY(BlueprintReadOnly, Category="Cycle")
    bool bEmergencyLatched = false;

    UPROPERTY(BlueprintReadOnly, Category="Cycle")
    float StableTowingSeconds = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> PreparationOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> DeploymentOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> DepthOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> TowingOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> HaulingOperation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Operations")
    TObjectPtr<UWorkOperationComponent> SecuringOperation;

    virtual FText GetReadout() const override;
    virtual void ConfirmCommand() override;
    virtual void RestartWorkstation() override;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    float StableSeconds = 0;
    float OverloadSeconds = 0;
    float CycleSeconds = 0;
    bool AreControlsNeutral() const;
    bool IsLoadSafe() const;
    bool IsAtLength(float Length) const;
    void EmergencyStop();

    UFUNCTION()
    void ReceiveEmergency(AWorkItem* Item, EWorkAction Action);
};
