#include "ScenarioDirector.h"

#include "EngineUtils.h"
#include "Kismet/KismetSystemLibrary.h"
#include "WorkItem.h"
#include "WorkOperationComponent.h"

void AScenarioDirector::BeginPlay()
{
    Super::BeginPlay();
    FText ConfigurationError;
    if (!ValidateSteps(ConfigurationError))
    {
        UKismetSystemLibrary::PrintString(this, ConfigurationError.ToString(), true, true, FLinearColor::Red);
        return;
    }
    ActorSpawnSubscription = GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &AScenarioDirector::RegisterSpawnedActor));
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

void AScenarioDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorld()->RemoveOnActorSpawnedHandler(ActorSpawnSubscription);
    Super::EndPlay(EndPlayReason);
}

void AScenarioDirector::RegisterSpawnedActor(AActor* Actor)
{
    if (AWorkItem* Item = Cast<AWorkItem>(Actor))
    {
        RegisterItem(Item);
    }
}

bool AScenarioDirector::ValidateSteps(FText& Error) const
{
    TSet<FName> StepIdentifiers;
    TSet<UWorkOperationComponent*> Operations;
    for (const FScenarioStepDefinition& Step : Steps)
    {
        if (Step.StepIdentifier.IsNone() || StepIdentifiers.Contains(Step.StepIdentifier))
        {
            Error = FText::FromString(TEXT("Scenario step identifiers must be nonempty and unique"));
            return false;
        }
        if (!Step.Operation || Operations.Contains(Step.Operation))
        {
            Error = FText::FromString(TEXT("Each scenario step must have its own operation component"));
            return false;
        }
        StepIdentifiers.Add(Step.StepIdentifier);
        Operations.Add(Step.Operation);
    }
    for (const FScenarioStepDefinition& Step : Steps)
    {
        for (const FName RequiredStep : Step.RequiredSteps)
        {
            if (!StepIdentifiers.Contains(RequiredStep))
            {
                Error = FText::FromString(FString::Format(TEXT("Scenario dependency does not exist: {0}"), { RequiredStep.ToString() }));
                return false;
            }
        }
    }
    TSet<FName> ResolvedSteps;
    bool bMadeProgress = true;
    while (bMadeProgress)
    {
        bMadeProgress = false;
        for (const FScenarioStepDefinition& Step : Steps)
        {
            if (ResolvedSteps.Contains(Step.StepIdentifier))
            {
                continue;
            }
            bool bDependenciesResolved = true;
            for (const FName RequiredStep : Step.RequiredSteps)
            {
                if (!ResolvedSteps.Contains(RequiredStep))
                {
                    bDependenciesResolved = false;
                    break;
                }
            }
            if (bDependenciesResolved)
            {
                ResolvedSteps.Add(Step.StepIdentifier);
                bMadeProgress = true;
            }
        }
    }
    if (ResolvedSteps.Num() != Steps.Num())
    {
        Error = FText::FromString(TEXT("Scenario dependencies contain a cycle"));
        return false;
    }
    Error = FText::GetEmpty();
    return true;
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
        return FText::FromString(TEXT("Завершено."));
    }
    return FText::GetEmpty();
}
