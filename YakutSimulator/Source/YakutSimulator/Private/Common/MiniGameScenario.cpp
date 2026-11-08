#include "Common/MiniGameScenario.h"
#include "Common/Interaction.h"
#include "Common/GrabbableItem.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "StateTreeExecutionContext.h"
#include "TimerManager.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "Misc/DataValidation.h"

DEFINE_LOG_CATEGORY(LogMiniGame);
DEFINE_LOG_CATEGORY(LogInteraction);

void UMiniGameStateTreeComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    if (bProcessing) return;
    TGuardValue<bool> Guard(bProcessing, true);
    Super::TickComponent(Delta, Type, Function);
    if (auto* Scenario = Cast<AMiniGameScenario>(GetOwner()))
        if (Scenario->Status == EMiniGameStatus::Aborted && IsRunning()) { Scenario->AbortRun(); return; }
    if (auto* Scenario = Cast<AMiniGameScenario>(GetOwner())) if (Scenario->Status == EMiniGameStatus::Running)
    {
        if (GetStateTreeRunStatus() == EStateTreeRunStatus::Succeeded) Scenario->CompleteRun();
        else if (GetStateTreeRunStatus() == EStateTreeRunStatus::Failed) Scenario->AbortRun();
    }
}
void UMiniGameStateTreeComponent::Dispatch(FGameplayTag Event)
{
    if (!IsRunning() || !Event.IsValid()) return;
    SendStateTreeEvent(Event);
    if (!bProcessing) TickComponent(0, LEVELTICK_All, nullptr);
}
EStateTreeRunStatus FMiniGameObjectiveTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
    auto& Data = Context.GetInstanceData(*this);
    Data.Elapsed = 0;
    AMiniGameScenario* Scenario = Cast<AMiniGameScenario>(static_cast<UObject*>(Context.GetOwner()));
    if (!Scenario || !Scenario->ActivateObjective(Data.ObjectiveId)) return EStateTreeRunStatus::Failed;
    return Scenario->Status == EMiniGameStatus::Completed ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
void FMiniGameObjectiveTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult&) const
{
    if (AMiniGameScenario* Scenario = Cast<AMiniGameScenario>(static_cast<UObject*>(Context.GetOwner()))) Scenario->ExitObjective(Context.GetInstanceData(*this).ObjectiveId);
}
EStateTreeRunStatus FMiniGameObjectiveTask::Tick(FStateTreeExecutionContext& Context, float Delta) const
{
    auto& Data = Context.GetInstanceData(*this);
    Data.Elapsed += Delta;
    return Data.AutoCompleteSeconds > 0 && Data.Elapsed >= Data.AutoCompleteSeconds ? EStateTreeRunStatus::Succeeded : EStateTreeRunStatus::Running;
}
AMiniGameScenario::AMiniGameScenario()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    StateTree = CreateDefaultSubobject<UMiniGameStateTreeComponent>(TEXT("ScenarioFlow"));
    StateTree->SetStartLogicAutomatically(false);
}
void AMiniGameScenario::BeginPlay()
{
    Super::BeginPlay();
    if (bAutoStart) StartRun();
}
void AMiniGameScenario::EndPlay(const EEndPlayReason::Type Reason)
{
    AbortRun();
    if (auto* World = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>())
        if (World->ActiveScenario == this) World->ActiveScenario = nullptr;
    Super::EndPlay(Reason);
}
bool AMiniGameScenario::ValidateScene(TArray<FText>& Errors) const
{
    if (!Definition) Errors.Add(NSLOCTEXT("MiniGame.Validation", "Definition", "Assign a mini-game definition."));
    else Definition->ValidateConfiguration(Errors);
    if (Definition) for (const auto& Objective : Definition->Objectives)
        if (!Objective.Workplace.IsNone() && (!Workplaces.Contains(Objective.Workplace) || !IsValid(Workplaces[Objective.Workplace])))
            Errors.Add(FText::Format(NSLOCTEXT("MiniGame.Validation", "Workplace", "Missing workplace: {0}"), FText::FromName(Objective.Workplace)));
    if (GetWorld()) for (TActorIterator<AMiniGameStation> It(GetWorld()); It; ++It) if (It->Scenario == this) It->ValidateStation(Errors);
    return Errors.IsEmpty();
}
#if WITH_EDITOR
EDataValidationResult AMiniGameScenario::IsDataValid(FDataValidationContext& Context) const
{
    if (HasAnyFlags(RF_ClassDefaultObject)) return EDataValidationResult::NotValidated;
    TArray<FText> Errors; ValidateScene(Errors);
    for (const FText& Error : Errors) Context.AddError(Error);
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
bool AMiniGameScenario::StartRun()
{
    TArray<FText> Errors;
    if (!ValidateScene(Errors))
    {
        for (const FText& Error : Errors) UE_LOG(LogMiniGame, Error, TEXT("[Configuration.Invalid] actor=%s reason=%s"), *GetName(), *Error.ToString());
        return false;
    }
    auto* World = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>();
    if (!World || (IsValid(World->ActiveScenario) && World->ActiveScenario != this && World->ActiveScenario->Status == EMiniGameStatus::Running)) return false;
    StateTree->StopLogic(TEXT("Restart"));
    GetWorldTimerManager().ClearAllTimersForObject(this);
    ReleaseInteractions();
    World->ActiveScenario = this;
    RunDefinition = DuplicateObject<UMiniGameDefinition>(Definition, this);
    RunId = FGuid::NewGuid();
    Status = EMiniGameStatus::Running;
    ElapsedSeconds = UISeconds = 0;
    ObjectiveId = NAME_None;
    Feedback = FText::GetEmpty();
    OnRunReset();
    StateTree->SetStateTree(ActiveDefinition()->Flow);
    UE_LOG(LogMiniGame, Display, TEXT("[Run.Start] game=%s run=%s actor=%s"), *ActiveDefinition()->Id.ToString(), *RunId.ToString(), *GetName());
    StateTree->StartLogic();
    if (Status == EMiniGameStatus::Aborted) { AbortRun(); return false; }
    if (!StateTree->IsRunning() && Status == EMiniGameStatus::Running)
    {
        if (StateTree->GetStateTreeRunStatus() == EStateTreeRunStatus::Succeeded) CompleteRun();
        else AbortRun();
    }
    NotifyPresentation();
    return StateTree->IsRunning() || Status == EMiniGameStatus::Completed;
}
void AMiniGameScenario::AbortRun()
{
    StateTree->StopLogic(TEXT("Abort"));
    GetWorldTimerManager().ClearAllTimersForObject(this);
    ReleaseInteractions();
    if (Status == EMiniGameStatus::Running)
    {
        Status = EMiniGameStatus::Aborted;
        UE_LOG(LogMiniGame, Display, TEXT("[Run.Abort] run=%s objective=%s"), *RunId.ToString(), *ObjectiveId.ToString());
    }
    OnRunReset();
    NotifyPresentation();
}
void AMiniGameScenario::CompleteRun()
{
    if (Status != EMiniGameStatus::Running) return;
    Status = EMiniGameStatus::Completed;
    ReleaseInteractions();
    UE_LOG(LogMiniGame, Display, TEXT("[Run.Complete] game=%s run=%s elapsed=%.2f"), *ActiveDefinition()->Id.ToString(), *RunId.ToString(), ElapsedSeconds);
    NotifyPresentation();
}
const FMiniGameObjective* AMiniGameScenario::CurrentObjective() const { return Definition ? ActiveDefinition()->FindObjective(ObjectiveId) : nullptr; }
bool AMiniGameScenario::ActivateObjective(FName Id)
{
    const auto* Objective = Definition ? ActiveDefinition()->FindObjective(Id) : nullptr;
    if (!Objective)
    {
        UE_LOG(LogMiniGame, Error, TEXT("[Objective.Missing] id=%s"), *Id.ToString());
        Status = EMiniGameStatus::Aborted;
        return false;
    }
    ObjectiveId = Id;
    Feedback = FText::GetEmpty();
    FeedbackSeverity = EMiniGameFeedback::Normal;
    UE_LOG(LogMiniGame, Display, TEXT("[Objective.Enter] game=%s run=%s objective=%s operation=%s"), *ActiveDefinition()->Id.ToString(), *RunId.ToString(), *Id.ToString(), *Objective->Operation.ToString());
    OnObjectiveEntered(Id);
    if (!Objective->Workplace.IsNone()) MoveToWorkplace(Objective->Workplace);
    if (Objective->bFinal) CompleteRun();
    NotifyPresentation();
    return true;
}
void AMiniGameScenario::ExitObjective(FName Id)
{
    UE_LOG(LogMiniGame, Log, TEXT("[Objective.Exit] run=%s objective=%s"), *RunId.ToString(), *Id.ToString());
    OnObjectiveExited(Id);
}
void AMiniGameScenario::CompleteObjective() { if (const auto* Objective = CurrentObjective()) SubmitEvent(Objective->CompletionEvent, RunId); }
bool AMiniGameScenario::SubmitEvent(FGameplayTag Event, FGuid SourceRun)
{
    if (Status != EMiniGameStatus::Running || SourceRun != RunId || !Event.IsValid()) return false;
    UE_LOG(LogMiniGame, Log, TEXT("[Event.Submit] run=%s objective=%s event=%s"), *RunId.ToString(), *ObjectiveId.ToString(), *Event.ToString());
    StateTree->Dispatch(Event);
    return true;
}
void AMiniGameScenario::PublishFeedback(FName Key, EMiniGameFeedback Severity)
{
    Feedback = Definition ? ActiveDefinition()->Text(Key) : FText::FromName(Key);
    FeedbackSeverity = Severity;
    UE_LOG(LogMiniGame, Log, TEXT("[Feedback] run=%s objective=%s code=%s message=%s"), *RunId.ToString(), *ObjectiveId.ToString(), *Key.ToString(), *Feedback.ToString());
    NotifyPresentation();
}
void AMiniGameScenario::PublishInteractionFeedback(FText Message)
{
    if (Message.IsEmpty()) return;
    Feedback = Message;
    FeedbackSeverity = EMiniGameFeedback::Warning;
    NotifyPresentation();
}
bool AMiniGameScenario::IsActionAllowed(FName Action) const
{
    if (!Definition || Action.IsNone()) return false;
    if (Action == ActiveDefinition()->RestartAction) return Status == EMiniGameStatus::Completed || Status == EMiniGameStatus::Aborted;
    const auto* Objective = CurrentObjective();
    return Status == EMiniGameStatus::Running && Objective && Objective->AllowedActions.Contains(Action);
}
bool AMiniGameScenario::ExecuteAction_Implementation(FName Action, AActor*)
{
    if (!Definition || !IsActionAllowed(Action)) return false;
    if (Action == ActiveDefinition()->RestartAction) return StartRun();
    const auto* Event = ActiveDefinition()->ActionEvents.Find(Action);
    return Event && SubmitEvent(*Event, RunId);
}
bool AMiniGameScenario::TryReceive_Implementation(UItemReceiverComponent*, AGrabbableItem*) { return false; }
void AMiniGameScenario::RecoverItem_Implementation(AGrabbableItem* Item) { if (Item) Item->ReturnHome(); }
void AMiniGameScenario::OnRunReset_Implementation() {}
void AMiniGameScenario::OnObjectiveEntered_Implementation(FName) {}
void AMiniGameScenario::OnObjectiveExited_Implementation(FName) {}
FMiniGamePresentation AMiniGameScenario::GetPresentation_Implementation(FName, AActor*) const
{
    FMiniGamePresentation Result;
    if (const auto* Objective = CurrentObjective()) { Result.Title = Objective->Title; Result.Body = Objective->Instruction; }
    if (!Feedback.IsEmpty() && ActiveDefinition() && ActiveDefinition()->Style) Result.Body = FText::Format(ActiveDefinition()->Style->FeedbackFormat, Result.Body, Feedback);
    Result.Feedback = FeedbackSeverity;
    return Result;
}
void AMiniGameScenario::NotifyPresentation() { OnPresentationChanged.Broadcast(); }
void AMiniGameScenario::Tick(float Delta)
{
    Super::Tick(Delta);
    if (!FMath::IsFinite(Delta) || Delta <= 0) return;
    if (Status == EMiniGameStatus::Running) ElapsedSeconds += Delta;
    UISeconds += Delta;
    if (Definition && ActiveDefinition()->Style && UISeconds >= ActiveDefinition()->Style->RefreshInterval) { UISeconds = 0; NotifyPresentation(); }
}
void AMiniGameScenario::ReleaseInteractions()
{
    if (auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>())
        for (const auto& Target : Registry->Targets)
            if (Target.IsValid() && Target->Scenario == this)
            {
                if (auto* Grab = Cast<UGrabInteractionComponent>(Target.Get())) Grab->Release();
                if (auto* Lever = Cast<ULeverInteractionComponent>(Target.Get())) Lever->Release();
            }
}
void AMiniGameScenario::MoveToWorkplace(FName Id)
{
    USceneComponent* Destination = Workplaces.FindRef(Id);
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!IsValid(Destination) || !IsValid(Player) || !Definition) return;
    ReleaseInteractions();
    APlayerController* PC = Cast<APlayerController>(Player->GetController());
    if (PC && PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0, 1, ActiveDefinition()->FadeOutSeconds, FLinearColor::Black, false, true);
    const FTransform Target = Destination->GetComponentTransform();
    const FGuid ExpectedRun = RunId;
    TWeakObjectPtr<APawn> WeakPlayer(Player);
    const auto Move = [this, WeakPlayer, Target, ExpectedRun]()
    {
        if (!WeakPlayer.IsValid() || RunId != ExpectedRun || Status != EMiniGameStatus::Running) return;
        APawn* Pawn = WeakPlayer.Get();
        UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>();
        const float Yaw = Camera ? Camera->GetComponentRotation().Yaw : Pawn->GetActorRotation().Yaw;
        Pawn->AddActorWorldRotation(FRotator(0, FMath::FindDeltaAngleDegrees(Yaw, Target.Rotator().Yaw), 0));
        FVector Offset = Camera ? Camera->GetComponentLocation() - Pawn->GetActorLocation() : FVector::ZeroVector;
        Offset.Z = 0;
        Pawn->SetActorLocation(Target.GetLocation() - Offset, false, nullptr, ETeleportType::TeleportPhysics);
        if (auto* Controller = Cast<APlayerController>(Pawn->GetController()))
            if (Controller->PlayerCameraManager) Controller->PlayerCameraManager->StartCameraFade(1, 0, ActiveDefinition()->FadeInSeconds, FLinearColor::Black, false, false);
        UE_LOG(LogMiniGame, Log, TEXT("[Travel.Complete] run=%s player=%s position=%s"), *RunId.ToString(), *Pawn->GetName(), *Pawn->GetActorLocation().ToCompactString());
    };
    if (ActiveDefinition()->FadeOutSeconds > 0) GetWorldTimerManager().SetTimer(TravelTimer, FTimerDelegate::CreateWeakLambda(this, Move), ActiveDefinition()->FadeOutSeconds, false);
    else Move();
}
namespace
{
FAutoConsoleCommand Dump(TEXT("MiniGame.DumpState"), TEXT("Dump the active mini-game, run, objective and interactor state."),
FConsoleCommandDelegate::CreateLambda([] {
    for (TObjectIterator<UMiniGameWorldSubsystem> It; It; ++It)
    {
        if (!It->GetWorld() || !It->GetWorld()->IsGameWorld() || !IsValid(It->ActiveScenario)) continue;
        const auto* Scenario = It->ActiveScenario.Get();
        UE_LOG(LogMiniGame, Display, TEXT("[Snapshot] game=%s run=%s status=%d objective=%s targets=%d"), *GetNameSafe(Scenario->Definition), *Scenario->RunId.ToString(), int32(Scenario->Status), *Scenario->ObjectiveId.ToString(), It->Targets.Num());
        for (TActorIterator<APawn> Pawn(It->GetWorld()); Pawn; ++Pawn)
            if (auto* Input = Pawn->FindComponentByClass<UXRInteractorComponent>()) Input->LogDiagnostics();
    }
}));
}
