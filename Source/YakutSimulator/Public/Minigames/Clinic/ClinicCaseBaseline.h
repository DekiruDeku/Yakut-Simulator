#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DataTable.h"
#include "ClinicCaseBaseline.generated.h"

class UClinicActionDefinition;
class USoundBase;
class UTexture2D;
struct FClinicIndicatorRow;

USTRUCT()
struct FClinicFactKey
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta=(DisplayName="Показатель", RowType="/Script/YakutSimulator.ClinicIndicatorRow"))
    FDataTableRowHandle Indicator;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Область"))
    FName AreaIdentifier;

    const FClinicIndicatorRow* GetIndicator() const;

    bool operator==(const FClinicFactKey& Other) const
    {
        return Indicator == Other.Indicator && AreaIdentifier == Other.AreaIdentifier;
    }
};

UENUM()
enum class EClinicReferenceRangeMode : uint8
{
    Inherit UMETA(DisplayName="Из базового набора"),
    Hidden UMETA(DisplayName="Без интервала"),
    Custom UMETA(DisplayName="Задать интервал")
};

USTRUCT()
struct FClinicCaseValue
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere)
    FClinicFactKey Fact;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Число"))
    double NumericValue = 0;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Вариант"))
    FName CategoryValue;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Примечание", MultiLine=true))
    FText Description;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Изображение показателя"))
    TObjectPtr<UTexture2D> Image;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Референс"))
    EClinicReferenceRangeMode ReferenceRange = EClinicReferenceRangeMode::Inherit;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Нижняя граница", EditCondition="ReferenceRange == EClinicReferenceRangeMode::Custom", EditConditionHides))
    double ReferenceMinimum = 0;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Верхняя граница", EditCondition="ReferenceRange == EClinicReferenceRangeMode::Custom", EditConditionHides))
    double ReferenceMaximum = 0;
};

USTRUCT()
struct FClinicCaseMessage
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta=(DisplayName="Действие"))
    TObjectPtr<UClinicActionDefinition> Action;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Область"))
    FName AreaIdentifier;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Текст", MultiLine=true))
    FText Response;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Звук"))
    TObjectPtr<USoundBase> Sound;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Изображение"))
    TObjectPtr<UTexture2D> Image;
};

USTRUCT()
struct FClinicCaseData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, meta=(DisplayName="Значения показателей", TitleProperty="{Fact.Indicator.RowName} {Fact.AreaIdentifier}"))
    TArray<FClinicCaseValue> Values;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Реплики", TitleProperty="Action"))
    TArray<FClinicCaseMessage> Answers;

    UPROPERTY(EditAnywhere, meta=(DisplayName="Заключения и материалы", TitleProperty="{Action} {AreaIdentifier}"))
    TArray<FClinicCaseMessage> SpecialResults;
};

UCLASS(BlueprintType, Blueprintable)
class YAKUTSIMULATOR_API UClinicCaseBaseline : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category="Клиника", meta=(DisplayName="Базовые результаты", ShowOnlyInnerProperties))
    FClinicCaseData Data;
};
