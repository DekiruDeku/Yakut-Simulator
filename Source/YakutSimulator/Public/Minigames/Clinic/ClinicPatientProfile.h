#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ClinicPatientProfile.generated.h"

UCLASS(BlueprintType, Blueprintable)
class YAKUTSIMULATOR_API UClinicPatientProfile : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Пациент", meta=(DisplayName="Имя"))
    FText PatientName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Пациент", meta=(DisplayName="Возраст", ClampMin="0"))
    int32 Age = 30;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Пациент", meta=(DisplayName="Пол"))
    FText Sex;
};
