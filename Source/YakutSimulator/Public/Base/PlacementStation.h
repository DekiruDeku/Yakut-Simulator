#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PlacementStation.generated.h"

class UInteractionZoneComponent;
class UStaticMeshComponent;

UCLASS()
class YAKUTSIMULATOR_API APlacementStation : public AActor
{
    GENERATED_BODY()

public:
    APlacementStation();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Placement")
    TObjectPtr<UStaticMeshComponent> Surface;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Placement")
    TObjectPtr<UInteractionZoneComponent> AcceptanceZone;
};
