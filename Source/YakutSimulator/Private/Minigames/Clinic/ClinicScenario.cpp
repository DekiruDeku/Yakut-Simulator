#include "Minigames/Clinic/ClinicScenario.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Base/WorkOperationComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Minigames/Clinic/ClinicActionDefinition.h"
#include "Minigames/Clinic/ClinicDiseaseDefinition.h"
#include "Minigames/Clinic/ClinicInstrument.h"
#include "Minigames/Clinic/ClinicPatient.h"
#include "Minigames/Clinic/ClinicPatientProfile.h"
#include "Misc/DataValidation.h"
#include "UObject/ObjectPtr.h"

AClinicScenario::AClinicScenario()
{
    ScenarioTitle = FText::FromString(TEXT("Кабинет терапевта"));
    StartOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("StartOperation"));
    ExaminationOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("ExaminationOperation"));
    DecisionOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("DecisionOperation"));
    AddOperationStep(TEXT("StartVisit"), FText::FromString(TEXT("Начало приёма")), StartOperation);
    AddOperationStep(TEXT("FinishExamination"), FText::FromString(TEXT("Обследуйте пациента. Завершите обследование, когда решите.")), ExaminationOperation, {TEXT("StartVisit")});
    AddOperationStep(TEXT("SubmitDecision"), FText::FromString(TEXT("Выберите диагноз.")), DecisionOperation, {TEXT("FinishExamination")});
}

void AClinicScenario::BeginPlay()
{
    Super::BeginPlay();
    FText Error;
    if (!ValidateClinicSetup(Error))
    {
        UKismetSystemLibrary::PrintString(this, Error.ToString(), true, true, FLinearColor::Red);
        return;
    }
    if (!StartVisitAtIndex(InitialCaseIndex))
    {
        UKismetSystemLibrary::PrintString(this, TEXT("Could not start the configured clinic visit"), true, true, FLinearColor::Red);
    }
}

void AClinicScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (Visit.Phase != EClinicVisitPhase::Examination)
    {
        return;
    }
    bool bBecameReady = false;
    for (FClinicInvestigationOrder& Order : Visit.InvestigationOrders)
    {
        if (Order.State == EClinicInvestigationState::InProgress)
        {
            Order.SecondsRemaining = FMath::Max(0.0f, Order.SecondsRemaining - DeltaSeconds);
            if (Order.SecondsRemaining <= 0)
            {
                Order.State = EClinicInvestigationState::Ready;
                bBecameReady = true;
            }
        }
    }
    if (bBecameReady)
    {
        OnScenarioChanged.Broadcast();
    }
}

void AClinicScenario::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ++Visit.VisitIdentifier;
    Visit.Phase = EClinicVisitPhase::NotStarted;
    StopExecutors(true);
    Super::EndPlay(EndPlayReason);
}

static bool ValidatePatientSetup(const AClinicPatient* Patient, const TArray<TObjectPtr<UClinicActionDefinition>>& Actions,
    FName CasePose, FText& Error)
{
    TSet<FName> PatientAreas;
    TSet<UBoxComponent*> AreaZones;
    for (const FClinicExaminationArea& Area : Patient->Areas)
    {
        if (Area.AreaIdentifier.IsNone() || !Area.Zone || (!Patient->IsTemplate() && Area.Zone->GetOwner() != Patient)
            || PatientAreas.Contains(Area.AreaIdentifier) || AreaZones.Contains(Area.Zone))
        {
            Error = FText::FromString(TEXT("Patient areas require unique identifiers and owned collision zones"));
            return false;
        }
        PatientAreas.Add(Area.AreaIdentifier);
        AreaZones.Add(Area.Zone);
    }
    if (!Patient->ValidateAreaBindings(nullptr, Error) || !Patient->ValidatePoseDefinitions(Error))
    {
        return false;
    }
    const FName InitialPose = Patient->ResolveInitialPose(CasePose);
    if (!Patient->PoseDefinitions.IsEmpty() && !Patient->HasPoseDefinition(InitialPose))
    {
        Error = FText::FromString(FString::Format(TEXT("Initial patient pose {0} is not configured"), {InitialPose.ToString()}));
        return false;
    }
    for (const UClinicActionDefinition* Action : Actions)
    {
        for (const FClinicExaminationStage& Stage : Action->PresentationStages)
        {
            if (!Patient->HasPoseDefinition(Stage.PoseIdentifier))
            {
                Error = FText::FromString(FString::Format(TEXT("Patient pose {0} is not configured"), {Stage.PoseIdentifier.ToString()}));
                return false;
            }
            for (FName Area : Stage.Areas)
            {
                if (!Patient->HasArea(Area))
                {
                    Error = FText::FromString(FString::Format(TEXT("Patient is missing area {0}"), {Area.ToString()}));
                    return false;
                }
            }
        }
    }
    return true;
}

