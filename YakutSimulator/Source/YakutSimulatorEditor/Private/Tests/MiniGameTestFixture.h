#pragma once
#include "Misc/AutomationTest.h"
#include "Common/GrabbableItem.h"
#include "Common/MiniGameScenario.h"
#include "Fishing/FishingMiniGameScenario.h"
#include "Fishing/FishingFish.h"
#include "Fishing/FishingMechanics.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/WorldSettings.h"

namespace MiniGameTests
{
inline UClass* Class(const TCHAR* Path) { return LoadObject<UClass>(nullptr, Path); }
struct FFixture
{
    UWorld* World;
    AMiniGameScenario* Scenario;
    AFishingMiniGameScenario* Fishing = nullptr;
    APawn* Pawn;
    USceneComponent* Hands[2];
    UXRInteractorComponent* Input;
    UFishingDefinition* Config = nullptr;
    TArray<AMiniGameStation*> Buttons;
    explicit FFixture(bool Fish = false, bool ThreeSpecies = false)
    {
        World = UWorld::CreateWorld(EWorldType::Game, false);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Pawn = World->SpawnActor<APawn>();
        auto* Root = NewObject<USceneComponent>(Pawn, TEXT("TestRoot"));
        Pawn->SetRootComponent(Root); Pawn->AddInstanceComponent(Root); Root->RegisterComponent();
        for (int32 I=0; I<2; ++I)
        {
            Hands[I] = NewObject<USceneComponent>(Pawn, I ? TEXT("RightHand") : TEXT("LeftHand"));
            Hands[I]->SetupAttachment(Root); Pawn->AddInstanceComponent(Hands[I]); Hands[I]->RegisterComponent();
        }
        Pawn->InputComponent = NewObject<UEnhancedInputComponent>(Pawn); Pawn->InputComponent->RegisterComponent();
        Input = NewObject<UXRInteractorComponent>(Pawn); Pawn->AddInstanceComponent(Input);
        Input->LeftHand.ComponentProperty = NAME_None; Input->LeftHand.OverrideComponent = Hands[0];
        Input->RightHand.OverrideComponent = Hands[1]; Input->bRequireTracking = false;
        Input->RegisterComponent();
        if (Fish)
        {
            Fishing = World->SpawnActor<AFishingMiniGameScenario>(); Scenario = Fishing;
            Config = DuplicateObject<UFishingDefinition>(LoadObject<UFishingDefinition>(nullptr, ThreeSpecies ? TEXT("/Game/Tests/MiniGames/DA_Fishing_ThreeSpecies") : TEXT("/Game/MiniGames/Fishing/Data/DA_Fishing_Default")), World);
            Config->Catch.FishCount = ThreeSpecies ? 3 : 2;
            Config->Winch.DeploySeconds = Config->Winch.HaulSeconds = .1f; Config->Winch.TrawlSeconds = .1f;
            Config->Catch.FeedInterval = .1f;
            Scenario->Definition = Config;
            Fishing->WinchStation = Station(TEXT("/Game/Common/Blueprints/BP_InteractionLever.BP_InteractionLever_C"), FVector(500,0,100));
            Fishing->WinchStation->Lever->ActionId = Config->WinchAction;
            Fishing->ProcessingStation = Station(TEXT("/Game/Common/Blueprints/BP_ItemReceiver.BP_ItemReceiver_C"), FVector(0,1000,100));
            Fishing->DeliveryStation = Station(TEXT("/Game/Common/Blueprints/BP_ItemReceiver.BP_ItemReceiver_C"), FVector(500,1000,100));
            for (int32 I=0; I<Config->SpeciesCycle.Num(); ++I)
            {
                auto* Bin = Station(TEXT("/Game/Common/Blueprints/BP_ItemReceiver.BP_ItemReceiver_C"), FVector(I*150,500,100));
                Fishing->SortingBins.Add({Bin, Config->SpeciesCycle[I]});
            }
            Fishing->ConveyorPath->AddSplinePoint(FVector(0,0,100), ESplineCoordinateSpace::World);
            Fishing->ConveyorPath->AddSplinePoint(FVector(1000,0,100), ESplineCoordinateSpace::World);
            Fishing->RecoveryPoint->SetWorldLocation(FVector(0,200,100));
            Fishing->ProcessingPoint->SetWorldLocation(FVector(0,1000,100));
            Fishing->ProcessingSupplyPoint->SetWorldLocation(FVector(-150,1000,100));
            Fishing->KnifeHome->SetWorldLocation(FVector(150,1000,100));
            Fishing->FinishedPoint->SetWorldLocation(FVector(500,1000,100));
            Fishing->CutGuide->SetStaticMesh(Fishing->WinchStation->Mesh->GetStaticMesh());
            Scenario->Workplaces = {{TEXT("Deck"),Fishing->DeckPlayerPoint},{TEXT("Sorting"),Fishing->SortingPlayerPoint},{TEXT("Processing"),Fishing->ProcessingPlayerPoint}};
        }
        else
        {
            Scenario = World->SpawnActor<AMiniGameScenario>();
            Scenario->Definition = DuplicateObject<UMiniGameDefinition>(LoadObject<UMiniGameDefinition>(nullptr, TEXT("/Game/Tests/MiniGames/DA_CommonScenario")), World);
        }
        Scenario->bAutoStart = false;
        Scenario->Definition->FadeOutSeconds = Scenario->Definition->FadeInSeconds = 0;
        Input->Profile = Scenario->Definition->Interaction;
        World->InitializeActorsForPlay(FURL()); World->BeginPlay();
        World->GetWorldSettings()->NotifyBeginPlay();
        check(Scenario->StartRun());
    }
    ~FFixture() { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
    AMiniGameStation* Station(const TCHAR* Path, FVector Position)
    {
        auto* Result = World->SpawnActor<AMiniGameStation>(Class(Path), Position, FRotator::ZeroRotator);
        Result->Scenario = Scenario;
        Result->Button->Scenario = Result->Lever->Scenario = Scenario;
        Result->Receiver->Scenario = Scenario;
        return Result;
    }
    AMiniGameStation* Button(FName Action, FVector Position)
    {
        auto* Result = Station(TEXT("/Game/Common/Blueprints/BP_InteractionButton.BP_InteractionButton_C"), Position);
        Result->Button->ActionId = Action; Buttons.Add(Result); return Result;
    }
    FInteractionContext Ctx(int32 I=0) const { return Input->Context(I != 0); }
    void Tick(float Seconds=.02f)
    {
        // Unit tests advance deterministically; the saved-level test exercises the real frame scheduler.
        Scenario->Tick(Seconds);
        Scenario->StateTree->TickComponent(Seconds, LEVELTICK_All, nullptr);
        if (Fishing)
        {
            Fishing->Winch->TickComponent(Seconds, LEVELTICK_All, nullptr);
            Fishing->Conveyor->TickComponent(Seconds, LEVELTICK_All, nullptr);
            Fishing->Processing->TickComponent(Seconds, LEVELTICK_All, nullptr);
        }
    }
    bool Use(AMiniGameStation* Target)
    {
        Hands[0]->SetWorldLocation(Target->PressZone->GetComponentLocation());
        auto* Found = Input->FindTarget(Ctx(), EInteractionAction::Use);
        Input->Use(false); return Found == Target->Button;
    }
    bool ToSorting()
    {
        if (!Fishing->ExecuteAction(Config->EquipAction,nullptr) || !Fishing->ExecuteAction(Config->StopAction,nullptr) || !Fishing->ExecuteAction(Config->StartAction,nullptr)) return false;
        for(int32 I=0; I<100 && Fishing->Operation!=EFishingOperation::None; ++I) { Fishing->Winch->SetInput(.5f); Tick(); }
        return Fishing->ExecuteAction(Config->SortingAction,nullptr);
    }
    bool SortAll()
    {
        if (!ToSorting()) return false;
        for (AFishingFish* Fish : Fishing->Conveyor->Fish)
        {
            Fishing->Conveyor->TickComponent(.11f, LEVELTICK_All, nullptr);
            if (!Fishing->Conveyor->Sort(Fish,Fish->Species)) return false;
        }
        return Fishing->ExecuteAction(Config->ProcessingAction,nullptr);
    }
    bool Grab(AGrabbableItem* Item, int32 I=0)
    {
        Hands[I]->SetWorldLocation(Item->GetActorLocation()); Input->ReleaseGrip(I != 0); Input->Grip(I != 0);
        return Item->Grab->GetHoldingHand() == Hands[I];
    }
    void DropAt(AGrabbableItem* Item, FVector Position, int32 I=0)
    {
        const FVector Delta = Position - Item->GetActorLocation(); Hands[I]->AddWorldOffset(Delta); Input->ReleaseGrip(I != 0);
    }
};
}
