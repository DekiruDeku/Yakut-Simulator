#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishingSonar.generated.h"

class AFishSchool;
class APhysicalLever;
class ATrawlerVessel;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSonarChanged);

UCLASS()
class YAKUTSIMULATOR_API AFishingSonar : public AActor
{
    GENERATED_BODY()

public:
    AFishingSonar();

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Sonar")
    TObjectPtr<ATrawlerVessel> Vessel;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Sonar")
    TObjectPtr<APhysicalLever> ScanLever;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sonar")
    float DetectionRange = 8000;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Sonar")
    float WaterHeight = -100;

    UPROPERTY(BlueprintReadOnly, Category="Sonar")
    TArray<TObjectPtr<AFishSchool>> FoundSchools;

    UPROPERTY(BlueprintReadOnly, Category="Sonar")
    TObjectPtr<AFishSchool> SelectedSchool;

    UPROPERTY(BlueprintAssignable, Category="Sonar")
    FSonarChanged OnSonarChanged;

    UFUNCTION(BlueprintCallable, Category="Sonar")
    void Scan();

    UFUNCTION(BlueprintCallable, Category="Sonar")
    void SelectNextSchool();

    UFUNCTION(BlueprintPure, Category="Sonar")
    FText GetReadout() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    bool bScanArmed = true;

    UFUNCTION()
    void ProcessScanLever(float Value);
};
