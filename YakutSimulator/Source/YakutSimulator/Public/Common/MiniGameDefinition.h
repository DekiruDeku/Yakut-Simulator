#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MiniGameDefinition.generated.h"

class UStateTree;
class UInputAction;
class UInputMappingContext;
class UMiniGamePanelWidget;

UENUM(BlueprintType)
enum class EMiniGameStatus : uint8 { Inactive, Running, Completed, Aborted };
UENUM(BlueprintType)
enum class EInteractionAction : uint8 { Grip, Use };
UENUM(BlueprintType)
enum class EMiniGameFeedback : uint8 { Normal, Success, Warning, Error, Disabled };

USTRUCT(BlueprintType)
struct FMiniGameMetric
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Value = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Maximum = 1;
};

USTRUCT(BlueprintType)
struct FMiniGamePresentation
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine=true)) FText Body;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FMiniGameMetric> Metrics;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EMiniGameFeedback Feedback = EMiniGameFeedback::Normal;
};

USTRUCT(BlueprintType)
struct FMiniGameObjective
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FText Instruction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Operation;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Workplace;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FName> AllowedActions;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag CompletionEvent;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bFinal = false;
};

UCLASS(BlueprintType)
class YAKUTSIMULATOR_API UXRInteractionProfile : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UInputAction> LeftGrab;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UInputAction> LeftRelease;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UInputAction> RightGrab;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UInputAction> RightRelease;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UInputAction> LeftUse;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UInputAction> RightUse;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UInputMappingContext> MappingContext;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MappingPriority = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0", Units="cm")) float Reach = 22;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1", Units="s")) float DiagnosticInterval = 5;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float RawInputLogThreshold = .05f;
};

UCLASS(BlueprintType)
class YAKUTSIMULATOR_API UMiniGameUIStyle : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UMiniGamePanelWidget> PanelClass;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<EMiniGameFeedback, FLinearColor> Colors;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText MetricFormat = NSLOCTEXT("MiniGame.UI", "Metric", "{0}: {1}%");
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText FeedbackFormat = NSLOCTEXT("MiniGame.UI", "Feedback", "{0}\n{1}");
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.02", Units="s")) float RefreshInterval = .1f;
};

UCLASS(BlueprintType)
class YAKUTSIMULATOR_API UMiniGameDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId(TEXT("MiniGame"), Id); }
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UStateTree> Flow;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UXRInteractionProfile> Interaction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UMiniGameUIStyle> Style;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMiniGameObjective> Objectives;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FText> Messages;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TMap<FName, FGameplayTag> ActionEvents;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RestartAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName StopAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName ResumeAction;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="s", ClampMin="0")) float FadeOutSeconds = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Units="s", ClampMin="0")) float FadeInSeconds = .15f;
    const FMiniGameObjective* FindObjective(FName ObjectiveId) const;
    UFUNCTION(BlueprintPure) FText Text(FName Key) const;
    virtual bool ValidateConfiguration(TArray<FText>& Errors) const;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
