#pragma once
#include "CoreMinimal.h"
#include "Common/MiniGameDefinition.h"
#include "FishingDefinition.generated.h"

class AFishingFish;
class AGrabbableItem;

UENUM(BlueprintType)
enum class EFishingOperation : uint8 { None, Safety, Deploy, Trawl, Haul, Sorting, Processing, Results };
UENUM(BlueprintType)
enum class EFishWorkState : uint8 { Queued, Conveyor, Loose, Sorted, OnBoard, Processed, Delivered };
UENUM(BlueprintType)
enum class EWinchStopCause : uint8 { None, Button, Overload };

USTRUCT(BlueprintType)
struct FFishingWinchSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="s")) float DeploySeconds = 8;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="s")) float TrawlSeconds = 4;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="s")) float HaulSeconds = 20;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="cm")) FVector LoweredOffset = FVector(0, 0, -500);
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ResistanceBase = .3f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ResistanceAmplitude = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float ResistanceFrequency = 1.7f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float InputLoadScale = .6f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float WarningLoad = .8f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", ClampMax="1")) float OverloadLoad = .9f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="s")) float OverloadSeconds = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bEnableOverloads = true;
};

USTRUCT(BlueprintType)
struct FFishingCatchSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 FishCount = 8;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="cm/s")) float ConveyorSpeed = 12;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="s")) float FeedInterval = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) int32 RecoveryColumns = 2;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="cm")) FVector2D RecoverySpacing = FVector2D(22, 14);
};

USTRUCT(BlueprintType)
struct FFishingProcessingSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="cm")) float CutTolerance = 3;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="cm")) float BraceDistance = 30;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin=".1", Units="cm")) float MaxKnifeStep = 40;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", Units="s")) float StrokeWarningSeconds = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRequireSecondHand = true;
};

USTRUCT(BlueprintType)
struct FFishingScoreSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float DeliveredPoints = 100;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float SortingPenalty = 10;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float MissPenalty = 10;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float OverloadPenalty = 10;
};

UCLASS(BlueprintType)
class YAKUTSIMULATOR_API UFishSpeciesDefinition : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<AFishingFish> ActorClass;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType)
class YAKUTSIMULATOR_API UFishingDefinition : public UMiniGameDefinition
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") FFishingWinchSettings Winch;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") FFishingCatchSettings Catch;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") FFishingProcessingSettings Processing;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") FFishingScoreSettings Scoring;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") TArray<TObjectPtr<UFishSpeciesDefinition>> SpeciesCycle;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") TSubclassOf<AGrabbableItem> KnifeClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") TMap<FName, EFishingOperation> Operations;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") bool bRequireEquipment = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing") bool bRequireStopTest = true;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing|Actions") FName EquipAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing|Actions") FName StartAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing|Actions") FName SortingAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing|Actions") FName ProcessingAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing|Actions") FName WinchAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Fishing|UI") FName WinchPanel;
    virtual bool ValidateConfiguration(TArray<FText>& Errors) const override;
};