bool AClinicScenario::ValidateClinicSetup(FText& Error) const
{
    if (!Patient || InitialCaseIndex < 0 || InitialCaseIndex >= GetVisitCount()
        || ActionDefinitions.IsEmpty() || DiseaseDefinitions.IsEmpty())
    {
        Error = FText::FromString(TEXT("Clinic requires a patient placement, catalogs and a valid initial visit"));
        return false;
    }
    TSet<FName> Actions;
    TSet<FName> Diseases;
    TSet<FName> LegacyCaseIdentifiers;
    for (const UClinicDiseaseDefinition* Disease : DiseaseDefinitions)
    {
        if (!Disease)
        {
            Error = FText::FromString(TEXT("Disease catalog contains an empty entry"));
            return false;
        }
        if (!Disease->ValidateDefinition(Error))
        {
            return false;
        }
        if (Diseases.Contains(Disease->DiseaseIdentifier))
        {
            Error = FText::FromString(TEXT("Disease identifiers must be unique"));
            return false;
        }
        Diseases.Add(Disease->DiseaseIdentifier);
    }
    TSet<AClinicInstrument*> UniqueInstruments;
    for (AClinicInstrument* Instrument : Instruments)
    {
        if (!Instrument || !Instrument->ActionDefinition || UniqueInstruments.Contains(Instrument) || !ActionDefinitions.Contains(Instrument->ActionDefinition)
            || Instrument->ActionDefinition->ActionType != EClinicActionType::Examination || !Instrument->ActionDefinition->bUsesInstrument)
        {
            Error = FText::FromString(TEXT("Clinic instruments must be unique and reference catalog instrument actions"));
            return false;
        }
        UniqueInstruments.Add(Instrument);
    }
    for (const UClinicActionDefinition* Action : ActionDefinitions)
    {
        if (!Action)
        {
            Error = FText::FromString(TEXT("Action catalog contains an empty entry"));
            return false;
        }
        if (!Action->ValidateDefinition(Error))
        {
            return false;
        }
        if (Actions.Contains(Action->ActionIdentifier))
        {
            Error = FText::FromString(TEXT("Action identifiers must be unique"));
            return false;
        }
        Actions.Add(Action->ActionIdentifier);
        if (Action->bUsesInstrument && !Instruments.ContainsByPredicate([Action](const AClinicInstrument* Instrument)
        {
            return Instrument->ActionDefinition == Action;
        }))
        {
            Error = FText::FromString(FString::Format(TEXT("Missing instrument for {0}"), {Action->ActionIdentifier.ToString()}));
            return false;
        }
    }
    for (int32 Index = 0; Index < GetVisitCount(); ++Index)
    {
        const AClinicPatient* ConfiguredPatient = Patient;
        const UClinicPatientCaseDefinition* PatientCase = nullptr;
        if (Visits.IsEmpty())
        {
            PatientCase = PatientCases[Index];
        }
        else
        {
            const FClinicVisitDefinition& Entry = Visits[Index];
            PatientCase = Entry.Case;
            if (!Entry.PatientClass || Entry.PatientClass->HasAnyClassFlags(CLASS_Abstract))
            {
                Error = FText::FromString(TEXT("Each clinic visit requires a concrete patient Blueprint"));
                return false;
            }
            ConfiguredPatient = Entry.PatientClass.GetDefaultObject();
            const UClinicPatientProfile* Profile = ConfiguredPatient->PatientProfile;
            if (!Profile || Profile->PatientName.IsEmpty() || Profile->Age < 0 || Profile->Sex.IsEmpty())
            {
                Error = FText::FromString(TEXT("Patient Blueprint requires a profile with a name, nonnegative age and sex"));
                return false;
            }
        }
        if (!PatientCase)
        {
            Error = FText::FromString(TEXT("Case catalog contains an empty entry"));
            return false;
        }
        FClinicResolvedCase CaseData;
        if (!PatientCase->Resolve(ActionDefinitions, CaseData, Error))
        {
            return false;
        }
        FName InitialPose = PatientCase->InitialPose;
        if (PatientCase->UsesLegacyData())
        {
            InitialPose = PatientCase->Presentation.PoseIdentifier;
            if (PatientCase->Presentation.Mesh && !ConfiguredPatient->ValidateAreaBindings(PatientCase->Presentation.Mesh, Error))
            {
                return false;
            }
        }
        if (!ValidatePatientSetup(ConfiguredPatient, ActionDefinitions, InitialPose, Error))
        {
            return false;
        }
        if (!DiseaseDefinitions.Contains(PatientCase->ExpectedDisease))
        {
            Error = FText::FromString(TEXT("Each clinic case requires a disease from the scenario catalog"));
            return false;
        }
        if (PatientCase->UsesLegacyData())
        {
            if (Visits.IsEmpty() && LegacyCaseIdentifiers.Contains(PatientCase->CaseIdentifier))
            {
                Error = FText::FromString(TEXT("Legacy case identifiers must be unique"));
                return false;
            }
            LegacyCaseIdentifiers.Add(PatientCase->CaseIdentifier);
            for (const FClinicCaseOutcome& Outcome : CaseData.Outcomes)
            {
                if (const UClinicActionDefinition* Action = FindAction(Outcome.ActionIdentifier))
                {
                    const TArray<FName> Areas = Action->GetAreas();
                    if ((Areas.IsEmpty() && Outcome.AreaIdentifier.IsNone()) || Areas.Contains(Outcome.AreaIdentifier))
                    {
                        continue;
                    }
                }
                Error = FText::FromString(TEXT("Case outcome references an unknown action or area"));
                return false;
            }
            for (const UClinicActionDefinition* Action : ActionDefinitions)
            {
                TArray<FName> Areas = Action->GetAreas();
                if (Areas.IsEmpty())
                {
                    Areas.Add(NAME_None);
                }
                for (FName Area : Areas)
                {
                    if (!CaseData.FindOutcome(Action->ActionIdentifier, Area))
                    {
                        Error = FText::FromString(FString::Format(TEXT("Case {0} is missing outcome {1}/{2}"),
                            {PatientCase->GetName(), Action->ActionIdentifier.ToString(), Area.ToString()}));
                        return false;
                    }
                }
            }
        }
    }
    Error = FText::GetEmpty();
    return true;
}

