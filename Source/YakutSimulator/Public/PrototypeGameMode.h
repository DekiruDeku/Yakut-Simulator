#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WorkTypes.h"
#include "PrototypeGameMode.generated.h"

UCLASS()
class YAKUTSIMULATOR_API APrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APrototypeGameMode();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maps")
    TArray<FScenarioMapEntry> ScenarioMaps;
};
