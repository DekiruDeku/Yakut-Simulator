#include "Fishing/FishingInteractionComponent.h"
#include "Fishing/FishingScenario.h"
#include "Fishing/FishingItem.h"
#include "Fishing/FishingStation.h"
#include "Fishing/FishingLog.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedActionKeyMapping.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "Misc/App.h"
#include "MotionControllerComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

UFishingInteractionComponent::UFishingInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    const TCHAR* Names[] = { TEXT("IA_Grab_Left_Pressed"), TEXT("IA_Grab_Left_Released"), TEXT("IA_Grab_Right_Pressed"),
        TEXT("IA_Grab_Right_Released"), TEXT("IA_Shoot_Left"), TEXT("IA_Shoot_Right") };
    for (const TCHAR* Name : Names)
    {
        ConstructorHelpers::FObjectFinder<UInputAction> Action(*FString::Printf(TEXT("/Game/XRFramework/Input/Actions/%s.%s"), Name, Name));
        Actions.Add(Action.Object);
    }
}

void UFishingInteractionComponent::BindInput()
{
    APawn* Pawn = Cast<APawn>(GetOwner());
    UEnhancedInputComponent* Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
    if (!Input || BoundInput.Get() == Input) return;
    if (BoundInput.IsValid()) for (uint32 Handle : BindingHandles) BoundInput->RemoveBindingByHandle(Handle);
    BindingHandles.Empty();
    BoundInput = Input;
    for (int32 Index = 0; Index < Actions.Num(); ++Index)
    {
        if (!Actions[Index])
        {
            UE_LOG(LogFishing, Error, TEXT("[Input.BindingMissing] pawn=%s actionIndex=%d reason=ActionAssetNotLoaded"), *GetNameSafe(Pawn), Index);
            continue;
        }
        for (ETriggerEvent Event : {ETriggerEvent::Started, ETriggerEvent::Completed, ETriggerEvent::Canceled})
            BindingHandles.Add(Input->BindAction(Actions[Index], Event, this, &UFishingInteractionComponent::LogInputEvent,
                Event, Index == 2 || Index == 3 || Index == 5).GetHandle());
        UE_LOG(LogFishing, Log, TEXT("[Input.Bound] pawn=%s input=%s action=%s event=%s"), *GetNameSafe(Pawn), *GetNameSafe(Input),
            *GetPathNameSafe(Actions[Index]), Index >= 4 ? TEXT("Started") : TEXT("Triggered"));
    }
    if (Actions[0]) BindingHandles.Add(Input->BindAction(Actions[0], ETriggerEvent::Triggered, this, &UFishingInteractionComponent::Grip, false).GetHandle());
    if (Actions[1]) BindingHandles.Add(Input->BindAction(Actions[1], ETriggerEvent::Triggered, this, &UFishingInteractionComponent::ReleaseGrip, false).GetHandle());
    if (Actions[2]) BindingHandles.Add(Input->BindAction(Actions[2], ETriggerEvent::Triggered, this, &UFishingInteractionComponent::Grip, true).GetHandle());
    if (Actions[3]) BindingHandles.Add(Input->BindAction(Actions[3], ETriggerEvent::Triggered, this, &UFishingInteractionComponent::ReleaseGrip, true).GetHandle());
    // Buttons perform one operation per press; a held trigger must not repeat safety/transition requests every frame.
    if (Actions[4]) BindingHandles.Add(Input->BindAction(Actions[4], ETriggerEvent::Started, this, &UFishingInteractionComponent::Use, false).GetHandle());
    if (Actions[5]) BindingHandles.Add(Input->BindAction(Actions[5], ETriggerEvent::Started, this, &UFishingInteractionComponent::Use, true).GetHandle());
    UE_LOG(LogFishing, Log, TEXT("[Input.Ready] pawn=%s component=%s bindings=%d scenario=%s"),
        *GetNameSafe(Pawn), *GetName(), BindingHandles.Num(), *GetNameSafe(Scenario));
}

void UFishingInteractionComponent::LogInputEvent(const FInputActionInstance& Instance, ETriggerEvent Event, bool bRightHand)
{
    UE_LOG(LogFishing, Log, TEXT("[Input.Action] pawn=%s hand=%s action=%s event=%s evaluatedValue=%s elapsed=%.3f"),
        *GetNameSafe(GetOwner()), bRightHand ? TEXT("Right") : TEXT("Left"), *GetNameSafe(Instance.GetSourceAction()),
        *UEnum::GetValueAsString(Event), *Instance.GetValue().ToString(), Instance.GetElapsedTime());
}

void UFishingInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    BindInput();
    if (!Hands[0].IsValid() || !Hands[1].IsValid())
    {
        TArray<UMotionControllerComponent*> Controllers;
        GetOwner()->GetComponents(Controllers);
        for (UMotionControllerComponent* Controller : Controllers)
        {
            if (Controller->MotionSource == TEXT("LeftGrip") || Controller->MotionSource == TEXT("Left")) Hands[0] = Controller;
            if (Controller->MotionSource == TEXT("RightGrip") || Controller->MotionSource == TEXT("Right")) Hands[1] = Controller;
        }
    }
    for (int32 Index = 0; Index < 2; ++Index)
    {
        UMotionControllerComponent* Controller = Cast<UMotionControllerComponent>(Hands[Index].Get());
        if (!Controller || !Controller->IsTracked() || (FApp::UseVRFocus() && !FApp::HasVRFocus()) ||
            !IsValid(Scenario) || Scenario->Stage == EFishingStage::Complete)
        {
            if (HeldItems[Index].IsValid() || HeldLevers[Index].IsValid())
                UE_LOG(LogFishing, Log, TEXT("[Interaction.AutoRelease] hand=%s reason=%s"), Index ? TEXT("Right") : TEXT("Left"),
                    !Controller ? TEXT("MissingHand") : !Controller->IsTracked() ? TEXT("TrackingLost") :
                    FApp::UseVRFocus() && !FApp::HasVRFocus() ? TEXT("VRFocusLost") : !IsValid(Scenario) ? TEXT("MissingScenario") : TEXT("ShiftComplete"));
            ReleaseGrip(Index == 1);
        }
        else if (HeldItems[Index].IsValid() && HeldItems[Index]->GetHoldingHand() != Hands[Index].Get())
        {
            UE_LOG(LogFishing, Log, TEXT("[Interaction.OwnershipLost] hand=%s item=%s"), Index ? TEXT("Right") : TEXT("Left"), *GetNameSafe(HeldItems[Index].Get()));
            HeldItems[Index].Reset();
        }
        else if (HeldLevers[Index].IsValid() && HeldLevers[Index]->GetLeverHand() != Hands[Index].Get())
        {
            UE_LOG(LogFishing, Log, TEXT("[Interaction.OwnershipLost] hand=%s lever=%s"), Index ? TEXT("Right") : TEXT("Left"), *GetNameSafe(HeldLevers[Index].Get()));
            HeldLevers[Index].Reset();
        }
    }
    UpdateDiagnostics(DeltaTime);
}

void UFishingInteractionComponent::UpdateDiagnostics(float DeltaTime)
{
    if (!UE_LOG_ACTIVE(LogFishing, Verbose)) return;
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    const UEnhancedPlayerInput* PlayerInput = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
    const UMotionControllerComponent* Left = Cast<UMotionControllerComponent>(Hands[0].Get());
    const UMotionControllerComponent* Right = Cast<UMotionControllerComponent>(Hands[1].Get());
    const FString Readiness = FString::Printf(TEXT("pawn=%s controller=%s input=%s boundInput=%s scenario=%s left=%s leftTracked=%d right=%s rightTracked=%d useVRFocus=%d hasVRFocus=%d"),
        *GetNameSafe(Pawn), *GetNameSafe(PC), *GetNameSafe(PlayerInput), *GetNameSafe(BoundInput.Get()), *GetNameSafe(Scenario),
        *GetNameSafe(Left), Left && Left->IsTracked(), *GetNameSafe(Right), Right && Right->IsTracked(), FApp::UseVRFocus(), FApp::HasVRFocus());
    if (Readiness != LastReadiness)
    {
        UE_LOG(LogFishing, Log, TEXT("[Interaction.ReadinessChanged] %s"), *Readiness);
        LastReadiness = Readiness;
    }
    if (PlayerInput)
    {
        for (const FEnhancedActionKeyMapping& Mapping : PlayerInput->GetEnhancedActionMappingsView())
        {
            if (!Actions.Contains(Mapping.Action)) continue;
            const float Value = PlayerInput->GetKeyValue(Mapping.Key);
            const float* Previous = LastRawInputValues.Find(Mapping.Key.GetFName());
            if (!Previous || !FMath::IsNearlyEqual(*Previous, Value, .05f))
            {
                UE_LOG(LogFishing, Log, TEXT("[Input.RawChanged] pawn=%s key=%s previous=%.3f value=%.3f"),
                    *GetNameSafe(Pawn), *Mapping.Key.ToString(), Previous ? *Previous : 0.f, Value);
                LastRawInputValues.Add(Mapping.Key.GetFName(), Value);
            }
        }
    }
    DiagnosticSeconds += DeltaTime;
    if (DiagnosticSeconds >= 5.f) { DiagnosticSeconds = 0; LogDiagnostics(); }
}