#if WITH_EDITOR
EDataValidationResult AClinicScenario::IsDataValid(FDataValidationContext& Context) const
{
    if (IsTemplate())
    {
        return EDataValidationResult::NotValidated;
    }
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    if (ParentResult == EDataValidationResult::Invalid)
    {
        return ParentResult;
    }
    FText Error;
    if (!ValidateClinicSetup(Error))
    {
        Context.AddError(Error);
        return EDataValidationResult::Invalid;
    }
    return EDataValidationResult::Valid;
}
#endif

bool AClinicScenario::StartVisit(UClinicPatientCaseDefinition* PatientCase)
{
    return Visits.IsEmpty() && StartVisitAtIndex(PatientCases.IndexOfByKey(PatientCase));
}

int32 AClinicScenario::GetVisitCount() const
{
    if (Visits.IsEmpty())
    {
        return PatientCases.Num();
    }
    return Visits.Num();
}

bool AClinicScenario::StartVisitAtIndex(int32 CaseIndex)
{
    if (CaseIndex < 0 || CaseIndex >= GetVisitCount() || !IsValid(Patient))
    {
        return false;
    }
    UClinicPatientCaseDefinition* PatientCase = nullptr;
    TSubclassOf<AClinicPatient> PatientClass;
    if (Visits.IsEmpty())
    {
        PatientCase = PatientCases[CaseIndex];
    }
    else
    {
        PatientCase = Visits[CaseIndex].Case;
        PatientClass = Visits[CaseIndex].PatientClass;
        if (!PatientClass)
        {
            return false;
        }
    }
    FText Error;
    FClinicResolvedCase CaseData;
    if (!PatientCase || !PatientCase->Resolve(ActionDefinitions, CaseData, Error))
    {
        return false;
    }
    AClinicPatient* NextPatient = Patient;
    if (PatientClass && (!PatientPlacement || Patient->GetClass() != PatientClass.Get()))
    {
        const AClinicPatient* Placement = PatientPlacement.Get();
        if (!Placement)
        {
            Placement = Patient;
        }
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = this;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        NextPatient = GetWorld()->SpawnActor<AClinicPatient>(PatientClass.Get(), Placement->GetActorTransform(), SpawnParameters);
        if (!NextPatient || !NextPatient->PatientProfile)
        {
            if (NextPatient)
            {
                NextPatient->Destroy();
            }
            return false;
        }
    }
    AClinicPatient* PreviousPatient = Patient;
    const int32 NextIdentifier = Visit.VisitIdentifier + 1;
    Visit.Phase = EClinicVisitPhase::NotStarted;
    Visit.VisitIdentifier = NextIdentifier;
    StopExecutors(true);
    Visit = FClinicVisitSnapshot();
    Visit.VisitIdentifier = NextIdentifier;
    CurrentCase = PatientCase;
    CurrentCaseIndex = CaseIndex;
    ResolvedCase = MoveTemp(CaseData);
    Patient = NextPatient;
    Super::RestartWorkstation();
    Visit.Complaint = PatientCase->Complaint;
    Visit.KnownObservations = ResolvedCase.InitialObservations;
    FClinicPatientPresentation Presentation;
    Presentation.PoseIdentifier = PatientCase->InitialPose;
    if (PatientCase->UsesLegacyData())
    {
        Presentation = PatientCase->Presentation;
    }
    if (const UClinicPatientProfile* Profile = Patient->PatientProfile)
    {
        Visit.PatientName = Profile->PatientName;
        Visit.Age = Profile->Age;
        Visit.Sex = Profile->Sex;
    }
    else
    {
        Visit.PatientName = PatientCase->PatientName;
        Visit.Age = PatientCase->Age;
        Visit.Sex = PatientCase->Sex;
    }
    if (PreviousPatient != Patient && !PatientPlacement)
    {
        PatientPlacement = PreviousPatient;
        PatientPlacement->SetActorHiddenInGame(true);
        PatientPlacement->SetActorEnableCollision(false);
        PatientPlacement->SetActorTickEnabled(false);
    }
    Patient->InitializePatient(this, NextIdentifier, Presentation);
    for (AClinicInstrument* Instrument : Instruments)
    {
        if (Instrument)
        {
            Instrument->InitializeInstrument(this, NextIdentifier);
        }
    }
    if (PreviousPatient != Patient && PreviousPatient != PatientPlacement)
    {
        PreviousPatient->Destroy();
    }
    Visit.Phase = EClinicVisitPhase::Examination;
    ReportResult(TEXT("StartVisit"));
    return true;
}

