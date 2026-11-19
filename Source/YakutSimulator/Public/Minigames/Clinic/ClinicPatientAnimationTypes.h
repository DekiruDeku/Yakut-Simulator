#pragma once

#include "CoreMinimal.h"
#include "ClinicPatientAnimationTypes.generated.h"

class UAnimMontage;
class UAnimSequence;

UENUM(BlueprintType)
enum class EClinicPresentationPhase : uint8
{
    Idle,
    Transitioning,
    Ready,
    Failed
};

UENUM(BlueprintType)
enum class EClinicPresentationResult : uint8
{
    Completed,
    Interrupted,
    Failed
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicExaminationStage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName PoseIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FName> Areas;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicPatientPoseDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName PoseIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UAnimSequence> LoopAnimation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<UAnimMontage> TransitionMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="deg"))
    float YawOffset = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", Units="s"))
    float TransitionSeconds = 0.35f;
};

USTRUCT(BlueprintType)
struct YAKUTSIMULATOR_API FClinicPatientPoseState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 VisitIdentifier = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RequestIdentifier = 0;

    UPROPERTY(BlueprintReadOnly)
    FName PoseIdentifier;

    UPROPERTY(BlueprintReadOnly)
    EClinicPresentationPhase Phase = EClinicPresentationPhase::Idle;
};
