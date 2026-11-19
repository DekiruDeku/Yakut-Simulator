#include "Minigames/Clinic/ClinicReferenceBook.h"

#include "Base/VirtualRealityPawn.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Minigames/Clinic/ClinicDiseaseDefinition.h"
#include "Minigames/Clinic/ClinicPatient.h"
#include "Minigames/Clinic/ClinicScenario.h"
#include "Misc/DataValidation.h"
#include "MotionControllerComponent.h"

AClinicReferenceBook::AClinicReferenceBook()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Cover = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Cover"));
    Cover->SetupAttachment(Root);
    LeftHandGrip = CreateDefaultSubobject<USceneComponent>(TEXT("LeftHandGrip"));
    LeftHandGrip->SetupAttachment(Root);
    BookGripZone = CreateDefaultSubobject<UBoxComponent>(TEXT("BookGripZone"));
    BookGripZone->SetupAttachment(Root);
    BookGripZone->SetCollisionObjectType(ECC_WorldDynamic);
    BookGripZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    BookGripZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    BookGripZone->SetGenerateOverlapEvents(false);
    BookGripZone->SetBoxExtent(FVector::OneVector);
    PagePivot = CreateDefaultSubobject<USceneComponent>(TEXT("PagePivot"));
    PagePivot->SetupAttachment(Root);
    LeftPage = CreateDefaultSubobject<UWidgetComponent>(TEXT("LeftPage"));
    RightPage = CreateDefaultSubobject<UWidgetComponent>(TEXT("RightPage"));
    TurningPageFront = CreateDefaultSubobject<UWidgetComponent>(TEXT("TurningPageFront"));
    TurningPageBack = CreateDefaultSubobject<UWidgetComponent>(TEXT("TurningPageBack"));
    LeftPage->SetupAttachment(Root);
    RightPage->SetupAttachment(Root);
    TurningPageFront->SetupAttachment(PagePivot);
    TurningPageBack->SetupAttachment(PagePivot);
    for (UWidgetComponent* Page : {LeftPage.Get(), RightPage.Get(), TurningPageFront.Get(), TurningPageBack.Get()})
    {
        Page->SetDrawSize(FVector2D(900, 1100));
        Page->SetWindowFocusable(false);
        Page->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Page->SetCollisionResponseToAllChannels(ECR_Ignore);
        Page->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Page->SetGenerateOverlapEvents(false);
        Page->SetCastShadow(false);
    }
    TurningPageFront->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TurningPageBack->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftPageEdge = CreateDefaultSubobject<UBoxComponent>(TEXT("LeftPageEdge"));
    RightPageEdge = CreateDefaultSubobject<UBoxComponent>(TEXT("RightPageEdge"));
    for (UBoxComponent* Edge : {LeftPageEdge.Get(), RightPageEdge.Get()})
    {
        Edge->SetupAttachment(Root);
        Edge->SetCollisionObjectType(ECC_WorldDynamic);
        Edge->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        Edge->SetCollisionResponseToAllChannels(ECR_Ignore);
        Edge->SetGenerateOverlapEvents(false);
        Edge->SetBoxExtent(FVector(18, 4, 5));
    }
}

void AClinicReferenceBook::BeginPlay()
{
    Super::BeginPlay();
    InitialPivotRotation = PagePivot->GetRelativeRotation().Quaternion();
    FText Error;
    if (!ValidateBookSetup(Error))
    {
        bSetupValid = false;
        UKismetSystemLibrary::PrintString(this, Error.ToString(), true, true, FLinearColor::Red);
        SetActorTickEnabled(false);
        return;
    }
    CurrentSpread = InitialSpread;
    Scenario->OnScenarioChanged.AddDynamic(this, &AClinicReferenceBook::RefreshEvidence);
    RefreshEvidence();
    InitializePages(UGameplayStatics::GetPlayerController(this, 0));
    UpdatePageVisibility();
    RefreshPages();
}

