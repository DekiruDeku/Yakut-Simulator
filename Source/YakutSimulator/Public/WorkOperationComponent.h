#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorkTypes.h"
#include "WorkOperationComponent.generated.h"

class AWorkItem;
class UWorkOperationComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWorkOperationChanged, UWorkOperationComponent*, Operation);

UCLASS(ClassGroup=(Scenario), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UWorkOperationComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Operation")
    EWorkAction RequiredAction = EWorkAction::Used;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Operation")
    FName RequiredItemType;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Operation", meta=(ClampMin="1"))
    int32 RequiredCount = 1;

    UPROPERTY(BlueprintReadOnly, Category="Operation")
    int32 CurrentCount = 0;

    UPROPERTY(BlueprintReadOnly, Category="Operation")
    EWorkOperationState State = EWorkOperationState::Locked;

    UPROPERTY(BlueprintAssignable, Category="Operation")
    FWorkOperationChanged OnOperationChanged;

    UFUNCTION(BlueprintCallable, Category="Operation")
    void SetAvailable(bool bAvailable);

    UFUNCTION(BlueprintCallable, Category="Operation")
    virtual bool TryBeginOperation(AWorkItem* Item);

    UFUNCTION(BlueprintCallable, Category="Operation")
    void ObserveAction(AWorkItem* Item, EWorkAction Action);

    UFUNCTION(BlueprintCallable, Category="Operation")
    void CancelOperation();

    UFUNCTION(BlueprintCallable, Category="Operation")
    void CompleteOperation();

    UFUNCTION(BlueprintCallable, Category="Operation")
    void ResetOperation();
};
