#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FishingInteractionComponent.generated.h"

class AFishingScenario;
class AFishingItem;
class AFishingStation;
class UInputAction;
class UEnhancedInputComponent;
struct FInputActionInstance;
enum class ETriggerEvent : uint8;

UCLASS(ClassGroup=(Fishing), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UFishingInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UFishingInteractionComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(BlueprintReadWrite, Category="Fishing") TObjectPtr<AFishingScenario> Scenario;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="5", ClampMax="40")) float ReachRadius = 22.f;
    UFUNCTION(BlueprintCallable, Category="Fishing") void Grip(bool bRightHand);
    UFUNCTION(BlueprintCallable, Category="Fishing") void ReleaseGrip(bool bRightHand);
    UFUNCTION(BlueprintCallable, Category="Fishing") void Use(bool bRightHand);
    UFUNCTION(BlueprintCallable, Category="Fishing|Debug") void LogDiagnostics() const;

private:
    UPROPERTY() TArray<TObjectPtr<UInputAction>> Actions;
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    TArray<uint32> BindingHandles;
    TWeakObjectPtr<USceneComponent> Hands[2];
    TWeakObjectPtr<AFishingItem> HeldItems[2];
    TWeakObjectPtr<AFishingStation> HeldLevers[2];
    TMap<FName, float> LastRawInputValues;
    FString LastReadiness;
    float DiagnosticSeconds = 5.f;
    void BindInput();
    void LogInputEvent(const FInputActionInstance& Instance, ETriggerEvent Event, bool bRightHand);
    void UpdateDiagnostics(float DeltaTime);
    AActor* FindTarget(int32 HandIndex, bool bButtonsOnly) const;
};
