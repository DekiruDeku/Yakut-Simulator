#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainGameMode.generated.h"

class AScenarioDirector;

UCLASS()
class YAKUTSIMULATOR_API AMainGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category="Scenario")
    AScenarioDirector* FindScenarioDirector() const;
};