void UFishingInteractionComponent::LogDiagnostics() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
    const UEnhancedPlayerInput* PlayerInput = PC ? Cast<UEnhancedPlayerInput>(PC->PlayerInput) : nullptr;
    const UCameraComponent* Camera = Pawn ? Pawn->FindComponentByClass<UCameraComponent>() : nullptr;
    UE_LOG(LogFishing, Display, TEXT("[Snapshot.Interaction] world=%s pawn=%s local=%d controller=%s playerInput=%s boundInput=%s bindings=%d scenario=%s stage=%s equipped=%d stopTested=%d reachCm=%.1f useVRFocus=%d hasVRFocus=%d head=%s"),
        *GetNameSafe(GetWorld()), *GetNameSafe(Pawn), Pawn && Pawn->IsLocallyControlled(), *GetNameSafe(PC), *GetNameSafe(PlayerInput),
        *GetNameSafe(BoundInput.Get()), BindingHandles.Num(), *GetNameSafe(Scenario), IsValid(Scenario) ? *UEnum::GetValueAsString(Scenario->Stage) : TEXT("None"),
        IsValid(Scenario) && Scenario->bSafetyEquipped, IsValid(Scenario) && Scenario->bStopTested, ReachRadius, FApp::UseVRFocus(), FApp::HasVRFocus(),
        Camera ? *Camera->GetComponentLocation().ToCompactString() : TEXT("None"));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const UMotionControllerComponent* Hand = Cast<UMotionControllerComponent>(Hands[Index].Get());
        UE_LOG(LogFishing, Display, TEXT("[Snapshot.Hand] hand=%s source=%s tracked=%d position=%s rotation=%s heldItem=%s heldLever=%s"),
            Index ? TEXT("Right") : TEXT("Left"), Hand ? *Hand->MotionSource.ToString() : TEXT("Missing"), Hand && Hand->IsTracked(),
            Hand ? *Hand->GetComponentLocation().ToCompactString() : TEXT("None"), Hand ? *Hand->GetComponentRotation().ToCompactString() : TEXT("None"),
            *GetNameSafe(HeldItems[Index].Get()), *GetNameSafe(HeldLevers[Index].Get()));
        FindTarget(Index, true);
    }
    if (PlayerInput)
    {
        UE_LOG(LogFishing, Verbose, TEXT("[Snapshot.InputMode] mode=%s"), *PlayerInput->GetCurrentInputMode().ToStringSimple());
        for (const UInputAction* Action : Actions)
        {
            FString Keys;
            for (const FEnhancedActionKeyMapping& Mapping : PlayerInput->GetEnhancedActionMappingsView())
                if (Mapping.Action == Action) Keys += FString::Printf(TEXT("%s=%.3f;"), *Mapping.Key.ToString(), PlayerInput->GetKeyValue(Mapping.Key));
            const FInputActionInstance* Instance = Action ? PlayerInput->FindActionInstanceData(Action) : nullptr;
            UE_LOG(LogFishing, Verbose, TEXT("[Snapshot.Action] action=%s keys=[%s] event=%s value=%s consumeInput=%d"),
                *GetNameSafe(Action), *Keys, Instance ? *UEnum::GetValueAsString(Instance->GetTriggerEvent()) : TEXT("NoInstance"),
                Instance ? *Instance->GetValue().ToString() : TEXT("None"), Action && Action->bConsumeInput);
        }
    }
}

