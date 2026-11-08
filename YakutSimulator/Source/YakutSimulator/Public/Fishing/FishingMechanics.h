#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Common/MiniGameScenario.h"
#include "Fishing/FishingDefinition.h"
#include "FishingMechanics.generated.h"

class ULeverInteractionComponent;
class USplineComponent;
class UFishingDefinition;
class AFishingFish;
class AGrabbableItem;
class UStaticMeshComponent;

UCLASS(ClassGroup=(Fishing), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UFishingWinchComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFishingWinchComponent();
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Winch") TObjectPtr<ULeverInteractionComponent> Lever;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Winch") TObjectPtr<USceneComponent> Net;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Winch") FFishingWinchSettings Settings;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Winch") float Depth = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Winch") float Load = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Winch") float Input = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Winch") float OverloadTime = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Winch") EFishingOperation Mode = EFishingOperation::None;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Winch") EWinchStopCause StopCause = EWinchStopCause::None;
    UPROPERTY(BlueprintAssignable, Category="Winch") FMiniGameChanged OnFinished;
    UPROPERTY(BlueprintAssignable, Category="Winch") FMiniGameChanged OnStopped;
    void Reset(const FFishingWinchSettings& Configuration);
    void Start(EFishingOperation Operation);
    void Deactivate();
    UFUNCTION(BlueprintCallable, Category="Winch") void Stop(EWinchStopCause Cause);
    UFUNCTION(BlueprintCallable, Category="Winch") void Resume();
    UFUNCTION() void SetInput(float Value);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    FVector RaisedLocation = FVector::ZeroVector;
    bool bHasRaisedLocation = false;
    float Seconds = 0;
};

UCLASS(ClassGroup=(Fishing), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UFishingConveyorComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFishingConveyorComponent();
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Conveyor") TObjectPtr<USplineComponent> Path;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Conveyor") TObjectPtr<USceneComponent> Recovery;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Conveyor") TArray<TObjectPtr<AFishingFish>> Fish;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Conveyor") int32 Sorted = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Conveyor") int32 Mistakes = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Conveyor") int32 Missed = 0;
    UPROPERTY(BlueprintAssignable, Category="Conveyor") FMiniGameChanged OnFinished;
    bool Start(UFishingDefinition* Definition);
    void Clear();
    void Deactivate();
    bool Sort(AFishingFish* Item, UFishSpeciesDefinition* Species);
    void Recover(AFishingFish* Item, bool bMissed);
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    UPROPERTY(Transient) TObjectPtr<UFishingDefinition> Configuration;
    bool bActive = false;
    int32 NextFish = 0;
    float FeedSeconds = 0;
};

UCLASS(ClassGroup=(Fishing), meta=(BlueprintSpawnableComponent))
class YAKUTSIMULATOR_API UFishingProcessingComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UFishingProcessingComponent();
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Processing") TObjectPtr<USceneComponent> Board;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Processing") TObjectPtr<USceneComponent> Supply;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Processing") TObjectPtr<USceneComponent> KnifeHome;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Processing") TObjectPtr<USceneComponent> Finished;
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Processing") TObjectPtr<UStaticMeshComponent> Guide;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Processing") TObjectPtr<AGrabbableItem> Knife;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Processing") TObjectPtr<AFishingFish> BoardFish;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Processing") int32 Processed = 0;
    UPROPERTY(BlueprintReadOnly, Transient, Category="Processing") int32 Delivered = 0;
    UPROPERTY(BlueprintAssignable, Category="Processing") FMiniGameChanged OnFinished;
    bool Start(UFishingDefinition* Definition, UFishingConveyorComponent* Conveyor);
    void Clear();
    bool Load(AFishingFish* Item);
    bool Deliver(AFishingFish* Item);
    void Recover(AFishingFish* Item);
    void AdvanceStroke(const FVector& From, const FVector& To);
    UFUNCTION(BlueprintPure, Category="Processing") FName GetCutBlockReason() const;
    UFUNCTION(BlueprintPure, Category="Processing") FName GetCutStatus() const;
    UFUNCTION(BlueprintPure, Category="Processing") FMiniGamePresentation GetPresentation() const;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction) override;
private:
    UPROPERTY(Transient) TObjectPtr<UFishingDefinition> Configuration;
    UPROPERTY(Transient) TObjectPtr<UFishingConveyorComponent> Catch;
    bool bActive = false;
    bool bPreviousTip = false;
    FVector PreviousTip = FVector::ZeroVector;
    FName StrokeIssue;
    float StrokeWarningTime = 0;
    FName LastStatus;
    int32 LastCutPoint = INDEX_NONE;
    void PresentNext();
};
