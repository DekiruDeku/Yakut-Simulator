#pragma once
#include "CoreMinimal.h"
#include "Common/GrabbableItem.h"
#include "Fishing/FishingDefinition.h"
#include "FishingFish.generated.h"

class USplineComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishingFish : public AGrabbableItem
{
    GENERATED_BODY()
public:
    AFishingFish();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Processing") TObjectPtr<USplineComponent> CutPath;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Processing") TObjectPtr<UStaticMeshComponent> BraceIndicator;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fish") TObjectPtr<UFishSpeciesDefinition> Species;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fish") EFishWorkState State = EFishWorkState::Queued;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fish") int32 BatchIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fish") int32 NextCutPoint = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fish") bool bSorted = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fish") bool bSortingMistakeRecorded = false;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Fish") bool bMissRecorded = false;
    float ConveyorDistance = 0;
    void Place(const FTransform& Transform, EFishWorkState NewState);
    UFUNCTION(BlueprintPure, Category="Processing") bool IsBraced() const;
    TArray<FVector> GetCutPoints() const;
    UFUNCTION(BlueprintImplementableEvent, Category="Fish") void OnWorkStateChanged(EFishWorkState NewState);
    virtual bool CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const override;
    virtual void InteractionEnded_Implementation(const FInteractionContext& Context) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ValidateItem(TArray<FText>& Errors) const override;
};
