#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Minigames/Clinic/ClinicCaseBaseline.h"
#include "Minigames/Clinic/ClinicTypes.h"
#include "ClinicPatientCaseDefinition.generated.h"

class UClinicDiseaseDefinition;

USTRUCT()
struct FClinicEvidenceGroup
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta=(DisplayName="Все эти сведения", TitleProperty="{Indicator.RowName} {AreaIdentifier}"))
    TArray<FClinicFactKey> Facts;
};

USTRUCT()
struct FClinicUnjustifiedAction
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta=(DisplayName="Действие"))
    TObjectPtr<UClinicActionDefinition> Action;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Область", ToolTip="Пустая область означает любое выполнение этого действия"))
    FName AreaIdentifier;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Причина", MultiLine=true))
    FText Rationale;
};

USTRUCT()
struct FClinicResolvedCase
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FClinicObservation> InitialObservations;

    UPROPERTY()
    TArray<FClinicCaseOutcome> Outcomes;

    UPROPERTY()
    TArray<FClinicEvidenceSet> EvidenceSets;

    const FClinicCaseOutcome* FindOutcome(FName ActionIdentifier, FName AreaIdentifier) const;
};

UCLASS(BlueprintType, Blueprintable)
class YAKUTSIMULATOR_API UClinicPatientCaseDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="Приём", meta=(DisplayName="Жалобы", MultiLine=true))
    FText Complaint;

    UPROPERTY(EditAnywhere, Category="Приём", meta=(DisplayName="Начальная поза"))
    FName InitialPose;

    UPROPERTY(EditAnywhere, Category="Приём", meta=(DisplayName="Правильный диагноз"))
    TObjectPtr<UClinicDiseaseDefinition> ExpectedDisease;

    UPROPERTY(EditAnywhere, Category="Приём", meta=(DisplayName="Базовый набор"))
    TObjectPtr<UClinicCaseBaseline> Baseline;

    UPROPERTY(EditAnywhere, Category="Приём", meta=(DisplayName="Известно с начала приёма", TitleProperty="{Indicator.RowName} {AreaIdentifier}"))
    TArray<FClinicFactKey> InitialFacts;

    UPROPERTY(EditAnywhere, Category="Данные кейса", meta=(DisplayName="Отличия от базового набора", ShowOnlyInnerProperties))
    FClinicCaseData Overrides;

    UPROPERTY(EditAnywhere, Category="Оценка", meta=(DisplayName="Достаточные наборы сведений"))
    TArray<FClinicEvidenceGroup> EvidenceSets;

    UPROPERTY(EditAnywhere, Category="Оценка", meta=(DisplayName="Необоснованные действия", TitleProperty="{Action} {AreaIdentifier}"))
    TArray<FClinicUnjustifiedAction> UnjustifiedActions;

    UPROPERTY(EditAnywhere, Category="Оценка", meta=(DisplayName="Разбор", MultiLine=true))
    FText Debrief;

    // Existing packages retain their original fields until the case is converted.
    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    FName CaseIdentifier;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    FText PatientName;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    int32 Age = 30;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    FText Sex;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    FClinicPatientPresentation Presentation;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    TArray<FClinicObservation> InitialObservations;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    TArray<FClinicCaseOutcome> Outcomes;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    TArray<FClinicEvidenceSet> SufficientEvidenceSets;

    bool UsesLegacyData() const;
    TArray<FClinicCaseValue> GetEffectiveValues() const;
    bool Resolve(const TArray<TObjectPtr<UClinicActionDefinition>>& Actions, FClinicResolvedCase& Result, FText& Error) const;
    bool ValidateDefinition(FText& Error) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
    bool ValidateAndResolveValues(TArray<FClinicCaseValue>& Values, FText& Error) const;
    bool ValidateLegacyDefinition(FText& Error) const;
};
