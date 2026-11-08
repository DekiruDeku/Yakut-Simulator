#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/ActorComponent.h"
#include "Common/MiniGameDefinition.h"
#include "Engine/EngineTypes.h"
#include "UObject/Interface.h"
#include "Interaction.generated.h"

class AMiniGameScenario;
class UPrimitiveComponent;
class UEnhancedInputComponent;
class AGrabbableItem;

USTRUCT(BlueprintType)
struct FInteractionContext
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) TObjectPtr<APawn> Player;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<USceneComponent> Hand;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AMiniGameScenario> Scenario;
    UPROPERTY(BlueprintReadOnly) FGuid RunId;
    UPROPERTY(BlueprintReadOnly) bool bRightHand = false;
};

UINTERFACE(BlueprintType)
class YAKUTSIMULATOR_API UMiniGameInteractable : public UInterface { GENERATED_BODY() };
class YAKUTSIMULATOR_API IMiniGameInteractable
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, Category="Interaction") bool CanInteract(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const;
    UFUNCTION(BlueprintNativeEvent, Category="Interaction") void InteractionStarted(const FInteractionContext& Context);
    UFUNCTION(BlueprintNativeEvent, Category="Interaction") void InteractionEnded(const FInteractionContext& Context);
};

UCLASS(Blueprintable, ClassGroup=(MiniGame), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UInteractionTargetComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UInteractionTargetComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") bool bEnabled = true;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Interaction") TObjectPtr<AMiniGameScenario> Scenario;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.PrimitiveComponent")) FComponentReference Shape;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") int32 Priority = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction", meta=(ClampMin="0", Units="cm")) float MaxHandDistance = 22;
    UFUNCTION(BlueprintNativeEvent, Category="Interaction") bool CanInteract(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const;
    virtual bool BeginInteraction(const FInteractionContext& Context);
    virtual void EndInteraction(const FInteractionContext& Context);
    virtual bool Use(const FInteractionContext& Context);
    virtual bool Supports(EInteractionAction Action) const { return false; }
    virtual USceneComponent* GetHoldingHand() const { return nullptr; }
    float DistanceToShape(const FVector& Position) const;
    UPrimitiveComponent* GetShape() const;
    bool ValidContext(const FInteractionContext& Context) const;
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
};

UCLASS(Blueprintable, ClassGroup=(MiniGame), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UPressableInteractionComponent : public UInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UPressableInteractionComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction") FName ActionId;
    virtual bool Supports(EInteractionAction Action) const override { return true; }
    virtual bool CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const override;
    virtual bool BeginInteraction(const FInteractionContext& Context) override { return Use(Context); }
    virtual bool Use(const FInteractionContext& Context) override;
};

UCLASS(Blueprintable, ClassGroup=(MiniGame), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UGrabInteractionComponent : public UInteractionTargetComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grab", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.SceneComponent")) FComponentReference MoveComponent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grab") FName Socket;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grab") bool bSnapToHand = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grab") bool bAttachOnGrab = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grab") bool bSimulateOnDrop = true;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Grab") FInteractionContext HoldingContext;
    virtual bool Supports(EInteractionAction Action) const override { return Action == EInteractionAction::Grip; }
    virtual bool CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const override;
    virtual bool BeginInteraction(const FInteractionContext& Context) override;
    virtual void EndInteraction(const FInteractionContext& Context) override;
    virtual USceneComponent* GetHoldingHand() const override { return HoldingContext.Hand; }
    UFUNCTION(BlueprintCallable, Category="Grab") void Release();
    USceneComponent* GetMoveComponent() const;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLeverValueChanged, float, Value);

UCLASS(Blueprintable, ClassGroup=(MiniGame), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API ULeverInteractionComponent : public UInteractionTargetComponent
{
    GENERATED_BODY()
public:
    ULeverInteractionComponent();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lever", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.SceneComponent")) FComponentReference Handle;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lever") FName ActionId;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lever") FVector LocalAxis = FVector::ForwardVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lever", meta=(ClampMin=".1", Units="cm")) float Travel = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lever", meta=(ClampMin="0", Units="cm")) float DeadZone = 2;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lever", meta=(ClampMin=".01")) float Acceleration = .75f;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Lever") float Value = 0;
    UPROPERTY(BlueprintAssignable, Category="Lever") FLeverValueChanged OnValueChanged;
    UPROPERTY(Transient, BlueprintReadOnly, Category="Lever") FInteractionContext HoldingContext;
    virtual bool Supports(EInteractionAction Action) const override { return Action == EInteractionAction::Grip; }
    virtual bool CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const override;
    virtual bool BeginInteraction(const FInteractionContext& Context) override;
    virtual void EndInteraction(const FInteractionContext& Context) override;
    virtual USceneComponent* GetHoldingHand() const override { return HoldingContext.Hand; }
    UFUNCTION(BlueprintCallable, Category="Lever") void Release();
    USceneComponent* GetHandle() const;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    FVector GrabLocal = FVector::ZeroVector;
    FTransform Home;
    TWeakObjectPtr<USceneComponent> HomeParent;
    void SetValue(float NewValue);
};

UCLASS(Blueprintable, ClassGroup=(MiniGame), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UItemReceiverComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Receiver") bool bEnabled = true;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Receiver") TObjectPtr<AMiniGameScenario> Scenario;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Receiver") FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Receiver", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.PrimitiveComponent")) FComponentReference Shape;
    UFUNCTION(BlueprintPure, Category="Receiver") bool Contains(const FVector& Position) const;
    UFUNCTION(BlueprintCallable, Category="Receiver") bool TryReceive(AGrabbableItem* Item);
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
};

UCLASS(ClassGroup=(MiniGame), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UXRInteractorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UXRInteractorComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction") TObjectPtr<UXRInteractionProfile> Profile;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.SceneComponent")) FComponentReference LeftHand;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Interaction", meta=(UseComponentPicker, AllowedClasses="/Script/Engine.SceneComponent")) FComponentReference RightHand;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction|Debug") bool bRequireTracking = true;
    UFUNCTION(BlueprintCallable, Category="Interaction") void Grip(bool bRight);
    UFUNCTION(BlueprintCallable, Category="Interaction") void ReleaseGrip(bool bRight);
    UFUNCTION(BlueprintCallable, Category="Interaction") void Use(bool bRight);
    UFUNCTION(BlueprintCallable, Category="Interaction") void ReleaseAll();
    UFUNCTION(BlueprintCallable, Category="Interaction|Debug") void LogDiagnostics() const;
    FInteractionContext Context(bool bRight) const;
    UInteractionTargetComponent* FindTarget(const FInteractionContext& Context, EInteractionAction Action, FText* Rejection = nullptr) const;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    UPROPERTY(Transient) TObjectPtr<UInteractionTargetComponent> HeldLeft;
    UPROPERTY(Transient) TObjectPtr<UInteractionTargetComponent> HeldRight;
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    TArray<uint32> Bindings;
    TMap<FName, float> LastRaw;
    float DiagnosticSeconds = 0;
    bool GripDown[2] = {false, false};
    void BindInput();
};