void AClinicScenario::RestartWorkstation()
{
    if (CurrentCase)
    {
        StartVisitAtIndex(CurrentCaseIndex);
    }
}

void AClinicScenario::StopExecutors(bool bClearResults)
{
    if (IsValid(Patient))
    {
        Patient->CancelInspection();
    }
    for (AClinicInstrument* Instrument : Instruments)
    {
        if (Instrument)
        {
            Instrument->CancelMeasurement(bClearResults);
        }
    }
}

bool AClinicScenario::IsExamining(int32 ExpectedVisitIdentifier) const
{
    return CurrentCase && Visit.VisitIdentifier == ExpectedVisitIdentifier && Visit.Phase == EClinicVisitPhase::Examination;
}

UClinicActionDefinition* AClinicScenario::FindAction(FName Identifier) const
{
    for (UClinicActionDefinition* Action : ActionDefinitions)
    {
        if (Action && Action->ActionIdentifier == Identifier)
        {
            return Action;
        }
    }
    return nullptr;
}

FClinicVisitSnapshot AClinicScenario::GetVisitSnapshot() const
{
    return Visit;
}

bool AClinicScenario::GetReferenceSymptomStatus(FName DiseaseIdentifier, FName SymptomIdentifier,
    int32 ExpectedVisitIdentifier, EClinicReferenceSymptomStatus& Status) const
{
    Status = EClinicReferenceSymptomStatus::Unknown;
    if (Visit.Phase == EClinicVisitPhase::NotStarted || ExpectedVisitIdentifier != Visit.VisitIdentifier)
    {
        return false;
    }
    for (const UClinicDiseaseDefinition* Disease : DiseaseDefinitions)
    {
        if (!Disease || Disease->DiseaseIdentifier != DiseaseIdentifier)
        {
            continue;
        }
        for (const FClinicReferenceSymptom& Symptom : Disease->Symptoms)
        {
            if (Symptom.SymptomIdentifier == SymptomIdentifier)
            {
                FText Error;
                if (!Symptom.Validate(Error))
                {
                    return false;
                }
                Status = Symptom.Evaluate(Visit.KnownObservations);
                return true;
            }
        }
    }
    return false;
}

