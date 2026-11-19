#include "Minigames/Clinic/ClinicPatient.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Minigames/Clinic/ClinicActionDefinition.h"
#include "Minigames/Clinic/ClinicScenario.h"
#include "Minigames/Clinic/ClinicPatientAnimInstance.h"

AClinicPatient::AClinicPatient()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    Body = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Root);
    Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Voice = CreateDefaultSubobject<UAudioComponent>(TEXT("Voice"));
    Voice->SetupAttachment(Body);
    Voice->bAutoActivate = false;
    AddArea(TEXT("Mouth"), Mouth, MouthHighlight);
    AddArea(TEXT("LeftFinger"), LeftFinger, LeftFingerHighlight);
    AddArea(TEXT("LeftChest"), LeftChest, LeftChestHighlight);
    AddArea(TEXT("RightChest"), RightChest, RightChestHighlight);
    AddArea(TEXT("LeftBack"), LeftBack, LeftBackHighlight);
    AddArea(TEXT("RightBack"), RightBack, RightBackHighlight);
    AddArea(TEXT("LeftForearm"), LeftForearm, LeftForearmHighlight);
    AddArea(TEXT("LeftUpperArm"), LeftUpperArm, LeftUpperArmHighlight);
}

void AClinicPatient::AddArea(FName Identifier, TObjectPtr<UBoxComponent>& Zone, TObjectPtr<UStaticMeshComponent>& Highlight)
{
    Zone = CreateDefaultSubobject<UBoxComponent>(Identifier);
    Zone->SetupAttachment(Body, Identifier);
    Zone->InitBoxExtent(FVector::OneVector);
    Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Zone->SetCollisionResponseToAllChannels(ECR_Ignore);
    Zone->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    const FName HighlightName(*FString::Format(TEXT("{0}Highlight"), {Identifier.ToString()}));
    Highlight = CreateDefaultSubobject<UStaticMeshComponent>(HighlightName);
    Highlight->SetupAttachment(Zone);
    Highlight->SetMobility(EComponentMobility::Movable);
    Highlight->SetRelativeScale3D(FVector(0.02, 0.16, 0.16));
    Highlight->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Highlight->SetCastShadow(false);
    Highlight->SetHiddenInGame(true);
    Areas.Add(FClinicExaminationArea{Identifier, Zone, Highlight});
}

void AClinicPatient::RefreshAreaAttachments()
{
    for (const FClinicExaminationArea& Area : Areas)
    {
        if (Area.Zone && Area.Zone->GetOwner() == this && !Area.AreaIdentifier.IsNone()
            && (Area.Zone->GetAttachParent() != Body || Area.Zone->GetAttachSocketName() != Area.AreaIdentifier))
        {
            Area.Zone->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, Area.AreaIdentifier);
        }
    }
}

void AClinicPatient::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshAreaAttachments();
}

void AClinicPatient::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    RefreshAreaAttachments();
}

void AClinicPatient::InitializePatient(AClinicScenario* InScenario, int32 InVisitIdentifier, const FClinicPatientPresentation& Presentation)
{
    if (Scenario)
    {
        Scenario->OnClinicActionCompleted.RemoveDynamic(this, &AClinicPatient::HandleActionCompleted);
    }
    CancelInspection();
    Scenario = InScenario;
    VisitIdentifier = InVisitIdentifier;
    InitialPoseIdentifier = ResolveInitialPose(Presentation.PoseIdentifier);
    if (!bHasInitialTransform)
    {
        InitialTransform = GetActorTransform();
        InitialMesh = Body->GetSkeletalMeshAsset();
        for (int32 MaterialIndex = 0; MaterialIndex < Body->GetNumMaterials(); ++MaterialIndex)
        {
            InitialMaterials.Add(Body->GetMaterial(MaterialIndex));
        }
        bHasInitialTransform = true;
    }
    SetActorTransform(InitialTransform);
    if (Presentation.Mesh)
    {
        Body->SetSkeletalMeshAsset(Presentation.Mesh);
    }
    else
    {
        Body->SetSkeletalMeshAsset(InitialMesh);
    }
    Body->EmptyOverrideMaterials();
    if (Presentation.Material)
    {
        for (int32 MaterialIndex = 0; MaterialIndex < Body->GetNumMaterials(); ++MaterialIndex)
        {
            Body->SetMaterial(MaterialIndex, Presentation.Material);
        }
    }
    else if (Body->GetSkeletalMeshAsset() == InitialMesh)
    {
        for (int32 MaterialIndex = 0; MaterialIndex < InitialMaterials.Num(); ++MaterialIndex)
        {
            Body->SetMaterial(MaterialIndex, InitialMaterials[MaterialIndex]);
        }
    }
    if (Scenario)
    {
        Scenario->OnClinicActionCompleted.AddUniqueDynamic(this, &AClinicPatient::HandleActionCompleted);
    }
    ReceivePresentationChanged(Presentation);
    ReturnToInitialPose();
}

