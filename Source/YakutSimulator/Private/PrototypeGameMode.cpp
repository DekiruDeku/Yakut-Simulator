#include "PrototypeGameMode.h"

#include "PrototypePlayerController.h"
#include "VirtualRealityPawn.h"
#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "ScenarioDirector.h"
#include "GameMapsSettings.h"

APrototypeGameMode::APrototypeGameMode()
{
    DefaultPawnClass = AVirtualRealityPawn::StaticClass();
    PlayerControllerClass = APrototypePlayerController::StaticClass();
    MainMenuMap = TSoftObjectPtr<UWorld>(FSoftObjectPath(UGameMapsSettings::GetGameDefaultMap()));

    FScenarioMapEntry FishingMap;
    FishingMap.DisplayName = FText::FromString(TEXT("Рыболовный траулер"));
    FishingMap.Map = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Prototype/Maps/L_Fishing.L_Fishing")));
    ScenarioMaps.Add(FishingMap);

    FScenarioMapEntry ClinicMap;
    ClinicMap.DisplayName = FText::FromString(TEXT("Кабинет терапевта"));
    ClinicMap.Map = TSoftObjectPtr<UWorld>(FSoftObjectPath(TEXT("/Game/Prototype/Maps/L_Clinic.L_Clinic")));
    ScenarioMaps.Add(ClinicMap);
}

AScenarioDirector* APrototypeGameMode::FindScenarioDirector() const
{
    AScenarioDirector* SelectedDirector = nullptr;
    for (TActorIterator<AScenarioDirector> DirectorIterator(GetWorld()); DirectorIterator; ++DirectorIterator)
    {
        if (DirectorIterator->Steps.IsEmpty())
        {
            continue;
        }
        if (SelectedDirector)
        {
            UKismetSystemLibrary::PrintString(this, TEXT("Multiple configured scenario directors found; scenario UI requires a unique director"), true, true, FLinearColor::Red);
            return nullptr;
        }
        SelectedDirector = *DirectorIterator;
    }
    return SelectedDirector;
}
