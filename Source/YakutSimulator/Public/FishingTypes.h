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
