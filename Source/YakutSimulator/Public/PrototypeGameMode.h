#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WorkTypes.h"
#include "PrototypeGameMode.generated.h"

class AScenarioDirector;

UCLASS()
class YAKUTSIMULATOR_API APrototypeGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    APrototypeGameMode();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maps")
    TSoftObjectPtr<UWorld> MainMenuMap;

    UFUNCTION(BlueprintPure, Category="Scenario")
    AScenarioDirector* FindScenarioDirector() const;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maps")
    TArray<FScenarioMapEntry> ScenarioMaps;
};
