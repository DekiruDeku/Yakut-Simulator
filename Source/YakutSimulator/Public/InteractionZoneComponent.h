#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "InteractionZoneComponent.generated.h"

class AWorkItem;
class UInteractionZoneComponent;

DECLARE_DYNAMIC_DELEGATE_RetVal_OneParam(bool, FWorkItemAcceptance, AWorkItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWorkItemAccepted, AWorkItem*, Item, UInteractionZoneComponent*, Zone);

UCLASS(Blueprintable, ClassGroup=(Interaction), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UInteractionZoneComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UInteractionZoneComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement")
    FName AcceptedItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement")
    bool bSnapOnPlacement = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement")
    bool bAcceptReleasedItemsOnOverlap = false;

    UPROPERTY()
    FWorkItemAcceptance AcceptanceRule;

    UPROPERTY(BlueprintAssignable, Category="Placement")
    FWorkItemAccepted OnItemAccepted;

    UFUNCTION(BlueprintPure, Category="Placement")
    bool CanAcceptItem(AWorkItem* Item) const;

    UFUNCTION(BlueprintCallable, Category="Placement")
    bool TryAcceptItem(AWorkItem* Item);

protected:
    virtual void BeginPlay() override;

private:
    bool bAcceptingItem = false;

    UFUNCTION()
    void ReceiveOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
