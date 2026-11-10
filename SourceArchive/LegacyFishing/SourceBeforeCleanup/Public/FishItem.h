#pragma once

#include "CoreMinimal.h"
#include "WorkItem.h"
#include "FishingTypes.h"
#include "FishItem.generated.h"

class AScenarioDirector;
class AKnifeItem;
class USphereComponent;
class UInteractionZoneComponent;

UCLASS()
class YAKUTSIMULATOR_API AFishItem : public AWorkItem
{
    GENERATED_BODY()

public:
    AFishItem();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fish")
    TObjectPtr<UFishSpeciesDefinition> Species;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    TObjectPtr<AScenarioDirector> Scenario;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fish")
    TObjectPtr<USphereComponent> CuttingTarget;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    EFishProductState ProductState = EFishProductState::Whole;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    bool bSorted = false;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    bool bOnProcessingBench = false;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    float CuttingProgress = 0;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    bool bWashed = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fish")
    bool bDefective = false;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    float WashingProgress = 0;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Fish")
    TSubclassOf<AWorkItem> WasteItemClass;

    UFUNCTION(BlueprintCallable, Category="Fish")
    void InitializeFish(UFishSpeciesDefinition* Definition, AScenarioDirector* FishingScenario);

    bool ApplyKnifeMovement(AKnifeItem* Knife, const FVector& BladePosition);
    virtual bool CanBeGrabbed() const override;
    virtual bool TryGrab(USceneComponent* Hand) override;

private:
    UPROPERTY()
    TObjectPtr<AKnifeItem> CuttingKnife;

    FVector PreviousBladePosition;
};

UCLASS()
class YAKUTSIMULATOR_API AFishContainerItem : public AWorkItem
{
    GENERATED_BODY()

public:
    AFishContainerItem();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Container")
    TObjectPtr<UInteractionZoneComponent> Intake;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Container")
    int32 Capacity = 6;

    UPROPERTY(BlueprintReadOnly, Category="Container")
    TArray<TObjectPtr<AFishItem>> Contents;

    UPROPERTY(BlueprintReadOnly, Category="Container")
    bool bFreezing = false;

    UPROPERTY(BlueprintReadOnly, Category="Container")
    bool bFrozen = false;

    virtual bool CanBeGrabbed() const override;
    void FreezeContents();

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    bool CanReceiveFish(AWorkItem* Item);

    UFUNCTION()
    void ReceiveFish(AWorkItem* Item, UInteractionZoneComponent* Zone);
};
