#include "Common/Interaction.h"
#include "Common/MiniGameScenario.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "MotionControllerComponent.h"
#include "Misc/App.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UXRInteractorComponent::UXRInteractorComponent() { PrimaryComponentTick.bCanEverTick = true; }
void UXRInteractorComponent::BeginPlay()
{
    Super::BeginPlay();
    BindInput();
}
FInteractionContext UXRInteractorComponent::Context(bool Right) const
{
    FInteractionContext Result;
    Result.Player = Cast<APawn>(GetOwner());
    Result.Hand = Cast<USceneComponent>((Right ? RightHand : LeftHand).GetComponent(GetOwner()));
    Result.bRightHand = Right;
    if (GetWorld()) if (auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>()) Result.Scenario = Registry->ActiveScenario;
    if (Result.Scenario) Result.RunId = Result.Scenario->RunId;
    return Result;
}
void UXRInteractorComponent::BindInput()
{
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
    if (!Input || !Profile || BoundInput.Get() == Input) return;
    if (BoundInput.IsValid()) for (uint32 Id : Bindings) BoundInput->RemoveBindingByHandle(Id);
    Bindings.Empty();
    BoundInput = Input;
    UInputAction* Actions[] = {Profile->LeftGrab, Profile->LeftRelease, Profile->RightGrab, Profile->RightRelease, Profile->LeftUse, Profile->RightUse};
    if (Actions[0]) Bindings.Add(Input->BindAction(Actions[0], ETriggerEvent::Triggered, this, &UXRInteractorComponent::Grip, false).GetHandle());
    if (Actions[1]) Bindings.Add(Input->BindAction(Actions[1], ETriggerEvent::Triggered, this, &UXRInteractorComponent::ReleaseGrip, false).GetHandle());
    if (Actions[2]) Bindings.Add(Input->BindAction(Actions[2], ETriggerEvent::Triggered, this, &UXRInteractorComponent::Grip, true).GetHandle());
    if (Actions[3]) Bindings.Add(Input->BindAction(Actions[3], ETriggerEvent::Triggered, this, &UXRInteractorComponent::ReleaseGrip, true).GetHandle());
    if (Actions[4]) Bindings.Add(Input->BindAction(Actions[4], ETriggerEvent::Started, this, &UXRInteractorComponent::Use, false).GetHandle());
    if (Actions[5]) Bindings.Add(Input->BindAction(Actions[5], ETriggerEvent::Started, this, &UXRInteractorComponent::Use, true).GetHandle());
    if (Profile->MappingContext) if (auto* PC = Cast<APlayerController>(Pawn->GetController()))
        if (auto* Local = PC->GetLocalPlayer()) if (auto* Subsystem = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            Subsystem->AddMappingContext(Profile->MappingContext, Profile->MappingPriority);
    UE_LOG(LogInteraction, Log, TEXT("[Input.Ready] pawn=%s profile=%s bindings=%d"), *GetNameSafe(Pawn), *GetNameSafe(Profile), Bindings.Num());
}
UInteractionTargetComponent* UXRInteractorComponent::FindTarget(const FInteractionContext& Ctx, EInteractionAction Action, FText* Rejection) const
{
    if (!Ctx.Hand || !Ctx.Scenario || !Profile) return nullptr;
    if (bRequireTracking)
    {
        if (auto* Motion = Cast<UMotionControllerComponent>(Ctx.Hand)) if (!Motion->IsTracked()) return nullptr;
        if (FApp::UseVRFocus() && !FApp::HasVRFocus()) return nullptr;
    }
    auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>();
    if (!Registry) return nullptr;
    UInteractionTargetComponent* Best = nullptr;
    float BestDistance = TNumericLimits<float>::Max();
    float RejectedDistance = TNumericLimits<float>::Max();
    for (const auto& Weak : Registry->Targets)
    {
        UInteractionTargetComponent* Target = Weak.Get();
        if (!Target || !Target->bEnabled || Target->Scenario != Ctx.Scenario || !Target->Supports(Action)) continue;
        const float Distance = Target->DistanceToShape(Ctx.Hand->GetComponentLocation());
        if (!FMath::IsFinite(Distance) || Distance > Profile->Reach) continue;
        FText Reason;
        const bool Allowed = Target->CanInteract(Ctx, Action, Reason);
        UE_LOG(LogInteraction, Verbose, TEXT("[Target.Candidate] run=%s hand=%s actor=%s component=%s distance=%.1f allowed=%d reason=%s"),
            *Ctx.RunId.ToString(), Ctx.bRightHand ? TEXT("Right") : TEXT("Left"), *Target->GetOwner()->GetName(), *Target->GetName(), Distance, Allowed, *Reason.ToString());
        if (!Allowed)
        {
            if (Rejection && Distance <= Target->MaxHandDistance && Distance < RejectedDistance)
            { *Rejection = Reason; RejectedDistance = Distance; }
            continue;
        }
        if (!Best || Target->Priority > Best->Priority || (Target->Priority == Best->Priority && Distance < BestDistance))
        { Best = Target; BestDistance = Distance; }
    }
    UE_LOG(LogInteraction, Log, TEXT("[Target.Result] hand=%s action=%d target=%s reason=%s"), Ctx.bRightHand ? TEXT("Right") : TEXT("Left"), int32(Action),
        *GetNameSafe(Best), Best ? TEXT("Eligible") : TEXT("NoEligibleTarget"));
    return Best;
}
void UXRInteractorComponent::Grip(bool Right)
{
    if (GripDown[Right]) return;
    GripDown[Right] = true;
    UE_LOG(LogInteraction, Log, TEXT("[Input.Action] action=Grip event=Triggered hand=%s"), Right ? TEXT("Right") : TEXT("Left"));
    auto& Held = Right ? HeldRight : HeldLeft;
    if (IsValid(Held) && Held->GetHoldingHand()) return;
    Held = nullptr;
    const auto Ctx = Context(Right);
    FText Rejection;
    if (auto* Target = FindTarget(Ctx, EInteractionAction::Grip, &Rejection))
    {
        if (Target->BeginInteraction(Ctx) && Target->GetHoldingHand()) Held = Target;
    }
    else if (Ctx.Scenario) Ctx.Scenario->PublishInteractionFeedback(Rejection);
    if (Ctx.Scenario) Ctx.Scenario->NotifyPresentation();
}
void UXRInteractorComponent::ReleaseGrip(bool Right)
{
    GripDown[Right] = false;
    UE_LOG(LogInteraction, Log, TEXT("[Input.Action] action=ReleaseGrip event=Triggered hand=%s"), Right ? TEXT("Right") : TEXT("Left"));
    auto& Held = Right ? HeldRight : HeldLeft;
    if (IsValid(Held)) Held->EndInteraction(Context(Right));
    Held = nullptr;
}
void UXRInteractorComponent::Use(bool Right)
{
    UE_LOG(LogInteraction, Log, TEXT("[Input.Action] action=Use event=Started hand=%s"), Right ? TEXT("Right") : TEXT("Left"));
    const auto Ctx = Context(Right);
    UE_LOG(LogInteraction, Log, TEXT("[Interaction.Use] run=%s hand=%s"), *Ctx.RunId.ToString(), Right ? TEXT("Right") : TEXT("Left"));
    FText Rejection;
    if (auto* Target = FindTarget(Ctx, EInteractionAction::Use, &Rejection)) Target->Use(Ctx);
    else if (Ctx.Scenario) Ctx.Scenario->PublishInteractionFeedback(Rejection);
}
void UXRInteractorComponent::ReleaseAll() { ReleaseGrip(false); ReleaseGrip(true); }
void UXRInteractorComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    BindInput();
    for (bool Right : {false, true})
    {
        auto& Held = Right ? HeldRight : HeldLeft;
        const auto Ctx = Context(Right);
        auto* Motion = Cast<UMotionControllerComponent>(Ctx.Hand);
        if (IsValid(Held) && (!Ctx.Hand || !Held->ValidContext(Ctx) || Held->GetHoldingHand() != Ctx.Hand ||
            (bRequireTracking && ((Motion && !Motion->IsTracked()) || (FApp::UseVRFocus() && !FApp::HasVRFocus())))))
        {
            UE_LOG(LogInteraction, Log, TEXT("[Interaction.AutoRelease] hand=%s"), Right ? TEXT("Right") : TEXT("Left"));
            ReleaseGrip(Right);
        }
    }
    if (!Profile || !UE_LOG_ACTIVE(LogInteraction, Verbose)) return;
    auto* Pawn = Cast<APawn>(GetOwner());
    auto* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    const auto* PlayerInput = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
    if (PlayerInput)
    {
        const TArray<const UInputAction*> Actions{Profile->LeftGrab, Profile->LeftRelease, Profile->RightGrab, Profile->RightRelease, Profile->LeftUse, Profile->RightUse};
        for (const auto& Mapping : PlayerInput->GetEnhancedActionMappingsView())
        {
            if (!Actions.Contains(Mapping.Action.Get())) continue;
            const float Value = PlayerInput->GetKeyValue(Mapping.Key);
            const float* Previous = LastRaw.Find(Mapping.Key.GetFName());
            if (!Previous || FMath::Abs(Value - *Previous) >= Profile->RawInputLogThreshold)
            {
                UE_LOG(LogInteraction, Log, TEXT("[Input.RawChanged] key=%s value=%.3f"), *Mapping.Key.ToString(), Value);
                LastRaw.Add(Mapping.Key.GetFName(), Value);
            }
        }
    }
    DiagnosticSeconds += Delta;
    if (DiagnosticSeconds >= Profile->DiagnosticInterval) { DiagnosticSeconds = 0; LogDiagnostics(); }
}
void UXRInteractorComponent::LogDiagnostics() const
{
    for (bool Right : {false, true})
    {
        const auto Ctx = Context(Right);
        const auto* Motion = Cast<UMotionControllerComponent>(Ctx.Hand);
        const auto* Held = Right ? HeldRight.Get() : HeldLeft.Get();
        UE_LOG(LogInteraction, Display, TEXT("[Snapshot.Hand] pawn=%s run=%s hand=%s tracked=%d position=%s held=%s focus=%d"),
            *GetNameSafe(GetOwner()), *Ctx.RunId.ToString(), Right ? TEXT("Right") : TEXT("Left"), Motion && Motion->IsTracked(),
            Ctx.Hand ? *Ctx.Hand->GetComponentLocation().ToCompactString() : TEXT("None"), *GetNameSafe(Held), FApp::HasVRFocus());
    }
}
void UXRInteractorComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseAll();
    if (BoundInput.IsValid()) for (uint32 Id : Bindings) BoundInput->RemoveBindingByHandle(Id);
    if (Profile && Profile->MappingContext) if (auto* Pawn = Cast<APawn>(GetOwner())) if (auto* PC = Cast<APlayerController>(Pawn->GetController()))
        if (auto* Local = PC->GetLocalPlayer()) if (auto* Subsystem = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()) Subsystem->RemoveMappingContext(Profile->MappingContext);
    Super::EndPlay(Reason);
}
