#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishingTypes.h"
#include "Trawl.generated.h"

class ATrawlerVessel;
class AFishingScenario;
class UStaticMeshComponent;
class AFishItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTrawlChanged);

UCLASS()
class YAKUTSIMULATOR_API ATrawl : public AActor
{
    GENERATED_BODY()

public:
    ATrawl();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trawl")
    TObjectPtr<UStaticMeshComponent> Net;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trawl")
    TObjectPtr<UStaticMeshComponent> PortCable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Trawl")
    TObjectPtr<UStaticMeshComponent> StarboardCable;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Trawl")
    TObjectPtr<ATrawlerVessel> Vessel;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Trawl")
    TObjectPtr<AFishingScenario> Scenario;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Trawl")
    TSubclassOf<AFishItem> FishItemClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trawl")
    float MaximumCableLength = 2500;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trawl")
    float MinimumFishingLength = 800;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trawl")
    float WaterHeight = -100;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trawl")
    float FishingDepth = 300;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Trawl")
    int32 Capacity = 6;

    UPROPERTY(BlueprintReadOnly, Category="Trawl")
    float CableLength = 0;

    UPROPERTY(BlueprintReadOnly, Category="Trawl")
    ETrawlState State = ETrawlState::Stowed;

    UPROPERTY(BlueprintReadOnly, Category="Trawl")
    TArray<FFishCatchEntry> Catch;

    UPROPERTY(BlueprintAssignable, Category="Trawl")
    FTrawlChanged OnTrawlChanged;

    UFUNCTION(BlueprintPure, Category="Trawl")
    int32 GetCatchCount() const;

    UFUNCTION(BlueprintCallable, Category="Trawl")
    void MoveCable(float Distance);

    UFUNCTION(BlueprintCallable, Category="Trawl")
    bool UnloadCatch();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    float CatchAccumulator = 0;
    void UpdateCable(UStaticMeshComponent* Cable, const FVector& Start, const FVector& End);
    void CollectFish(const FVector& PreviousPosition);
};
