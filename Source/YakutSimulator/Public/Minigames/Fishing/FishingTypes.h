#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "FishingTypes.generated.h"

class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ETrawlState : uint8
{
    Stowed,
    Deploying,
    Towing,
    Hauling,
    ReadyToUnload
};

UENUM(BlueprintType)
enum class EFishProductState : uint8
{
    Whole,
    Processed,
    Packed,
    Frozen
};

USTRUCT(BlueprintType)
struct FFishCutDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName CutIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Instruction;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector Start = FVector(-20, 0, 15);

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FVector End = FVector(18, 0, 15);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.5", Units="cm"))
    float PositionTolerance = 3;
};

UCLASS(BlueprintType, Blueprintable)
class YAKUTSIMULATOR_API UFishSpeciesDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fish")
    FName SpeciesIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fish")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fish")
    TObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fish")
    TObjectPtr<UMaterialInterface> Material;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fish")
    float WeightKilograms = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fish")
    FVector CuttingTargetOffset = FVector(0, 0, 8);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cutting")
    TArray<FFishCutDefinition> CuttingSteps;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cutting", meta=(ClampMin="1", Units="cm"))
    float MaximumBladeTravelPerSample = 12;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Cutting")
    TObjectPtr<UStaticMesh> ProcessedMesh;
};

USTRUCT(BlueprintType)
struct FFishCatchEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UFishSpeciesDefinition> Species;

    UPROPERTY(BlueprintReadOnly)
    int32 Count = 0;
};
