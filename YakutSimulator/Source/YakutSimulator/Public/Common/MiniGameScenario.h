#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/WorldSubsystem.h"
#include "Components/StateTreeComponent.h"
#include "StateTreeTaskBase.h"
#include "Common/MiniGameDefinition.h"
#include "MiniGameScenario.generated.h"

class UInteractionTargetComponent;
class UItemReceiverComponent;
class UXRInteractorComponent;
class AGrabbableItem;
class AMiniGameScenario;

YAKUTSIMULATOR_API DECLARE_LOG_CATEGORY_EXTERN(LogMiniGame, Log, All);
YAKUTSIMULATOR_API DECLARE_LOG_CATEGORY_EXTERN(LogInteraction, Verbose, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMiniGameChanged);

UCLASS()
class YAKUTSIMULATOR_API UMiniGameWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(Transient) TObjectPtr<AMiniGameScenario> ActiveScenario;
    TArray<TWeakObjectPtr<UInteractionTargetComponent>> Targets;
    TArray<TWeakObjectPtr<UItemReceiverComponent>> Receivers;
};

UCLASS()
class YAKUTSIMULATOR_API UMiniGameStateTreeComponent : public UStateTreeComponent
{
    GENERATED_BODY()
public:
    bool bProcessing = false;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    void Dispatch(FGameplayTag Event);
};

USTRUCT()
struct FMiniGameObjectiveTaskData
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Parameter") FName ObjectiveId;
    UPROPERTY(EditAnywhere, Category="Parameter", meta=(ClampMin="0", Units="s")) float AutoCompleteSeconds = 0;
    UPROPERTY() float Elapsed = 0;
};

USTRUCT(meta=(DisplayName="Mini-game objective"))
struct YAKUTSIMULATOR_API FMiniGameObjectiveTask : public FStateTreeTaskCommonBase
{
    GENERATED_BODY()
    using FInstanceDataType = FMiniGameObjectiveTaskData;
    virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
    virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
    virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, float DeltaTime) const override;
};

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AMiniGameScenario : public AActor
{
    GENERATED_BODY()
public:
    AMiniGameScenario();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MiniGame") TObjectPtr<UMiniGameDefinition> Definition;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="MiniGame") bool bAutoStart = true;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="MiniGame") TObjectPtr<UMiniGameStateTreeComponent> StateTree;
    UPROPERTY(BlueprintReadOnly, Transient, Category="MiniGame") EMiniGameStatus Status = EMiniGameStatus::Inactive;
    UPROPERTY(BlueprintReadOnly, Transient, Category="MiniGame") FGuid RunId;
    UPROPERTY(BlueprintReadOnly, Transient, Category="MiniGame") FName ObjectiveId;
    UPROPERTY(BlueprintReadOnly, Transient, Category="MiniGame") float ElapsedSeconds = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="MiniGame") FText Feedback;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="MiniGame") TMap<FName, TObjectPtr<USceneComponent>> Workplaces;
    UPROPERTY(BlueprintAssignable, Category="MiniGame") FMiniGameChanged OnPresentationChanged;
    UFUNCTION(BlueprintCallable, Category="MiniGame") bool StartRun();
    UFUNCTION(BlueprintPure, Category="MiniGame") UMiniGameDefinition* ActiveDefinition() const { return RunDefinition ? RunDefinition.Get() : Definition.Get(); }
    UFUNCTION(BlueprintCallable, Category="MiniGame") void AbortRun();
    UFUNCTION(BlueprintCallable, Category="MiniGame") void CompleteRun();
    UFUNCTION(BlueprintCallable, Category="MiniGame") bool ActivateObjective(FName Id);
    UFUNCTION(BlueprintCallable, Category="MiniGame") void CompleteObjective();
    UFUNCTION(BlueprintCallable, Category="MiniGame") bool SubmitEvent(FGameplayTag Event, FGuid SourceRun);
    UFUNCTION(BlueprintCallable, Category="MiniGame") void PublishFeedback(FName MessageId, EMiniGameFeedback Severity = EMiniGameFeedback::Normal);
    UFUNCTION(BlueprintCallable, Category="MiniGame") void PublishInteractionFeedback(FText Message);
    UFUNCTION(BlueprintPure, Category="MiniGame") virtual bool IsActionAllowed(FName Action) const;
    UFUNCTION(BlueprintNativeEvent, Category="MiniGame") bool ExecuteAction(FName Action, AActor* Source);
    UFUNCTION(BlueprintNativeEvent, Category="MiniGame") bool TryReceive(UItemReceiverComponent* Receiver, AGrabbableItem* Item);
    UFUNCTION(BlueprintNativeEvent, Category="MiniGame") void RecoverItem(AGrabbableItem* Item);
    UFUNCTION(BlueprintNativeEvent, Category="MiniGame") FMiniGamePresentation GetPresentation(FName Panel, AActor* Source) const;
    UFUNCTION(BlueprintNativeEvent, Category="MiniGame") void OnRunReset();
    UFUNCTION(BlueprintNativeEvent, Category="MiniGame") void OnObjectiveEntered(FName Id);
    UFUNCTION(BlueprintNativeEvent, Category="MiniGame") void OnObjectiveExited(FName Id);
    virtual bool ValidateScene(TArray<FText>& Errors) const;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
    const FMiniGameObjective* CurrentObjective() const;
    void ExitObjective(FName Id);
    void MoveToWorkplace(FName Id);
    void NotifyPresentation();
    void ReleaseInteractions();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float DeltaSeconds) override;
protected:
    EMiniGameFeedback FeedbackSeverity = EMiniGameFeedback::Normal;
private:
    UPROPERTY(Transient) TObjectPtr<UMiniGameDefinition> RunDefinition;
    FTimerHandle TravelTimer;
    float UISeconds = 0;
};
