#pragma once

#include "CoreMinimal.h"
#include "Minigames/Fishing/FishingStations.h"
#include "FishReceivingBin.generated.h"

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AFishReceivingBin : public AFishingStation
{
    GENERATED_BODY()

public:
    AFishReceivingBin();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Receiving", meta=(ClampMin="1"))
    int32 Capacity = 6;

    UPROPERTY(BlueprintReadOnly, Category="Receiving")
    TArray<TObjectPtr<AWorkItem>> Contents;

protected:
    virtual bool CanReceiveItem(AWorkItem* Item) override;
    virtual void ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone) override;
};