TArray<UClinicActionDefinition*> AClinicScenario::GetActionDefinitions() const
{
    return ObjectPtrDecay(ActionDefinitions);
}

TArray<UClinicDiseaseDefinition*> AClinicScenario::GetDiseaseDefinitions() const
{
    return ObjectPtrDecay(DiseaseDefinitions);
}

FClinicCommandResult AClinicScenario::RevealOutcome(const FClinicCaseOutcome& Outcome, AActor* Executor)
{
    FClinicActionResult Result;
    Result.VisitIdentifier = Visit.VisitIdentifier;
    Result.ActionIdentifier = Outcome.ActionIdentifier;
    Result.AreaIdentifier = Outcome.AreaIdentifier;
    Result.Executor = Executor;
    Result.Response = Outcome.Response;
    Result.Sound = Outcome.Sound;
    Result.Image = Outcome.Image;
    Result.Observations = Outcome.Observations;
    for (const FClinicObservation& Observation : Outcome.Observations)
    {
        if (!Visit.KnownObservations.ContainsByPredicate([&Observation](const FClinicObservation& Known)
        {
            return Known.ObservationIdentifier == Observation.ObservationIdentifier;
        }))
        {
            Visit.KnownObservations.Add(Observation);
        }
    }
    FClinicActionRecord Record;
    Record.ActionIdentifier = Outcome.ActionIdentifier;
    Record.AreaIdentifier = Outcome.AreaIdentifier;
    Record.Response = Outcome.Response;
    Visit.ActionHistory.Add(Record);
    OnClinicActionCompleted.Broadcast(Result);
    OnScenarioChanged.Broadcast();
    if (Visit.VisitIdentifier != Result.VisitIdentifier)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Приём изменился. Повторите действие."))};
    }
    return {EClinicCommandStatus::Applied, Result.Response};
}

FClinicCommandResult AClinicScenario::AskQuestion(FName ActionIdentifier, int32 ExpectedVisitIdentifier)
{
    const UClinicActionDefinition* Action = FindAction(ActionIdentifier);
    if (!IsExamining(ExpectedVisitIdentifier) || !Action || Action->ActionType != EClinicActionType::Question)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Сейчас нельзя задать этот вопрос."))};
    }
    const FClinicCaseOutcome* Outcome = ResolvedCase.FindOutcome(ActionIdentifier, NAME_None);
    if (!Outcome)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Ответ на этот вопрос не настроен."))};
    }
    return RevealOutcome(*Outcome, Patient);
}

FClinicCommandResult AClinicScenario::RequestExamination(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier)
{
    UClinicActionDefinition* Action = FindAction(ActionIdentifier);
    if (!IsExamining(ExpectedVisitIdentifier) || !Action || Action->ActionType != EClinicActionType::Examination || !IsValid(Patient))
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Этот осмотр недоступен в текущем состоянии приёма."))};
    }
    const TArray<FName> Areas = Action->GetAreas();
    if ((!AreaIdentifier.IsNone() && !Areas.Contains(AreaIdentifier))
        || (!Action->bUsesInstrument && !Areas.IsEmpty() && AreaIdentifier.IsNone()))
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Этот осмотр недоступен в текущем состоянии приёма."))};
    }
    if (Action->bUsesInstrument)
    {
        if (!Patient->BeginInstrumentExamination(Action, ExpectedVisitIdentifier))
        {
            return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Области обследования недоступны."))};
        }
        if (Instruments.ContainsByPredicate([Action](const AClinicInstrument* Instrument)
        {
            return IsValid(Instrument) && Instrument->ActionDefinition == Action && Instrument->UsesFingerClip();
        }))
        {
            return {EClinicCommandStatus::Applied, FText::FromString(
                TEXT("Поднесите пульсоксиметр к подсвеченному пальцу и нажмите Trigger. Когда измерение будет готово, снимите прибор кнопкой Grip."))};
        }
        return {EClinicCommandStatus::Applied, Action->Instruction};
    }
    if (!Patient->BeginInspection(Action, AreaIdentifier, ExpectedVisitIdentifier))
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Дождитесь завершения текущего осмотра."))};
    }
    return {EClinicCommandStatus::Applied, Action->Instruction};
}

