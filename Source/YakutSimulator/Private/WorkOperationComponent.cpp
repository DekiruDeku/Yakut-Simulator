#include "WorkOperationComponent.h"

#include "WorkItem.h"

void UWorkOperationComponent::SetAvailable(bool bAvailable)
{
    if (State == EWorkOperationState::Completed)
    {
        return;
    }
    if (bAvailable && State == EWorkOperationState::Locked)
    {
        State = EWorkOperationState::Ready;
        OnOperationChanged.Broadcast(this);
    }
    else if (!bAvailable && State != EWorkOperationState::Locked)
    {
        CurrentCount = 0;
        State = EWorkOperationState::Locked;
        OnOperationChanged.Broadcast(this);
    }
}

bool UWorkOperationComponent::TryBeginOperation(AWorkItem* Item)
{
    if (State != EWorkOperationState::Ready || (!Item && !bUseExplicitResults))
    {
        return false;
    }
    if (!RequiredItemType.IsNone() && (!Item || Item->ItemType != RequiredItemType))
    {
        return false;
    }
    State = EWorkOperationState::InProgress;
    OnOperationChanged.Broadcast(this);
    return true;
}

void UWorkOperationComponent::ObserveAction(AWorkItem* Item, EWorkAction Action)
{
    if (bUseExplicitResults || Action != RequiredAction || !Item)
    {
        return;
    }
    if (!RequiredItemType.IsNone() && Item->ItemType != RequiredItemType)
    {
        return;
    }
    if (State == EWorkOperationState::Ready)
    {
        TryBeginOperation(Item);
    }
    if (State != EWorkOperationState::InProgress)
    {
        return;
    }
    ++CurrentCount;
    if (CurrentCount >= FMath::Max(1, RequiredCount))
    {
        CompleteOperation();
    }
    else
    {
        OnOperationChanged.Broadcast(this);
    }
}

bool UWorkOperationComponent::AdvanceProgress(int32 Amount)
{
    if (!bUseExplicitResults || Amount <= 0)
    {
        return false;
    }
    if (State == EWorkOperationState::Ready)
    {
        TryBeginOperation();
    }
    if (State != EWorkOperationState::InProgress)
    {
        return false;
    }
    CurrentCount = FMath::Min(CurrentCount + Amount, RequiredCount);
    if (CurrentCount >= RequiredCount)
    {
        CompleteOperation();
    }
    else
    {
        OnOperationChanged.Broadcast(this);
    }
    return true;
}

void UWorkOperationComponent::CancelOperation()
{
    if (State != EWorkOperationState::InProgress)
    {
        return;
    }
    CurrentCount = 0;
    State = EWorkOperationState::Ready;
    OnOperationChanged.Broadcast(this);
}

void UWorkOperationComponent::CompleteOperation()
{
    if (State != EWorkOperationState::InProgress)
    {
        return;
    }
    State = EWorkOperationState::Completed;
    OnOperationChanged.Broadcast(this);
}

void UWorkOperationComponent::ResetOperation()
{
    State = EWorkOperationState::Locked;
    CurrentCount = 0;
    OnOperationChanged.Broadcast(this);
}
