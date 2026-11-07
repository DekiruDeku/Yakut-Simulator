#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Fishing/FishingTypes.h"
#include "FishingItem.generated.h"

class AFishingScenario;
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishingItem : public AActor
{
    GENERATED_BODY()

public:
    AFishingItem();
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UBoxComponent> Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<USceneComponent> BladeTip;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing") EFishingItemKind Kind = EFishingItemKind::Fish;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing") EFishSpecies Species = EFishSpecies::A;
    UPROPERTY(BlueprintReadOnly, Category="Fishing") EFishingItemState State = EFishingItemState::Queued;
    UPROPERTY(BlueprintReadOnly, Category="Fishing") TObjectPtr<AFishingScenario> Scenario;
    UPROPERTY(BlueprintReadOnly, Category="Fishing") int32 BatchIndex = INDEX_NONE;
    UPROPERTY(BlueprintReadOnly, Category="Fishing") int32 NextCutPoint = 0;

    UFUNCTION(BlueprintCallable, Category="Fishing") bool TryGrab(USceneComponent* Hand);
    UFUNCTION(BlueprintCallable, Category="Fishing") void Release();
    UFUNCTION(BlueprintPure, Category="Fishing") bool IsHeld() const { return HoldingHand.IsValid(); }
    UFUNCTION(BlueprintPure, Category="Fishing") bool IsBraced() const;
    USceneComponent* GetHoldingHand() const { return HoldingHand.Get(); }
    TArray<FVector> GetCutPoints() const;
    void Park(const FTransform& Transform, EFishingItemState NewState);
    void RefreshAppearance();

    float ConveyorDistance = 0.f;
    bool bSortingMistakeRecorded = false;
    bool bMissRecorded = false;
    bool bSorted = false;

private:
    TWeakObjectPtr<USceneComponent> HoldingHand;
    bool bBracing = false;
    float LooseSeconds = 0.f;
};
