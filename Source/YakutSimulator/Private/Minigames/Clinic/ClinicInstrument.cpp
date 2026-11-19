#include "Minigames/Clinic/ClinicInstrument.h"

#include "Base/VirtualRealityPawn.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Minigames/Clinic/ClinicActionDefinition.h"
#include "Minigames/Clinic/ClinicPatient.h"
#include "Minigames/Clinic/ClinicScenario.h"

AClinicInstrument::AClinicInstrument()
{
    WorkingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WorkingPoint"));
    WorkingPoint->SetupAttachment(Body);
    ExaminationAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ExaminationAudio"));
    ExaminationAudio->SetupAttachment(WorkingPoint);
    ExaminationAudio->bAutoActivate = false;
}

void AClinicInstrument::InitializeInstrument(AClinicScenario* InScenario, int32 InVisitIdentifier)
{
    if (!bHasInitialTransform)
    {
        InitialTransform = GetActorTransform();
        bHasInitialTransform = true;
    }
    if (IsValid(MeasurementPatient))
    {
        RemoveTickPrerequisiteComponent(MeasurementPatient->Body);
    }
    Scenario = InScenario;
    MeasurementPatient = nullptr;
    if (Scenario)
    {
        MeasurementPatient = Scenario->Patient;
    }
    if (IsValid(MeasurementPatient))
    {
        AddTickPrerequisiteComponent(MeasurementPatient->Body);
    }
    VisitIdentifier = InVisitIdentifier;
    EndUse();
    CancelMeasurement(true);
    if (!bIsHeld)
    {
        ReturnToInitialPosition();
    }
}

void AClinicInstrument::ReturnToInitialPosition()
{
    SetActorTransform(InitialTransform, false, nullptr, ETeleportType::TeleportPhysics);
    Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
    Body->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
}

FClinicMeasurementState AClinicInstrument::GetMeasurementState() const
{
    return Measurement;
}

void AClinicInstrument::ReceiveUseStarted_Implementation()
{
    Super::ReceiveUseStarted_Implementation();
    CancelMeasurement(true);
    if (Scenario && ActionDefinition)
    {
        const FClinicCommandResult Request = Scenario->RequestExamination(ActionDefinition->ActionIdentifier, NAME_None, VisitIdentifier);
        if (Request.Status == EClinicCommandStatus::Applied && UsesFingerClip())
        {
            ClipOntoFinger();
        }
    }
}

void AClinicInstrument::ReceiveUseEnded_Implementation()
{
    Super::ReceiveUseEnded_Implementation();
    CancelMeasurement(false);
}

bool AClinicInstrument::UsesFingerClip() const
{
    static const FName PulseAndOxygenAction(TEXT("MeasurePulseAndOxygen"));
    return ActionDefinition && ActionDefinition->ActionIdentifier == PulseAndOxygenAction;
}

