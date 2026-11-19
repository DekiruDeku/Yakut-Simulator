#include "Minigames/Clinic/ClinicPatientCaseDefinition.h"

#include "Minigames/Clinic/ClinicActionDefinition.h"
#include "Minigames/Clinic/ClinicDiseaseDefinition.h"
#include "Misc/DataValidation.h"

const FClinicCaseOutcome* FClinicResolvedCase::FindOutcome(FName ActionIdentifier, FName AreaIdentifier) const
{
    return Outcomes.FindByPredicate([ActionIdentifier, AreaIdentifier](const FClinicCaseOutcome& Outcome)
    {
        return Outcome.ActionIdentifier == ActionIdentifier && Outcome.AreaIdentifier == AreaIdentifier;
    });
}

bool UClinicPatientCaseDefinition::UsesLegacyData() const
{
    return !Baseline && EvidenceSets.IsEmpty() && !Outcomes.IsEmpty();
}

static const FClinicCaseValue* FindCaseValue(const TArray<FClinicCaseValue>& Values, const FClinicFactKey& Fact)
{
    return Values.FindByPredicate([&Fact](const FClinicCaseValue& Value)
    {
        return Value.Fact == Fact;
    });
}

TArray<FClinicCaseValue> UClinicPatientCaseDefinition::GetEffectiveValues() const
{
    TArray<FClinicCaseValue> Values;
    if (Baseline)
    {
        Values = Baseline->Data.Values;
    }
    for (const FClinicCaseValue& Override : Overrides.Values)
    {
        const int32 Index = Values.IndexOfByPredicate([&Override](const FClinicCaseValue& Value)
        {
            return Value.Fact == Override.Fact;
        });
        if (Index == INDEX_NONE)
        {
            Values.Add(Override);
            continue;
        }
        FClinicCaseValue Value = Override;
        if (Value.ReferenceRange == EClinicReferenceRangeMode::Inherit)
        {
            Value.ReferenceRange = Values[Index].ReferenceRange;
            Value.ReferenceMinimum = Values[Index].ReferenceMinimum;
            Value.ReferenceMaximum = Values[Index].ReferenceMaximum;
        }
        Values[Index] = MoveTemp(Value);
    }
    return Values;
}

const FClinicIndicatorRow* FClinicFactKey::GetIndicator() const
{
    if (Indicator.RowName.IsNone() || !Indicator.DataTable || Indicator.DataTable->GetRowStruct() != FClinicIndicatorRow::StaticStruct())
    {
        return nullptr;
    }
    return Indicator.DataTable->FindRow<FClinicIndicatorRow>(Indicator.RowName, TEXT("Clinic case"), false);
}

static bool ValidateCaseValues(const TArray<FClinicCaseValue>& Values, FText& Error)
{
    TArray<FClinicFactKey> Seen;
    for (const FClinicCaseValue& Value : Values)
    {
        const FClinicIndicatorRow* Indicator = Value.Fact.GetIndicator();
        if (!Indicator || Seen.Contains(Value.Fact)
            || (Indicator->ValueType == EClinicValueType::Choice && !Indicator->Categories.Contains(Value.CategoryValue))
            || (Indicator->ValueType == EClinicValueType::Number && !FMath::IsFinite(Value.NumericValue)))
        {
            Error = FText::FromString(FString::Format(TEXT("Missing, invalid or duplicate case value: {0}/{1}"),
                {Value.Fact.Indicator.RowName.ToString(), Value.Fact.AreaIdentifier.ToString()}));
            return false;
        }
        if (Indicator->ValueType == EClinicValueType::Number && Value.ReferenceRange == EClinicReferenceRangeMode::Custom
            && (!FMath::IsFinite(Value.ReferenceMinimum)
                || !FMath::IsFinite(Value.ReferenceMaximum) || Value.ReferenceMinimum > Value.ReferenceMaximum))
        {
            Error = FText::FromString(TEXT("Case reference interval is invalid"));
            return false;
        }
        Seen.Add(Value.Fact);
    }
    return true;
}

static const FClinicCaseMessage* FindCaseMessage(const TArray<FClinicCaseMessage>& Messages, const UClinicActionDefinition* Action, FName Area)
{
    return Messages.FindByPredicate([Action, Area](const FClinicCaseMessage& Message)
    {
        return Message.Action == Action && Message.AreaIdentifier == Area;
    });
}

