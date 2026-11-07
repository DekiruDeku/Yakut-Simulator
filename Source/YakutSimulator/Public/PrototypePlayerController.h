#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PrototypePlayerController.generated.h"

class UMainMenuWidget;
class UScenarioPanelWidget;

UCLASS()
class YAKUTSIMULATOR_API APrototypePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="Menu")
    bool bMenuOpen = false;

    UPROPERTY(BlueprintReadOnly, Category="Menu")
    FText MenuError;

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

private:
    UPROPERTY()
    TObjectPtr<UMainMenuWidget> Menu;

    UPROPERTY()
    TObjectPtr<UScenarioPanelWidget> ScenarioPanel;

    void InitializeInterface();
    void SetMenuOpen(bool bOpen);
};
