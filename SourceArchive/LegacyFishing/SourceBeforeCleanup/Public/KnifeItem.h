#pragma once

#include "CoreMinimal.h"
#include "WorkItem.h"
#include "KnifeItem.generated.h"

UCLASS()
class YAKUTSIMULATOR_API AKnifeItem : public AWorkItem
{
    GENERATED_BODY()

public:
    AKnifeItem();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Knife")
    TObjectPtr<USceneComponent> BladeTip;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Knife", meta=(ClampMin="0", ClampMax="1"))
    float Sharpness = 0.55f;

    UFUNCTION(BlueprintCallable, Category="Knife")
    void Sharpen(float TravelDistance);

protected:
    virtual void Tick(float DeltaSeconds) override;

private:
    FVector PreviousBladePosition;
    bool bBladeInitialized = false;
};