static bool ValidateCaseMessages(const TArray<FClinicCaseMessage>& Messages, bool bQuestions, FText& Error)
{
    for (const FClinicCaseMessage& Message : Messages)
    {
        if (!Message.Action || (Message.Action->ActionType == EClinicActionType::Question) != bQuestions)
        {
            Error = FText::FromString(TEXT("Case answers require questions; reports require examinations or investigations"));
            return false;
        }
        const TArray<FName> Areas = Message.Action->GetAreas();
        if ((Areas.IsEmpty() && !Message.AreaIdentifier.IsNone()) || (!Areas.IsEmpty() && !Areas.Contains(Message.AreaIdentifier))
            || FindCaseMessage(Messages, Message.Action, Message.AreaIdentifier) != &Message)
        {
            Error = FText::FromString(TEXT("Each case answer or report needs a unique action and valid area"));
            return false;
        }
    }
    return true;
}

bool UClinicPatientCaseDefinition::ValidateDefinition(FText& Error) const
{
    TArray<FClinicCaseValue> Values;
    return ValidateAndResolveValues(Values, Error);
}

bool UClinicPatientCaseDefinition::ValidateAndResolveValues(TArray<FClinicCaseValue>& Values, FText& Error) const
{
    if (UsesLegacyData())
    {
        return ValidateLegacyDefinition(Error);
    }
    if (Complaint.IsEmpty() || !ExpectedDisease || EvidenceSets.IsEmpty())
    {
        Error = FText::FromString(TEXT("Complaint, expected disease and evidence groups are required"));
        return false;
    }
    if (!ValidateCaseValues(Overrides.Values, Error) || !ValidateCaseMessages(Overrides.Answers, true, Error)
        || !ValidateCaseMessages(Overrides.SpecialResults, false, Error)
        || (Baseline && (!ValidateCaseValues(Baseline->Data.Values, Error) || !ValidateCaseMessages(Baseline->Data.Answers, true, Error)
            || !ValidateCaseMessages(Baseline->Data.SpecialResults, false, Error))))
    {
        return false;
    }
    Values = GetEffectiveValues();
    for (const FClinicFactKey& Fact : InitialFacts)
    {
        if (!FindCaseValue(Values, Fact))
        {
            Error = FText::FromString(TEXT("An initial fact has no case value"));
            return false;
        }
    }
    for (const FClinicEvidenceGroup& Group : EvidenceSets)
    {
        if (Group.Facts.IsEmpty())
        {
            Error = FText::FromString(TEXT("Evidence groups must not be empty"));
            return false;
        }
        TArray<FClinicFactKey> Seen;
        for (const FClinicFactKey& Fact : Group.Facts)
        {
            if (!FindCaseValue(Values, Fact) || Seen.Contains(Fact))
            {
                Error = FText::FromString(TEXT("Evidence groups require unique facts with case values"));
                return false;
            }
            Seen.Add(Fact);
        }
    }
    Error = FText::GetEmpty();
    return true;
}

static FName FactIdentifier(const FClinicFactKey& Fact)
{
    return FName(*FString::Format(TEXT("{0}|{1}|{2}"),
        {Fact.Indicator.DataTable->GetPathName(), Fact.Indicator.RowName.ToString(), Fact.AreaIdentifier.ToString()}));
}

static FClinicObservation MakeObservation(const FClinicCaseValue& Value, FName SourceAction, const FText& Response)
{
    FClinicObservation Result;
    Result.ObservationIdentifier = FactIdentifier(Value.Fact);
    Result.Indicator = Value.Fact.Indicator;
    Result.AreaIdentifier = Value.Fact.AreaIdentifier;
    Result.SourceAction = SourceAction;
    Result.Description = Value.Description;
    Result.Image = Value.Image;
    if (Value.Fact.GetIndicator()->ValueType == EClinicValueType::Number)
    {
        Result.NumericValue = Value.NumericValue;
        Result.bHasReferenceRange = Value.ReferenceRange == EClinicReferenceRangeMode::Custom;
        Result.ReferenceMinimum = Value.ReferenceMinimum;
        Result.ReferenceMaximum = Value.ReferenceMaximum;
    }
    else
    {
        Result.CategoryValue = Value.CategoryValue;
        if (Result.Description.IsEmpty())
        {
            Result.Description = Response;
            if (Result.Description.IsEmpty())
            {
                Result.Description = FText::FromName(Result.CategoryValue);
            }
        }
    }
    return Result;
}

static bool CanInheritCaseMessage(const UClinicActionDefinition& Action, FName Area,
    const TArray<FClinicCaseValue>& BaselineValues, const TArray<FClinicCaseValue>& Values)
{
    for (const FDataTableRowHandle& Indicator : Action.RevealedIndicators)
    {
        const FClinicFactKey Fact{Indicator, Area};
        const FClinicIndicatorRow* Definition = Fact.GetIndicator();
        if (!Definition || Definition->ValueType != EClinicValueType::Choice)
        {
            continue;
        }
        const FClinicCaseValue* Value = FindCaseValue(Values, Fact);
        if (!Value)
        {
            continue;
        }
        const FClinicCaseValue* Baseline = FindCaseValue(BaselineValues, Fact);
        if (!Baseline || Value->CategoryValue != Baseline->CategoryValue)
        {
            return false;
        }
    }
    return true;
}

