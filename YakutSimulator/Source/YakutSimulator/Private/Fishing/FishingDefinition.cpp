#include "Fishing/FishingDefinition.h"
#include "Fishing/FishingFish.h"
#include "Misc/DataValidation.h"
bool UFishingDefinition::ValidateConfiguration(TArray<FText>& Errors) const
{
    Super::ValidateConfiguration(Errors);
    const auto Positive = [](float Value) { return FMath::IsFinite(Value) && Value > 0; };
    if (!Positive(Winch.DeploySeconds) || !Positive(Winch.TrawlSeconds) || !Positive(Winch.HaulSeconds) || !Positive(Winch.OverloadSeconds) ||
        !FMath::IsFinite(Winch.WarningLoad) || !FMath::IsFinite(Winch.OverloadLoad) || Winch.WarningLoad < 0 || Winch.WarningLoad >= Winch.OverloadLoad || Winch.OverloadLoad > 1 ||
        Winch.LoweredOffset.ContainsNaN() || !FMath::IsFinite(Winch.ResistanceBase) || !FMath::IsFinite(Winch.ResistanceAmplitude) ||
        !FMath::IsFinite(Winch.ResistanceFrequency) || !FMath::IsFinite(Winch.InputLoadScale))
        Errors.Add(NSLOCTEXT("Fishing.Validation", "Winch", "Invalid winch durations, load model or warning/overload thresholds."));
    if (Catch.FishCount < 1 || !Positive(Catch.ConveyorSpeed) || !Positive(Catch.FeedInterval) || Catch.RecoveryColumns < 1 || Catch.RecoverySpacing.ContainsNaN())
        Errors.Add(NSLOCTEXT("Fishing.Validation", "Catch", "Invalid catch size, conveyor or recovery layout."));
    if (!Positive(Processing.CutTolerance) || !Positive(Processing.BraceDistance) || !Positive(Processing.MaxKnifeStep) ||
        !FMath::IsFinite(Processing.StrokeWarningSeconds) || Processing.StrokeWarningSeconds < 0)
        Errors.Add(NSLOCTEXT("Fishing.Validation", "Processing", "Processing distances must be finite and positive."));
    if (!FMath::IsFinite(Scoring.DeliveredPoints) || !FMath::IsFinite(Scoring.SortingPenalty) || !FMath::IsFinite(Scoring.MissPenalty) || !FMath::IsFinite(Scoring.OverloadPenalty) ||
        Scoring.DeliveredPoints < 0 || Scoring.SortingPenalty < 0 || Scoring.MissPenalty < 0 || Scoring.OverloadPenalty < 0)
        Errors.Add(NSLOCTEXT("Fishing.Validation", "Score", "Score values must be finite and nonnegative."));
    if (SpeciesCycle.IsEmpty() || !KnifeClass) Errors.Add(NSLOCTEXT("Fishing.Validation", "Content", "Assign fish species and a knife Blueprint."));
    TMap<FName, UFishSpeciesDefinition*> Ids;
    for (UFishSpeciesDefinition* Species : SpeciesCycle)
    {
        if (!Species || Species->Id.IsNone() || !Species->ActorClass) { Errors.Add(NSLOCTEXT("Fishing.Validation", "Species", "Assign an identifier and actor Blueprint for every species.")); continue; }
        if (const auto* Existing = Ids.Find(Species->Id))
        {
            if (*Existing != Species) Errors.Add(NSLOCTEXT("Fishing.Validation", "DuplicateSpecies", "Different species assets must have distinct identifiers."));
            continue;
        }
        Ids.Add(Species->Id, Species);
        Species->ActorClass.GetDefaultObject()->ValidateItem(Errors);
    }
    if (KnifeClass) KnifeClass.GetDefaultObject()->ValidateItem(Errors);
    for (const auto& Objective : Objectives)
        if (!Objective.Operation.IsNone() && !Operations.Contains(Objective.Operation)) Errors.Add(FText::Format(NSLOCTEXT("Fishing.Validation", "Operation", "Unknown fishing operation: {0}"), FText::FromName(Objective.Operation)));
    return Errors.IsEmpty();
}
#if WITH_EDITOR
EDataValidationResult UFishSpeciesDefinition::IsDataValid(FDataValidationContext& Context) const
{
    TArray<FText> Errors;
    if (Id.IsNone() || !ActorClass) Errors.Add(NSLOCTEXT("Fishing.Validation", "SpeciesClass", "Assign a species identifier and actor Blueprint."));
    if (ActorClass) ActorClass.GetDefaultObject()->ValidateItem(Errors);
    for (const auto& Error : Errors) Context.AddError(Error);
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
