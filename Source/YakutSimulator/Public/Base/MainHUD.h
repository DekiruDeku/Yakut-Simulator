#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainHUD.generated.h"

class UWidgetComponent;

UCLASS()
class YAKUTSIMULATOR_API AMainHUD : public AHUD
{
    GENERATED_BODY()

public:
    AMainHUD();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Settings")
    TObjectPtr<UWidgetComponent> SettingsDisplay;
};
