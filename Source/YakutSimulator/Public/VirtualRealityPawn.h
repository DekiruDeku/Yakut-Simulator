#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "VirtualRealityPawn.generated.h"

class AWorkItem;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
class UMotionControllerComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class UWidgetInteractionComponent;

struct FHandMotionState
{
    FVector PreviousLocation = FVector::ZeroVector;
    FQuat PreviousRotation = FQuat::Identity;
    FVector LinearVelocity = FVector::ZeroVector;
    FVector AngularVelocity = FVector::ZeroVector;
    bool bInitialized = false;
};

UCLASS()
class YAKUTSIMULATOR_API AVirtualRealityPawn : public APawn
{
    GENERATED_BODY()

public:
    AVirtualRealityPawn();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tracking")
    TObjectPtr<USceneComponent> TrackingOrigin;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tracking")
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tracking")
    TObjectPtr<UMotionControllerComponent> LeftHand;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Tracking")
    TObjectPtr<UMotionControllerComponent> RightHand;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interface")
    TObjectPtr<UWidgetComponent> MenuDisplay;

    UPROPERTY(BlueprintReadOnly, Category="Interaction")
    TObjectPtr<AWorkItem> LeftItem;

    UPROPERTY(BlueprintReadOnly, Category="Interaction")
    TObjectPtr<AWorkItem> RightItem;

    UPROPERTY(BlueprintReadOnly, Category="Movement")
    TObjectPtr<UPrimitiveComponent> MovementBase;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Interaction")
    float GrabRadius = 30;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputMappingContext> InteractionMapping;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> GrabLeftAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> GrabRightAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> UseLeftAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> UseRightAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> TeleportAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> SnapTurnAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> ToggleMenuAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> CancelAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> DesktopMoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> DesktopLookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input")
    TObjectPtr<UInputAction> TeleportHoldAction;

    UFUNCTION(BlueprintCallable, Category="Interaction")
    bool TryGrabItem(bool bLeftHand);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    void ReleaseItem(bool bLeftHand);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    void SetItemUse(bool bLeftHand, bool bUse);

    UFUNCTION(BlueprintCallable, Category="Interaction")
    void ReleaseAllItems();

    UFUNCTION(BlueprintCallable, Category="Interface")
    void ShowMenu(UUserWidget* Menu, bool bShow);

    UFUNCTION(BlueprintCallable, Category="Movement")
    void CancelTeleport();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY()
    TObjectPtr<UMotionControllerComponent> LeftAim;

    UPROPERTY()
    TObjectPtr<UMotionControllerComponent> RightAim;

    UPROPERTY()
    TObjectPtr<UWidgetInteractionComponent> LeftPointer;

    UPROPERTY()
    TObjectPtr<UWidgetInteractionComponent> RightPointer;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> LeftRay;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> RightRay;

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> TeleportMarker;

    FHandMotionState LeftMotion;
    FHandMotionState RightMotion;
    bool bLeftGripPressed = false;
    bool bRightGripPressed = false;
    bool bLeftUsePressed = false;
    bool bRightUsePressed = false;
    bool bLeftPointerPressed = false;
    bool bRightPointerPressed = false;
    bool bTurnReady = true;
    bool bTeleportAiming = false;
    bool bTeleportValid = false;
    bool bTeleportCancelled = false;
    FVector TeleportDestination = FVector::ZeroVector;
    FVector2D DesktopMovement = FVector2D::ZeroVector;
    FTransform PreviousBaseTransform;

    bool IsMenuOpen() const;
    bool IsVirtualRealityActive() const;
    void UpdateMotion(USceneComponent* Hand, FHandMotionState& Motion, float DeltaSeconds);
    void UpdatePointer(UWidgetInteractionComponent* Pointer, UStaticMeshComponent* Ray);
    void UpdateTeleport();
    void CommitTeleport();
    void UpdateMovementBase();
    void SetMovementBase(UPrimitiveComponent* Base);
    void MoveHeldItemsWithPlayer(const FTransform& PreviousPlayerTransform);
    void ProcessGrip(bool bLeftHand, float Value);
    void ProcessUse(bool bLeftHand, float Value);

    void GrabLeft(const FInputActionValue& Value);
    void GrabRight(const FInputActionValue& Value);
    void StopGrabLeft(const FInputActionValue& Value);
    void StopGrabRight(const FInputActionValue& Value);
    void UseLeft(const FInputActionValue& Value);
    void UseRight(const FInputActionValue& Value);
    void StopUseLeft(const FInputActionValue& Value);
    void StopUseRight(const FInputActionValue& Value);
    void AimTeleport(const FInputActionValue& Value);
    void FinishTeleport(const FInputActionValue& Value);
    void HoldTeleport(const FInputActionValue& Value);
    void Turn(const FInputActionValue& Value);
    void FinishTurn(const FInputActionValue& Value);
    void MoveDesktop(const FInputActionValue& Value);
    void StopMoveDesktop(const FInputActionValue& Value);
    void LookDesktop(const FInputActionValue& Value);
    void ToggleMenu();
    void CancelInteraction();
};