FClinicPatientPoseState AClinicPatient::GetPoseState() const
{
    return PoseState;
}

FName AClinicPatient::ResolveInitialPose(FName CasePose) const
{
    if (CasePose.IsNone())
    {
        return DefaultPoseIdentifier;
    }
    return CasePose;
}

bool AClinicPatient::HasPoseDefinition(FName PoseIdentifier) const
{
    return PoseDefinitions.ContainsByPredicate([PoseIdentifier](const FClinicPatientPoseDefinition& Definition)
    {
        return Definition.PoseIdentifier == PoseIdentifier;
    });
}

bool AClinicPatient::ValidatePoseDefinitions(FText& Error) const
{
    TSet<FName> Identifiers;
    for (const FClinicPatientPoseDefinition& Definition : PoseDefinitions)
    {
        if (Definition.PoseIdentifier.IsNone() || Identifiers.Contains(Definition.PoseIdentifier)
            || !FMath::IsFinite(Definition.YawOffset) || !FMath::IsFinite(Definition.TransitionSeconds)
            || Definition.TransitionSeconds < 0)
        {
            Error = FText::FromString(TEXT("Patient poses require unique identifiers, finite yaw and nonnegative transition durations"));
            return false;
        }
        Identifiers.Add(Definition.PoseIdentifier);
    }
    return true;
}

void AClinicPatient::CancelPoseRequest()
{
    ++PoseState.RequestIdentifier;
    PoseState.Phase = EClinicPresentationPhase::Idle;
    bAnimationComplete = false;
    if (UClinicPatientAnimInstance* Animation = Cast<UClinicPatientAnimInstance>(Body->GetAnimInstance()))
    {
        Animation->CancelPoseRequest();
    }
}

bool AClinicPatient::RequestPose(FName PoseIdentifier)
{
    if (PoseState.VisitIdentifier == VisitIdentifier && PoseState.PoseIdentifier == PoseIdentifier
        && (PoseState.Phase == EClinicPresentationPhase::Ready || PoseState.Phase == EClinicPresentationPhase::Transitioning))
    {
        return true;
    }
    CancelPoseRequest();
    PoseState.VisitIdentifier = VisitIdentifier;
    PoseState.PoseIdentifier = PoseIdentifier;
    PoseState.Phase = EClinicPresentationPhase::Transitioning;
    PoseElapsed = 0;
    PoseStartRotation = GetActorRotation();
    PoseTargetRotation = InitialTransform.Rotator();
    PoseTransitionSeconds = 0;
    const FClinicPatientPoseDefinition* Definition = PoseDefinitions.FindByPredicate([PoseIdentifier](const FClinicPatientPoseDefinition& Candidate)
    {
        return Candidate.PoseIdentifier == PoseIdentifier;
    });
    FClinicPatientPoseDefinition DefaultDefinition;
    DefaultDefinition.PoseIdentifier = PoseIdentifier;
    DefaultDefinition.TransitionSeconds = 0;
    if (!Definition)
    {
        if (!PoseDefinitions.IsEmpty())
        {
            PoseState.Phase = EClinicPresentationPhase::Failed;
            OnPatientPoseChanged.Broadcast(PoseState);
            return false;
        }
        Definition = &DefaultDefinition;
    }
    PoseTargetRotation.Yaw += Definition->YawOffset;
    PoseTransitionSeconds = Definition->TransitionSeconds;
    const FClinicPatientPoseState Request = PoseState;
    OnPatientPoseChanged.Broadcast(Request);
    if (PoseState.VisitIdentifier != Request.VisitIdentifier || PoseState.RequestIdentifier != Request.RequestIdentifier
        || PoseState.Phase != EClinicPresentationPhase::Transitioning)
    {
        return false;
    }
    if (UClinicPatientAnimInstance* Animation = Cast<UClinicPatientAnimInstance>(Body->GetAnimInstance()))
    {
        Animation->BeginPoseRequest(Request, *Definition);
    }
    else if (Definition->TransitionMontage || Definition->LoopAnimation)
    {
        CompletePoseRequest(VisitIdentifier, PoseState.RequestIdentifier, EClinicPresentationResult::Failed);
    }
    else
    {
        CompletePoseRequest(VisitIdentifier, PoseState.RequestIdentifier, EClinicPresentationResult::Completed);
    }
    return PoseState.Phase != EClinicPresentationPhase::Failed;
}

