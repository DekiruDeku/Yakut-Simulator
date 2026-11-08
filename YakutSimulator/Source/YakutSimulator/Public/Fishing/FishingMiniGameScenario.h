#pragma once
#include "CoreMinimal.h"
#include "Common/MiniGameScenario.h"
#include "Fishing/FishingDefinition.h"
#include "FishingMiniGameScenario.generated.h"

class UFishingWinchComponent;
class UFishingConveyorComponent;
class UFishingProcessingComponent;
class AMiniGameStation;
class UFishSpeciesDefinition;
class USplineComponent;
class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FFishingBinBinding
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<AMiniGameStation> Station;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<UFishSpeciesDefinition> Species;
};

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishingMiniGameScenario : public AMiniGameScenario
{
    GENERATED_BODY()
public:
    AFishingMiniGameScenario();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UFishingWinchComponent> Winch;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UFishingConveyorComponent> Conveyor;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UFishingProcessingComponent> Processing;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USplineComponent> ConveyorPath;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> RecoveryPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> ProcessingPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> ProcessingSupplyPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> KnifeHome;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> FinishedPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> DeckPlayerPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> SortingPlayerPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> ProcessingPlayerPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<USceneComponent> NetPosition;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Layout") TObjectPtr<UStaticMeshComponent> CutGuide;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Bindings") TObjectPtr<AMiniGameStation> WinchStation;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Bindings") TArray<FFishingBinBinding> SortingBins;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Bindings") TObjectPtr<AMiniGameStation> ProcessingStation;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Bindings") TObjectPtr<AMiniGameStation> DeliveryStation;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fishing") EFishingOperation Operation = EFishingOperation::None;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fishing") bool bEquipped = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fishing") bool bStopTested = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fishing") int32 Overloads = 0;
    UFUNCTION(BlueprintPure, Category="Fishing") UFishingDefinition* FishingDefinition() const;
    UFUNCTION(BlueprintPure, Category="Fishing") float Score() const;
    virtual void BeginPlay() override;
    virtual bool ValidateScene(TArray<FText>& Errors) const override;
    virtual bool IsActionAllowed(FName Action) const override;
    virtual bool ExecuteAction_Implementation(FName Action, AActor* Source) override;
    virtual bool TryReceive_Implementation(UItemReceiverComponent* Receiver, AGrabbableItem* Item) override;
    virtual void RecoverItem_Implementation(AGrabbableItem* Item) override;
    virtual FMiniGamePresentation GetPresentation_Implementation(FName Panel, AActor* Source) const override;
    virtual void OnRunReset_Implementation() override;
    virtual void OnObjectiveEntered_Implementation(FName Id) override;
    virtual void OnObjectiveExited_Implementation(FName Id) override;
private:
    UFUNCTION() void OnMechanicFinished();
    UFUNCTION() void OnWinchStopped();
};
