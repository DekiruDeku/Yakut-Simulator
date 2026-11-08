#include "Fishing/FishingMechanics.h"
#include "Common/Interaction.h"
#include "Fishing/FishingLog.h"

UFishingWinchComponent::UFishingWinchComponent() { PrimaryComponentTick.bCanEverTick = true; }
void UFishingWinchComponent::Reset(const FFishingWinchSettings& Configuration)
{
    Settings = Configuration;
    if (Lever)
    {
        Lever->OnValueChanged.AddUniqueDynamic(this, &UFishingWinchComponent::SetInput);
        Lever->Release();
    }
    if (Net)
    {
        if (!bHasRaisedLocation) { RaisedLocation = Net->GetRelativeLocation(); bHasRaisedLocation = true; }
        Net->SetRelativeLocation(RaisedLocation);
    }
    Input = Depth = Load = OverloadTime = Seconds = 0;
    StopCause = EWinchStopCause::None;
    Mode = EFishingOperation::None;
}
void UFishingWinchComponent::Start(EFishingOperation Operation)
{
    Mode = Operation;
    Seconds = OverloadTime = Input = Load = 0;
    StopCause = EWinchStopCause::None;
    if (Lever) Lever->Release();
}
void UFishingWinchComponent::Deactivate()
{
    Mode = EFishingOperation::None;
    Input = Load = OverloadTime = 0;
    if (Lever) Lever->Release();
}
void UFishingWinchComponent::SetInput(float Value)
{
    const float Previous = Input;
    Input = FMath::IsFinite(Value) && StopCause == EWinchStopCause::None && (Mode == EFishingOperation::Deploy || Mode == EFishingOperation::Haul) ? FMath::Clamp(Value, 0.f, 1.f) : 0;
    if (FMath::Abs(Input - Previous) > KINDA_SMALL_NUMBER) UE_LOG(LogFishing, Verbose, TEXT("[Winch.Input] value=%.3f load=%.3f depth=%.3f"), Input, Load, Depth);
}
void UFishingWinchComponent::Stop(EWinchStopCause Cause)
{
    if (Cause == EWinchStopCause::None) return;
    StopCause = Cause;
    Input = Load = OverloadTime = 0;
    if (Lever) Lever->Release();
    UE_LOG(LogFishing, Display, TEXT("[Winch.Stopped] cause=%s"), *UEnum::GetValueAsString(Cause));
    OnStopped.Broadcast();
}
void UFishingWinchComponent::Resume()
{
    StopCause = EWinchStopCause::None;
    Input = Load = OverloadTime = 0;
    if (Lever) Lever->Release();
}
void UFishingWinchComponent::TickComponent(float Delta, ELevelTick Type, FActorComponentTickFunction* Function)
{
    Super::TickComponent(Delta, Type, Function);
    if (!FMath::IsFinite(Delta) || Delta <= 0 || Mode == EFishingOperation::None || StopCause != EWinchStopCause::None) return;
    Seconds += Delta;
    if (Mode == EFishingOperation::Trawl)
    {
        if (Seconds >= Settings.TrawlSeconds) { Mode = EFishingOperation::None; OnFinished.Broadcast(); }
        return;
    }
    if (Mode != EFishingOperation::Deploy && Mode != EFishingOperation::Haul) return;
    const float Resistance = Settings.ResistanceBase + Settings.ResistanceAmplitude * FMath::Sin(Seconds * Settings.ResistanceFrequency);
    Load = FMath::Clamp(Resistance + Settings.InputLoadScale * Input, 0.f, 1.f);
    OverloadTime = Settings.bEnableOverloads && Load > Settings.OverloadLoad ? OverloadTime + Delta : 0;
    if (Settings.bEnableOverloads && OverloadTime >= Settings.OverloadSeconds) { Stop(EWinchStopCause::Overload); return; }
    const bool Deploying = Mode == EFishingOperation::Deploy;
    Depth = Deploying ? FMath::Min(1.f, Depth + Input * Delta / Settings.DeploySeconds) : FMath::Max(0.f, Depth - Input * Delta / Settings.HaulSeconds);
    if (Net) Net->SetRelativeLocation(RaisedLocation + Settings.LoweredOffset * Depth);
    if ((Deploying && Depth >= 1) || (!Deploying && Depth <= 0))
    {
        Mode = EFishingOperation::None;
        Input = Load = 0;
        if (Lever) Lever->Release();
        OnFinished.Broadcast();
    }
}
