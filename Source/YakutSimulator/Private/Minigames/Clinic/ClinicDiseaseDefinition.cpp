#include "Minigames/Clinic/ClinicDiseaseDefinition.h"

#include "Containers/ArrayView.h"
#include "Misc/DataValidation.h"
#include "Misc/Guid.h"

bool FClinicReferenceSymptom::Validate(FText& Error) const
{
    const FClinicIndicatorRow* Row = Indicator.GetRow<FClinicIndicatorRow>(TEXT("ClinicReference"));
    if (SymptomIdentifier.IsNone() || Description.IsEmpty() || !Row)
    {
        Error = FText::FromString(TEXT("Reference symptoms require an identifier, description and indicator"));
        return false;
    }
    TSet<FName> UniqueAreas;
    for (FName Area : Areas)
    {
        if (Area.IsNone() || UniqueAreas.Contains(Area))
        {
            Error = FText::FromString(TEXT("Reference symptom areas must be nonempty and unique"));
            return false;
        }
        UniqueAreas.Add(Area);
    }
    if (Row->ValueType == EClinicValueType::Number)
    {
        if ((!bHasMinimum && !bHasMaximum)
            || (bHasMinimum && !FMath::IsFinite(Minimum)) || (bHasMaximum && !FMath::IsFinite(Maximum))
            || (bHasMinimum && bHasMaximum && (Minimum > Maximum
                || (Minimum == Maximum && (!bIncludeMinimum || !bIncludeMaximum)))))
        {
            Error = FText::FromString(TEXT("Numeric reference symptoms require a valid nonempty interval"));
            return false;
        }
    }
    else
    {
        if (CategoryValues.IsEmpty())
        {
            Error = FText::FromString(TEXT("Choice reference symptoms require at least one category"));
            return false;
        }
        TSet<FName> UniqueCategories;
        for (FName Category : CategoryValues)
        {
            if (!Row->Categories.Contains(Category) || UniqueCategories.Contains(Category))
            {
                Error = FText::FromString(TEXT("Reference symptom categories must be valid and unique"));
                return false;
            }
            UniqueCategories.Add(Category);
        }
    }
    Error = FText::GetEmpty();
    return true;
}

EClinicReferenceSymptomStatus FClinicReferenceSymptom::Evaluate(const TArray<FClinicObservation>& Observations) const
{
    const FClinicIndicatorRow* Row = Indicator.GetRow<FClinicIndicatorRow>(TEXT("ClinicReference"));
    if (!Row)
    {
        return EClinicReferenceSymptomStatus::Unknown;
    }
    const FName WholePatient = NAME_None;
    TConstArrayView<FName> RequiredAreas = Areas;
    if (Areas.IsEmpty())
    {
        RequiredAreas = MakeArrayView(&WholePatient, 1);
    }
    bool bMissingData = false;
    for (FName Area : RequiredAreas)
    {
        bool bKnown = false;
        for (const FClinicObservation& Observation : Observations)
        {
            if (Observation.Indicator != Indicator || Observation.AreaIdentifier != Area)
            {
                continue;
            }
            if (Row->ValueType == EClinicValueType::Choice)
            {
                if (Row->Categories.Contains(Observation.CategoryValue))
                {
                    bKnown = true;
                    if (CategoryValues.Contains(Observation.CategoryValue))
                    {
                        return EClinicReferenceSymptomStatus::Confirmed;
                    }
                }
            }
            else if (FMath::IsFinite(Observation.NumericValue))
            {
                bKnown = true;
                const bool bBelow = bHasMinimum && (Observation.NumericValue < Minimum
                    || (!bIncludeMinimum && Observation.NumericValue == Minimum));
                const bool bAbove = bHasMaximum && (Observation.NumericValue > Maximum
                    || (!bIncludeMaximum && Observation.NumericValue == Maximum));
                if (!bBelow && !bAbove)
                {
                    return EClinicReferenceSymptomStatus::Confirmed;
                }
            }
        }
        if (!bKnown)
        {
            bMissingData = true;
        }
    }
    if (bMissingData)
    {
        return EClinicReferenceSymptomStatus::Unknown;
    }
    return EClinicReferenceSymptomStatus::NotConfirmed;
}

static FName MakeDiseaseIdentifier(const UClinicDiseaseDefinition& Disease)
{
    FString Name = Disease.GetName();
    Name.RemoveFromStart(TEXT("DA_Disease_"));
    const FName Identifier(*Name);
    if (Identifier.IsNone())
    {
        return Disease.GetFName();
    }
    return Identifier;
}

void UClinicDiseaseDefinition::PostInitProperties()
{
    Super::PostInitProperties();
    if (!IsTemplate() && DiseaseIdentifier.IsNone())
    {
        DiseaseIdentifier = MakeDiseaseIdentifier(*this);
    }
}

void UClinicDiseaseDefinition::PostLoad()
{
    Super::PostLoad();
    if (!IsTemplate())
    {
        if (DiseaseIdentifier.IsNone())
        {
            DiseaseIdentifier = MakeDiseaseIdentifier(*this);
        }
        EnsureSymptomIdentifiers();
    }
}

void UClinicDiseaseDefinition::EnsureSymptomIdentifiers()
{
    TSet<FName> Identifiers;
    for (FClinicReferenceSymptom& Symptom : Symptoms)
    {
        if (Symptom.SymptomIdentifier.IsNone() || Identifiers.Contains(Symptom.SymptomIdentifier))
        {
            Symptom.SymptomIdentifier = FName(*FGuid::NewGuid().ToString(EGuidFormats::Digits));
        }
        Identifiers.Add(Symptom.SymptomIdentifier);
    }
}

bool UClinicDiseaseDefinition::ValidateDefinition(FText& Error) const
{
    if (DiseaseIdentifier.IsNone() || DisplayName.IsEmpty() || ReferenceText.IsEmpty())
    {
        Error = FText::FromString(TEXT("Disease identifier, name and reference text are required"));
        return false;
    }
    TSet<FName> UniqueSymptoms;
    for (const FClinicReferenceSymptom& Symptom : Symptoms)
    {
        if (!Symptom.Validate(Error))
        {
            return false;
        }
        if (UniqueSymptoms.Contains(Symptom.SymptomIdentifier))
        {
            Error = FText::FromString(TEXT("Reference symptom identifiers must be unique within a disease"));
            return false;
        }
        UniqueSymptoms.Add(Symptom.SymptomIdentifier);
    }
    Error = FText::GetEmpty();
    return true;
}

#if WITH_EDITOR
void UClinicDiseaseDefinition::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
    Super::PostDuplicate(DuplicateMode);
    if (!IsTemplate() && DuplicateMode == EDuplicateMode::Normal)
    {
        DiseaseIdentifier = MakeDiseaseIdentifier(*this);
    }
}

void UClinicDiseaseDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    if (!IsTemplate())
    {
        EnsureSymptomIdentifiers();
    }
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

EDataValidationResult UClinicDiseaseDefinition::IsDataValid(FDataValidationContext& Context) const
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
