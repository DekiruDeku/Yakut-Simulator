#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Minigames/Clinic/ClinicTypes.h"
#include "Minigames/Clinic/ClinicPatientAnimationTypes.h"
#include "ClinicPatient.generated.h"

class AClinicScenario;
class UBoxComponent;
class USkeletalMeshComponent;
class UAudioComponent;
class UClinicActionDefinition;
class UClinicPatientProfile;
class USkeletalMesh;
class UMaterialInterface;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FClinicPatientPoseChanged, const FClinicPatientPoseState&, State);

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicExaminationArea
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName AreaIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UBoxComponent> Zone;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Highlight;
};

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AClinicPatient : public AActor
{
    GENERATED_BODY()

public:
    AClinicPatient();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic|Patient", meta=(DisplayName="Карточка пациента"))
    TObjectPtr<UClinicPatientProfile> PatientProfile;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic")
    TObjectPtr<USkeletalMeshComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic")
    TObjectPtr<UAudioComponent> Voice;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> Mouth;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> MouthHighlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> LeftFinger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> LeftFingerHighlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> LeftChest;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> LeftChestHighlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> RightChest;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> RightChestHighlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> LeftBack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> LeftBackHighlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> RightBack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> RightBackHighlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> LeftForearm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> LeftForearmHighlight;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UBoxComponent> LeftUpperArm;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Areas")
    TObjectPtr<UStaticMeshComponent> LeftUpperArmHighlight;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic", meta=(TitleProperty="AreaIdentifier"))
    TArray<FClinicExaminationArea> Areas;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic|Animation", meta=(TitleProperty="PoseIdentifier"))
    TArray<FClinicPatientPoseDefinition> PoseDefinitions;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Clinic|Animation", meta=(ToolTip="Pose used when the case has no initial pose."))
    FName DefaultPoseIdentifier = TEXT("Idle");

    UPROPERTY(BlueprintAssignable, Category="Clinic|Animation")
    FClinicPatientPoseChanged OnPatientPoseChanged;

    UFUNCTION(BlueprintPure, Category="Clinic|Animation")
    FClinicPatientPoseState GetPoseState() const;

    UFUNCTION(BlueprintCallable, Category="Clinic|Animation")
    bool CompletePoseRequest(int32 ExpectedVisitIdentifier, int32 ExpectedRequestIdentifier, EClinicPresentationResult Result);

    UPROPERTY(EditDefaultsOnly, Category="Clinic|Guidance")
    FLinearColor PendingAreaColor = FLinearColor(0.05f, 0.65f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category="Clinic|Guidance")
    FLinearColor ExaminedAreaColor = FLinearColor(0.1f, 1.0f, 0.25f);

    UFUNCTION(BlueprintPure, Category="Clinic")
    TArray<FName> GetExaminedAreas() const;

    UFUNCTION(BlueprintPure, Category="Clinic")
    FText GetExaminationReadout() const;

    UFUNCTION(BlueprintImplementableEvent, Category="Clinic")
    void ReceivePresentationChanged(const FClinicPatientPresentation& Presentation);

    UFUNCTION(BlueprintImplementableEvent, Category="Clinic")
    void ReceiveResponse(const FClinicActionResult& Result);

    UFUNCTION(BlueprintImplementableEvent, Category="Clinic")
    void ReceiveInspectionRequested(FName ActionIdentifier, FName AreaIdentifier);

    UFUNCTION(BlueprintImplementableEvent, Category="Clinic")
    void ReceiveVisitReset();

    void InitializePatient(AClinicScenario* InScenario, int32 InVisitIdentifier, const FClinicPatientPresentation& Presentation);
    FName ResolveInitialPose(FName CasePose) const;
    bool ValidateAreaBindings(const USkeletalMesh* Mesh, FText& Error) const;
    bool HasArea(FName AreaIdentifier) const;
    bool IsPointInArea(FName AreaIdentifier, const FVector& WorldPosition, float ContactTolerance = 0.0f) const;
    bool BeginInspection(UClinicActionDefinition* Action, FName AreaIdentifier, int32 ExpectedVisitIdentifier);
    bool CanRecordInspection(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const;
    void CancelInspection(bool bNotifyReset = true);
    bool BeginInstrumentExamination(UClinicActionDefinition* Action, int32 ExpectedVisitIdentifier);
    bool IsReadyForExamination(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const;
    UBoxComponent* FindAreaZone(FName AreaIdentifier) const;
    bool ValidatePoseDefinitions(FText& Error) const;
    bool HasPoseDefinition(FName PoseIdentifier) const;
    void ReturnToInitialPose();

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void PostInitializeComponents() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<AClinicScenario> Scenario;

    UPROPERTY(Transient)
    TObjectPtr<UClinicActionDefinition> Inspection;

    UPROPERTY(Transient)
    TObjectPtr<UClinicActionDefinition> InstrumentExamination;

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMesh> InitialMesh;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> InitialMaterials;

    int32 VisitIdentifier = 0;
    FName InspectionArea;
    float InspectionElapsed = 0;
    FTransform InitialTransform;
    bool bHasInitialTransform = false;
    FName InitialPoseIdentifier;
    FClinicPatientPoseState PoseState;
    TArray<FName> ActiveAreas;
    FRotator PoseStartRotation;
    FRotator PoseTargetRotation;
    float PoseElapsed = 0;
    float PoseTransitionSeconds = 0;
    bool bAnimationComplete = false;
    void AddArea(FName Identifier, TObjectPtr<UBoxComponent>& Zone, TObjectPtr<UStaticMeshComponent>& Highlight);
    void RefreshExaminationGuidance();
    void RefreshAreaAttachments();
    bool RequestPose(FName PoseIdentifier);
    void CancelPoseRequest();
    void UpdatePoseTransition(float DeltaSeconds);
    void RefreshExaminationStage();

    UFUNCTION()
    void HandleActionCompleted(const FClinicActionResult& Result);
};
