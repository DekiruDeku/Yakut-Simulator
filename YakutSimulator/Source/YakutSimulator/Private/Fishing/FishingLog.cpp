#include "Fishing/FishingLog.h"
#include "Fishing/FishingInteractionComponent.h"
#include "Fishing/FishingScenario.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogFishing);

namespace
{
FAutoConsoleCommand DumpFishingState(TEXT("Fishing.DumpState"),
    TEXT("Log fishing stage, player, input mappings, tracking and reachable buttons in every running game world."),
    FConsoleCommandDelegate::CreateLambda([]()
    {
        if (!GEngine) return;
        int32 Worlds = 0;
        for (const FWorldContext& Context : GEngine->GetWorldContexts())
        {
            UWorld* World = Context.World();
            if (!World || !World->IsGameWorld()) continue;
            ++Worlds;
            int32 Interactions = 0;
            for (TActorIterator<AFishingScenario> It(World); It; ++It)
                UE_LOG(LogFishing, Display, TEXT("[Snapshot.Scenario] world=%s scenario=%s stage=%s equipped=%d stopTested=%d emergencyStopped=%d attachVRInput=%d feedback=\"%s\""),
                    *World->GetName(), *It->GetName(), *UEnum::GetValueAsString(It->Stage), It->bSafetyEquipped, It->bStopTested,
                    It->bEmergencyStopped, It->bAttachVRInput, *It->LastFeedback.ToString());
            for (TActorIterator<APawn> It(World); It; ++It)
            {
                TArray<UFishingInteractionComponent*> Components;
                It->GetComponents(Components);
                for (UFishingInteractionComponent* Component : Components) { ++Interactions; Component->LogDiagnostics(); }
            }
            UE_LOG(LogFishing, Display, TEXT("[Snapshot.World] world=%s interactionComponents=%d"), *World->GetName(), Interactions);
        }
        if (!Worlds) UE_LOG(LogFishing, Display, TEXT("[Snapshot.Unavailable] reason=NoRunningGameWorld start=PlayOrVRPreview"));
    }));
}