bool UClinicPatientCaseDefinition::Resolve(const TArray<TObjectPtr<UClinicActionDefinition>>& Actions, FClinicResolvedCase& Result, FText& Error) const
{
    Result = FClinicResolvedCase();
    TArray<FClinicCaseValue> Values;
    if (!ValidateAndResolveValues(Values, Error))
    {
        return false;
    }
    if (UsesLegacyData())
    {
        Result.InitialObservations = InitialObservations;
        Result.Outcomes = Outcomes;
        Result.EvidenceSets = SufficientEvidenceSets;
        return true;
    }

    TSet<FName> Obtainable;
    for (const FClinicFactKey& Fact : InitialFacts)
    {
        FClinicObservation Observation = MakeObservation(*FindCaseValue(Values, Fact), NAME_None, FText());
        if (!Obtainable.Contains(Observation.ObservationIdentifier))
        {
            Obtainable.Add(Observation.ObservationIdentifier);
            Result.InitialObservations.Add(MoveTemp(Observation));
        }
    }
    for (const FClinicUnjustifiedAction& Rule : UnjustifiedActions)
    {
        if (!Rule.Action || !Actions.Contains(Rule.Action)
            || (!Rule.AreaIdentifier.IsNone() && !Rule.Action->GetAreas().Contains(Rule.AreaIdentifier)))
        {
            Error = FText::FromString(TEXT("An unjustified action rule references an unknown action or area"));
            return false;
        }
    }
    for (const UClinicActionDefinition* Action : Actions)
    {
        if (!Action)
        {
            continue;
        }
        TArray<FName> Areas = Action->GetAreas();
        if (Areas.IsEmpty())
        {
            Areas.Add(NAME_None);
        }
        for (FName Area : Areas)
        {
            FClinicCaseOutcome& Outcome = Result.Outcomes.AddDefaulted_GetRef();
            Outcome.ActionIdentifier = Action->ActionIdentifier;
            Outcome.AreaIdentifier = Area;
            const bool bQuestion = Action->ActionType == EClinicActionType::Question;
            const FClinicCaseMessage* Message = FindCaseMessage(bQuestion ? Overrides.Answers : Overrides.SpecialResults, Action, Area);
            if (!Message && Baseline && CanInheritCaseMessage(*Action, Area, Baseline->Data.Values, Values))
            {
                Message = FindCaseMessage(bQuestion ? Baseline->Data.Answers : Baseline->Data.SpecialResults, Action, Area);
            }
            if (Message)
            {
                Outcome.Response = Message->Response;
                Outcome.Sound = Message->Sound;
                Outcome.Image = Message->Image;
            }
            for (const FDataTableRowHandle& Indicator : Action->RevealedIndicators)
            {
                const FClinicFactKey Fact{Indicator, Area};
                if (const FClinicCaseValue* Value = FindCaseValue(Values, Fact))
                {
                    FClinicObservation Observation = MakeObservation(*Value, Action->ActionIdentifier, Outcome.Response);
                    Obtainable.Add(Observation.ObservationIdentifier);
                    Outcome.Observations.Add(MoveTemp(Observation));
                }
            }
            if (Outcome.Response.IsEmpty() && Outcome.Observations.IsEmpty() && !Outcome.Sound && !Outcome.Image)
            {
                Error = FText::FromString(FString::Format(TEXT("Case {0} is missing a result for {1}/{2}"),
                    {GetName(), Action->ActionIdentifier.ToString(), Area.ToString()}));
                return false;
            }
            if (Outcome.Image && !Outcome.Observations.IsEmpty() && !Outcome.Observations[0].Image)
            {
                Outcome.Observations[0].Image = Outcome.Image;
            }
            for (const FClinicUnjustifiedAction& Rule : UnjustifiedActions)
            {
                if (Rule.Action == Action && (Rule.AreaIdentifier.IsNone() || Rule.AreaIdentifier == Area))
                {
                    Outcome.bUnjustified = true;
                    Outcome.Rationale = Rule.Rationale;
                    break;
                }
            }
        }
    }
    for (const FClinicEvidenceGroup& Group : EvidenceSets)
    {
        FClinicEvidenceSet& Evidence = Result.EvidenceSets.AddDefaulted_GetRef();
        for (const FClinicFactKey& Fact : Group.Facts)
        {
            const FName Identifier = FactIdentifier(Fact);
            if (!Obtainable.Contains(Identifier))
            {
                Error = FText::FromString(FString::Format(TEXT("Evidence is unreachable in {0}: {1}/{2}"),
                    {GetName(), Fact.Indicator.RowName.ToString(), Fact.AreaIdentifier.ToString()}));
                return false;
            }
            Evidence.Observations.Add(Identifier);
        }
    }
    return true;
}

