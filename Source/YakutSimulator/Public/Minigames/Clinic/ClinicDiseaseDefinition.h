#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Minigames/Clinic/ClinicTypes.h"
#include "ClinicDiseaseDefinition.generated.h"

UCLASS(BlueprintType, Blueprintable)
class YAKUTSIMULATOR_API UClinicDiseaseDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="Clinic")
    FName DiseaseIdentifier;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Справочник", meta=(DisplayName="Название"))
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Справочник", meta=(DisplayName="Текст справочника", MultiLine=true))
    FText ReferenceText;

    UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadOnly, Category="Справочник", meta=(DisplayName="Источники"))
    TArray<FString> Sources;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Справочник", meta=(DisplayName="Симптомы", TitleProperty="Description"))
    TArray<FClinicReferenceSymptom> Symptoms;

    bool ValidateDefinition(FText& Error) const;
    virtual void PostInitProperties() override;
    virtual void PostLoad() override;

#if WITH_EDITOR
    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
    void EnsureSymptomIdentifiers();
};
