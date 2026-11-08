#include "Common/MiniGameDefinition.h"
#include "Common/MiniGamePanel.h"
#include "Misc/DataValidation.h"
#include "StateTree.h"

const FMiniGameObjective* UMiniGameDefinition::FindObjective(FName ObjectiveId) const
{
    return Objectives.FindByPredicate([&](const auto& Objective) { return Objective.Id == ObjectiveId; });
}
FText UMiniGameDefinition::Text(FName Key) const
{
    const FText* Value = Messages.Find(Key);
    return Value ? *Value : FText::FromName(Key);
}
bool UMiniGameDefinition::ValidateConfiguration(TArray<FText>& Errors) const
{
    if (Id.IsNone()) Errors.Add(NSLOCTEXT("MiniGame.Validation", "NoId", "Mini-game identifier is missing."));
    if (!Flow || !Flow->IsReadyToRun()) Errors.Add(NSLOCTEXT("MiniGame.Validation", "NoFlow", "A compiled StateTree is required."));
    if (!Interaction || !Interaction->LeftGrab || !Interaction->RightGrab || !Interaction->LeftRelease || !Interaction->RightRelease ||
        !Interaction->LeftUse || !Interaction->RightUse || !FMath::IsFinite(Interaction->Reach) || Interaction->Reach <= 0 ||
        !FMath::IsFinite(Interaction->DiagnosticInterval) || Interaction->DiagnosticInterval <= 0 || !FMath::IsFinite(Interaction->RawInputLogThreshold) || Interaction->RawInputLogThreshold < 0)
        Errors.Add(NSLOCTEXT("MiniGame.Validation", "Input", "Assign all six input actions and a positive interaction reach."));
    if (!Style || !Style->PanelClass || !FMath::IsFinite(Style->RefreshInterval) || Style->RefreshInterval <= 0)
        Errors.Add(NSLOCTEXT("MiniGame.Validation", "Style", "Assign a panel widget class and a positive refresh interval."));
    TSet<FName> Ids;
    for (const auto& Objective : Objectives)
    {
        if (Objective.Id.IsNone() || Ids.Contains(Objective.Id)) Errors.Add(NSLOCTEXT("MiniGame.Validation", "Objectives", "Objective identifiers must be nonempty and unique."));
        Ids.Add(Objective.Id);
    }
    if (Objectives.IsEmpty()) Errors.Add(NSLOCTEXT("MiniGame.Validation", "EmptyObjectives", "At least one objective is required."));
    if (!FMath::IsFinite(FadeOutSeconds) || !FMath::IsFinite(FadeInSeconds) || FadeOutSeconds < 0 || FadeInSeconds < 0)
        Errors.Add(NSLOCTEXT("MiniGame.Validation", "Fade", "Fade durations must be finite and nonnegative."));
    return Errors.IsEmpty();
}
#if WITH_EDITOR
EDataValidationResult UMiniGameDefinition::IsDataValid(FDataValidationContext& Context) const
{
    TArray<FText> Errors;
    ValidateConfiguration(Errors);
    for (const FText& Error : Errors) Context.AddError(Error);
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
