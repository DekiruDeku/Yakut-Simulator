#include "Minigames/Clinic/ClinicPatientAnimInstance.h"

#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Minigames/Clinic/ClinicPatient.h"

bool UClinicPatientAnimInstance::HasPoseAnimation() const
{
    return PoseAnimation != nullptr;
}

void UClinicPatientAnimInstance::BeginPoseRequest(const FClinicPatientPoseState& Request,
    const FClinicPatientPoseDefinition& Definition)
{
    CancelPoseRequest();
    PoseState = Request;
    PoseAnimation = Definition.LoopAnimation;
    ReceivePoseRequested(Request, Definition);
}

void UClinicPatientAnimInstance::ReceivePoseRequested_Implementation(const FClinicPatientPoseState& Request,
    const FClinicPatientPoseDefinition& Definition)
{
    USkeletalMesh* Mesh = GetSkelMeshComponent()->GetSkeletalMeshAsset();
    if ((PoseAnimation && (!PoseAnimation->GetSkeleton() || !PoseAnimation->GetSkeleton()->IsCompatibleMesh(Mesh)))
        || (Definition.TransitionMontage && (!Definition.TransitionMontage->GetSkeleton()
            || !Definition.TransitionMontage->GetSkeleton()->IsCompatibleMesh(Mesh))))
    {
        PoseAnimation = nullptr;
        CompletePoseRequest(Request.VisitIdentifier, Request.RequestIdentifier, EClinicPresentationResult::Failed);
        return;
    }
    if (!Definition.TransitionMontage)
    {
        CompletePoseRequest(Request.VisitIdentifier, Request.RequestIdentifier, EClinicPresentationResult::Completed);
        return;
    }
    ActiveMontage = Definition.TransitionMontage;
    if (Montage_Play(ActiveMontage, 1, EMontagePlayReturnType::MontageLength, 0, false) <= 0)
    {
        ActiveMontage = nullptr;
        CompletePoseRequest(Request.VisitIdentifier, Request.RequestIdentifier, EClinicPresentationResult::Failed);
        return;
    }
    FOnMontageEnded Ended;
    Ended.BindUObject(this, &UClinicPatientAnimInstance::HandleMontageEnded, Request.VisitIdentifier, Request.RequestIdentifier);
    Montage_SetEndDelegate(Ended, ActiveMontage);
}

bool UClinicPatientAnimInstance::CompletePoseRequest(int32 ExpectedVisitIdentifier, int32 ExpectedRequestIdentifier,
    EClinicPresentationResult Result)
{
    if (PoseState.Phase != EClinicPresentationPhase::Transitioning || PoseState.VisitIdentifier != ExpectedVisitIdentifier
        || PoseState.RequestIdentifier != ExpectedRequestIdentifier)
    {
        return false;
    }
    AClinicPatient* Patient = Cast<AClinicPatient>(GetOwningActor());
    if (!Patient || !Patient->CompletePoseRequest(ExpectedVisitIdentifier, ExpectedRequestIdentifier, Result))
    {
        return false;
    }
    if (PoseState.VisitIdentifier == ExpectedVisitIdentifier && PoseState.RequestIdentifier == ExpectedRequestIdentifier)
    {
        PoseState = Patient->GetPoseState();
    }
    return true;
}

void UClinicPatientAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);
    if (const AClinicPatient* Patient = Cast<AClinicPatient>(GetOwningActor()))
    {
        PoseState = Patient->GetPoseState();
    }
}

void UClinicPatientAnimInstance::HandleMontageEnded(UAnimMontage* Montage, bool bInterrupted,
    int32 ExpectedVisitIdentifier, int32 ExpectedRequestIdentifier)
{
    if (Montage != ActiveMontage || PoseState.VisitIdentifier != ExpectedVisitIdentifier
        || PoseState.RequestIdentifier != ExpectedRequestIdentifier)
    {
        return;
    }
    ActiveMontage = nullptr;
    EClinicPresentationResult Result = EClinicPresentationResult::Completed;
    if (bInterrupted)
    {
        Result = EClinicPresentationResult::Interrupted;
    }
    CompletePoseRequest(ExpectedVisitIdentifier, ExpectedRequestIdentifier, Result);
}

void UClinicPatientAnimInstance::CancelPoseRequest()
{
    PoseState.Phase = EClinicPresentationPhase::Idle;
    if (ActiveMontage)
    {
        FOnMontageEnded Ended;
        Montage_SetEndDelegate(Ended, ActiveMontage);
        Montage_Stop(0.1f, ActiveMontage);
        ActiveMontage = nullptr;
    }
}

void UClinicPatientAnimInstance::NativeUninitializeAnimation()
{
    const FClinicPatientPoseState PreviousRequest = PoseState;
    CancelPoseRequest();
    if (PreviousRequest.Phase == EClinicPresentationPhase::Transitioning)
    {
        if (AClinicPatient* Patient = Cast<AClinicPatient>(GetOwningActor()))
        {
            Patient->CompletePoseRequest(PreviousRequest.VisitIdentifier, PreviousRequest.RequestIdentifier,
                EClinicPresentationResult::Interrupted);
        }
    }
    Super::NativeUninitializeAnimation();
}
