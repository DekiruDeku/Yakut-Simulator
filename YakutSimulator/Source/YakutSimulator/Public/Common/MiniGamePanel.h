#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/WidgetComponent.h"
#include "Common/MiniGameDefinition.h"
#include "MiniGamePanel.generated.h"

class UTextBlock;
class UProgressBar;
class AMiniGameScenario;

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API UMiniGamePanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Heading;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Description;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> PrimaryLabel;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SecondaryLabel;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> PrimaryGauge;
    UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> SecondaryGauge;
    UPROPERTY(BlueprintReadOnly, Transient) FMiniGamePresentation Presentation;
    UFUNCTION(BlueprintCallable, Category="MiniGame") void Present(const FMiniGamePresentation& Data, UMiniGameUIStyle* Style);
    UFUNCTION(BlueprintImplementableEvent, Category="MiniGame") void OnPresentationChanged(const FMiniGamePresentation& Data);
};

UCLASS(ClassGroup=(MiniGame), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UMiniGameDisplayComponent : public UWidgetComponent
{
    GENERATED_BODY()
public:
    UMiniGameDisplayComponent();
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="MiniGame") TObjectPtr<AMiniGameScenario> Scenario;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MiniGame") FName PanelId;
    UFUNCTION() void Refresh();
    void BindScenario(AMiniGameScenario* InScenario);
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