bool AClinicReferenceBook::CanTurn(int32 Direction) const
{
    return bSetupValid && IsValid(Scenario) && TurnPhase == EClinicBookTurnPhase::Ready && (Direction == -1 || Direction == 1)
        && Scenario->DiseaseDefinitions.IsValidIndex(CurrentSpread + Direction);
}

EHandGripResult AClinicReferenceBook::TryBeginHandGrip(AVirtualRealityPawn* Pawn, bool bLeftHand)
{
    if (!IsValid(Pawn) || Pawn->IsHandBusy(bLeftHand))
    {
        return EHandGripResult::Ignored;
    }
    USceneComponent* Hand = Pawn->RightHand;
    if (bLeftHand)
    {
        Hand = Pawn->LeftHand;
        if (ContainsHand(BookGripZone, Hand))
        {
            if (!bSetupValid || bEndingPlay || !IsValid(Scenario) || HoldingPawn || TurnPhase != EClinicBookTurnPhase::Ready)
            {
                return EHandGripResult::Blocked;
            }
            RestTransform = GetActorTransform();
            bInitialAbsoluteScale = Root->IsUsingAbsoluteScale();
            Root->SetUsingAbsoluteScale(true);
            const FQuat Rotation = Hand->GetComponentQuat() * LeftHandGrip->GetRelativeRotation().Quaternion().Inverse();
            SetActorTransform(FTransform(Rotation,
                Hand->GetComponentLocation() - Rotation.RotateVector(RestTransform.GetScale3D() * LeftHandGrip->GetRelativeLocation()),
                RestTransform.GetScale3D()));
            if (!AttachToComponent(Hand, FAttachmentTransformRules::KeepWorldTransform))
            {
                Root->SetUsingAbsoluteScale(bInitialAbsoluteScale);
                SetActorTransform(RestTransform);
                return EHandGripResult::Blocked;
            }
            HoldingPawn = Pawn;
            HoldingHand = Hand;
            AddTickPrerequisiteActor(Pawn);
            AddTickPrerequisiteComponent(Hand);
            return EHandGripResult::Started;
        }
    }
    UBoxComponent* StartEdge = nullptr;
    UBoxComponent* EndEdge = nullptr;
    int32 Direction = 1;
    for (UBoxComponent* Edge : {RightPageEdge.Get(), LeftPageEdge.Get()})
    {
        if (ContainsHand(Edge, Hand))
        {
            StartEdge = Edge;
            break;
        }
    }
    if (StartEdge == RightPageEdge)
    {
        EndEdge = LeftPageEdge;
    }
    else if (StartEdge == LeftPageEdge)
    {
        EndEdge = RightPageEdge;
        Direction = -1;
    }
    if (!StartEdge || !EndEdge)
    {
        return EHandGripResult::Ignored;
    }
    if (bLeftHand || bEndingPlay || !CanTurn(Direction) || (HoldingPawn && HoldingPawn != Pawn))
    {
        return EHandGripResult::Blocked;
    }
    DragStart = GetActorTransform().InverseTransformPosition(Hand->GetComponentLocation());
    DragEnd = GetActorTransform().InverseTransformPosition(EndEdge->GetComponentLocation());
    if (FVector::DistSquared(DragStart, DragEnd) < 1)
    {
        return EHandGripResult::Blocked;
    }
    RefreshEvidence();
    TurningPawn = Pawn;
    TurningHand = Hand;
    AddTickPrerequisiteActor(Pawn);
    AddTickPrerequisiteComponent(Hand);
    TurnDirection = Direction;
    TurnProgress = 0;
    TurnPhase = EClinicBookTurnPhase::Dragging;
    UpdatePagePresentation();
    return EHandGripResult::Started;
}

