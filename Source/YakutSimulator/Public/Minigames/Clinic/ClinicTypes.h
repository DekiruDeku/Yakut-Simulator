#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ClinicTypes.generated.h"

class AActor;
class USoundBase;
class UTexture2D;
class USkeletalMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class EClinicValueType : uint8
{
    Number,
    Choice
};

UENUM(BlueprintType)
enum class EClinicActionType : uint8
{
    Question,
    Examination,
    Investigation
};

UENUM(BlueprintType)
enum class EClinicVisitPhase : uint8
{
    NotStarted,
    Examination,
    Diagnosis,
    Completed
};

UENUM(BlueprintType)
enum class EClinicCommandStatus : uint8
{
    Applied,
    Unchanged,
    Rejected
};

UENUM(BlueprintType)
enum class EClinicInvestigationState : uint8
{
    InProgress,
    Ready
};

UENUM(BlueprintType)
enum class EClinicMeasurementPhase : uint8
{
    Idle,
    Measuring,
    Completed,
    ReadyToRemove
};

UENUM(BlueprintType)
enum class EClinicReferenceSymptomStatus : uint8
{
    Unknown,
    Confirmed,
    NotConfirmed
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicIndicatorRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EClinicValueType ValueType = EClinicValueType::Number;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Unit;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FName> Categories;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicObservation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ObservationIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FDataTableRowHandle Indicator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double NumericValue = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CategoryValue;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AreaIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName SourceAction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bHasReferenceRange = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double ReferenceMinimum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    double ReferenceMaximum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UTexture2D> Image;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicReferenceSymptom
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName SymptomIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Подпись в справочнике", MultiLine=true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Показатель", RowType="/Script/YakutSimulator.ClinicIndicatorRow"))
    FDataTableRowHandle Indicator;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Области", ToolTip="Имена областей из этапов обследования. Пустой список означает показатель без области"))
    TArray<FName> Areas;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Подходящие варианты"))
    TArray<FName> CategoryValues;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(InlineEditConditionToggle))
    bool bHasMinimum = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Нижняя граница", EditCondition="bHasMinimum"))
    double Minimum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Включая нижнюю границу", EditCondition="bHasMinimum", EditConditionHides, HideEditConditionToggle))
    bool bIncludeMinimum = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(InlineEditConditionToggle))
    bool bHasMaximum = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Верхняя граница", EditCondition="bHasMaximum"))
    double Maximum = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Включая верхнюю границу", EditCondition="bHasMaximum", EditConditionHides, HideEditConditionToggle))
    bool bIncludeMaximum = true;

    bool Validate(FText& Error) const;
    EClinicReferenceSymptomStatus Evaluate(const TArray<FClinicObservation>& Observations) const;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicCommandResult
{
    GENERATED_BODY()

    FClinicCommandResult() = default;
    FClinicCommandResult(EClinicCommandStatus InStatus, const FText& InMessage)
        : Status(InStatus), Message(InMessage)
    {
    }

    UPROPERTY(BlueprintReadOnly)
    EClinicCommandStatus Status = EClinicCommandStatus::Rejected;

    UPROPERTY(BlueprintReadOnly)
    FText Message;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicPatientPresentation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<USkeletalMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UMaterialInterface> Material;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName PoseIdentifier;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicCaseOutcome
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FName ActionIdentifier;

    UPROPERTY(EditAnywhere)
    FName AreaIdentifier;

    UPROPERTY(EditAnywhere)
    FText Response;

    UPROPERTY(EditAnywhere)
    TArray<FClinicObservation> Observations;

    UPROPERTY(EditAnywhere)
    TObjectPtr<USoundBase> Sound;

    UPROPERTY()
    TObjectPtr<UTexture2D> Image;

    UPROPERTY(EditAnywhere)
    bool bUnjustified = false;

    UPROPERTY(EditAnywhere)
    FText Rationale;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicEvidenceSet
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    TArray<FName> Observations;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicActionRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName ActionIdentifier;

    UPROPERTY(BlueprintReadOnly)
    FName AreaIdentifier;

    UPROPERTY(BlueprintReadOnly)
    FText Response;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicInvestigationOrder
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName ActionIdentifier;

    UPROPERTY(BlueprintReadOnly)
    EClinicInvestigationState State = EClinicInvestigationState::InProgress;

    UPROPERTY(BlueprintReadOnly)
    float SecondsRemaining = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bResultRead = false;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicDecision
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName DiseaseIdentifier;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicEvaluation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bHasResult = false;

    UPROPERTY(BlueprintReadOnly)
    bool bCorrectDiagnosis = false;

    UPROPERTY(BlueprintReadOnly)
    bool bSufficientEvidence = false;

    UPROPERTY(BlueprintReadOnly)
    TArray<FName> UnjustifiedActions;

    UPROPERTY(BlueprintReadOnly)
    FText ExpectedDiagnosis;

    UPROPERTY(BlueprintReadOnly)
    FText Explanation;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicVisitSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 VisitIdentifier = 0;

    UPROPERTY(BlueprintReadOnly)
    EClinicVisitPhase Phase = EClinicVisitPhase::NotStarted;

    UPROPERTY(BlueprintReadOnly)
    FText PatientName;

    UPROPERTY(BlueprintReadOnly)
    FText Complaint;

    UPROPERTY(BlueprintReadOnly)
    int32 Age = 0;

    UPROPERTY(BlueprintReadOnly)
    FText Sex;

    UPROPERTY(BlueprintReadOnly)
    TArray<FClinicObservation> KnownObservations;

    UPROPERTY(BlueprintReadOnly)
    TArray<FClinicActionRecord> ActionHistory;

    UPROPERTY(BlueprintReadOnly)
    TArray<FClinicInvestigationOrder> InvestigationOrders;

    UPROPERTY(BlueprintReadOnly)
    FClinicDecision SubmittedDecision;

    UPROPERTY(BlueprintReadOnly)
    FClinicEvaluation Evaluation;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicActionResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 VisitIdentifier = 0;

    UPROPERTY(BlueprintReadOnly)
    FName ActionIdentifier;

    UPROPERTY(BlueprintReadOnly)
    FName AreaIdentifier;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> Executor;

    UPROPERTY(BlueprintReadOnly)
    TArray<FClinicObservation> Observations;

    UPROPERTY(BlueprintReadOnly)
    FText Response;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<USoundBase> Sound;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UTexture2D> Image;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicMeasurementState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    EClinicMeasurementPhase Phase = EClinicMeasurementPhase::Idle;

    UPROPERTY(BlueprintReadOnly)
    int32 VisitIdentifier = 0;

    UPROPERTY(BlueprintReadOnly)
    FName AreaIdentifier;

    UPROPERTY(BlueprintReadOnly)
    float Progress = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<FClinicObservation> Observations;
};