bool AClinicPatient::CompletePoseRequest(int32 ExpectedVisitIdentifier, int32 ExpectedRequestIdentifier,
    EClinicPresentationResult Result)
{
    if (PoseState.Phase != EClinicPresentationPhase::Transitioning || VisitIdentifier != ExpectedVisitIdentifier
        || PoseState.VisitIdentifier != ExpectedVisitIdentifier || PoseState.RequestIdentifier != ExpectedRequestIdentifier
        || bAnimationComplete)
    {
        return false;
    }
    if (Result != EClinicPresentationResult::Completed)
    {
        PoseState.Phase = EClinicPresentationPhase::Failed;
        OnPatientPoseChanged.Broadcast(PoseState);
        RefreshExaminationGuidance();
        return true;
    }
    bAnimationComplete = true;
    UpdatePoseTransition(0);
    return true;
}

void AClinicPatient::UpdatePoseTransition(float DeltaSeconds)
{
    if (PoseState.Phase != EClinicPresentationPhase::Transitioning)
    {
        return;
    }
    PoseElapsed += DeltaSeconds;
    float Alpha = 1;
    if (PoseTransitionSeconds > 0)
    {
        Alpha = FMath::Clamp(PoseElapsed / PoseTransitionSeconds, 0.0f, 1.0f);
    }
    SetActorRotation(FQuat::Slerp(PoseStartRotation.Quaternion(), PoseTargetRotation.Quaternion(), Alpha));
    if (Alpha >= 1 && bAnimationComplete)
    {
        PoseState.Phase = EClinicPresentationPhase::Ready;
        OnPatientPoseChanged.Broadcast(PoseState);
        RefreshExaminationGuidance();
    }
}

void AClinicPatient::ReturnToInitialPose()
{
    if (bHasInitialTransform)
    {
        RequestPose(InitialPoseIdentifier);
    }
}

