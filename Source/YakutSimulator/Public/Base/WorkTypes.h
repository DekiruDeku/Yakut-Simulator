#pragma once

#include "CoreMinimal.h"
#include "WorkTypes.generated.h"

class UWorkOperationComponent;

UENUM(BlueprintType)
enum class EWorkAction : uint8
{
    Grabbed,
    Released,
    Used,
    Placed
};

UENUM(BlueprintType)
enum class EWorkOperationState : uint8
{
    Locked,
    Ready,
    InProgress,
    Completed
};

USTRUCT(BlueprintType)
struct FScenarioStepDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName StepIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Instruction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Hint;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UWorkOperationComponent> Operation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> RequiredSteps;
};

USTRUCT(BlueprintType)
struct FScenarioMapEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UWorld> Map;
};
