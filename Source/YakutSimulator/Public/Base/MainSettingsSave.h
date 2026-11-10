#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MainSettingsSave.generated.h"

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API UMainSettingsSave : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category="Player Settings", meta=(ClampMin="-100", ClampMax="100", Units="cm"))
    float CameraHeightOffset = 0.0f;
};
