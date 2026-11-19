#pragma once

#include "CoreMinimal.h"
#include "Base/WorkItem.h"
#include "Minigames/Clinic/ClinicTypes.h"
#include "ClinicInstrument.generated.h"

class AClinicScenario;
class AClinicPatient;
class UClinicActionDefinition;
class UAudioComponent;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AClinicInstrument : public AWorkItem
{
    GENERATED_BODY()

public:
    AClinicInstrument();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic")
    TObjectPtr<UClinicActionDefinition> ActionDefinition;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic", meta=(ClampMin="0.1", Units="s"))
    float FingerClipDurationSeconds = 2.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic", meta=(ClampMin="0", Units="cm"))
    float FingerClipContactTolerance = 3.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic")
    TObjectPtr<USceneComponent> WorkingPoint;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic")
    TObjectPtr<UAudioComponent> ExaminationAudio;

    UFUNCTION(BlueprintPure, Category="Clinic")
    FClinicMeasurementState GetMeasurementState() const;

    UFUNCTION(BlueprintImplementableEvent, Category="Clinic")
    void ReceiveMeasurementStateChanged(const FClinicMeasurementState& State);

    virtual void ReceiveUseStarted_Implementation() override;
    virtual void ReceiveUseEnded_Implementation() override;
    virtual bool CanBeGrabbed() const override;
    virtual bool TryGrab(USceneComponent* Hand) override;

    bool UsesFingerClip() const;
    void InitializeInstrument(AClinicScenario* InScenario, int32 InVisitIdentifier);
    void CancelMeasurement(bool bClearResults);
    bool CanRecordMeasurement(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const;
    bool HasValidContact(FName AreaIdentifier, int32 ExpectedVisitIdentifier) const;

protected:
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<AClinicScenario> Scenario;

    UPROPERTY(Transient)
    TObjectPtr<AClinicPatient> MeasurementPatient;

    UPROPERTY(Transient)
    FClinicMeasurementState Measurement;

    void ClipOntoFinger();
    void DetachFingerClip();
    void CompleteMeasurement();
    void ReturnToInitialPosition();

    float Elapsed = 0;
    int32 VisitIdentifier = 0;
    FTransform InitialTransform;
    bool bHasInitialTransform = false;
    bool bIsClippedOn = false;
    ECollisionEnabled::Type PreviousCollision = ECollisionEnabled::QueryAndPhysics;
};