bool AClinicReferenceBook::ContainsHand(const UBoxComponent* Zone, const USceneComponent* Hand) const
{
    const FVector Local = Zone->GetComponentTransform().InverseTransformPosition(Hand->GetComponentLocation());
    const FVector Extent = Zone->GetUnscaledBoxExtent();
    return FMath::PointBoxIntersection(Local, FBox(-Extent, Extent));
}

void AClinicReferenceBook::EndHandGrip(AVirtualRealityPawn* Pawn, bool bLeftHand, bool bCancelled)
{
    if (bLeftHand)
    {
        if (Pawn == HoldingPawn)
        {
            ReturnBook();
        }
        return;
    }
    if (Pawn == TurningPawn)
    {
        EndPageGrip(bCancelled);
    }
}

void AClinicReferenceBook::EndPageGrip(bool bCancelled)
{
    if (TurningHand)
    {
        RemoveTickPrerequisiteComponent(TurningHand);
    }
    if (TurningPawn && TurningPawn != HoldingPawn)
    {
        RemoveTickPrerequisiteActor(TurningPawn);
    }
    TurningPawn = nullptr;
    TurningHand = nullptr;
    if (TurnPhase != EClinicBookTurnPhase::Dragging)
    {
        return;
    }
    if (bCancelled)
    {
        TurnProgress = 0;
        TurnPhase = EClinicBookTurnPhase::Ready;
        UpdatePagePresentation();
        return;
    }
    SettleTarget = 0;
    if (TurnProgress >= PageTurnCommitProgress)
    {
        SettleTarget = 1;
    }
    TurnPhase = EClinicBookTurnPhase::Settling;
}

void AClinicReferenceBook::CancelTurn()
{
    if (IsValid(TurningPawn))
    {
        TurningPawn->CancelHandInteraction(false);
    }
    EndPageGrip(true);
    if (TurnPhase == EClinicBookTurnPhase::Settling)
    {
        TurnPhase = EClinicBookTurnPhase::Ready;
        TurnProgress = 0;
        UpdatePagePresentation();
    }
}

void AClinicReferenceBook::ReturnBook()
{
    if (!HoldingPawn && !HoldingHand)
    {
        return;
    }
    CancelTurn();
    if (HoldingHand)
    {
        RemoveTickPrerequisiteComponent(HoldingHand);
    }
    AVirtualRealityPawn* Pawn = HoldingPawn;
    if (Pawn)
    {
        RemoveTickPrerequisiteActor(Pawn);
    }
    HoldingPawn = nullptr;
    HoldingHand = nullptr;
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Root->SetUsingAbsoluteScale(bInitialAbsoluteScale);
    SetActorTransform(RestTransform);
    if (IsValid(Pawn))
    {
        Pawn->CancelHandInteraction(true);
    }
}

void AClinicReferenceBook::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsValid(Scenario))
    {
        if (TurnPhase != EClinicBookTurnPhase::Ready)
        {
            CancelTurn();
        }
        ReturnBook();
        return;
    }
    if ((HoldingPawn || HoldingHand) && (!IsValid(HoldingPawn) || !IsValid(HoldingHand)))
    {
        ReturnBook();
    }
    if (TurnPhase == EClinicBookTurnPhase::Dragging)
    {
        if (!IsValid(TurningHand) || !IsValid(TurningPawn))
        {
            CancelTurn();
            return;
        }
        const FVector Position = GetActorTransform().InverseTransformPosition(TurningHand->GetComponentLocation());
        if (FMath::PointDistToSegmentSquared(Position, DragStart, DragEnd) > FMath::Square(MaximumHandDeviation))
        {
            CancelTurn();
            return;
        }
        TurnProgress = FMath::Clamp(static_cast<float>(FVector::DotProduct(Position - DragStart, DragEnd - DragStart)
            / FVector::DistSquared(DragStart, DragEnd)), 0.0f, 1.0f);
        UpdatePageTransform();
    }
    else if (TurnPhase == EClinicBookTurnPhase::Settling)
    {
        TurnProgress = FMath::FInterpConstantTo(TurnProgress, SettleTarget, DeltaSeconds, 1.0f / FMath::Max(SettleSeconds, 0.01f));
        UpdatePageTransform();
        if (FMath::IsNearlyEqual(TurnProgress, SettleTarget))
        {
            if (SettleTarget == 1)
            {
                CurrentSpread += TurnDirection;
            }
            TurnPhase = EClinicBookTurnPhase::Ready;
            TurnProgress = 0;
            UpdatePagePresentation();
        }
    }
}