FClinicCommandResult AClinicScenario::OrderInvestigation(FName ActionIdentifier, int32 ExpectedVisitIdentifier)
{
    const UClinicActionDefinition* Action = FindAction(ActionIdentifier);
    if (!IsExamining(ExpectedVisitIdentifier) || !Action || Action->ActionType != EClinicActionType::Investigation)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Сейчас нельзя направить на это исследование."))};
    }
    if (Visit.InvestigationOrders.ContainsByPredicate([ActionIdentifier](const FClinicInvestigationOrder& Order)
    {
        return Order.ActionIdentifier == ActionIdentifier;
    }))
    {
        return {EClinicCommandStatus::Unchanged, FText::FromString(TEXT("Это исследование уже назначено."))};
    }
    FClinicInvestigationOrder Order;
    Order.ActionIdentifier = ActionIdentifier;
    Order.SecondsRemaining = Action->InvestigationDelaySeconds;
    Visit.InvestigationOrders.Add(Order);
    OnScenarioChanged.Broadcast();
    return {EClinicCommandStatus::Applied, FText::FromString(TEXT("Исследование назначено."))};
}

FClinicCommandResult AClinicScenario::ReadInvestigation(FName ActionIdentifier, int32 ExpectedVisitIdentifier)
{
    if (!IsExamining(ExpectedVisitIdentifier))
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Обследование уже завершено."))};
    }
    FClinicInvestigationOrder* Order = Visit.InvestigationOrders.FindByPredicate([ActionIdentifier](const FClinicInvestigationOrder& Candidate)
    {
        return Candidate.ActionIdentifier == ActionIdentifier;
    });
    if (!Order || Order->State != EClinicInvestigationState::Ready)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Результат ещё не готов или исследование не назначено."))};
    }
    if (Order->bResultRead)
    {
        return {EClinicCommandStatus::Unchanged, FText::FromString(TEXT("Результат исследования уже получен."))};
    }
    const FClinicCaseOutcome* Outcome = ResolvedCase.FindOutcome(ActionIdentifier, NAME_None);
    if (!Outcome)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Результат этого исследования не настроен."))};
    }
    Order->bResultRead = true;
    return RevealOutcome(*Outcome, this);
}

FClinicCommandResult AClinicScenario::RecordExamination(AActor* Executor, FName ActionIdentifier, FName AreaIdentifier,
    int32 ExpectedVisitIdentifier, TArray<FClinicObservation>* Observations)
{
    const UClinicActionDefinition* Action = FindAction(ActionIdentifier);
    if (!IsExamining(ExpectedVisitIdentifier) || !Action || Action->ActionType != EClinicActionType::Examination)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Сейчас нельзя зарегистрировать этот осмотр."))};
    }
    bool bPerformed = false;
    if (Action->bUsesInstrument)
    {
        AClinicInstrument* Instrument = Cast<AClinicInstrument>(Executor);
        bPerformed = Instrument && Instruments.Contains(Instrument)
            && Instrument->CanRecordMeasurement(ActionIdentifier, AreaIdentifier, ExpectedVisitIdentifier);
    }
    else
    {
        bPerformed = IsValid(Patient) && Executor == Patient
            && Patient->CanRecordInspection(ActionIdentifier, AreaIdentifier, ExpectedVisitIdentifier);
    }
    const FClinicCaseOutcome* Outcome = ResolvedCase.FindOutcome(ActionIdentifier, AreaIdentifier);
    if (!bPerformed || !Outcome)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Выполнение осмотра не подтверждено."))};
    }
    if (Observations)
    {
        *Observations = Outcome->Observations;
    }
    return RevealOutcome(*Outcome, Executor);
}

USoundBase* AClinicScenario::GetExaminationSound(AClinicInstrument* Instrument, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const
{
    if (!IsExamining(ExpectedVisitIdentifier) || !Instrument || !Instruments.Contains(Instrument)
        || !Instrument->HasValidContact(AreaIdentifier, ExpectedVisitIdentifier))
    {
        return nullptr;
    }
    const FClinicCaseOutcome* Outcome = ResolvedCase.FindOutcome(Instrument->ActionDefinition->ActionIdentifier, AreaIdentifier);
    if (Outcome)
    {
        return Outcome->Sound;
    }
    return nullptr;
}

FClinicCommandResult AClinicScenario::FinishExamination(int32 ExpectedVisitIdentifier)
{
    if (!IsExamining(ExpectedVisitIdentifier))
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Обследование уже завершено."))};
    }
    Visit.Phase = EClinicVisitPhase::Diagnosis;
    StopExecutors(false);
    if (IsValid(Patient))
    {
        Patient->ReturnToInitialPose();
    }
    ReportResult(TEXT("FinishExamination"));
    return {EClinicCommandStatus::Applied, FText::FromString(TEXT("Обследование завершено. Выберите диагноз."))};
}