bool AClinicPatient::IsReadyForExamination(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const
{
    if (!Scenario || !Scenario->IsExamining(ExpectedVisitIdentifier) || VisitIdentifier != ExpectedVisitIdentifier
        || PoseState.Phase != EClinicPresentationPhase::Ready)
    {
        return false;
    }
    if (InstrumentExamination && InstrumentExamination->ActionIdentifier == ActionIdentifier)
    {
        return ActiveAreas.Contains(AreaIdentifier);
    }
    return Inspection && Inspection->ActionIdentifier == ActionIdentifier && InspectionArea == AreaIdentifier;
}

void AClinicPatient::RefreshExaminationStage()
{
    UClinicActionDefinition* Action = InstrumentExamination;
    if (!Action)
    {
        Action = Inspection;
    }
    ActiveAreas.Reset();
    if (!Action)
    {
        return;
    }
    FName PoseIdentifier = InitialPoseIdentifier;
    const TArray<FName> Examined = GetExaminedAreas();
    for (const FClinicExaminationStage& Stage : Action->PresentationStages)
    {
        const bool bNeeded = Stage.Areas.ContainsByPredicate([this, &Examined](FName Area)
        {
            return (InstrumentExamination && !Examined.Contains(Area)) || (Inspection && Area == InspectionArea);
        });
        if (bNeeded)
        {
            ActiveAreas = Stage.Areas;
            PoseIdentifier = Stage.PoseIdentifier;
            break;
        }
    }
    RequestPose(PoseIdentifier);
    RefreshExaminationGuidance();
}

bool AClinicPatient::ValidateAreaBindings(const USkeletalMesh* Mesh, FText& Error) const
{
    if (!Mesh)
    {
        Mesh = Body->GetSkeletalMeshAsset();
        if (bHasInitialTransform)
        {
            Mesh = InitialMesh;
        }
    }
    for (const FClinicExaminationArea& Area : Areas)
    {
        if (!Area.Zone || Area.Zone->GetAttachParent() != Body)
        {
            Error = FText::FromString(FString::Format(TEXT("Patient area {0} must be attached to Body"), {Area.AreaIdentifier.ToString()}));
            return false;
        }
        const FName AttachmentName = Area.Zone->GetAttachSocketName();
        if (Mesh && (AttachmentName.IsNone()
            || (!Mesh->FindSocket(AttachmentName) && Mesh->GetRefSkeleton().FindBoneIndex(AttachmentName) == INDEX_NONE)))
        {
            Error = FText::FromString(FString::Format(TEXT("Patient area {0} requires a valid bone or socket on {1}"),
                {Area.AreaIdentifier.ToString(), Mesh->GetName()}));
            return false;
        }
    }
    return true;
}

UBoxComponent* AClinicPatient::FindAreaZone(FName AreaIdentifier) const
{
    const FClinicExaminationArea* Area = Areas.FindByPredicate([this, AreaIdentifier](const FClinicExaminationArea& Candidate)
    {
        if (Candidate.AreaIdentifier != AreaIdentifier || !Candidate.Zone || Candidate.Zone->GetAttachParent() != Body)
        {
            return false;
        }
        const FName AttachmentName = Candidate.Zone->GetAttachSocketName();
        return !Body->GetSkeletalMeshAsset() || (!AttachmentName.IsNone() && Body->DoesSocketExist(AttachmentName));
    });
    if (Area)
    {
        return Area->Zone.Get();
    }
    return nullptr;
}

bool AClinicPatient::HasArea(FName AreaIdentifier) const
{
    return FindAreaZone(AreaIdentifier) != nullptr;
}

bool AClinicPatient::IsPointInArea(FName AreaIdentifier, const FVector& WorldPosition, float ContactTolerance) const
{
    if (const UBoxComponent* Zone = FindAreaZone(AreaIdentifier))
    {
        const FTransform& ZoneTransform = Zone->GetComponentTransform();
        const FVector LocalPosition = ZoneTransform.InverseTransformPositionNoScale(WorldPosition);
        const FVector Extent = Zone->GetUnscaledBoxExtent() * ZoneTransform.GetScale3D().GetAbs();
        return FBox(-Extent, Extent).ComputeSquaredDistanceToPoint(LocalPosition)
            <= FMath::Square(FMath::Max(ContactTolerance, 0.0f));
    }
    return false;
}

bool AClinicPatient::BeginInstrumentExamination(UClinicActionDefinition* Action, int32 ExpectedVisitIdentifier)
{
    if (!Scenario || !Scenario->IsExamining(ExpectedVisitIdentifier) || VisitIdentifier != ExpectedVisitIdentifier
        || !Action || !Action->bUsesInstrument)
    {
        return false;
    }
    const TArray<FName> ActionAreas = Action->GetAreas();
    if (ActionAreas.IsEmpty())
    {
        return false;
    }
    for (FName AreaIdentifier : ActionAreas)
    {
        if (!HasArea(AreaIdentifier))
        {
            return false;
        }
    }
    if (InstrumentExamination == Action && PoseState.Phase != EClinicPresentationPhase::Failed)
    {
        return true;
    }
    CancelInspection(false);
    InstrumentExamination = Action;
    RefreshExaminationStage();
    return true;
}

TArray<FName> AClinicPatient::GetExaminedAreas() const
{
    TArray<FName> Examined;
    if (!Scenario || !InstrumentExamination || !Scenario->IsExamining(VisitIdentifier))
    {
        return Examined;
    }
    const TArray<FName> ActionAreas = InstrumentExamination->GetAreas();
    for (const FClinicActionRecord& Record : Scenario->GetVisitState().ActionHistory)
    {
        if (Record.ActionIdentifier == InstrumentExamination->ActionIdentifier && ActionAreas.Contains(Record.AreaIdentifier))
        {
            Examined.AddUnique(Record.AreaIdentifier);
        }
    }
    return Examined;
}

FText AClinicPatient::GetExaminationReadout() const
{
    if (!InstrumentExamination)
    {
        return FText::GetEmpty();
    }
    if (PoseState.Phase == EClinicPresentationPhase::Transitioning)
    {
        return FText::FromString(TEXT("Пациент готовится к обследованию."));
    }
    if (PoseState.Phase == EClinicPresentationPhase::Failed)
    {
        return FText::FromString(TEXT("Подготовка пациента прервана или не настроена. Повторите запрос обследования."));
    }
    return FText::FromString(FString::Format(TEXT("{0}: {1}/{2} областей"),
        {InstrumentExamination->DisplayName.ToString(), GetExaminedAreas().Num(), InstrumentExamination->GetAreas().Num()}));
}

void AClinicPatient::RefreshExaminationGuidance()
{
    const TArray<FName> Examined = GetExaminedAreas();
    for (const FClinicExaminationArea& Area : Areas)
    {
        if (!Area.Highlight)
        {
            continue;
        }
        const bool bHighlighted = InstrumentExamination && PoseState.Phase == EClinicPresentationPhase::Ready
            && ActiveAreas.Contains(Area.AreaIdentifier);
        Area.Highlight->SetVisibility(bHighlighted);
        Area.Highlight->SetHiddenInGame(!bHighlighted);
        if (bHighlighted)
        {
            FLinearColor Color = PendingAreaColor;
            if (Examined.Contains(Area.AreaIdentifier))
            {
                Color = ExaminedAreaColor;
            }
            Area.Highlight->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(Color.R, Color.G, Color.B));
        }
    }
}

