#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishingTypes.h"
#include "FishSchool.generated.h"

class UInstancedStaticMeshComponent;

UCLASS()
class YAKUTSIMULATOR_API AFishSchool : public AActor
{
    GENERATED_BODY()

public:
    AFishSchool();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="School")
    TObjectPtr<UInstancedStaticMeshComponent> FishVisuals;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School")
    TObjectPtr<UFishSpeciesDefinition> Species;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School")
    float Radius = 1200;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School")
    int32 RemainingFish = 60;

    UFUNCTION(BlueprintCallable, Category="School")
    int32 ConsumeFish(int32 RequestedCount);

protected:
    virtual void BeginPlay() override;
};
