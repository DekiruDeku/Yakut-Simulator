#include "Minigames/Clinic/ClinicActionDefinition.h"

#include "Misc/DataValidation.h"

TArray<FName> UClinicActionDefinition::GetAreas() const
{
    TArray<FName> Areas;
    for (const FClinicExaminationStage& Stage : PresentationStages)
    {
        Areas.Append(Stage.Areas);
    }
    return Areas;
}

bool UClinicActionDefinition::ValidateDefinition(FText& Error) const
{
    if (ActionIdentifier.IsNone() || DisplayName.IsEmpty())
    {
        Error = FText::FromString(TEXT("Action identifier and display name are required"));
        return false;
    }
    TArray<FDataTableRowHandle> UniqueIndicators;
    for (const FDataTableRowHandle& Indicator : RevealedIndicators)
    {
        if (!Indicator.DataTable || Indicator.DataTable->GetRowStruct() != FClinicIndicatorRow::StaticStruct()
            || !Indicator.DataTable->FindRow<FClinicIndicatorRow>(Indicator.RowName, TEXT("Clinic action"), false)
            || UniqueIndicators.Contains(Indicator))
        {
            Error = FText::FromString(TEXT("Revealed indicators must be valid and unique"));
            return false;
        }
        UniqueIndicators.Add(Indicator);
    }
    if (ActionType == EClinicActionType::Examination
        && (!FMath::IsFinite(DurationSeconds) || DurationSeconds <= 0))
    {
        Error = FText::FromString(TEXT("Examination duration must be finite and positive"));
        return false;
    }
    if (ActionType == EClinicActionType::Investigation
        && (!FMath::IsFinite(InvestigationDelaySeconds) || InvestigationDelaySeconds <= 0))
    {
        Error = FText::FromString(TEXT("Investigation delay must be finite and positive"));
        return false;
    }
    if (ActionType != EClinicActionType::Examination && (bUsesInstrument || !PresentationStages.IsEmpty()))
    {
        Error = FText::FromString(TEXT("Only examinations can define instruments and body areas"));
        return false;
    }
    if (bUsesInstrument && PresentationStages.IsEmpty())
    {
        Error = FText::FromString(TEXT("Instrument examinations require a body area"));
        return false;
    }
    TSet<FName> StagedAreas;
    for (const FClinicExaminationStage& Stage : PresentationStages)
    {
        if (Stage.PoseIdentifier.IsNone() || Stage.Areas.IsEmpty())
        {
            Error = FText::FromString(TEXT("Examination stages require a pose and body areas"));
            return false;
        }
        for (FName Area : Stage.Areas)
        {
            if (Area.IsNone() || StagedAreas.Contains(Area))
            {
                Error = FText::FromString(TEXT("Examination areas must be nonempty and unique across stages"));
                return false;
            }
            StagedAreas.Add(Area);
        }
    }
    Error = FText::GetEmpty();
    return true;
}

#if WITH_EDITOR
EDataValidationResult UClinicActionDefinition::IsDataValid(FDataValidationContext& Context) const
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
