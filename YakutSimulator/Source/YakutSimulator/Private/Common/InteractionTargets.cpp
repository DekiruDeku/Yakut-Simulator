#include "Common/Interaction.h"
#include "Common/MiniGameScenario.h"
#include "Common/GrabbableItem.h"
#include "Components/PrimitiveComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"

UInteractionTargetComponent::UInteractionTargetComponent() { PrimaryComponentTick.bCanEverTick = false; }
void UInteractionTargetComponent::OnRegister()
{
    Super::OnRegister();
    if (GetWorld()) if (auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>()) Registry->Targets.AddUnique(this);
}
void UInteractionTargetComponent::OnUnregister()
{
    if (GetWorld()) if (auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>()) Registry->Targets.Remove(this);
    Super::OnUnregister();
}
UPrimitiveComponent* UInteractionTargetComponent::GetShape() const { return Cast<UPrimitiveComponent>(Shape.GetComponent(GetOwner())); }
float UInteractionTargetComponent::DistanceToShape(const FVector& Position) const
{
    const auto* Primitive = GetShape();
    if (!Primitive) return TNumericLimits<float>::Max();
    if (const auto* Box = Cast<UBoxComponent>(Primitive))
    {
        const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Position);
        const FVector Extent = Box->GetUnscaledBoxExtent();
        const FVector Closest(FMath::Clamp(Local.X, -Extent.X, Extent.X), FMath::Clamp(Local.Y, -Extent.Y, Extent.Y), FMath::Clamp(Local.Z, -Extent.Z, Extent.Z));
        return FVector::Distance(Position, Box->GetComponentTransform().TransformPosition(Closest));
    }
    return FVector::Distance(Position, Primitive->Bounds.GetBox().GetClosestPointTo(Position));
}
bool UInteractionTargetComponent::ValidContext(const FInteractionContext& Context) const
{
    return IsValid(Scenario) && Scenario == Context.Scenario && Scenario->RunId == Context.RunId && IsValid(Context.Hand);
}
bool UInteractionTargetComponent::CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const
{
    if (!bEnabled || !Supports(Action) || !ValidContext(Context) || !GetShape() || GetOwner()->IsHidden() ||
        !GetOwner()->GetActorEnableCollision() || GetShape()->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
    { if (Scenario && Scenario->ActiveDefinition()) Reason = Scenario->ActiveDefinition()->Text(TEXT("ActionUnavailable")); return false; }
    if (!FMath::IsFinite(MaxHandDistance) || DistanceToShape(Context.Hand->GetComponentLocation()) > MaxHandDistance)
    { Reason = FText::FromName(TEXT("HandTooFarFromInteractionShape")); return false; }
    if (GetOwner()->GetClass()->ImplementsInterface(UMiniGameInteractable::StaticClass()))
        return IMiniGameInteractable::Execute_CanInteract(GetOwner(), Context, Action, Reason);
    return true;
}
bool UInteractionTargetComponent::BeginInteraction(const FInteractionContext&) { return false; }
void UInteractionTargetComponent::EndInteraction(const FInteractionContext&) {}
bool UInteractionTargetComponent::Use(const FInteractionContext&) { return false; }

UPressableInteractionComponent::UPressableInteractionComponent() { MaxHandDistance = 8; }
bool UPressableInteractionComponent::CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const
{
    if (!Super::CanInteract_Implementation(Context, Action, Reason)) return false;
    if (!Scenario->IsActionAllowed(ActionId)) { Reason = Scenario->ActiveDefinition()->Text(TEXT("ActionUnavailable")); return false; }
    return true;
}
bool UPressableInteractionComponent::Use(const FInteractionContext& Context)
{
    FText Reason;
    if (!CanInteract(Context, EInteractionAction::Use, Reason)) return false;
    const bool Accepted = Scenario->ExecuteAction(ActionId, GetOwner());
    UE_LOG(LogInteraction, Log, TEXT("[Button.Use] run=%s actor=%s action=%s accepted=%d"), *Context.RunId.ToString(), *GetNameSafe(GetOwner()), *ActionId.ToString(), Accepted);
    return Accepted;
}

USceneComponent* UGrabInteractionComponent::GetMoveComponent() const { return Cast<USceneComponent>(MoveComponent.GetComponent(GetOwner())); }
bool UGrabInteractionComponent::CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const
{
    if (!Super::CanInteract_Implementation(Context, Action, Reason)) return false;
    const bool Allowed = !IsValid(HoldingContext.Hand) && GetMoveComponent() && Scenario->Status == EMiniGameStatus::Running;
    if (!Allowed) Reason = Scenario->ActiveDefinition()->Text(TEXT("ActionUnavailable"));
    return Allowed;
}
bool UGrabInteractionComponent::BeginInteraction(const FInteractionContext& Context)
{
    FText Reason;
    if (!CanInteract(Context, EInteractionAction::Grip, Reason)) return false;
    auto* Move = GetMoveComponent();
    if (auto* Primitive = Cast<UPrimitiveComponent>(Move)) Primitive->SetSimulatePhysics(false);
    if (bAttachOnGrab && !Move->AttachToComponent(Context.Hand, bSnapToHand ? FAttachmentTransformRules::SnapToTargetNotIncludingScale : FAttachmentTransformRules::KeepWorldTransform, Socket)) return false;
    HoldingContext = Context;
    if (GetOwner()->GetClass()->ImplementsInterface(UMiniGameInteractable::StaticClass())) IMiniGameInteractable::Execute_InteractionStarted(GetOwner(), Context);
    UE_LOG(LogInteraction, Log, TEXT("[Grab.Begin] run=%s actor=%s hand=%s attached=%d"), *Context.RunId.ToString(), *GetOwner()->GetName(), *GetNameSafe(Context.Hand), bAttachOnGrab);
    return true;
}
void UGrabInteractionComponent::EndInteraction(const FInteractionContext& Context) { if (HoldingContext.Hand == Context.Hand) Release(); }
void UGrabInteractionComponent::Release()
{
    if (!HoldingContext.Hand) return;
    const auto Old = HoldingContext;
    const bool Attached = bAttachOnGrab;
    HoldingContext = {};
    USceneComponent* Move = GetMoveComponent();
    if (Attached && Move) Move->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    if (GetOwner()->GetClass()->ImplementsInterface(UMiniGameInteractable::StaticClass())) IMiniGameInteractable::Execute_InteractionEnded(GetOwner(), Old);
    bool Handled = false;
    if (Attached) if (auto* Item = Cast<AGrabbableItem>(GetOwner())) Handled = Item->TryReceivers();
    if (Attached && !Handled && bSimulateOnDrop) if (auto* Primitive = Cast<UPrimitiveComponent>(Move))
    {
        Primitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Primitive->SetSimulatePhysics(true);
    }
    UE_LOG(LogInteraction, Log, TEXT("[Grab.End] run=%s actor=%s receiverHandled=%d"), *Old.RunId.ToString(), *GetOwner()->GetName(), Handled);
}
void UGrabInteractionComponent::EndPlay(const EEndPlayReason::Type Reason) { HoldingContext = {}; Super::EndPlay(Reason); }

ULeverInteractionComponent::ULeverInteractionComponent() { PrimaryComponentTick.bCanEverTick = true; }
USceneComponent* ULeverInteractionComponent::GetHandle() const { return Cast<USceneComponent>(Handle.GetComponent(GetOwner())); }
bool ULeverInteractionComponent::CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction Action, FText& Reason) const
{
    if (!Super::CanInteract_Implementation(Context, Action, Reason)) return false;
    const bool Allowed = !IsValid(HoldingContext.Hand) && GetHandle() && Scenario->IsActionAllowed(ActionId);
    if (!Allowed) Reason = Scenario->ActiveDefinition()->Text(TEXT("ActionUnavailable"));
    return Allowed;
}
bool ULeverInteractionComponent::BeginInteraction(const FInteractionContext& Context)
{
    FText Reason;
    if (!CanInteract(Context, EInteractionAction::Grip, Reason) || !FMath::IsFinite(Travel) || Travel <= 0 || LocalAxis.IsNearlyZero()) return false;
    USceneComponent* Visual = GetHandle();
    Home = Visual->GetRelativeTransform();
    HomeParent = Visual->GetAttachParent();
    if (!Visual->AttachToComponent(Context.Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale)) return false;
    HoldingContext = Context;
    GrabLocal = GetComponentTransform().InverseTransformPosition(Context.Hand->GetComponentLocation());
    AddTickPrerequisiteComponent(Context.Hand);
    SetValue(0);
    Scenario->NotifyPresentation();
    UE_LOG(LogInteraction, Log, TEXT("[Lever.Grabbed] run=%s actor=%s hand=%s"), *Context.RunId.ToString(), *GetOwner()->GetName(), *GetNameSafe(Context.Hand));
    return true;
}
void ULeverInteractionComponent::SetValue(float NewValue)
{
    NewValue = FMath::IsFinite(NewValue) ? FMath::Clamp(NewValue, 0.f, 1.f) : 0.f;
    if (FMath::IsNearlyEqual(Value, NewValue)) return;
    Value = NewValue;
    OnValueChanged.Broadcast(Value);
}
void ULeverInteractionComponent::EndInteraction(const FInteractionContext& Context) { if (HoldingContext.Hand == Context.Hand) Release(); }
void ULeverInteractionComponent::Release()
{
    if (!HoldingContext.Hand) { SetValue(0); return; }
    if (IsValid(HoldingContext.Hand)) RemoveTickPrerequisiteComponent(HoldingContext.Hand);
    HoldingContext = {};
    if (auto* Visual = GetHandle())
    {
        if (HomeParent.IsValid()) Visual->AttachToComponent(HomeParent.Get(), FAttachmentTransformRules::KeepRelativeTransform);
        else Visual->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        Visual->SetRelativeTransform(Home);
    }
    SetValue(0);
    if (IsValid(Scenario)) Scenario->NotifyPresentation();
    UE_LOG(LogInteraction, Log, TEXT("[Lever.Released] actor=%s"), *GetOwner()->GetName());
}
void ULeverInteractionComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    if (!HoldingContext.Hand) return;
    if (!ValidContext(HoldingContext) || !IsValid(HoldingContext.Hand) || !Scenario->IsActionAllowed(ActionId)) { Release(); return; }
    const FVector Local = GetComponentTransform().InverseTransformPosition(HoldingContext.Hand->GetComponentLocation());
    const float Displacement = FVector::DotProduct(Local - GrabLocal, LocalAxis.GetSafeNormal());
    const float Target = Travel > 0 && Displacement > DeadZone ? FMath::Clamp(Displacement / Travel, 0.f, 1.f) : 0.f;
    SetValue(FMath::Min(Target, Value + FMath::Max(Delta, 0.f) * FMath::Max(Acceleration, 0.f)));
}
void ULeverInteractionComponent::EndPlay(const EEndPlayReason::Type Reason) { HoldingContext = {}; Super::EndPlay(Reason); }