void AClinicReferenceBook::UpdatePageTransform()
{
    float Angle = TurnProgress * 180;
    if (TurnDirection < 0)
    {
        Angle = (1 - TurnProgress) * 180;
    }
    if (TurnPhase == EClinicBookTurnPhase::Ready)
    {
        Angle = 0;
    }
    PagePivot->SetRelativeRotation(InitialPivotRotation * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Angle)));
}

void AClinicReferenceBook::UpdatePageVisibility()
{
    const bool bReady = TurnPhase == EClinicBookTurnPhase::Ready;
    TurningPageFront->SetVisibility(!bReady);
    TurningPageBack->SetVisibility(!bReady);
    ECollisionEnabled::Type PageCollision = ECollisionEnabled::NoCollision;
    if (bReady)
    {
        PageCollision = ECollisionEnabled::QueryOnly;
    }
    LeftPage->SetCollisionEnabled(PageCollision);
    RightPage->SetCollisionEnabled(PageCollision);
    LeftPageEdge->SetVisibility(CanTurn(-1));
    RightPageEdge->SetVisibility(CanTurn(1));
}

void AClinicReferenceBook::UpdatePagePresentation()
{
    UpdatePageTransform();
    UpdatePageVisibility();
    if (!bEndingPlay)
    {
        RefreshPages();
    }
}

UClinicDiseaseDefinition* AClinicReferenceBook::GetPageDisease(int32 SurfaceIndex) const
{
    if (!IsValid(Scenario) || SurfaceIndex < 0 || SurfaceIndex > 3)
    {
        return nullptr;
    }
    int32 Index = CurrentSpread;
    if (TurnPhase != EClinicBookTurnPhase::Ready)
    {
        if (TurnDirection > 0 && (SurfaceIndex == 1 || SurfaceIndex == 3))
        {
            ++Index;
        }
        else if (TurnDirection < 0 && (SurfaceIndex == 0 || SurfaceIndex == 2))
        {
            --Index;
        }
    }
    if (!Scenario->DiseaseDefinitions.IsValidIndex(Index))
    {
        return nullptr;
    }
    return Scenario->DiseaseDefinitions[Index];
}

bool AClinicReferenceBook::IsSymptomsPage(int32 SurfaceIndex) const
{
    return SurfaceIndex == 1 || SurfaceIndex == 2;
}

int32 AClinicReferenceBook::GetVisitIdentifier() const
{
    return VisitIdentifier;
}

bool AClinicReferenceBook::CheckSymptom(FName DiseaseIdentifier, FName SymptomIdentifier, int32 ExpectedVisitIdentifier)
{
    EClinicReferenceSymptomStatus Status;
    if (!IsValid(Scenario) || TurnPhase != EClinicBookTurnPhase::Ready || ExpectedVisitIdentifier != VisitIdentifier
        || !Scenario->GetReferenceSymptomStatus(DiseaseIdentifier, SymptomIdentifier, ExpectedVisitIdentifier, Status))
    {
        return false;
    }
    CheckedSymptoms.FindOrAdd(DiseaseIdentifier).Add(SymptomIdentifier);
    OnEvidenceChanged.Broadcast();
    return true;
}

