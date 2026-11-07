#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Fishing/FishingTypes.h"
#include "FishingStation.generated.h"

class AFishingItem;
class AFishingScenario;
class UBoxComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishingStation : public AActor
{
    GENERATED_BODY()

public:
    AFishingStation();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UBoxComponent> Zone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UStaticMeshComponent> Handle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Fishing") TObjectPtr<UTextRenderComponent> Label;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Fishing") TObjectPtr<AFishingScenario> Scenario;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing") EFishingStationKind Kind = EFishingStationKind::StartOrResume;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing") EFishSpecies Species = EFishSpecies::A;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="5")) float LeverTravel = 20.f;

    UFUNCTION(BlueprintCallable, Category="Fishing") bool Use();
    UFUNCTION(BlueprintCallable, Category="Fishing") bool AcceptItem(AFishingItem* Item);
    UFUNCTION(BlueprintPure, Category="Fishing") bool ContainsItem(const AFishingItem* Item) const;
    bool IsButton() const;
    bool BeginLever(USceneComponent* Hand);
    void EndLever(USceneComponent* Hand);
    USceneComponent* GetLeverHand() const { return LeverHand.Get(); }

private:
    TWeakObjectPtr<USceneComponent> LeverHand;
    FVector LeverGrabLocation = FVector::ZeroVector;
    FTransform HandleHomeTransform = FTransform::Identity;
    UFUNCTION() void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
    FText GetLabelText() const;
    FColor GetLabelColor() const;
    void UpdateLabel();
};
