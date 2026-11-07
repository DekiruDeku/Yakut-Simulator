#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "PrototypeWidgets.generated.h"

class APrototypePlayerController;
class AScenarioDirector;
class UTextBlock;

UCLASS()
class YAKUTSIMULATOR_API UMapSelectionButton : public UButton
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TSoftObjectPtr<UWorld> Destination;

    UPROPERTY()
    TObjectPtr<APrototypePlayerController> Player;

    UFUNCTION()
    void SelectMap();
};

UCLASS()
class YAKUTSIMULATOR_API UMainMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Menu")
    void RefreshMenu();

protected:
    virtual void NativeOnInitialized() override;

private:
    UPROPERTY()
    TObjectPtr<UTextBlock> ErrorMessage;

    UFUNCTION()
    void ResumeScenario();

    UFUNCTION()
    void OpenMainMenu();
};

UCLASS()
class YAKUTSIMULATOR_API UScenarioPanelWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY()
    TObjectPtr<AScenarioDirector> Director;

    UPROPERTY()
    TObjectPtr<UTextBlock> Heading;

    UPROPERTY()
    TObjectPtr<UTextBlock> Instruction;

    UPROPERTY()
    TObjectPtr<UTextBlock> Progress;

    UPROPERTY()
    TObjectPtr<UTextBlock> Feedback;

    UFUNCTION()
    void RefreshScenario();
};
