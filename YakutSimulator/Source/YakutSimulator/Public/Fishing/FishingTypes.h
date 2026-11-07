#pragma once

#include "CoreMinimal.h"
#include "FishingTypes.generated.h"

UENUM(BlueprintType)
enum class EFishingStage : uint8
{
    Safety, Deploying, Trawling, Hauling, AwaitSorting, Sorting, AwaitProcessing, Processing, Complete
};

UENUM(BlueprintType)
enum class EFishSpecies : uint8 { A, B };

UENUM(BlueprintType)
enum class EFishingItemKind : uint8 { Fish, Knife };

UENUM(BlueprintType)
enum class EFishingItemState : uint8 { Queued, Conveyor, Loose, Sorted, OnBoard, Processed, Delivered };

UENUM(BlueprintType)
enum class EFishingStationKind : uint8
{
    SafetyEquipment, EmergencyStop, StartOrResume, WinchLever, GoToSorting,
    SortingBin, GoToProcessing, ProcessingBoard, DeliveryBin, Restart, StatusBoard
};

USTRUCT(BlueprintType)
struct FFishingResults
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly) int32 TotalFish = 0;
    UPROPERTY(BlueprintReadOnly) int32 SortedFish = 0;
    UPROPERTY(BlueprintReadOnly) int32 ProcessedFish = 0;
    UPROPERTY(BlueprintReadOnly) int32 DeliveredFish = 0;
    UPROPERTY(BlueprintReadOnly) int32 SortingMistakes = 0;
    UPROPERTY(BlueprintReadOnly) int32 MissedFish = 0;
    UPROPERTY(BlueprintReadOnly) int32 Overloads = 0;
    UPROPERTY(BlueprintReadOnly) float ElapsedSeconds = 0.f;
};
