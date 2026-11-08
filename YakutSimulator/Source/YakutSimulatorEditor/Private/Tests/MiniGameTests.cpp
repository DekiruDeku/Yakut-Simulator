#if WITH_DEV_AUTOMATION_TESTS
#include "MiniGameTestFixture.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/ScopeLock.h"
#include "MotionControllerComponent.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "Components/StateTreeComponentSchema.h"

namespace MiniGameTests
{
struct FLogs : FOutputDevice
{
    FString Lines; FCriticalSection Lock;
    FLogs() { GLog->AddOutputDevice(this); }
    ~FLogs() { GLog->RemoveOutputDevice(this); }
    virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type, const FName& Category) override
    { if (Category==TEXT("LogInteraction") || Category==TEXT("LogMiniGame")) { FScopeLock Guard(&Lock); Lines += FString(Text)+TEXT("\n"); } }
    FString Read() { GLog->FlushThreadedLogs(); FScopeLock Guard(&Lock); return Lines; }
    int32 Count(const TCHAR* Match) { TArray<FString> Rows; Read().ParseIntoArrayLines(Rows); return Rows.FilterByPredicate([&](const FString& Row){return Row.Contains(Match);}).Num(); }
};
}
using namespace MiniGameTests;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiniGameCustomFlowTest,"Yakut.MiniGames.Common.CustomStateTree",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMiniGameCustomFlowTest::RunTest(const FString&)
{
    FFixture F;
    auto* Definition=F.Scenario->Definition.Get();UStateTree* OriginalFlow=Definition->Flow;
    F.Scenario->AbortRun();
    FMiniGameObjective Wait;Wait.Id=TEXT("DesignerWait");Wait.Title=FText::FromString(TEXT("Designer-authored intermediate task"));
    Definition->Objectives.Insert(Wait,1);
    auto* Tree=NewObject<UStateTree>(F.World);auto* Data=NewObject<UStateTreeEditorData>(Tree);Tree->EditorData=Data;
    Data->Schema=NewObject<UStateTreeComponentSchema>(Data);auto& Root=Data->AddRootState();
    TArray<UStateTreeState*> States;
    for(const auto& Objective:Definition->Objectives)
    {
        auto& State=Root.AddChildState(Objective.Id);auto& Task=State.AddTask<FMiniGameObjectiveTask>();
        Task.GetInstanceData().ObjectiveId=Objective.Id;
        if(Objective.Id==Wait.Id)Task.GetInstanceData().AutoCompleteSeconds=.05f;
        States.Add(&State);
    }
    States[0]->AddTransition(EStateTreeTransitionTrigger::OnEvent,Definition->Objectives[0].CompletionEvent,EStateTreeTransitionType::GotoState,States[1]);
    States[1]->AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded,EStateTreeTransitionType::GotoState,States[2]);
    States[2]->AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded,EStateTreeTransitionType::Succeeded);
    FStateTreeCompilerLog Log;FStateTreeCompiler Compiler(Log);if(!TestTrue(TEXT("Designer flow compiles"),Compiler.Compile(Tree)))return false;
    Definition->Flow=Tree;TestTrue(TEXT("Custom flow starts"),F.Scenario->StartRun());
    F.Scenario->ExecuteAction(TEXT("Continue"),nullptr);TestEqual(TEXT("Configured event enters intermediate task"),F.Scenario->ObjectiveId,Wait.Id);
    F.Tick(.1f);TestEqual(TEXT("Timer transition completes custom sequence"),F.Scenario->Status,EMiniGameStatus::Completed);
    TestTrue(TEXT("Original asset flow is preserved"),OriginalFlow!=Tree&&OriginalFlow->IsReadyToRun());
    auto* BadTree=NewObject<UStateTree>(F.World);auto* BadData=NewObject<UStateTreeEditorData>(BadTree);BadTree->EditorData=BadData;
    BadData->Schema=NewObject<UStateTreeComponentSchema>(BadData);
    auto& BadState=BadData->AddRootState().AddChildState(TEXT("InvalidAuthoring"));
    BadState.AddTask<FMiniGameObjectiveTask>().GetInstanceData().ObjectiveId=TEXT("MissingObjective");
    FStateTreeCompilerLog BadLog;FStateTreeCompiler BadCompiler(BadLog);
    if(!TestTrue(TEXT("Structurally valid tree with missing objective compiles"),BadCompiler.Compile(BadTree)))return false;
    Definition->Flow=BadTree;AddExpectedError(TEXT("[Objective.Missing]"),EAutomationExpectedErrorFlags::Contains,1,false);
    TestFalse(TEXT("Missing objective cannot start the run"),F.Scenario->StartRun());
    TestEqual(TEXT("Failed tree does not leave a running session"),F.Scenario->Status,EMiniGameStatus::Aborted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMiniGameValidationTest,"Yakut.MiniGames.Common.DesignerDataValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMiniGameValidationTest::RunTest(const FString&)
{
    FFixture F(true);
    TArray<FText> Errors;TestTrue(TEXT("Configured scene validates"),F.Scenario->ValidateScene(Errors));
    F.Config->Catch.FishCount=0;Errors.Empty();TestFalse(TEXT("Zero-sized batch rejected"),F.Config->ValidateConfiguration(Errors));F.Config->Catch.FishCount=2;
    F.Config->Processing.StrokeWarningSeconds=-1;Errors.Empty();TestFalse(TEXT("Negative cutting feedback duration rejected"),F.Config->ValidateConfiguration(Errors));F.Config->Processing.StrokeWarningSeconds=1;
    const FMiniGameObjective Duplicate=F.Config->Objectives[0];F.Config->Objectives.Add(Duplicate);Errors.Empty();TestFalse(TEXT("Duplicate objective IDs rejected"),F.Config->ValidateConfiguration(Errors));F.Config->Objectives.Pop();
    auto* Missing=F.World->SpawnActor<AGrabbableItem>();Errors.Empty();TestFalse(TEXT("Missing designer mesh rejected"),Missing->ValidateItem(Errors));
    auto* Lever=F.Fishing->WinchStation->Lever.Get();Lever->Travel=0;Errors.Empty();TestFalse(TEXT("Invalid lever range rejected"),F.Scenario->ValidateScene(Errors));Lever->Travel=20;
    auto* Fish=F.Config->SpeciesCycle[0]->ActorClass.GetDefaultObject();const FTransform PathTransform=Fish->CutPath->GetRelativeTransform();
    TestTrue(TEXT("Authored fish has a usable cut path"),Fish->GetCutPoints().Num()>=2);
    TestTrue(TEXT("Validation keeps designer transforms"),Fish->CutPath->GetRelativeTransform().Equals(PathTransform));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonSessionTest,"Yakut.MiniGames.Common.LifecycleAndIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCommonSessionTest::RunTest(const FString&)
{
    FFixture F;
    TestEqual(TEXT("Common flow starts without fishing"), F.Scenario->ObjectiveId,FName(TEXT("Interact")));
    const FGuid Old = F.Scenario->RunId;
    auto* Source = F.Scenario->Definition.Get(); Source->Messages.Add(TEXT("Snapshot"),FText::FromString(TEXT("changed")));
    TestFalse(TEXT("Run owns independent settings"),F.Scenario->ActiveDefinition()->Messages.Contains(TEXT("Snapshot")));
    auto* Other = F.World->SpawnActorDeferred<AMiniGameScenario>(AMiniGameScenario::StaticClass(),FTransform::Identity);
    Other->bAutoStart=false;Other->Definition=Source;Other->FinishSpawning(FTransform::Identity);
    TestFalse(TEXT("Only one scenario owns the player"),Other->StartRun());
    TestTrue(TEXT("Common action completes the generic StateTree"),F.Scenario->ExecuteAction(TEXT("Continue"),nullptr));
    TestEqual(TEXT("Final state completes session"),F.Scenario->Status,EMiniGameStatus::Completed);
    TestTrue(TEXT("Restart works"),F.Scenario->StartRun());
    TestTrue(TEXT("Restart refreshes data snapshot"),F.Scenario->ActiveDefinition()->Messages.Contains(TEXT("Snapshot")));
    TestTrue(TEXT("New run has new identity"),Old!=F.Scenario->RunId);
    TestFalse(TEXT("Late event from old run rejected"),F.Scenario->SubmitEvent(F.Scenario->CurrentObjective()->CompletionEvent,Old));
    TestEqual(TEXT("Late event cannot advance progress"),F.Scenario->ObjectiveId,FName(TEXT("Interact")));
    F.Scenario->AbortRun(); TestEqual(TEXT("Abort records status"),F.Scenario->Status,EMiniGameStatus::Aborted);
    TestTrue(TEXT("Another common scenario can start after abort"),Other->StartRun());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonInputTest,"Yakut.MiniGames.Common.InputAndDiagnostics",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCommonInputTest::RunTest(const FString&)
{
    FFixture F; FLogs Logs;
    auto* Button=F.Button(TEXT("Continue"),FVector(1000,0,100));
    F.Input->Use(false); TestEqual(TEXT("Distant button cannot complete objective"),F.Scenario->Status,EMiniGameStatus::Running);
    auto* Input=CastChecked<UEnhancedInputComponent>(F.Pawn->InputComponent);
    const int32 Initial=Input->GetActionEventBindings().Num(); F.Input->TickComponent(.01f,LEVELTICK_All,nullptr);
    TestEqual(TEXT("Binding retry does not duplicate actions"),Input->GetActionEventBindings().Num(),Initial);
    TestEqual(TEXT("Exactly six gameplay action bindings"),Initial,6);
    auto* PC=F.World->SpawnActor<APlayerController>(); auto* PlayerInput=NewObject<UEnhancedPlayerInput>(PC); PC->PlayerInput=PlayerInput;
    const UInputAction* Action=F.Input->Profile->LeftUse;
    auto Frame=[&](float Value){FInputActionValue V(Value); V.ConvertToType(Action->ValueType); PlayerInput->InjectInputForAction(Action,V); PlayerInput->ProcessInputStack({Input},.016f,false);};
    const int32 Before=Logs.Count(TEXT("[Interaction.Use]"));
    Frame(1);Frame(1);Frame(1); TestEqual(TEXT("Press and hold dispatches once"),Logs.Count(TEXT("[Interaction.Use]"))-Before,1);
    Frame(0);Frame(1); TestEqual(TEXT("Next press dispatches once"),Logs.Count(TEXT("[Interaction.Use]"))-Before,2);
    TestTrue(TEXT("Input event includes source event name"),Logs.Read().Contains(TEXT("event=Started")));
    TestTrue(TEXT("No target rejection is logged"),Logs.Read().Contains(TEXT("NoEligibleTarget")));
    F.Input->Grip(false);F.Input->Grip(false);TestEqual(TEXT("Grip hold dispatches once"),Logs.Count(TEXT("action=Grip event=Triggered")),1);
    F.Input->ReleaseGrip(false);F.Input->Grip(false);TestEqual(TEXT("New grip dispatches again"),Logs.Count(TEXT("action=Grip event=Triggered")),2);F.Input->ReleaseGrip(false);
    F.Input->LogDiagnostics(); TestTrue(TEXT("Hand state is logged"),Logs.Read().Contains(TEXT("[Snapshot.Hand]")));
    TestTrue(TEXT("Correct button can be selected"),F.Use(Button));
    TestEqual(TEXT("Use reaches common scenario"),F.Scenario->Status,EMiniGameStatus::Completed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonLeverTest,"Yakut.MiniGames.Common.LeverAndVisualPreservation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCommonLeverTest::RunTest(const FString&)
{
    FFixture F;
    auto* Station=F.Station(TEXT("/Game/Common/Blueprints/BP_InteractionLever.BP_InteractionLever_C"),FVector(100,200,300));
    Station->Lever->ActionId=TEXT("Lever"); Station->SetActorRotation(FRotator(0,90,0));
    const FTransform Home(FRotator(10,20,30),FVector(2,3,4),FVector(.13,.14,.15)); Station->Handle->SetRelativeTransform(Home);
    UStaticMesh* Mesh=Station->Handle->GetStaticMesh(); UMaterialInterface* Mat=Station->Handle->GetMaterial(0);
    const FVector GrabLocal(12,7,15); F.Hands[0]->SetWorldLocation(Station->GetActorTransform().TransformPosition(GrabLocal));
    TestTrue(TEXT("Common lever accepts grip"),Station->Lever->BeginInteraction(F.Ctx()));
    TestTrue(TEXT("Handle attaches and snaps to hand"),Station->Handle->GetAttachParent()==F.Hands[0] && Station->Handle->GetComponentLocation().Equals(F.Hands[0]->GetComponentLocation()));
    Station->Lever->TickComponent(.016f,LEVELTICK_All,nullptr);TestEqual(TEXT("Grab begins in neutral"),Station->Lever->Value,0.f);
    TestFalse(TEXT("Other hand cannot steal lever"),Station->Lever->BeginInteraction(F.Ctx(1)));
    F.Hands[0]->AddWorldOffset(Station->GetActorForwardVector()); Station->Lever->TickComponent(.016f,LEVELTICK_All,nullptr);TestEqual(TEXT("Jitter stays neutral"),Station->Lever->Value,0.f);
    F.Hands[0]->SetWorldLocation(Station->GetActorTransform().TransformPosition(GrabLocal+FVector(10,0,0)));
    Station->Lever->TickComponent(.016f,LEVELTICK_All,nullptr);TestTrue(TEXT("Speed ramps instead of jumping"),Station->Lever->Value>0 && Station->Lever->Value<.05f);
    Station->Lever->TickComponent(1,LEVELTICK_All,nullptr); TestEqual(TEXT("Authored travel gives half throttle"),Station->Lever->Value,.5f);
    Station->Lever->EndInteraction(F.Ctx(1));TestTrue(TEXT("Other hand cannot release lever"),Station->Lever->GetHoldingHand()==F.Hands[0]);
    Station->Lever->Release();TestTrue(TEXT("Release restores authored transform"),Station->Handle->GetRelativeTransform().Equals(Home));
    TestTrue(TEXT("Mechanism preserves mesh and material"),Station->Handle->GetStaticMesh()==Mesh && Station->Handle->GetMaterial(0)==Mat);
    F.Scenario->StartRun();TestTrue(TEXT("Restart preserves visual transform"),Station->Handle->GetRelativeTransform().Equals(Home));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCommonGrabTest,"Yakut.MiniGames.Common.GrabRecoveryAndOwnership",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FCommonGrabTest::RunTest(const FString&)
{
    FFixture F;
    auto* Item=F.World->SpawnActorDeferred<AGrabbableItem>(Class(TEXT("/Game/Tests/MiniGames/Blueprints/BP_TestItem.BP_TestItem_C")),FTransform(FVector(500,0,100)));
    Item->Scenario=F.Scenario;Item->SourceRun=F.Scenario->RunId;Item->Home=F.Scenario->GetRootComponent();Item->FinishSpawning(FTransform(FVector(500,0,100)));
    Item->Mesh->SetRelativeScale3D(FVector(.22,.31,.41)); const FTransform Visual=Item->Mesh->GetRelativeTransform();
    TestTrue(TEXT("Reusable item grabs without fishing"),F.Grab(Item)); TestFalse(TEXT("Other hand cannot steal item"),Item->Grab->BeginInteraction(F.Ctx(1)));
    F.Input->ReleaseGrip(true); TestTrue(TEXT("Other hand release keeps item held"),Item->IsHeld());
    F.DropAt(Item,FVector(500,0,-1000));TestTrue(TEXT("Drop enables physics"),Item->Body->IsSimulatingPhysics());
    Item->Tick(.1f);TestTrue(TEXT("Below-home recovery returns item"),Item->GetActorLocation().Equals(Item->Home->GetComponentLocation()));
    TestTrue(TEXT("Recovery keeps designer visual scale"),Item->Mesh->GetRelativeTransform().Equals(Visual));
    auto* Motion=NewObject<UMotionControllerComponent>(F.Pawn);F.Pawn->AddInstanceComponent(Motion);Motion->RegisterComponent();Motion->SetComponentTickEnabled(false);
    F.Hands[0]=Motion;F.Input->LeftHand.OverrideComponent=Motion;
    TestTrue(TEXT("Synthetic controller grabs before tracking guard"),F.Grab(Item));F.Input->bRequireTracking=true;
    F.Input->TickComponent(.016f,LEVELTICK_All,nullptr);TestFalse(TEXT("Tracking loss releases the held item"),Item->IsHeld());
    TestTrue(TEXT("Untracked controller cannot acquire target"),F.Input->FindTarget(F.Ctx(),EInteractionAction::Grip)==nullptr);
    F.Input->bRequireTracking=false;Item->ReturnHome();
    TestTrue(TEXT("Item grabs after recovery"),F.Grab(Item));F.Scenario->StartRun();TestFalse(TEXT("Restart releases held items"),Item->IsHeld());
    FText Reason;TestFalse(TEXT("Prior-run item cannot be grabbed"),Item->Grab->CanInteract(F.Ctx(),EInteractionAction::Grip,Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingWinchTest,"Yakut.MiniGames.Fishing.SafetyButtonsAndOverload",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFishingWinchTest::RunTest(const FString&)
{
    FFixture F(true); auto* Game=F.Fishing;
    TestTrue(TEXT("Inactive winch does not ask for Grip during safety"),Game->GetPresentation(F.Config->WinchPanel,Game->WinchStation).Title.IsEmpty());
    TestFalse(TEXT("Cannot skip safety"),Game->ExecuteAction(F.Config->StartAction,nullptr));
    auto* Equip=F.Button(F.Config->EquipAction,FVector(100,0,100));TestTrue(TEXT("Equipment through shared button"),F.Use(Equip));
    auto* Stop=F.Button(F.Config->StopAction,FVector(-740,-20,410));Stop->SetActorRotation(FRotator(0,90,0));
    Game->WinchStation->SetActorLocationAndRotation(FVector(-700,0,410),FRotator(0,90,0));
    auto* Start=F.Button(F.Config->StartAction,FVector(-660,-20,410));Start->SetActorRotation(FRotator(0,90,0));
    TestTrue(TEXT("Stop drill through button"),F.Use(Stop));TestTrue(TEXT("Start through button"),F.Use(Start));
    TestEqual(TEXT("Active winch shows speed and load"),Game->GetPresentation(F.Config->WinchPanel,Game->WinchStation).Metrics.Num(),2);
    F.Hands[0]->SetWorldLocation(FVector(-699.32,-4.07,414.68));F.Hands[1]->SetWorldLocation(FVector(-702.07,6.48,389.67));
    TestTrue(TEXT("Recorded pose targets lever"),F.Input->FindTarget(F.Ctx(),EInteractionAction::Grip)==Game->WinchStation->Lever);
    F.Input->Use(false);F.Input->Use(true);TestEqual(TEXT("Neighbor stop cannot fire from recorded lever poses"),Game->Winch->StopCause,EWinchStopCause::None);
    Game->Winch->Settings.DeploySeconds=100;Game->Winch->Settings.ResistanceBase=1;
    Game->Winch->SetInput(1);Game->Winch->TickComponent(.6f,LEVELTICK_All,nullptr);TestEqual(TEXT("Load gives recovery time"),Game->Winch->StopCause,EWinchStopCause::None);
    Game->Winch->TickComponent(1,LEVELTICK_All,nullptr);TestEqual(TEXT("Sustained overload stops"),Game->Winch->StopCause,EWinchStopCause::Overload);TestEqual(TEXT("Overload counted once"),Game->Overloads,1);
    TestTrue(TEXT("Reset through button"),F.Use(Start));TestEqual(TEXT("Reset starts neutral"),Game->Winch->Input,0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingSortTest,"Yakut.MiniGames.Fishing.SortingRecoveryAndThirdSpecies",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFishingSortTest::RunTest(const FString&)
{
    FFixture F(true,true); if (!TestTrue(TEXT("Flow reaches sorting"),F.ToSorting())) return false;
    auto* Game=F.Fishing;TestEqual(TEXT("Third-species preset spawns three fish"),Game->Conveyor->Fish.Num(),3);
    TestFalse(TEXT("Preset disables overloads"),Game->Winch->Settings.bEnableOverloads);
    TestTrue(TEXT("Third species has a different authored path"),Game->Conveyor->Fish[2]->CutPath->GetNumberOfSplinePoints()!=Game->Conveyor->Fish[0]->CutPath->GetNumberOfSplinePoints());
    TestTrue(TEXT("Third species has a different assigned mesh"),Game->Conveyor->Fish[2]->Mesh->GetStaticMesh()!=Game->Conveyor->Fish[0]->Mesh->GetStaticMesh());
    auto* Fish=Game->Conveyor->Fish[0].Get();Game->Conveyor->TickComponent(.01f,LEVELTICK_All,nullptr);
    Fish->Grab->bSimulateOnDrop=false;
    TestTrue(TEXT("Wrong bin handled with recovery"),Game->Conveyor->Sort(Fish,F.Config->SpeciesCycle[1]));
    TestFalse(TEXT("Recovery preserves the designer's drop-physics setting"),Fish->Grab->bSimulateOnDrop);
    Fish->Grab->bSimulateOnDrop=true;
    Game->Conveyor->Sort(Fish,F.Config->SpeciesCycle[1]);TestEqual(TEXT("Repeat wrong bin counted once"),Game->Conveyor->Mistakes,1);
    Game->Conveyor->Recover(Fish,true);Game->Conveyor->Recover(Fish,true);TestEqual(TEXT("Repeated miss counted once"),Game->Conveyor->Missed,1);
    TestEqual(TEXT("Recovery is available for retry"),Fish->State,EFishWorkState::Loose);
    for (int32 I=0;I<3;++I)
    {
        Fish=Game->Conveyor->Fish[I]; Game->Conveyor->TickComponent(.11f,LEVELTICK_All,nullptr);
        TestTrue(TEXT("Species comes from content cycle"),Fish->Species==F.Config->SpeciesCycle[I]);
        auto* Bin=Game->SortingBins[I].Station.Get(); TestTrue(TEXT("Fish grabs through shared interactor"),F.Grab(Fish));
        F.DropAt(Fish,Bin->GetActorLocation());TestEqual(TEXT("Release into matching bin sorts"),Fish->State,EFishWorkState::Sorted);
    }
    TestEqual(TEXT("All species advance the StateTree"),Game->ObjectiveId,FName(TEXT("GoProcessing")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingProcessingTest,"Yakut.MiniGames.Fishing.CompleteShiftAndRestart",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFishingProcessingTest::RunTest(const FString&)
{
    FFixture F(true);if (!TestTrue(TEXT("Reach processing through flow"),F.SortAll())) return false;auto* Game=F.Fishing;auto* Knife=Game->Processing->Knife.Get();
    for (AFishingFish* Fish : Game->Conveyor->Fish)
    {
        TestTrue(TEXT("Supply fish grabs"),F.Grab(Fish));F.DropAt(Fish,Game->ProcessingStation->GetActorLocation());TestTrue(TEXT("Drop loads board"),Game->Processing->BoardFish==Fish);
        TestTrue(TEXT("Knife grabs with other hand"),F.Grab(Knife,1));
        const auto Path=Fish->GetCutPoints();const FVector First=Fish->GetActorTransform().TransformPosition(Path[0]);
        Game->Processing->AdvanceStroke(First,First);TestEqual(TEXT("Unbraced fish cannot be cut"),Fish->NextCutPoint,0);
        TestTrue(TEXT("First hand braces fish"),F.Grab(Fish));TestTrue(TEXT("Fish stays anchored while braced"),Fish->GetRootComponent()->GetAttachParent()!=F.Hands[0]);
        const FVector Last=Fish->GetActorTransform().TransformPosition(Path.Last());Game->Processing->AdvanceStroke(Last,Last);TestEqual(TEXT("Checkpoints cannot be skipped"),Fish->NextCutPoint,0);
        FVector Previous=First;for (const FVector Local : Path){const FVector Point=Fish->GetActorTransform().TransformPosition(Local);Game->Processing->AdvanceStroke(Previous,Point);Previous=Point;}
        TestEqual(TEXT("Ordered path processes fish"),Fish->State,EFishWorkState::Processed);
        F.Input->ReleaseGrip(false);F.Input->ReleaseGrip(true);
        Game->Processing->Recover(Fish);TestEqual(TEXT("Recovery preserves processed state"),Fish->State,EFishWorkState::Processed);
        TestTrue(TEXT("Processed fish can be picked up"),F.Grab(Fish));F.DropAt(Fish,Game->DeliveryStation->GetActorLocation());TestEqual(TEXT("Release delivers fish"),Fish->State,EFishWorkState::Delivered);
    }
    TestEqual(TEXT("Full shift completes"),Game->Status,EMiniGameStatus::Completed);TestEqual(TEXT("Score uses preset values"),Game->Score(),200.f);
    TWeakObjectPtr<AGrabbableItem> OldKnife=Knife;const FGuid OldRun=Game->RunId;TestTrue(TEXT("Full shift restarts"),Game->StartRun());
    TestTrue(TEXT("Restart removes old tools"),!OldKnife.IsValid());TestTrue(TEXT("Restart creates new run"),OldRun!=Game->RunId);
    TestEqual(TEXT("Restart clears catch"),Game->Conveyor->Fish.Num(),0);TestFalse(TEXT("Restart clears safety flags"),Game->bEquipped||Game->bStopTested);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFishingProcessingFeedbackTest,"Yakut.MiniGames.Fishing.ProcessingFeedback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFishingProcessingFeedbackTest::RunTest(const FString&)
{
    FFixture F(true);
    if (!TestTrue(TEXT("Reach processing through the production flow"), F.SortAll())) return false;
    auto* Game = F.Fishing;
    auto* Work = Game->Processing.Get();
    auto* Fish = Game->Conveyor->Fish[0].Get();
    auto* Knife = Work->Knife.Get();
    const auto IsStatus = [&](const TCHAR* Expected) { return TestEqual(TEXT("Visible reason matches the actual cutting gate"), Work->GetCutStatus(), FName(Expected)); };
    IsStatus(TEXT("ProcessingNeedBoard"));
    TestTrue(TEXT("Grab supply fish"), F.Grab(Fish));
    TestEqual(TEXT("Loose fish confirms its holding hand"), Game->GetPresentation(NAME_None, Fish).Feedback, EMiniGameFeedback::Success);
    F.DropAt(Fish, Game->ProcessingStation->GetActorLocation());
    IsStatus(TEXT("ProcessingNeedFishGrip"));
    TestTrue(TEXT("Brace fish with the left hand"), F.Grab(Fish));
    Fish->Tick(.02f);
    TestTrue(TEXT("Brace marker confirms a valid anchored grip"), Fish->BraceIndicator->IsVisible());
    TestTrue(TEXT("Braced fish presentation names its actual hand"), Game->GetPresentation(NAME_None, Fish).Body.ToString().Contains(Game->FishingDefinition()->Text(TEXT("LeftHand")).ToString()));
    IsStatus(TEXT("ProcessingNeedKnifeGrip"));
    TestTrue(TEXT("Grab knife independently with right hand"), F.Grab(Knife, 1));
    TestTrue(TEXT("Knife working tip marker is visible while held"), Knife->ToolTipVisual->IsVisible());
    TestTrue(TEXT("Tip marker stays at the authored working point"), Knife->ToolTipVisual->GetComponentLocation().Equals(Knife->ToolTip->GetComponentLocation()));
    IsStatus(TEXT("ProcessingAim"));
    const auto Path = Fish->GetCutPoints();
    const FVector First = Fish->GetActorTransform().TransformPosition(Path[0]);
    const FVector Last = Fish->GetActorTransform().TransformPosition(Path.Last());
    F.Hands[0]->AddWorldOffset(FVector(Game->FishingDefinition()->Processing.BraceDistance + 1, 0, 0));
    Fish->Tick(.02f);
    IsStatus(TEXT("ProcessingBraceTooFar"));
    TestFalse(TEXT("Brace marker switches off when the holding hand leaves the fish"), Fish->BraceIndicator->IsVisible());
    Work->AdvanceStroke(First, First);
    TestEqual(TEXT("Moving the knife cannot cut while the brace is too far"), Fish->NextCutPoint, 0);
    F.Hands[0]->SetWorldLocation(Fish->GetActorLocation());
    const auto KnifeContext = Knife->Grab->HoldingContext;
    Knife->Grab->HoldingContext = Fish->Grab->HoldingContext;
    IsStatus(TEXT("ProcessingDifferentHands"));
    Knife->Grab->HoldingContext = KnifeContext;
    auto* RunConfig = Game->FishingDefinition();
    F.Input->ReleaseGrip(false);
    RunConfig->Processing.bRequireSecondHand = false;
    TestTrue(TEXT("Optional second hand uses the same readiness rules"), Work->GetCutBlockReason().IsNone());
    RunConfig->Processing.bRequireSecondHand = true;
    TestTrue(TEXT("Restore configured brace"), F.Grab(Fish));
    const auto AuthoredPath = Fish->CutPath->GetNumberOfSplinePoints();
    Fish->CutPath->ClearSplinePoints();
    IsStatus(TEXT("ProcessingInvalidPath"));
    for (const auto& Point : Path) Fish->CutPath->AddSplinePoint(Point, ESplineCoordinateSpace::Local);
    TestEqual(TEXT("Restore authored cutting path"), Fish->CutPath->GetNumberOfSplinePoints(), AuthoredPath);
    Work->AdvanceStroke(First - FVector(Game->FishingDefinition()->Processing.MaxKnifeStep + 1, 0, 0), First);
    IsStatus(TEXT("ProcessingMoveTooFast"));
    TestEqual(TEXT("Oversized stroke does not fill the progress bar"), Fish->NextCutPoint, 0);
    Work->TickComponent(Game->FishingDefinition()->Processing.StrokeWarningSeconds + .01f, LEVELTICK_All, nullptr);
    IsStatus(TEXT("ProcessingAim"));
    Work->AdvanceStroke(Last, Last);
    TestEqual(TEXT("Touching a later marker does not skip the current one"), Fish->NextCutPoint, 0);
    Work->AdvanceStroke(First, First);
    const auto Panel = Game->GetPresentation(NAME_None, Game);
    TestEqual(TEXT("Current fish progress advances after the first checkpoint"), Panel.Metrics[0].Value, 1.f);
    TestEqual(TEXT("Current fish progress uses this fish's path length"), Panel.Metrics[0].Maximum, float(Path.Num()));
    TestEqual(TEXT("Partial cut is visible before a whole fish is processed"), Work->Processed, 0);
    TestTrue(TEXT("Work panel reports the actual knife-tip distance"), Panel.Body.ToString().Contains(TEXT("см")));
    TestEqual(TEXT("Board panel uses the same live cutting reason"), Game->GetPresentation(NAME_None, Game->ProcessingStation).Title.ToString(), Panel.Title.ToString());
    F.Input->ReleaseGrip(false);
    Fish->Tick(.02f);
    IsStatus(TEXT("ProcessingNeedFishGrip"));
    TestFalse(TEXT("Releasing fish clears its confirmation marker"), Fish->BraceIndicator->IsVisible());
    TestTrue(TEXT("Releasing left grip keeps right-hand knife held"), Knife->IsHeld() && Knife->ToolTipVisual->IsVisible());
    TestEqual(TEXT("Losing the brace preserves accepted checkpoint progress"), Fish->NextCutPoint, 1);
    TestTrue(TEXT("Reacquire brace"), F.Grab(Fish));
    for (const FVector Local : Path)
    {
        const FVector Point = Fish->GetActorTransform().TransformPosition(Local);
        Work->AdvanceStroke(Point, Point);
    }
    IsStatus(TEXT("ProcessingReadyToDeliver"));
    const auto Finished = Game->GetPresentation(NAME_None, Game);
    TestEqual(TEXT("Current fish bar reaches 100 percent before delivery"), Finished.Metrics[0].Value, Finished.Metrics[0].Maximum);
    F.Input->ReleaseGrip(true);
    TestFalse(TEXT("Releasing the knife hides its working-tip marker"), Knife->ToolTipVisual->IsVisible());
    const FGuid OldRun = Game->RunId;
    TestTrue(TEXT("Restart resets the processing feedback"), Game->StartRun());
    TestTrue(TEXT("Restart changes the run identity"), OldRun != Game->RunId);
    TestTrue(TEXT("Inactive processing presentation is empty"), Work->GetPresentation().Title.IsEmpty());
    Game->Operation = EFishingOperation::Processing;
    Game->PublishFeedback(TEXT("SpawnFailed"), EMiniGameFeedback::Error);
    TestTrue(TEXT("Tool-spawn failure remains visible on the task panel"),
        Game->GetPresentation(NAME_None, Game).Body.ToString().Contains(Game->FishingDefinition()->Text(TEXT("SpawnFailed")).ToString()));
    return true;
}
#endif
