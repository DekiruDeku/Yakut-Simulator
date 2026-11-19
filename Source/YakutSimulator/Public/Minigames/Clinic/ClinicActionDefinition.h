#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Minigames/Clinic/ClinicTypes.h"
#include "Minigames/Clinic/ClinicPatientAnimationTypes.h"
#include "ClinicActionDefinition.generated.h"

UCLASS(BlueprintType, Blueprintable)
class YAKUTSIMULATOR_API UClinicActionDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic")
    EClinicActionType ActionType = EClinicActionType::Question;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic")
    FName ActionIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic|Результат",
        meta=(DisplayName="Открываемые показатели", TitleProperty="RowName", RowType="/Script/YakutSimulator.ClinicIndicatorRow"))
    TArray<FDataTableRowHandle> RevealedIndicators;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic",
        meta=(EditCondition="ActionType == EClinicActionType::Examination", EditConditionHides))
    FText Instruction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic",
        meta=(DisplayName="Этапы обследования", TitleProperty="PoseIdentifier", EditCondition="ActionType == EClinicActionType::Examination", EditConditionHides))
    TArray<FClinicExaminationStage> PresentationStages;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic",
        meta=(DisplayName="Требуется прибор", EditCondition="ActionType == EClinicActionType::Examination", EditConditionHides))
    bool bUsesInstrument = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic",
        meta=(ClampMin="0.1", Units="s", EditCondition="ActionType == EClinicActionType::Examination", EditConditionHides))
    float DurationSeconds = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic",
        meta=(ClampMin="0.1", Units="s", EditCondition="ActionType == EClinicActionType::Investigation", EditConditionHides))
    float InvestigationDelaySeconds = 5;

    UFUNCTION(BlueprintPure, Category="Clinic")
    TArray<FName> GetAreas() const;

    bool ValidateDefinition(FText& Error) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