FClinicCommandResult AClinicScenario::SubmitDecision(const FClinicDecision& Decision, int32 ExpectedVisitIdentifier)
{
    if (!CurrentCase || Visit.VisitIdentifier != ExpectedVisitIdentifier || Visit.Phase != EClinicVisitPhase::Diagnosis
        || !DiseaseDefinitions.ContainsByPredicate([&Decision](const UClinicDiseaseDefinition* Disease)
        {
            return Disease && Disease->DiseaseIdentifier == Decision.DiseaseIdentifier;
        }))
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Выберите диагноз после завершения обследования."))};
    }
    Visit.SubmittedDecision = Decision;
    Visit.Evaluation.bHasResult = true;
    Visit.Evaluation.bCorrectDiagnosis = Decision.DiseaseIdentifier == CurrentCase->ExpectedDisease->DiseaseIdentifier;
    TSet<FName> Known;
    for (const FClinicObservation& Observation : Visit.KnownObservations)
    {
        Known.Add(Observation.ObservationIdentifier);
    }
    Visit.Evaluation.bSufficientEvidence = Algo::AnyOf(ResolvedCase.EvidenceSets, [&Known](const FClinicEvidenceSet& Evidence)
    {
        return Algo::AllOf(Evidence.Observations, [&Known](FName Identifier)
        {
            return Known.Contains(Identifier);
        });
    });
    FString Explanation = CurrentCase->Debrief.ToString();
    for (const FClinicCaseOutcome& Outcome : ResolvedCase.Outcomes)
    {
        if (!Outcome.bUnjustified || Visit.Evaluation.UnjustifiedActions.Contains(Outcome.ActionIdentifier))
        {
            continue;
        }
        const bool bUsed = Visit.ActionHistory.ContainsByPredicate([&Outcome](const FClinicActionRecord& Record)
        {
            return Record.ActionIdentifier == Outcome.ActionIdentifier && Record.AreaIdentifier == Outcome.AreaIdentifier;
        }) || Visit.InvestigationOrders.ContainsByPredicate([&Outcome](const FClinicInvestigationOrder& Order)
        {
            return Order.ActionIdentifier == Outcome.ActionIdentifier;
        });
        if (bUsed)
        {
            Visit.Evaluation.UnjustifiedActions.AddUnique(Outcome.ActionIdentifier);
            const UClinicActionDefinition* Action = FindAction(Outcome.ActionIdentifier);
            if (Action && !Outcome.Rationale.IsEmpty())
            {
                Explanation += FString::Format(TEXT("\n{0}: {1}"), {Action->DisplayName.ToString(), Outcome.Rationale.ToString()});
            }
        }
    }
    Visit.Evaluation.ExpectedDiagnosis = CurrentCase->ExpectedDisease->DisplayName;
    const FText Debrief = FText::FromString(Explanation);
    Visit.Evaluation.Explanation = Debrief;
    Visit.Phase = EClinicVisitPhase::Completed;
    ReportResult(TEXT("SubmitDecision"));
    return {EClinicCommandStatus::Applied, Debrief};
}

FClinicCommandResult AClinicScenario::StartNextPatient(int32 ExpectedVisitIdentifier)
{
    if (Visit.VisitIdentifier != ExpectedVisitIdentifier || Visit.Phase != EClinicVisitPhase::Completed || GetVisitCount() == 0)
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Завершите текущий приём перед переходом к следующему пациенту."))};
    }
    const int32 NextIndex = (CurrentCaseIndex + 1) % GetVisitCount();
    if (!StartVisitAtIndex(NextIndex))
    {
        return {EClinicCommandStatus::Rejected, FText::FromString(TEXT("Следующий случай пациента не настроен."))};
    }
    return {EClinicCommandStatus::Applied, FText::FromString(TEXT("Начат приём следующего пациента."))};
}

