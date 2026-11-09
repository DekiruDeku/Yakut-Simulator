#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkTypes.h"
#include "FishingWinch.generated.h"

class APhysicalLever;
class AWorkItem;
class ATrawl;
class UStaticMeshComponent;

UCLASS()
class YAKUTSIMULATOR_API AFishingWinch : public AActor
{
    GENERATED_BODY()

public:
    AFishingWinch();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Winch")
    TObjectPtr<UStaticMeshComponent> Foundation;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Winch")
    TObjectPtr<UStaticMeshComponent> Drum;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Winch")
    TObjectPtr<APhysicalLever> ControlLever;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Winch")
    TObjectPtr<APhysicalLever> UnloadLever;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Winch")
    TObjectPtr<ATrawl> Trawl;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Winch")
    float CableSpeed = 250;

    UPROPERTY(BlueprintReadOnly, Category="Winch")
    bool bFaulted = false;

    UFUNCTION(BlueprintCallable, Category="Winch")
    void SetFaulted(bool bFault);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UFUNCTION()
    void TryUnload(float Value);

    UFUNCTION()
    void TryRepair(AWorkItem* Item, EWorkAction Action);
};
