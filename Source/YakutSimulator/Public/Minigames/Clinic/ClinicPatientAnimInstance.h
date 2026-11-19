#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Minigames/Clinic/ClinicPatientAnimationTypes.h"
#include "ClinicPatientAnimInstance.generated.h"

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API UClinicPatientAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient, BlueprintReadOnly, Category="Clinic")
    FClinicPatientPoseState PoseState;

    UPROPERTY(Transient, BlueprintReadOnly, Category="Clinic")
    TObjectPtr<UAnimSequence> PoseAnimation;

    UFUNCTION(BlueprintPure, Category="Clinic", meta=(BlueprintThreadSafe))
    bool HasPoseAnimation() const;

    UFUNCTION(BlueprintNativeEvent, Category="Clinic")
    void ReceivePoseRequested(const FClinicPatientPoseState& Request, const FClinicPatientPoseDefinition& Definition);

    UFUNCTION(BlueprintCallable, Category="Clinic")
    bool CompletePoseRequest(int32 ExpectedVisitIdentifier, int32 ExpectedRequestIdentifier, EClinicPresentationResult Result);

    void CancelPoseRequest();
    void BeginPoseRequest(const FClinicPatientPoseState& Request, const FClinicPatientPoseDefinition& Definition);

protected:
    virtual void NativeUninitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveMontage;

    void HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted, int32 ExpectedVisitIdentifier, int32 ExpectedRequestIdentifier);
};
