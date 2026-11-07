#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WorkTypes.h"
#include "WorkItem.generated.h"

class AWorkItem;
class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWorkItemAction, AWorkItem*, Item, EWorkAction, Action);

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AWorkItem : public AActor
{
    GENERATED_BODY()

public:
    AWorkItem();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
    TObjectPtr<UStaticMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
    TObjectPtr<USceneComponent> GripPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item")
    TObjectPtr<UPointLightComponent> UseIndicator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
    FName ItemType = "PracticeItem";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category="Item")
    bool bIsHeld = false;

    UPROPERTY(BlueprintReadOnly, Category="Item")
    bool bIsUsing = false;

    UPROPERTY(BlueprintReadOnly, Category="Item")
    int32 UseCount = 0;

    UPROPERTY(BlueprintAssignable, Category="Item")
    FWorkItemAction OnItemAction;

    UFUNCTION(BlueprintCallable, Category="Item")
    bool TryGrab(USceneComponent* Hand);

    UFUNCTION(BlueprintCallable, Category="Item")
    void Release(const FVector& LinearVelocity, const FVector& AngularVelocity);

    UFUNCTION(BlueprintCallable, Category="Item")
    void BeginUse();

    UFUNCTION(BlueprintCallable, Category="Item")
    void EndUse();

    UFUNCTION(BlueprintNativeEvent, Category="Item")
    void ReceiveUseStarted();

    UFUNCTION(BlueprintNativeEvent, Category="Item")
    void ReceiveUseEnded();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
