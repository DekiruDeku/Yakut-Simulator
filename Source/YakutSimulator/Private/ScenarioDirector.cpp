#include "ScenarioDirector.h"

#include "EngineUtils.h"
#include "WorkItem.h"
#include "WorkOperationComponent.h"

AScenarioDirector::AScenarioDirector()
{
    UWorkOperationComponent* GrabOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("GrabOperation"));
    GrabOperation->RequiredAction = EWorkAction::Grabbed;
    UWorkOperationComponent* UseOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("UseOperation"));
    UseOperation->RequiredAction = EWorkAction::Used;
    UWorkOperationComponent* PlaceOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("PlaceOperation"));
    PlaceOperation->RequiredAction = EWorkAction::Placed;

    FScenarioStepDefinition GrabStep;
    GrabStep.StepIdentifier = "Grab";
    GrabStep.Instruction = FText::FromString(TEXT("Возьмите учебный предмет."));
    GrabStep.Hint = FText::FromString(TEXT("Поднесите руку и удерживайте Grip."));
    GrabStep.Operation = GrabOperation;
    Steps.Add(GrabStep);

    FScenarioStepDefinition UseStep;
    UseStep.StepIdentifier = "Use";
    UseStep.Instruction = FText::FromString(TEXT("Используйте предмет: нажмите Trigger."));
    UseStep.Operation = UseOperation;
    UseStep.RequiredSteps.Add("Grab");
    Steps.Add(UseStep);

    FScenarioStepDefinition PlaceStep;
    PlaceStep.StepIdentifier = "Place";
    PlaceStep.Instruction = FText::FromString(TEXT("Положите предмет на отмеченную площадку."));
    PlaceStep.Operation = PlaceOperation;
    PlaceStep.RequiredSteps.Add("Use");
    Steps.Add(PlaceStep);
}

void AScenarioDirector::BeginPlay()
{
    Super::BeginPlay();
    for (const FScenarioStepDefinition& Step : Steps)
    {
        if (Step.Operation)
        {
            Step.Operation->OnOperationChanged.AddUniqueDynamic(this, &AScenarioDirector::ProcessOperationChanged);
        }
    }
    for (TActorIterator<AWorkItem> ItemIterator(GetWorld()); ItemIterator; ++ItemIterator)
    {
        RegisterItem(*ItemIterator);
    }
    RefreshAvailability();
}

void AScenarioDirector::RegisterItem(AWorkItem* Item)
{
    if (Item)
    {
        Item->OnItemAction.AddUniqueDynamic(this, &AScenarioDirector::ProcessItemAction);
    }
}

void AScenarioDirector::ProcessItemAction(AWorkItem* Item, EWorkAction Action)
{
    TArray<UWorkOperationComponent*> AvailableOperations;
    for (const FScenarioStepDefinition& Step : Steps)
    {
        if (Step.Operation && (Step.Operation->State == EWorkOperationState::Ready || Step.Operation->State == EWorkOperationState::InProgress))
        {
            AvailableOperations.Add(Step.Operation);
        }
    }
    for (UWorkOperationComponent* Operation : AvailableOperations)
    {
        Operation->ObserveAction(Item, Action);
    }
    FText ActionDescription;
    switch (Action)
    {
        case EWorkAction::Grabbed:
        {
            ActionDescription = FText::FromString(TEXT("Предмет взят"));
            break;
        }
        case EWorkAction::Released:
        {
            ActionDescription = FText::FromString(TEXT("Предмет отпущен"));
            break;
        }
        case EWorkAction::Used:
        {
            ActionDescription = FText::FromString(TEXT("Предмет использован"));
            break;
        }
        case EWorkAction::Placed:
        {
            ActionDescription = FText::FromString(TEXT("Предмет установлен"));
            break;
        }
    }
    LastAction = FText::Format(FText::FromString(TEXT("{0}: {1}")), ActionDescription, Item->DisplayName);
    OnScenarioChanged.Broadcast();
}

void AScenarioDirector::ProcessOperationChanged(UWorkOperationComponent* Operation)
{
    if (bRefreshing)
    {
        return;
    }
    RefreshAvailability();
    OnScenarioChanged.Broadcast();
}

void AScenarioDirector::RefreshAvailability()
{
    TGuardValue<bool> RefreshGuard(bRefreshing, true);
    for (const FScenarioStepDefinition& Step : Steps)
    {
        if (!Step.Operation)
        {
            continue;
        }
        bool bRequirementsMet = true;
        for (const FName RequiredStep : Step.RequiredSteps)
        {
            const FScenarioStepDefinition* Dependency = Steps.FindByPredicate([RequiredStep](const FScenarioStepDefinition& Candidate)
            {
                return Candidate.StepIdentifier == RequiredStep;
            });
            if (!Dependency || !Dependency->Operation || Dependency->Operation->State != EWorkOperationState::Completed)
            {
                bRequirementsMet = false;
                break;
            }
        }
        Step.Operation->SetAvailable(bRequirementsMet);
    }
}

int32 AScenarioDirector::GetCompletedStepCount() const
{
    int32 CompletedCount = 0;
    for (const FScenarioStepDefinition& Step : Steps)
    {
        if (Step.Operation && Step.Operation->State == EWorkOperationState::Completed)
        {
            ++CompletedCount;
        }
    }
    return CompletedCount;
}

FText AScenarioDirector::GetCurrentInstruction() const
{
    for (const FScenarioStepDefinition& Step : Steps)
    {
        if (Step.Operation && (Step.Operation->State == EWorkOperationState::Ready || Step.Operation->State == EWorkOperationState::InProgress))
        {
            return Step.Instruction;
        }
    }
    if (!Steps.IsEmpty() && GetCompletedStepCount() == Steps.Num())
    {
        return FText::FromString(TEXT("Задание выполнено. Можно продолжить работу с предметами."));
    }
    return FText::FromString(TEXT("Нет доступных заданий."));
}