void UItemReceiverComponent::OnRegister()
{
    Super::OnRegister();
    if (GetWorld()) if (auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>()) Registry->Receivers.AddUnique(this);
}
void UItemReceiverComponent::OnUnregister()
{
    if (GetWorld()) if (auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>()) Registry->Receivers.Remove(this);
    Super::OnUnregister();
}
bool UItemReceiverComponent::Contains(const FVector& Position) const
{
    if (!bEnabled) return false;
    const auto* Primitive = Cast<UPrimitiveComponent>(Shape.GetComponent(GetOwner()));
    if (!Primitive || Primitive->GetCollisionEnabled() == ECollisionEnabled::NoCollision) return false;
    if (const auto* Box = Cast<UBoxComponent>(Primitive))
    {
        const FVector Local = Box->GetComponentTransform().InverseTransformPosition(Position);
        const FVector Extent = Box->GetUnscaledBoxExtent();
        return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
    }
    return Primitive->Bounds.GetBox().IsInsideOrOn(Position);
}
bool UItemReceiverComponent::TryReceive(AGrabbableItem* Item)
{
    if (!bEnabled || !IsValid(Scenario) || !IsValid(Item) || Item->Scenario != Scenario || Item->SourceRun != Scenario->RunId ||
        Scenario->Status != EMiniGameStatus::Running || Item->IsHeld() || !Contains(Item->GetActorLocation())) return false;
    return Scenario->TryReceive(this, Item);
}
