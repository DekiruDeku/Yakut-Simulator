#pragma once

#include "CoreMinimal.h"
#include "Base/WorkItem.h"
#include "KnifeItem.generated.h"

class AFishItem;

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

protected:
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<AFishItem> ContactFish;
};