AActor* UFishingInteractionComponent::FindTarget(int32 HandIndex, bool bButtonsOnly) const
{
    if (!Hands[HandIndex].IsValid() || !IsValid(Scenario))
    {
        UE_LOG(LogFishing, Log, TEXT("[Target.Unavailable] hand=%s handComponent=%s scenario=%s reason=%s"), HandIndex ? TEXT("Right") : TEXT("Left"),
            *GetNameSafe(Hands[HandIndex].Get()), *GetNameSafe(Scenario), !Hands[HandIndex].IsValid() ? TEXT("MissingHand") : TEXT("MissingScenario"));
        return nullptr;
    }
    const FVector Position = Hands[HandIndex]->GetComponentLocation();
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(FishingReach), false, GetOwner());
    const float Radius = FMath::IsFinite(ReachRadius) ? FMath::Clamp(ReachRadius, 5.f, 40.f) : 22.f;
    GetWorld()->OverlapMultiByObjectType(Overlaps, Position, FQuat::Identity,
        FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllDynamicObjects), FCollisionShape::MakeSphere(Radius), Query);
    AActor* Best = nullptr;
    float BestDistance = TNumericLimits<float>::Max();
    UE_LOG(LogFishing, Verbose, TEXT("[Target.Query] hand=%s position=%s radiusCm=%.1f buttonsOnly=%d overlaps=%d"),
        HandIndex ? TEXT("Right") : TEXT("Left"), *Position.ToCompactString(), Radius, bButtonsOnly, Overlaps.Num());
    for (const FOverlapResult& Hit : Overlaps)
    {
        AActor* Actor = Hit.GetActor();
        AFishingItem* Item = Cast<AFishingItem>(Actor);
        AFishingStation* Station = Cast<AFishingStation>(Actor);
        const float ButtonDistance = Station && Station->IsButton()
            ? FVector::Distance(Position, Station->Mesh->Bounds.GetBox().GetClosestPointTo(Position)) : 0.f;
        const bool bButtonInReach = ButtonDistance <= 8.f;
        const TCHAR* Reason = TEXT("Eligible");
        if (Item)
        {
            if (bButtonsOnly) Reason = TEXT("ButtonsOnly");
            else if (Item->Scenario != Scenario) Reason = TEXT("DifferentScenario");
            else if (Item->IsHeld()) Reason = TEXT("AlreadyHeld");
            else if (Item->IsHidden()) Reason = TEXT("Hidden");
        }
        else if (Station)
        {
            if (Station->Scenario != Scenario) Reason = TEXT("DifferentScenario");
            else if (!bButtonInReach) Reason = TEXT("HandTooFarFromButtonFace");
            else if (!Station->IsButton() && (bButtonsOnly || Station->Kind != EFishingStationKind::WinchLever)) Reason = TEXT("NotButtonOrLever");
        }
        else Reason = TEXT("NotFishingActor");
        UE_LOG(LogFishing, Verbose, TEXT("[Target.Candidate] hand=%s actor=%s kind=%s centerDistanceCm=%.1f buttonFaceDistanceCm=%.1f reason=%s"),
            HandIndex ? TEXT("Right") : TEXT("Left"), *GetNameSafe(Actor), Station ? *UEnum::GetValueAsString(Station->Kind) : Item ? TEXT("Item") : TEXT("Other"),
            Actor ? FVector::Distance(Position, Actor->GetActorLocation()) : -1.f, ButtonDistance, Reason);
        const bool bEligible = Item ? !bButtonsOnly && Item->Scenario == Scenario && !Item->IsHeld() && !Item->IsHidden()
            : Station && Station->Scenario == Scenario && bButtonInReach && (Station->IsButton() || (!bButtonsOnly && Station->Kind == EFishingStationKind::WinchLever));
        if (!bEligible) continue;
        const float Distance = FVector::DistSquared(Position, Actor->GetActorLocation());
        if (Distance < BestDistance) { BestDistance = Distance; Best = Actor; }
    }
    UE_LOG(LogFishing, Verbose, TEXT("[Target.Result] hand=%s target=%s reason=%s"), HandIndex ? TEXT("Right") : TEXT("Left"),
        *GetNameSafe(Best), Best ? TEXT("ClosestEligibleOverlap") : TEXT("NoEligibleOverlap"));
    if (!Best && UE_LOG_ACTIVE(LogFishing, Verbose))
    {
        AFishingStation* Nearest = nullptr;
        float NearestDistance = TNumericLimits<float>::Max();
        for (TActorIterator<AFishingStation> It(GetWorld()); It; ++It)
        {
            if (It->Scenario != Scenario || !It->IsButton()) continue;
            const float Distance = FVector::DistSquared(Position, It->GetActorLocation());
            if (Distance < NearestDistance) { NearestDistance = Distance; Nearest = *It; }
        }
        if (Nearest)
            UE_LOG(LogFishing, Verbose, TEXT("[Target.NearestButton] hand=%s actor=%s kind=%s centerDistanceCm=%.1f radiusCm=%.1f zoneExtent=%s collision=%d actorCollision=%d hint=MoveHandToButtonThenPressTriggerOrGrip"),
                HandIndex ? TEXT("Right") : TEXT("Left"), *Nearest->GetName(), *UEnum::GetValueAsString(Nearest->Kind), FMath::Sqrt(NearestDistance), Radius,
                *Nearest->Zone->GetScaledBoxExtent().ToCompactString(), static_cast<int32>(Nearest->Zone->GetCollisionEnabled()), Nearest->GetActorEnableCollision());
    }
    return Best;
}