bool AClinicReferenceBook::GetCheckedSymptomStatus(FName DiseaseIdentifier, FName SymptomIdentifier,
    EClinicReferenceSymptomStatus& Status) const
{
    Status = EClinicReferenceSymptomStatus::Unknown;
    const TSet<FName>* Checked = CheckedSymptoms.Find(DiseaseIdentifier);
    if (!Checked || !Checked->Contains(SymptomIdentifier) || !IsValid(Scenario))
    {
        return false;
    }
    return Scenario->GetReferenceSymptomStatus(DiseaseIdentifier, SymptomIdentifier, VisitIdentifier, Status);
}

void AClinicReferenceBook::RefreshEvidence()
{
    const int32 CurrentVisit = Scenario->GetVisitState().VisitIdentifier;
    if (CurrentVisit != VisitIdentifier)
    {
        CancelTurn();
        CheckedSymptoms.Empty();
        VisitIdentifier = CurrentVisit;
    }
    OnEvidenceChanged.Broadcast();
}

bool AClinicReferenceBook::ValidateBookSetup(FText& Error) const
{
    const FVector Extent = BookGripZone->GetUnscaledBoxExtent();
    const FVector Scale = GetActorScale3D();
    if (Extent.ContainsNaN() || Extent.GetMin() <= 0 || Scale.ContainsNaN() || Scale.GetMin() <= 0
        || LeftHandGrip->GetRelativeTransform().ContainsNaN() || Root->Mobility != EComponentMobility::Movable)
    {
        Error = FText::FromString(TEXT("Reference book requires a movable root, positive finite grip extent and actor scale, and a finite hand grip transform"));
        return false;
    }
    if (!IsValid(Scenario) || !Scenario->Patient
        || !Scenario->DiseaseDefinitions.IsValidIndex(InitialSpread) || SettleSeconds <= 0 || !FMath::IsFinite(SettleSeconds)
        || !FMath::IsFinite(PageTurnCommitProgress) || PageTurnCommitProgress < 0 || PageTurnCommitProgress > 1
        || MaximumHandDeviation <= 0 || !FMath::IsFinite(MaximumHandDeviation)
        || FVector::DistSquared(LeftPageEdge->GetRelativeLocation(), RightPageEdge->GetRelativeLocation()) < 1)
    {
        Error = FText::FromString(TEXT("Reference book requires a scenario, catalog, valid spread, distinct edges and positive gesture settings"));
        return false;
    }
    for (const UClinicDiseaseDefinition* Disease : Scenario->DiseaseDefinitions)
    {
        if (!Disease || !Disease->ValidateDefinition(Error) || Disease->Symptoms.IsEmpty())
        {
            Error = FText::FromString(TEXT("Every book disease requires valid reference symptoms"));
            return false;
        }
        for (const FClinicReferenceSymptom& Symptom : Disease->Symptoms)
        {
            for (FName Area : Symptom.Areas)
            {
                if (!Scenario->Patient->Areas.ContainsByPredicate([Area](const FClinicExaminationArea& Candidate)
                {
                    return Candidate.AreaIdentifier == Area;
                }))
                {
                    Error = FText::FromString(TEXT("Reference symptom uses an unknown patient area"));
                    return false;
                }
            }
        }
    }
    Error = FText::GetEmpty();
    return true;
}

void AClinicReferenceBook::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    bEndingPlay = true;
    CancelTurn();
    ReturnBook();
    if (IsValid(Scenario))
    {
        Scenario->OnScenarioChanged.RemoveDynamic(this, &AClinicReferenceBook::RefreshEvidence);
    }
    CheckedSymptoms.Empty();
    Super::EndPlay(EndPlayReason);
}

#if WITH_EDITOR
EDataValidationResult AClinicReferenceBook::IsDataValid(FDataValidationContext& Context) const
{
    if (IsTemplate())
    {
        return EDataValidationResult::NotValidated;
    }
    FText Error;
    if (!ValidateBookSetup(Error))
    {
        Context.AddError(Error);
        return EDataValidationResult::Invalid;
    }
    return EDataValidationResult::Valid;
}
#endif
