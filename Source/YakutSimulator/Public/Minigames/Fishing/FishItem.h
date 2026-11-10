#pragma once

#include "CoreMinimal.h"
#include "Base/WorkItem.h"
#include "Minigames/Fishing/FishingTypes.h"
#include "FishItem.generated.h"

class AScenarioDirector;
class AKnifeItem;
class USphereComponent;
class UStaticMeshComponent;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cutting")
    TObjectPtr<UStaticMeshComponent> CutGuide;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cutting")
    TObjectPtr<UStaticMeshComponent> CutStartMarker;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cutting")
    TObjectPtr<UStaticMeshComponent> CutEndMarker;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    EFishProductState ProductState = EFishProductState::Whole;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    bool bSorted = false;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    bool bOnProcessingBench = false;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    float CuttingProgress = 0;

    UPROPERTY(BlueprintReadOnly, Category="Cutting")
    int32 CurrentCutIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category="Cutting")
    float CurrentCutProgress = 0;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    bool bWashed = false;

    UPROPERTY(BlueprintReadOnly, Category="Fish")
    float WashingProgress = 0;

    UFUNCTION(BlueprintCallable, Category="Fish")
    void InitializeFish(UFishSpeciesDefinition* Definition, AScenarioDirector* FishingScenario);

    bool ApplyKnifeMovement(AKnifeItem* Knife, const FVector& BladePosition);
    void ResetKnifeContact(AKnifeItem* Knife);

    UFUNCTION(BlueprintPure, Category="Cutting")
    FText GetCutInstruction() const;

    virtual bool CanBeGrabbed() const override;
    virtual bool TryGrab(USceneComponent* Hand) override;

protected:
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY()
    TObjectPtr<AKnifeItem> CuttingKnife;

    FVector PreviousBladePosition;
    bool bCutStrokeStarted = false;
};
