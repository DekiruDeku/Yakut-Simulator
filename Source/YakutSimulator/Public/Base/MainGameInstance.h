#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Base/WorkTypes.h"
#include "MainGameInstance.generated.h"

UCLASS()
class YAKUTSIMULATOR_API UMainGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category="Settings")
    float MasterVolume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maps")
    TArray<FScenarioMapEntry> ScenarioMaps;
};
