#pragma once

#include "CoreMinimal.h"
#include "Base/HandGripInteraction.h"
#include "GameFramework/Actor.h"
#include "Minigames/Clinic/ClinicTypes.h"
#include "ClinicReferenceBook.generated.h"

class AClinicScenario;
class AVirtualRealityPawn;
class UBoxComponent;
class UClinicDiseaseDefinition;
class UStaticMeshComponent;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EClinicBookTurnPhase : uint8
{
    Ready,
    Dragging,
    Settling
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FClinicBookEvidenceChanged);

UCLASS(Blueprintable)
class YAKUTSIMULATOR_API AClinicReferenceBook : public AActor, public IHandGripInteraction
{
    GENERATED_BODY()

public:
    AClinicReferenceBook();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UStaticMeshComponent> Cover;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UBoxComponent> BookGripZone;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<USceneComponent> LeftHandGrip;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UWidgetComponent> LeftPage;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UWidgetComponent> RightPage;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<USceneComponent> PagePivot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UWidgetComponent> TurningPageFront;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UWidgetComponent> TurningPageBack;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UBoxComponent> LeftPageEdge;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<UBoxComponent> RightPageEdge;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Clinic|Book")
    TObjectPtr<AClinicScenario> Scenario;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic|Book", meta=(ClampMin="0.01", Units="s"))
    float SettleSeconds = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic|Book", meta=(ClampMin="1", Units="cm"))
    float MaximumHandDeviation = 20;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic|Book", meta=(ClampMin="0", ClampMax="1", ToolTip="Fraction of the page turn required to finish the turn when the player releases the page."))
    float PageTurnCommitProgress = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Clinic|Book", meta=(ClampMin="0", ToolTip="Index of the first displayed disease in the scenario catalog."))
    int32 InitialSpread = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    int32 CurrentSpread = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    EClinicBookTurnPhase TurnPhase = EClinicBookTurnPhase::Ready;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Clinic|Book")
    float TurnProgress = 0;

    UPROPERTY(BlueprintAssignable, Category="Clinic|Book")
    FClinicBookEvidenceChanged OnEvidenceChanged;

    UFUNCTION(BlueprintPure, Category="Clinic|Book")
    bool CanTurn(int32 Direction) const;

    UFUNCTION(BlueprintPure, Category="Clinic|Book")
    UClinicDiseaseDefinition* GetPageDisease(int32 SurfaceIndex) const;

    UFUNCTION(BlueprintPure, Category="Clinic|Book")
    bool IsSymptomsPage(int32 SurfaceIndex) const;

    UFUNCTION(BlueprintPure, Category="Clinic|Book")
    int32 GetVisitIdentifier() const;

    UFUNCTION(BlueprintCallable, Category="Clinic|Book")
    bool CheckSymptom(FName DiseaseIdentifier, FName SymptomIdentifier, int32 ExpectedVisitIdentifier);

    UFUNCTION(BlueprintPure, Category="Clinic|Book")
    bool GetCheckedSymptomStatus(FName DiseaseIdentifier, FName SymptomIdentifier,
        EClinicReferenceSymptomStatus& Status) const;

    UFUNCTION(BlueprintPure, Category="Clinic|Book")
    bool ValidateBookSetup(FText& Error) const;

    virtual EHandGripResult TryBeginHandGrip(AVirtualRealityPawn* Pawn, bool bLeftHand) override;
    virtual void EndHandGrip(AVirtualRealityPawn* Pawn, bool bLeftHand, bool bCancelled) override;
    virtual void Tick(float DeltaSeconds) override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION(BlueprintImplementableEvent, Category="Clinic|Book")
    void InitializePages(APlayerController* Player);

    UFUNCTION(BlueprintImplementableEvent, Category="Clinic|Book")
    void RefreshPages();

private:
    UPROPERTY(Transient)
    TObjectPtr<AVirtualRealityPawn> HoldingPawn;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> HoldingHand;

    UPROPERTY(Transient)
    TObjectPtr<AVirtualRealityPawn> TurningPawn;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> TurningHand;

    TMap<FName, TSet<FName>> CheckedSymptoms;
    FTransform RestTransform = FTransform::Identity;
    FVector DragStart = FVector::ZeroVector;
    FVector DragEnd = FVector::ZeroVector;
    FQuat InitialPivotRotation = FQuat::Identity;
    int32 VisitIdentifier = 0;
    int32 TurnDirection = 0;
    float SettleTarget = 0;
    bool bInitialAbsoluteScale = false;
    bool bSetupValid = true;
    bool bEndingPlay = false;

    UFUNCTION()
    void RefreshEvidence();

    void UpdatePageTransform();
    void UpdatePageVisibility();
    void UpdatePagePresentation();
    bool ContainsHand(const UBoxComponent* Zone, const USceneComponent* Hand) const;
    void EndPageGrip(bool bCancelled);
    void CancelTurn();
    void ReturnBook();
};
