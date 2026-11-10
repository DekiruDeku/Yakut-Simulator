#include "Base/MainGameMode.h"

#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Base/ScenarioDirector.h"

AScenarioDirector* AMainGameMode::FindScenarioDirector() const
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
            UKismetSystemLibrary::PrintString(this, TEXT("Multiple configured scenario directors found; a unique director is required"), true, true, FLinearColor::Red);
            return nullptr;
        }
        SelectedDirector = *DirectorIterator;
    }
    return SelectedDirector;
}
