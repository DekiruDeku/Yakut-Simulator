#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FishingTypes.h"
#include "FishSchool.generated.h"

class UNiagaraComponent;
class USplineComponent;

struct FFishSchoolCoastalBoundary
{
    TArray<FVector> Points;
    FBox2D Bounds = FBox2D(ForceInit);
    float SamplingPadding = 0;
};

UCLASS()
class YAKUTSIMULATOR_API AFishSchool : public AActor
{
    GENERATED_BODY()

public:
    AFishSchool();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="School")
    TObjectPtr<UNiagaraComponent> FishVisuals;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School|Visuals", meta=(ClampMin="1", ClampMax="512"))
    int32 MaximumVisualFish = 96;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School|Visuals", meta=(ClampMin="0", Units="cm"))
    float VerticalSpread = 120;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School|Visuals", meta=(ClampMin="0", Units="cm"))
    float SwimAmplitude = 45;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School|Visuals", meta=(ClampMin="0"))
    float SwimFrequency = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School")
    TObjectPtr<UFishSpeciesDefinition> Species;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School")
    float Radius = 1200;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="School")
    int32 RemainingFish = 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    bool bMovementEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="0", Units="cm/s"))
    float MovementSpeed = 35;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="1"))
    float MovementAcceleration = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="1", Units="deg/s"))
    float TurnRate = 45;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="100", Units="cm"))
    float MovementRadius = 6000;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement", meta=(ClampMin="0", Units="cm"))
    float ShoreClearance = 300;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
    FName CoastalZoneTag = "Shore";

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="25", Units="cm"))
    float BoundarySamplingDistance = 150;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement")
    int32 MovementSeed = 731;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
    FVector MovementOrigin = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
    FVector MovementDestination = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
    FVector MovementVelocity = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Movement")
    bool bAvoidingCoast = false;

    UFUNCTION(BlueprintCallable, Category="School")
    int32 ConsumeFish(int32 RequestedCount);

    UFUNCTION(BlueprintCallable, Category="Movement")
    void RefreshCoastalBoundaries();

    UFUNCTION(BlueprintCallable, Category="Movement")
    bool SetMovementDestination(const FVector& Destination);

    bool IsCoastalPathBlocked(const FVector& Start, const FVector& End) const;

#if WITH_EDITOR
    UFUNCTION(CallInEditor, Category="School|Visuals")
    void ConfigureVisualSystemBindings();
#endif

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<USplineComponent>> CoastalSplines;

    TArray<FFishSchoolCoastalBoundary> CoastalBoundaries;
    FRotator VisualHeading = FRotator::ZeroRotator;
    FRandomStream MovementRandom;
    float DestinationRetrySeconds = 0;
    bool bHasMovementDestination = false;
    bool bChooseDestinationAfterStopping = false;

    void ChooseMovementDestination();
    void ConfigureVisuals();
    static bool IsInsideBoundary(const FVector& Position, const FFishSchoolCoastalBoundary& Boundary);
};
