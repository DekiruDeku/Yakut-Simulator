#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PrototypePlayerController.generated.h"

class AScenarioDirector;
class UUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMenuErrorChanged);

UCLASS()
class YAKUTSIMULATOR_API APrototypePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    APrototypePlayerController();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interface")
    TSubclassOf<UUserWidget> MainMenuClass;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Interface")
    TSubclassOf<UUserWidget> ScenarioPanelClass;

    UPROPERTY(BlueprintReadOnly, Category="Scenario")
    TObjectPtr<AScenarioDirector> ScenarioDirector;

    UPROPERTY(BlueprintReadOnly, Category="Menu")
    bool bMenuOpen = false;

    UPROPERTY(BlueprintReadOnly, Category="Menu")
    FText MenuError;

    UPROPERTY(BlueprintAssignable, Category="Menu")
    FMenuErrorChanged OnMenuErrorChanged;

    UFUNCTION(BlueprintPure, Category="Menu")
    bool IsMainMenuMap() const;

    UFUNCTION(BlueprintCallable, Category="Menu")
    void ToggleMenu();

    UFUNCTION(BlueprintCallable, Category="Menu")
    void CancelInteraction();

    UFUNCTION(BlueprintCallable, Category="Menu")
    bool OpenScenarioMap(TSoftObjectPtr<UWorld> Map);

    UFUNCTION(BlueprintCallable, Category="Menu")
    void ReturnToMainMenu();

protected:
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;

private:
    UPROPERTY()
    TObjectPtr<UUserWidget> Menu;

    UPROPERTY()
    TObjectPtr<UUserWidget> ScenarioPanel;

    void InitializeInterface();
    void SetMenuOpen(bool bOpen);
    void InitializeScenarioPanel();
};
