#include "Fishing/FishingLog.h"
#include "Fishing/FishingMiniGameScenario.h"
#include "Fishing/FishingMechanics.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
DEFINE_LOG_CATEGORY(LogFishing);
namespace
{
FAutoConsoleCommand DumpFishingState(TEXT("Fishing.DumpState"), TEXT("Dump fishing operation and mechanics in running game worlds."), FConsoleCommandDelegate::CreateLambda([] {
    for (TObjectIterator<UMiniGameWorldSubsystem> It; It; ++It)
    {
        if (!It->GetWorld() || !It->GetWorld()->IsGameWorld()) continue;
        auto* Game = Cast<AFishingMiniGameScenario>(It->ActiveScenario);
        if (!Game) continue;
        UE_LOG(LogFishing, Display, TEXT("[Snapshot.Fishing] run=%s objective=%s operation=%s equipped=%d stopTested=%d stopCause=%d throttle=%.3f depth=%.3f load=%.3f sorted=%d processed=%d delivered=%d score=%.0f"),
            *Game->RunId.ToString(), *Game->ObjectiveId.ToString(), *UEnum::GetValueAsString(Game->Operation), Game->bEquipped, Game->bStopTested, int32(Game->Winch->StopCause), Game->Winch->Input, Game->Winch->Depth, Game->Winch->Load,
            Game->Conveyor->Sorted, Game->Processing->Processed, Game->Processing->Delivered, Game->Score());
    }
}));
}
