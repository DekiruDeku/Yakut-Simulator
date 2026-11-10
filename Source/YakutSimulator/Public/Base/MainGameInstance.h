#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Base/WorkTypes.h"
#include "MainGameInstance.generated.h"

class UMainSettingsSave;

UCLASS()
class YAKUTSIMULATOR_API UMainGameInstance : public UGameInstance
{
    GENERATED_BODY()

public:
    virtual void Init() override;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Settings")
    TSubclassOf<UMainSettingsSave> SettingsSaveClass;

    UPROPERTY(BlueprintReadOnly, Category="Settings")
    TObjectPtr<UMainSettingsSave> SettingsSave;

    UFUNCTION(BlueprintCallable, Category="Settings")
    bool SetCameraHeightOffset(float OffsetCentimeters);

    UPROPERTY(BlueprintReadWrite, Category="Settings")
    float MasterVolume = 1.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Maps")
    TArray<FScenarioMapEntry> ScenarioMaps;
};
