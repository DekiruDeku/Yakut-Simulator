#pragma once

#include "CoreMinimal.h"
#include "Base/WorkstationScenario.h"
#include "Minigames/Clinic/ClinicPatientCaseDefinition.h"
#include "Minigames/Clinic/ClinicTypes.h"
#include "ClinicScenario.generated.h"

class AClinicPatient;
class AClinicInstrument;
class UClinicActionDefinition;
class UClinicDiseaseDefinition;
class UWorkOperationComponent;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FClinicActionCompleted, const FClinicActionResult&, Result);

USTRUCT()
struct FClinicVisitDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta=(DisplayName="Пациент"))
    TSubclassOf<AClinicPatient> PatientClass;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Кейс"))
    TObjectPtr<UClinicPatientCaseDefinition> Case;
};

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AClinicScenario : public AWorkstationScenario
{
    GENERATED_BODY()

public:
    AClinicScenario();

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Clinic|Equipment", meta=(DisplayName="Место пациента"))
    TObjectPtr<AClinicPatient> Patient;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Clinic|Equipment")
    TArray<TObjectPtr<AClinicInstrument>> Instruments;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic|Catalog")
    TArray<TObjectPtr<UClinicActionDefinition>> ActionDefinitions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic|Catalog")
    TArray<TObjectPtr<UClinicDiseaseDefinition>> DiseaseDefinitions;

    UPROPERTY(EditDefaultsOnly, Category="Clinic|Cases", meta=(DisplayName="Список приёмов", TitleProperty="{Case} | {PatientClass}"))
    TArray<FClinicVisitDefinition> Visits;

    UPROPERTY(BlueprintReadOnly, Category="Clinic|Legacy")
    TArray<TObjectPtr<UClinicPatientCaseDefinition>> PatientCases;

    UPROPERTY(EditAnywhere, Category="Clinic|Cases", meta=(ClampMin="0"))
    int32 InitialCaseIndex = 0;

    UPROPERTY(BlueprintAssignable, Category="Clinic")
    FClinicActionCompleted OnClinicActionCompleted;

    UFUNCTION(BlueprintPure, Category="Clinic")
    FClinicVisitSnapshot GetVisitSnapshot() const;

    UFUNCTION(BlueprintPure, Category="Clinic")
    TArray<UClinicActionDefinition*> GetActionDefinitions() const;

    UFUNCTION(BlueprintPure, Category="Clinic")
    TArray<UClinicDiseaseDefinition*> GetDiseaseDefinitions() const;

    UFUNCTION(BlueprintPure, Category="Clinic|Reference")
    bool GetReferenceSymptomStatus(FName DiseaseIdentifier, FName SymptomIdentifier, int32 ExpectedVisitIdentifier,
        EClinicReferenceSymptomStatus& Status) const;

    UFUNCTION(BlueprintPure, Category="Clinic")
    bool ValidateClinicSetup(FText& Error) const;

    UFUNCTION(BlueprintCallable, Category="Clinic")
    FClinicCommandResult AskQuestion(FName ActionIdentifier, int32 ExpectedVisitIdentifier);

    UFUNCTION(BlueprintCallable, Category="Clinic")
    FClinicCommandResult RequestExamination(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier);

    UFUNCTION(BlueprintCallable, Category="Clinic")
    FClinicCommandResult OrderInvestigation(FName ActionIdentifier, int32 ExpectedVisitIdentifier);

    UFUNCTION(BlueprintCallable, Category="Clinic")
    FClinicCommandResult ReadInvestigation(FName ActionIdentifier, int32 ExpectedVisitIdentifier);

    UFUNCTION(BlueprintCallable, Category="Clinic")
    FClinicCommandResult FinishExamination(int32 ExpectedVisitIdentifier);

    UFUNCTION(BlueprintCallable, Category="Clinic")
    FClinicCommandResult SubmitDecision(const FClinicDecision& Decision, int32 ExpectedVisitIdentifier);

    UFUNCTION(BlueprintCallable, Category="Clinic")
    FClinicCommandResult StartNextPatient(int32 ExpectedVisitIdentifier);

    virtual void RestartWorkstation() override;
    virtual FText GetReadout() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

    bool StartVisit(UClinicPatientCaseDefinition* PatientCase);
    const FClinicVisitSnapshot& GetVisitState() const { return Visit; }
    bool IsExamining(int32 ExpectedVisitIdentifier) const;
    UClinicActionDefinition* FindAction(FName Identifier) const;
    FClinicCommandResult RecordExamination(AActor* Executor, FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier,
        TArray<FClinicObservation>* Observations = nullptr);
    USoundBase* GetExaminationSound(AClinicInstrument* Instrument, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, Category="Clinic|Operations")
    TObjectPtr<UWorkOperationComponent> StartOperation;

    UPROPERTY(VisibleAnywhere, Category="Clinic|Operations")
    TObjectPtr<UWorkOperationComponent> ExaminationOperation;

    UPROPERTY(VisibleAnywhere, Category="Clinic|Operations")
    TObjectPtr<UWorkOperationComponent> DecisionOperation;

    UPROPERTY(Transient)
    TObjectPtr<UClinicPatientCaseDefinition> CurrentCase;

    UPROPERTY(Transient)
    TObjectPtr<AClinicPatient> PatientPlacement;

    UPROPERTY(Transient)
    FClinicResolvedCase ResolvedCase;

    UPROPERTY(Transient)
    FClinicVisitSnapshot Visit;

    int32 CurrentCaseIndex = 0;
    int32 GetVisitCount() const;
    bool StartVisitAtIndex(int32 CaseIndex);
    void StopExecutors(bool bClearResults);
    FClinicCommandResult RevealOutcome(const FClinicCaseOutcome& Outcome, AActor* Executor);
};
