#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Common/Interaction.h"
#include "GrabbableItem.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class UMiniGameDisplayComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AGrabbableItem : public AActor, public IMiniGameInteractable
{
    GENERATED_BODY()
public:
    AGrabbableItem();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UBoxComponent> Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<USceneComponent> ToolTip;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UStaticMeshComponent> ToolTipVisual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UGrabInteractionComponent> Grab;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Item") TObjectPtr<UMiniGameDisplayComponent> Display;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FText DisplayName;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Item") TObjectPtr<AMiniGameScenario> Scenario;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Item") FGuid SourceRun;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Recovery") TObjectPtr<USceneComponent> Home;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery") bool bRecoverOnTimeout = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery") bool bRecoverBelowHome = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery", meta=(ClampMin=".1", Units="s")) float RecoverySeconds = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recovery", meta=(ClampMin="0", Units="cm")) float RecoveryDropDistance = 150;
    UFUNCTION(BlueprintPure, Category="Item") bool IsHeld() const;
    UFUNCTION(BlueprintCallable, Category="Item") void Park(const FTransform& Transform, bool bHideItem = false);
    UFUNCTION(BlueprintCallable, Category="Item") void ReturnHome();
    bool TryReceivers();
    virtual bool CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const override;
    virtual void InteractionStarted_Implementation(const FInteractionContext& Context) override;
    virtual void InteractionEnded_Implementation(const FInteractionContext& Context) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ValidateItem(TArray<FText>& Errors) const;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
protected:
    float LooseSeconds = 0;
};

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AMiniGameStation : public AActor
{
    GENERATED_BODY()
public:
    AMiniGameStation();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<UBoxComponent> Zone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<UBoxComponent> PressZone;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<UStaticMeshComponent> Mesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<UStaticMeshComponent> Handle;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<UPressableInteractionComponent> Button;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<ULeverInteractionComponent> Lever;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<UItemReceiverComponent> Receiver;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Station") TObjectPtr<UMiniGameDisplayComponent> Display;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Station") TObjectPtr<AMiniGameScenario> Scenario;
    bool ValidateStation(TArray<FText>& Errors) const;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
    virtual void BeginPlay() override;
};