FText AClinicScenario::GetReadout() const
{
    if (Visit.Phase == EClinicVisitPhase::NotStarted)
    {
        return FText::FromString(TEXT("Приём не начат."));
    }
    FString Readout = FString::Format(TEXT("{0}, {1}, {2}\nЖалобы: {3}\n"),
        {Visit.PatientName.ToString(), Visit.Age, Visit.Sex.ToString(), Visit.Complaint.ToString()});
    if (Patient)
    {
        const FText ExaminationReadout = Patient->GetExaminationReadout();
        if (!ExaminationReadout.IsEmpty())
        {
            Readout += ExaminationReadout.ToString() + TEXT("\n");
        }
    }
    for (const FClinicObservation& Observation : Visit.KnownObservations)
    {
        const FClinicIndicatorRow* Indicator = Observation.Indicator.GetRow<FClinicIndicatorRow>(TEXT("ClinicReadout"));
        if (Indicator)
        {
            FString Value = Observation.Description.ToString();
            if (Indicator->ValueType == EClinicValueType::Number)
            {
                Value = FString::Format(TEXT("{0} {1}"), {FText::AsNumber(Observation.NumericValue).ToString(), Indicator->Unit.ToString()});
                if (Observation.bHasReferenceRange)
                {
                    Value += FString::Format(TEXT(" (референс: {0}–{1})"),
                        {FText::AsNumber(Observation.ReferenceMinimum).ToString(), FText::AsNumber(Observation.ReferenceMaximum).ToString()});
                }
            }
            Readout += FString::Format(TEXT("{0}: {1}\n"), {Indicator->DisplayName.ToString(), Value});
        }
    }
    for (const FClinicInvestigationOrder& Order : Visit.InvestigationOrders)
    {
        const UClinicActionDefinition* Action = FindAction(Order.ActionIdentifier);
        if (Action)
        {
            FText State = FText::FromString(TEXT("выполняется"));
            if (Order.State == EClinicInvestigationState::Ready)
            {
                State = FText::FromString(TEXT("готово"));
            }
            else if (Visit.Phase != EClinicVisitPhase::Examination)
            {
                State = FText::FromString(TEXT("результат не получен"));
            }
            if (Order.bResultRead)
            {
                State = FText::FromString(TEXT("результат получен"));
            }
            Readout += FString::Format(TEXT("{0}: {1}\n"), {Action->DisplayName.ToString(), State.ToString()});
        }
    }
    for (const AClinicInstrument* Instrument : Instruments)
    {
        if (!Instrument || !Instrument->ActionDefinition)
        {
            continue;
        }
        const FClinicMeasurementState State = Instrument->GetMeasurementState();
        if (State.VisitIdentifier != Visit.VisitIdentifier)
        {
            continue;
        }
        if (State.Phase == EClinicMeasurementPhase::Measuring)
        {
            Readout += FString::Format(TEXT("{0}: измерение {1}%\n"),
                {Instrument->DisplayName.ToString(), FMath::RoundToInt(State.Progress * 100)});
        }
        else if (State.Phase == EClinicMeasurementPhase::ReadyToRemove)
        {
            Readout += FString::Format(TEXT("{0}: снимите прибор кнопкой Grip\n"), {Instrument->DisplayName.ToString()});
        }
        else if (State.Phase == EClinicMeasurementPhase::Completed)
        {
            Readout += FString::Format(TEXT("{0}: измерение завершено\n"), {Instrument->DisplayName.ToString()});
        }
        else if (Instrument->UsesFingerClip() && Instrument->bIsHeld && Visit.Phase == EClinicVisitPhase::Examination)
        {
            Readout += FString::Format(TEXT("{0}: поднесите к пальцу и нажмите Trigger\n"), {Instrument->DisplayName.ToString()});
        }
        else if (Instrument->bIsHeld && Instrument->bIsUsing && Visit.Phase == EClinicVisitPhase::Examination)
        {
            Readout += FString::Format(TEXT("{0}: поднесите рабочую часть к области измерения\n"), {Instrument->DisplayName.ToString()});
        }
    }
    if (Visit.Evaluation.bHasResult)
    {
        FText DiagnosisAssessment = FText::FromString(TEXT("ошибка в диагнозе"));
        FText EvidenceAssessment = FText::FromString(TEXT("сведений недостаточно"));
        if (Visit.Evaluation.bCorrectDiagnosis)
        {
            DiagnosisAssessment = FText::FromString(TEXT("диагноз выбран верно"));
        }
        if (Visit.Evaluation.bSufficientEvidence)
        {
            EvidenceAssessment = FText::FromString(TEXT("достаточные сведения собраны"));
        }
        Readout += FString::Format(TEXT("\nРазбор: {0}; {1}\nОжидаемый диагноз: {2}\nНеобоснованных назначений: {3}\n{4}"),
            {DiagnosisAssessment.ToString(), EvidenceAssessment.ToString(), Visit.Evaluation.ExpectedDiagnosis.ToString(),
                Visit.Evaluation.UnjustifiedActions.Num(), Visit.Evaluation.Explanation.ToString()});
    }
    return FText::FromString(Readout);
}