void AClinicInstrument::ClipOntoFinger()
{
    static const FName FingerArea(TEXT("LeftFinger"));
    if (!IsValid(HoldingHand) || !HasValidContact(FingerArea, VisitIdentifier))
    {
        return;
    }
    UBoxComponent* Zone = MeasurementPatient->FindAreaZone(FingerArea);
    if (!Zone)
    {
        return;
    }

    USceneComponent* Attachment = Zone;
    FName SocketName;
    if (MeasurementPatient->Body->GetSkeletalMeshAsset())
    {
        Attachment = MeasurementPatient->Body;
        SocketName = Zone->GetAttachSocketName();
    }

    AVirtualRealityPawn* Pawn = Cast<AVirtualRealityPawn>(HoldingHand->GetOwner());
    const int32 StartedVisit = VisitIdentifier;
    ReleaseGrip();
    if (Pawn)
    {
        Pawn->ForgetHeldItem(this);
    }
    if (!IsValid(Scenario) || !Scenario->IsExamining(StartedVisit) || VisitIdentifier != StartedVisit || !IsValid(Attachment))
    {
        OnItemAction.Broadcast(this, EWorkAction::Released);
        return;
    }

    const FTransform Anchor = Attachment->GetSocketTransform(SocketName);
    const FTransform LocalWorkingPoint = WorkingPoint->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    FTransform Placement(Anchor.GetRotation() * LocalWorkingPoint.GetRotation().Inverse(), FVector::ZeroVector, GetActorScale3D());
    Placement.SetLocation(Anchor.GetLocation() - Placement.TransformVector(LocalWorkingPoint.GetLocation()));

    PreviousCollision = Body->GetCollisionEnabled();
    Body->SetSimulatePhysics(false);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    if (!AttachToComponent(Attachment, FAttachmentTransformRules::KeepWorldTransform, SocketName))
    {
        Body->SetCollisionEnabled(PreviousCollision);
        Body->SetSimulatePhysics(true);
        OnItemAction.Broadcast(this, EWorkAction::Released);
        return;
    }
    SetActorTransform(Placement, false, nullptr, ETeleportType::TeleportPhysics);
    bIsClippedOn = true;
    Measurement.Phase = EClinicMeasurementPhase::Measuring;
    Measurement.AreaIdentifier = FingerArea;
    OnItemAction.Broadcast(this, EWorkAction::Released);
    ReceiveMeasurementStateChanged(Measurement);
}

void AClinicInstrument::DetachFingerClip()
{
    bIsClippedOn = false;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Body->SetCollisionEnabled(PreviousCollision);
    Body->SetSimulatePhysics(true);
}

bool AClinicInstrument::CanBeGrabbed() const
{
    return Super::CanBeGrabbed() && (!bIsClippedOn || Measurement.Phase == EClinicMeasurementPhase::ReadyToRemove);
}

bool AClinicInstrument::TryGrab(USceneComponent* Hand)
{
    if (!bIsClippedOn)
    {
        return Super::TryGrab(Hand);
    }
    if (!CanBeGrabbed() || !Hand)
    {
        return false;
    }
    const int32 RemovalVisit = VisitIdentifier;
    DetachFingerClip();
    if (!Super::TryGrab(Hand))
    {
        CancelMeasurement(true);
        ReturnToInitialPosition();
        return false;
    }
    if (VisitIdentifier == RemovalVisit && Measurement.Phase == EClinicMeasurementPhase::ReadyToRemove)
    {
        CompleteMeasurement();
    }
    return true;
}

void AClinicInstrument::CancelMeasurement(bool bClearResults)
{
    if (bIsClippedOn)
    {
        DetachFingerClip();
        ReturnToInitialPosition();
    }
    if (ExaminationAudio->GetPlayState() != EAudioComponentPlayState::Stopped)
    {
        ExaminationAudio->Stop();
    }
    Elapsed = 0;
    if (bClearResults || Measurement.Phase == EClinicMeasurementPhase::Measuring
        || Measurement.Phase == EClinicMeasurementPhase::ReadyToRemove)
    {
        Measurement = FClinicMeasurementState();
        Measurement.VisitIdentifier = VisitIdentifier;
        ReceiveMeasurementStateChanged(Measurement);
    }
}

bool AClinicInstrument::HasValidContact(FName AreaIdentifier, int32 ExpectedVisitIdentifier) const
{
    float ContactTolerance = 0.0f;
    if (UsesFingerClip())
    {
        ContactTolerance = FingerClipContactTolerance;
    }
    return Scenario && Scenario->IsExamining(ExpectedVisitIdentifier) && VisitIdentifier == ExpectedVisitIdentifier
        && bIsHeld && bIsUsing && ActionDefinition && ActionDefinition->bUsesInstrument
        && IsValid(MeasurementPatient)
        && MeasurementPatient->IsReadyForExamination(ActionDefinition->ActionIdentifier, AreaIdentifier, ExpectedVisitIdentifier)
        && MeasurementPatient->IsPointInArea(AreaIdentifier, WorkingPoint->GetComponentLocation(), ContactTolerance);
}

