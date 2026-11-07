#include "PrototypeGameMode.h"

#include "PrototypePlayerController.h"
#include "VirtualRealityPawn.h"

APrototypeGameMode::APrototypeGameMode()
{
    DefaultPawnClass = AVirtualRealityPawn::StaticClass();
    PlayerControllerClass = APrototypePlayerController::StaticClass();

    FScenarioMapEntry FishingMap;
    FishingMap.DisplayName = FText::FromString(TEXT("Рыболовный траулер"));
    FishingMap.Map = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Prototype/Maps/L_Fishing.L_Fishing")));
    ScenarioMaps.Add(FishingMap);

    FScenarioMapEntry ClinicMap;
    ClinicMap.DisplayName = FText::FromString(TEXT("Кабинет терапевта"));
    ClinicMap.Map = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Prototype/Maps/L_Clinic.L_Clinic")));
    ScenarioMaps.Add(ClinicMap);
}
