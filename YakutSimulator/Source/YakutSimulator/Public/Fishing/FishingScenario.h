#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Fishing/FishingTypes.h"
#include "FishingScenario.generated.h"

class AFishingItem;
class USplineComponent;
class UStaticMeshComponent;
class UFishingInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFishingStageChanged, EFishingStage, Stage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFishingFeedback, const FText&, Message);

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishingScenario : public AActor
{
    GENERATED_BODY()

public:
    AFishingScenario();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USplineComponent> ConveyorPath;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> RecoveryPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> ProcessingPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> ProcessingSupplyPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> KnifeHome;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> FinishedPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> DeckPlayerPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> SortingPlayerPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> ProcessingPlayerPoint;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<UStaticMeshComponent> CutGuide;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing|Layout") TObjectPtr<USceneComponent> NetPosition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings", meta=(ClampMin="2", ClampMax="32")) int32 FishCount = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings", meta=(ClampMin="1")) float DeploySeconds = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings", meta=(ClampMin="0.1")) float TrawlSeconds = 4.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings", meta=(ClampMin="1")) float HaulSeconds = 20.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings", meta=(ClampMin="1")) float ConveyorSpeed = 12.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings", meta=(ClampMin="0.5")) float FeedInterval = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings", meta=(ClampMin="1", ClampMax="8")) float CutTolerance = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings") bool bAttachVRInput = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings") TSubclassOf<AFishingItem> FishClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Settings") TSubclassOf<AFishingItem> KnifeClass;

    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") EFishingStage Stage = EFishingStage::Safety;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") FFishingResults Results;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") bool bSafetyEquipped = false;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") bool bStopTested = false;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") bool bEmergencyStopped = false;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") bool bStoppedByOverload = false;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") float WinchInput = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") float NetDepth = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") float WinchLoad = 0.f;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") FText LastFeedback;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") TArray<TObjectPtr<AFishingItem>> Catch;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") TObjectPtr<AFishingItem> Knife;
    UPROPERTY(BlueprintReadOnly, Category="Fishing|State") TObjectPtr<AFishingItem> BoardFish;
    UPROPERTY(BlueprintAssignable, Category="Fishing|Events") FFishingStageChanged OnStageChanged;
    UPROPERTY(BlueprintAssignable, Category="Fishing|Events") FFishingFeedback OnFeedback;

    UFUNCTION(BlueprintCallable, Category="Fishing") void RestartShift();
    UFUNCTION(BlueprintCallable, Category="Fishing") bool EquipSafety();
    UFUNCTION(BlueprintCallable, Category="Fishing") void EmergencyStop();
    UFUNCTION(BlueprintCallable, Category="Fishing") bool StartOrResume();
    UFUNCTION(BlueprintCallable, Category="Fishing") void SetWinchInput(float Value);
    UFUNCTION(BlueprintCallable, Category="Fishing") bool BeginSorting();
    UFUNCTION(BlueprintCallable, Category="Fishing") bool BeginProcessing();
    UFUNCTION(BlueprintCallable, Category="Fishing") bool TrySortFish(AFishingItem* Fish, EFishSpecies BinSpecies);
    UFUNCTION(BlueprintCallable, Category="Fishing") bool TryLoadFish(AFishingItem* Fish);
    UFUNCTION(BlueprintCallable, Category="Fishing") bool TryDeliverFish(AFishingItem* Fish);
    UFUNCTION(BlueprintCallable, Category="Fishing") void RecoverItem(AFishingItem* Item, bool bMissed = false);
    UFUNCTION(BlueprintCallable, Category="Fishing") bool TryPlaceReleasedItem(AFishingItem* Item);
    UFUNCTION(BlueprintPure, Category="Fishing") FText GetStatusText() const;
    UFUNCTION(BlueprintPure, Category="Fishing") FText GetWinchStatusText() const;
    UFUNCTION(BlueprintCallable, Category="Fishing") void AdvanceKnifeStroke(const FVector& From, const FVector& To);

private:
    UPROPERTY(Transient) TObjectPtr<UFishingInteractionComponent> VRInteraction;
    float StageSeconds = 0.f;
    float FeedSeconds = 0.f;
    float OverloadSeconds = 0.f;
    static constexpr float OverloadStopDelay = 1.5f;
    int32 NextFishToFeed = 0;
    FVector PreviousKnifeTip = FVector::ZeroVector;
    FVector RaisedNetLocation = FVector(-700, 150, 400);
    bool bHasPreviousKnifeTip = false;
    FString LastCuttingReadiness;
    FTimerHandle TravelTimer;

    void SetStage(EFishingStage NewStage);
    void Feedback(const TCHAR* Message);
    void MovePlayer(USceneComponent* Destination);
    void UpdateWinch(float DeltaSeconds);
    void StopWinch(bool bOverload);
    void UpdateConveyor(float DeltaSeconds);
    void UpdateCutting();
    bool OwnsFish(const AFishingItem* Fish) const;
    void PresentNextFish();
};
