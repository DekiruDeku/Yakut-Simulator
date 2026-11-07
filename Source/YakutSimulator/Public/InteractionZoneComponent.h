#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "InteractionZoneComponent.generated.h"

class AWorkItem;

UCLASS(ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UInteractionZoneComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UInteractionZoneComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement")
    FName AcceptedItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement")
    bool bSnapOnPlacement = true;

    UFUNCTION(BlueprintPure, Category="Placement")
    bool CanAcceptItem(AWorkItem* Item) const;

    UFUNCTION(BlueprintCallable, Category="Placement")
    bool TryAcceptItem(AWorkItem* Item);
};