void UFishingInteractionComponent::Grip(bool bRightHand)
{
    const int32 Index = bRightHand ? 1 : 0;
    UE_LOG(LogFishing, Log, TEXT("[Interaction.Grip] pawn=%s hand=%s stage=%s heldItem=%s heldLever=%s"), *GetNameSafe(GetOwner()),
        bRightHand ? TEXT("Right") : TEXT("Left"), IsValid(Scenario) ? *UEnum::GetValueAsString(Scenario->Stage) : TEXT("None"),
        *GetNameSafe(HeldItems[Index].Get()), *GetNameSafe(HeldLevers[Index].Get()));
    if (HeldItems[Index].IsValid() || HeldLevers[Index].IsValid())
    {
        UE_LOG(LogFishing, Log, TEXT("[Interaction.Rejected] operation=Grip hand=%s reason=HandAlreadyOccupied"), bRightHand ? TEXT("Right") : TEXT("Left"));
        return;
    }
    AActor* Target = FindTarget(Index, false);
    if (AFishingItem* Item = Cast<AFishingItem>(Target))
    {
        if (Item->TryGrab(Hands[Index].Get())) HeldItems[Index] = Item;
    }
    else if (AFishingStation* Station = Cast<AFishingStation>(Target))
    {
        if (Station->Kind == EFishingStationKind::WinchLever)
        {
            if (Station->BeginLever(Hands[Index].Get())) HeldLevers[Index] = Station;
        }
        else Station->Use();
    }
    else UE_LOG(LogFishing, Log, TEXT("[Interaction.Rejected] operation=Grip hand=%s reason=NoTarget"), bRightHand ? TEXT("Right") : TEXT("Left"));
}

void UFishingInteractionComponent::ReleaseGrip(bool bRightHand)
{
    const int32 Index = bRightHand ? 1 : 0;
    if (HeldItems[Index].IsValid() || HeldLevers[Index].IsValid())
        UE_LOG(LogFishing, Log, TEXT("[Interaction.Release] pawn=%s hand=%s item=%s lever=%s"), *GetNameSafe(GetOwner()),
            bRightHand ? TEXT("Right") : TEXT("Left"), *GetNameSafe(HeldItems[Index].Get()), *GetNameSafe(HeldLevers[Index].Get()));
    if (HeldItems[Index].IsValid()) HeldItems[Index]->Release();
    if (HeldLevers[Index].IsValid()) HeldLevers[Index]->EndLever(Hands[Index].Get());
    HeldItems[Index].Reset();
    HeldLevers[Index].Reset();
}

void UFishingInteractionComponent::Use(bool bRightHand)
{
    UE_LOG(LogFishing, Log, TEXT("[Interaction.Use] pawn=%s hand=%s stage=%s"), *GetNameSafe(GetOwner()), bRightHand ? TEXT("Right") : TEXT("Left"),
        IsValid(Scenario) ? *UEnum::GetValueAsString(Scenario->Stage) : TEXT("None"));
    AFishingStation* Station = Cast<AFishingStation>(FindTarget(bRightHand ? 1 : 0, true));
    const bool bAccepted = Station && Station->Use();
    UE_LOG(LogFishing, Log, TEXT("[Interaction.Result] operation=Use hand=%s target=%s result=%s reason=%s"), bRightHand ? TEXT("Right") : TEXT("Left"),
        *GetNameSafe(Station), bAccepted ? TEXT("Accepted") : TEXT("Rejected"), Station ? TEXT("SeeStationAndScenarioEvents") : TEXT("NoTargetInHandReach"));
}

void UFishingInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogFishing, Log, TEXT("[Interaction.EndPlay] pawn=%s reason=%d bindings=%d"), *GetNameSafe(GetOwner()), static_cast<int32>(EndPlayReason), BindingHandles.Num());
    ReleaseGrip(false);
    ReleaseGrip(true);
    if (BoundInput.IsValid()) for (uint32 Handle : BindingHandles) BoundInput->RemoveBindingByHandle(Handle);
    Super::EndPlay(EndPlayReason);
}