bool UClinicPatientCaseDefinition::ValidateLegacyDefinition(FText& Error) const
{
    if (CaseIdentifier.IsNone() || PatientName.IsEmpty() || Complaint.IsEmpty() || Age < 0
        || !ExpectedDisease || SufficientEvidenceSets.IsEmpty())
    {
        Error = FText::FromString(TEXT("Case identity, complaint, nonnegative age, disease and evidence sets are required"));
        return false;
    }
    TSet<FName> ObservationIdentifiers;
    TArray<FClinicObservation> AllObservations = InitialObservations;
    TSet<FString> OutcomeIdentifiers;
    for (const FClinicCaseOutcome& Outcome : Outcomes)
    {
        const FString Key = FString::Format(TEXT("{0}/{1}"), {Outcome.ActionIdentifier.ToString(), Outcome.AreaIdentifier.ToString()});
        if (Outcome.ActionIdentifier.IsNone() || OutcomeIdentifiers.Contains(Key)
            || (Outcome.Response.IsEmpty() && Outcome.Observations.IsEmpty()))
        {
            Error = FText::FromString(TEXT("Case outcomes must be unique and contain a response or observations"));
            return false;
        }
        OutcomeIdentifiers.Add(Key);
        for (const FClinicObservation& Observation : Outcome.Observations)
        {
            if (Observation.SourceAction != Outcome.ActionIdentifier || Observation.AreaIdentifier != Outcome.AreaIdentifier)
            {
                Error = FText::FromString(TEXT("Observation source and area must match its outcome"));
                return false;
            }
        }
        AllObservations.Append(Outcome.Observations);
    }
    for (const FClinicObservation& Observation : AllObservations)
    {
        if (Observation.ObservationIdentifier.IsNone() || ObservationIdentifiers.Contains(Observation.ObservationIdentifier)
            || !Observation.Indicator.DataTable
            || Observation.Indicator.DataTable->GetRowStruct() != FClinicIndicatorRow::StaticStruct())
        {
            Error = FText::FromString(TEXT("Observation identifiers must be unique and reference the indicator catalog"));
            return false;
        }
        const FClinicIndicatorRow* Indicator = Observation.Indicator.GetRow<FClinicIndicatorRow>(TEXT("ClinicValidation"));
        if (!Indicator || !FMath::IsFinite(Observation.NumericValue)
            || (Indicator->ValueType == EClinicValueType::Choice && !Indicator->Categories.Contains(Observation.CategoryValue)))
        {
            Error = FText::FromString(TEXT("Observation value does not match its indicator definition"));
            return false;
        }
        if (Observation.bHasReferenceRange
            && (Indicator->ValueType != EClinicValueType::Number || !FMath::IsFinite(Observation.ReferenceMinimum)
                || !FMath::IsFinite(Observation.ReferenceMaximum) || Observation.ReferenceMinimum > Observation.ReferenceMaximum))
        {
            Error = FText::FromString(TEXT("Observation reference interval is invalid"));
            return false;
        }
        ObservationIdentifiers.Add(Observation.ObservationIdentifier);
    }
    for (const FClinicEvidenceSet& Evidence : SufficientEvidenceSets)
    {
        TSet<FName> UniqueEvidence;
        if (Evidence.Observations.IsEmpty())
        {
            Error = FText::FromString(TEXT("Evidence sets must not be empty"));
            return false;
        }
        for (FName Identifier : Evidence.Observations)
        {
            if (!ObservationIdentifiers.Contains(Identifier) || UniqueEvidence.Contains(Identifier))
            {
                Error = FText::FromString(TEXT("Evidence must reference unique observations obtainable in this case"));
                return false;
            }
            UniqueEvidence.Add(Identifier);
        }
    }
    Error = FText::GetEmpty();
    return true;
}

#if WITH_EDITOR
EDataValidationResult UClinicPatientCaseDefinition::IsDataValid(FDataValidationContext& Context) const
{
    if (IsTemplate())
    {
        return EDataValidationResult::NotValidated;
    }
    FText Error;
    if (!ValidateDefinition(Error))
    {
        Context.AddError(Error);
        return EDataValidationResult::Invalid;
    }
    return EDataValidationResult::Valid;
}
#endif