bool AClinicPatient::BeginInspection(UClinicActionDefinition* Action, FName AreaIdentifier, int32 ExpectedVisitIdentifier)
{
    if (!Scenario || !Scenario->IsExamining(ExpectedVisitIdentifier) || VisitIdentifier != ExpectedVisitIdentifier
        || !Action || Action->bUsesInstrument || (!AreaIdentifier.IsNone() && !HasArea(AreaIdentifier)))
    {
        return false;
    }
    CancelInspection(false);
    Inspection = Action;
    InspectionArea = AreaIdentifier;
    RefreshExaminationStage();
    ReceiveInspectionRequested(Action->ActionIdentifier, AreaIdentifier);
    return true;
}

bool AClinicPatient::CanRecordInspection(FName ActionIdentifier, FName AreaIdentifier, int32 ExpectedVisitIdentifier) const
{
    return Inspection && InspectionElapsed >= Inspection->DurationSeconds
        && IsReadyForExamination(ActionIdentifier, AreaIdentifier, ExpectedVisitIdentifier);
}

void AClinicPatient::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdatePoseTransition(DeltaSeconds);
    if (!Inspection)
    {
        return;
    }
    if (!Scenario || !Scenario->IsExamining(VisitIdentifier))
    {
        CancelInspection();
        return;
    }
    if (!IsReadyForExamination(Inspection->ActionIdentifier, InspectionArea, VisitIdentifier))
    {
        return;
    }
    InspectionElapsed += DeltaSeconds;
    if (InspectionElapsed >= Inspection->DurationSeconds)
    {
        UClinicActionDefinition* CompletedInspection = Inspection;
        const int32 CompletedVisit = VisitIdentifier;
        Scenario->RecordExamination(this, Inspection->ActionIdentifier, InspectionArea, CompletedVisit);
        if (VisitIdentifier == CompletedVisit && Inspection == CompletedInspection)
        {
            Inspection = nullptr;
            InspectionElapsed = 0;
            InspectionArea = NAME_None;
            ActiveAreas.Reset();
            ReturnToInitialPose();
        }
    }
}

void AClinicPatient::CancelInspection(bool bNotifyReset)
{
    Inspection = nullptr;
    InspectionElapsed = 0;
    InspectionArea = NAME_None;
    InstrumentExamination = nullptr;
    ActiveAreas.Reset();
    CancelPoseRequest();
    RefreshExaminationGuidance();
    Voice->Stop();
    if (bNotifyReset)
    {
        ReceiveVisitReset();
    }
}

void AClinicPatient::HandleActionCompleted(const FClinicActionResult& Result)
{
    if (Result.VisitIdentifier != VisitIdentifier)
    {
        return;
    }
    if (InstrumentExamination && Result.ActionIdentifier == InstrumentExamination->ActionIdentifier)
    {
        RefreshExaminationStage();
    }
    if (Result.Executor != this)
    {
        return;
    }
    if (Result.Sound)
    {
        Voice->SetSound(Result.Sound);
        Voice->Play();
    }
    ReceiveResponse(Result);
}

void AClinicPatient::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Scenario)
    {
        Scenario->OnClinicActionCompleted.RemoveDynamic(this, &AClinicPatient::HandleActionCompleted);
    }
    CancelInspection();
    Super::EndPlay(EndPlayReason);
}