bool AClinicInstrument::CanRecordMeasurement(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const
{
    if (!ActionDefinition || ActionDefinition->ActionIdentifier != ActionIdentifier || Measurement.AreaIdentifier != AreaIdentifier)
    {
        return false;
    }
    if (UsesFingerClip())
    {
        return VisitIdentifier == ExpectedVisitIdentifier && bIsHeld && !bIsClippedOn
            && Measurement.Phase == EClinicMeasurementPhase::ReadyToRemove;
    }
    return HasValidContact(AreaIdentifier, ExpectedVisitIdentifier) && Measurement.Phase == EClinicMeasurementPhase::Measuring
        && Elapsed >= ActionDefinition->DurationSeconds;
}

void AClinicInstrument::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bIsClippedOn)
    {
        if (!Scenario || !Scenario->IsExamining(VisitIdentifier) || !IsValid(MeasurementPatient) || GetAttachParentActor() != MeasurementPatient)
        {
            CancelMeasurement(false);
        }
        else if (Measurement.Phase == EClinicMeasurementPhase::Measuring)
        {
            Elapsed += DeltaSeconds;
            Measurement.Progress = FMath::Clamp(Elapsed / FMath::Max(FingerClipDurationSeconds, 0.1f), 0.0f, 1.0f);
            if (Measurement.Progress >= 1.0f)
            {
                Measurement.Phase = EClinicMeasurementPhase::ReadyToRemove;
            }
            ReceiveMeasurementStateChanged(Measurement);
        }
        return;
    }
    if (UsesFingerClip())
    {
        return;
    }
    if (!Scenario || !ActionDefinition || !Scenario->IsExamining(VisitIdentifier) || !bIsUsing || !bIsHeld)
    {
        CancelMeasurement(false);
        return;
    }
    FName ContactArea;
    for (FName Area : ActionDefinition->GetAreas())
    {
        if (HasValidContact(Area, VisitIdentifier))
        {
            ContactArea = Area;
            break;
        }
    }
    if (ContactArea.IsNone())
    {
        CancelMeasurement(false);
        return;
    }
    if (Measurement.AreaIdentifier == ContactArea && Measurement.Phase == EClinicMeasurementPhase::Completed)
    {
        return;
    }
    if (Measurement.AreaIdentifier != ContactArea || Measurement.Phase != EClinicMeasurementPhase::Measuring)
    {
        CancelMeasurement(true);
        Measurement.Phase = EClinicMeasurementPhase::Measuring;
        Measurement.AreaIdentifier = ContactArea;
        if (USoundBase* Sound = Scenario->GetExaminationSound(this, ContactArea, VisitIdentifier))
        {
            ExaminationAudio->SetSound(Sound);
            ExaminationAudio->Play();
        }
    }
    Elapsed += DeltaSeconds;
    Measurement.Progress = FMath::Clamp(Elapsed / ActionDefinition->DurationSeconds, 0.0f, 1.0f);
    if (Elapsed >= ActionDefinition->DurationSeconds)
    {
        CompleteMeasurement();
        return;
    }
    ReceiveMeasurementStateChanged(Measurement);
}

void AClinicInstrument::CompleteMeasurement()
{
    const int32 CompletedVisit = VisitIdentifier;
    const FName ContactArea = Measurement.AreaIdentifier;
    const EClinicMeasurementPhase CompletionPhase = Measurement.Phase;
    TArray<FClinicObservation> Observations;
    const FClinicCommandResult Result = Scenario->RecordExamination(this, ActionDefinition->ActionIdentifier, ContactArea, CompletedVisit, &Observations);
    if (VisitIdentifier != CompletedVisit || Measurement.AreaIdentifier != ContactArea || Measurement.Phase != CompletionPhase)
    {
        return;
    }
    if (Result.Status != EClinicCommandStatus::Applied)
    {
        CancelMeasurement(true);
        return;
    }
    Measurement.Phase = EClinicMeasurementPhase::Completed;
    Measurement.Observations = MoveTemp(Observations);
    ReceiveMeasurementStateChanged(Measurement);
}

void AClinicInstrument::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelMeasurement(true);
    Super::EndPlay(EndPlayReason);
}
