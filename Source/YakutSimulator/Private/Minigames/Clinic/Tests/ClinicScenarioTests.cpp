#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Minigames/Clinic/ClinicActionDefinition.h"
#include "Minigames/Clinic/ClinicDiseaseDefinition.h"
#include "Minigames/Clinic/ClinicPatient.h"
#include "Minigames/Clinic/ClinicPatientCaseDefinition.h"
#include "Minigames/Clinic/ClinicScenario.h"
#include "Minigames/Clinic/ClinicReferenceBook.h"
#include "Base/VirtualRealityPawn.h"
#include "Base/WorkItem.h"
#include "MotionControllerComponent.h"
#include "ReferenceSkeleton.h"
#include <limits>

#if WITH_EDITOR
#include "Kismet2/ComponentEditorUtils.h"
#endif

struct FClinicFixture
{
    UWorld* World;
    AClinicScenario* Scenario;

    FClinicFixture()
    {
        World = UWorld::CreateWorld(EWorldType::Game, false);
        Scenario = World->SpawnActor<AClinicScenario>();
        Scenario->Patient = World->SpawnActor<AClinicPatient>();
        UDataTable* Catalog = NewObject<UDataTable>(Scenario);
        Catalog->RowStruct = FClinicIndicatorRow::StaticStruct();
        FClinicIndicatorRow Indicator;
        Indicator.DisplayName = FText::FromString(TEXT("Fixture indicator"));
        Catalog->AddRow(TEXT("Value"), Indicator);
        UClinicDiseaseDefinition* Disease = NewObject<UClinicDiseaseDefinition>(Scenario);
        Disease->DiseaseIdentifier = TEXT("Expected");
        Disease->DisplayName = FText::FromString(TEXT("Expected condition"));
        Disease->ReferenceText = FText::FromString(TEXT("Fixture reference"));
        Disease->Sources.Add(TEXT("Fixture source"));
        UClinicDiseaseDefinition* OtherDisease = NewObject<UClinicDiseaseDefinition>(Scenario);
        OtherDisease->DiseaseIdentifier = TEXT("Other");
        OtherDisease->DisplayName = FText::FromString(TEXT("Other condition"));
        OtherDisease->ReferenceText = Disease->ReferenceText;
        OtherDisease->Sources = Disease->Sources;
        Scenario->DiseaseDefinitions = {Disease, OtherDisease};
        UClinicActionDefinition* Question = NewObject<UClinicActionDefinition>(Scenario);
        Question->ActionIdentifier = TEXT("AskFixture");
        Question->DisplayName = FText::FromString(TEXT("Fixture question"));
        UClinicActionDefinition* Investigation = NewObject<UClinicActionDefinition>(Scenario);
        Investigation->ActionIdentifier = TEXT("OrderFixture");
        Investigation->DisplayName = FText::FromString(TEXT("Fixture investigation"));
        Investigation->ActionType = EClinicActionType::Investigation;
        Scenario->ActionDefinitions = {Question, Investigation};
        UClinicPatientCaseDefinition* PatientCase = NewObject<UClinicPatientCaseDefinition>(Scenario);
        PatientCase->CaseIdentifier = TEXT("Fixture");
        PatientCase->PatientName = FText::FromString(TEXT("Fixture patient"));
        PatientCase->Complaint = FText::FromString(TEXT("Fixture complaint"));
        PatientCase->ExpectedDisease = Disease;
        FClinicCaseOutcome Answer;
        Answer.ActionIdentifier = Question->ActionIdentifier;
        Answer.Response = FText::FromString(TEXT("Fixture answer"));
        FClinicObservation Observation;
        Observation.ObservationIdentifier = TEXT("Answer");
        Observation.Indicator.DataTable = Catalog;
        Observation.Indicator.RowName = TEXT("Value");
        Observation.SourceAction = Question->ActionIdentifier;
        Observation.NumericValue = 1;
        Answer.Observations.Add(Observation);
        FClinicCaseOutcome LabResult;
        LabResult.ActionIdentifier = Investigation->ActionIdentifier;
        LabResult.Response = FText::FromString(TEXT("Fixture report"));
        PatientCase->Outcomes = {Answer, LabResult};
        FClinicEvidenceSet Evidence;
        Evidence.Observations.Add(TEXT("Answer"));
        PatientCase->SufficientEvidenceSets.Add(Evidence);
        Scenario->PatientCases.Add(PatientCase);
        Scenario->StartVisit(PatientCase);
    }

    ~FClinicFixture()
    {
        World->DestroyWorld(false);
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicActionDurationValidationTest, "YakutSimulator.Clinic.ActionDurationValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicActionDurationValidationTest::RunTest(const FString& Parameters)
{
    UClinicActionDefinition* Action = NewObject<UClinicActionDefinition>();
    Action->ActionIdentifier = TEXT("DurationFixture");
    Action->DisplayName = FText::FromString(TEXT("Duration fixture"));
    FText Error;
    for (float InvalidDuration : {0.0f, -1.0f, std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()})
    {
        Action->ActionType = EClinicActionType::Question;
        Action->DurationSeconds = InvalidDuration;
        Action->InvestigationDelaySeconds = InvalidDuration;
        TestTrue(TEXT("Questions ignore unused timers"), Action->ValidateDefinition(Error));

        Action->ActionType = EClinicActionType::Examination;
        Action->DurationSeconds = 2;
        TestTrue(TEXT("Examinations ignore investigation delay"), Action->ValidateDefinition(Error));
        Action->DurationSeconds = InvalidDuration;
        TestFalse(TEXT("Examinations require a finite positive duration"), Action->ValidateDefinition(Error));

        Action->ActionType = EClinicActionType::Investigation;
        Action->InvestigationDelaySeconds = 5;
        TestTrue(TEXT("Investigations ignore examination duration"), Action->ValidateDefinition(Error));
        Action->InvestigationDelaySeconds = InvalidDuration;
        TestFalse(TEXT("Investigations require a finite positive delay"), Action->ValidateDefinition(Error));
    }
    Action->ActionType = EClinicActionType::Question;
    Action->bUsesInstrument = true;
    TestFalse(TEXT("Questions still reject instrument settings"), Action->ValidateDefinition(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicImmediateDecisionTest, "YakutSimulator.Clinic.ImmediateDecision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicImmediateDecisionTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    const int32 VisitIdentifier = Fixture.Scenario->GetVisitSnapshot().VisitIdentifier;
    TestTrue(TEXT("No hidden observations before actions"), Fixture.Scenario->GetVisitSnapshot().KnownObservations.IsEmpty());
    TestTrue(TEXT("Immediate finish is accepted"), Fixture.Scenario->FinishExamination(VisitIdentifier).Status == EClinicCommandStatus::Applied);
    FClinicDecision Decision;
    Decision.DiseaseIdentifier = TEXT("Other");
    TestTrue(TEXT("Wrong diagnosis is accepted"), Fixture.Scenario->SubmitDecision(Decision, VisitIdentifier).Status == EClinicCommandStatus::Applied);
    const FClinicVisitSnapshot Snapshot = Fixture.Scenario->GetVisitSnapshot();
    TestTrue(TEXT("Visit completes"), Snapshot.Phase == EClinicVisitPhase::Completed);
    TestFalse(TEXT("Wrong diagnosis is not credited"), Snapshot.Evaluation.bCorrectDiagnosis);
    TestFalse(TEXT("Missing evidence is not credited"), Snapshot.Evaluation.bSufficientEvidence);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicKnowledgeTest, "YakutSimulator.Clinic.KnowledgeAndOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicKnowledgeTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    const int32 VisitIdentifier = Fixture.Scenario->GetVisitSnapshot().VisitIdentifier;
    Fixture.Scenario->AskQuestion(TEXT("AskFixture"), VisitIdentifier);
    Fixture.Scenario->AskQuestion(TEXT("AskFixture"), VisitIdentifier);
    TestEqual(TEXT("Repeated questions do not duplicate facts"), Fixture.Scenario->GetVisitSnapshot().KnownObservations.Num(), 1);
    TestTrue(TEXT("Sufficient facts do not finish examination"), Fixture.Scenario->IsExamining(VisitIdentifier));
    Fixture.Scenario->OrderInvestigation(TEXT("OrderFixture"), VisitIdentifier);
    TestTrue(TEXT("Repeat order is unchanged"), Fixture.Scenario->OrderInvestigation(TEXT("OrderFixture"), VisitIdentifier).Status == EClinicCommandStatus::Unchanged);
    TestEqual(TEXT("One order is retained"), Fixture.Scenario->GetVisitSnapshot().InvestigationOrders.Num(), 1);
    TestTrue(TEXT("Pending report cannot be read"), Fixture.Scenario->ReadInvestigation(TEXT("OrderFixture"), VisitIdentifier).Status == EClinicCommandStatus::Rejected);
    TestTrue(TEXT("Pending report does not block finish"), Fixture.Scenario->FinishExamination(VisitIdentifier).Status == EClinicCommandStatus::Applied);
    FClinicDecision Decision;
    Decision.DiseaseIdentifier = TEXT("Expected");
    Fixture.Scenario->SubmitDecision(Decision, VisitIdentifier);
    TestTrue(TEXT("Obtained evidence is credited"), Fixture.Scenario->GetVisitSnapshot().Evaluation.bSufficientEvidence);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicResetTest, "YakutSimulator.Clinic.ResetAndIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicResetTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    const int32 OldVisit = Fixture.Scenario->GetVisitSnapshot().VisitIdentifier;
    Fixture.Scenario->AskQuestion(TEXT("AskFixture"), OldVisit);
    Fixture.Scenario->OrderInvestigation(TEXT("OrderFixture"), OldVisit);
    Fixture.Scenario->RestartWorkstation();
    const FClinicVisitSnapshot Snapshot = Fixture.Scenario->GetVisitSnapshot();
    TestTrue(TEXT("New visit has a new identifier"), Snapshot.VisitIdentifier != OldVisit);
    TestTrue(TEXT("Knowledge and pending orders are cleared"), Snapshot.KnownObservations.IsEmpty() && Snapshot.InvestigationOrders.IsEmpty());
    TestTrue(TEXT("Stale command is rejected"), Fixture.Scenario->AskQuestion(TEXT("AskFixture"), OldVisit).Status == EClinicCommandStatus::Rejected);
    TestTrue(TEXT("Forged examination is rejected"), Fixture.Scenario->RecordExamination(Fixture.Scenario, TEXT("AskFixture"), NAME_None, Snapshot.VisitIdentifier).Status == EClinicCommandStatus::Rejected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicGroupedExaminationTest, "YakutSimulator.Clinic.GroupedExamination",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicGroupedExaminationTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    UClinicActionDefinition* Auscultation = NewObject<UClinicActionDefinition>(Fixture.Scenario);
    Auscultation->ActionIdentifier = TEXT("AuscultateChest");
    Auscultation->ActionType = EClinicActionType::Examination;
    Auscultation->bUsesInstrument = true;
    FClinicExaminationStage Stage;
    Stage.PoseIdentifier = TEXT("Idle");
    Stage.Areas = {TEXT("LeftChest"), TEXT("RightChest"), TEXT("LeftBack"), TEXT("RightBack")};
    Auscultation->PresentationStages = {Stage};
    Fixture.Scenario->ActionDefinitions.Add(Auscultation);
    const int32 VisitIdentifier = Fixture.Scenario->GetVisitSnapshot().VisitIdentifier;
    TestTrue(TEXT("One command starts the whole examination"),
        Fixture.Scenario->RequestExamination(TEXT("AuscultateChest"), NAME_None, VisitIdentifier).Status == EClinicCommandStatus::Applied);
    TestTrue(TEXT("Starting guidance reveals no medical observations"), Fixture.Scenario->GetVisitSnapshot().KnownObservations.IsEmpty());
    for (const FClinicExaminationArea& Area : Fixture.Scenario->Patient->Areas)
    {
        if (Auscultation->GetAreas().Contains(Area.AreaIdentifier))
        {
            TestTrue(TEXT("Every auscultation area is highlighted"), Area.Highlight && Area.Highlight->IsVisible() && !Area.Highlight->bHiddenInGame);
        }
    }
    TestTrue(TEXT("An unrelated area is rejected"),
        Fixture.Scenario->RequestExamination(TEXT("AuscultateChest"), TEXT("Mouth"), VisitIdentifier).Status == EClinicCommandStatus::Rejected);
    Fixture.Scenario->FinishExamination(VisitIdentifier);
    TestTrue(TEXT("Ending examination clears guidance"), Fixture.Scenario->Patient->GetExaminationReadout().IsEmpty());
    for (const FClinicExaminationArea& Area : Fixture.Scenario->Patient->Areas)
    {
        TestTrue(TEXT("Highlights are hidden after ending examination"), !Area.Highlight || Area.Highlight->bHiddenInGame);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicBoneAttachmentsTest, "YakutSimulator.Clinic.BoneAttachments",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicBoneAttachmentsTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    AClinicPatient* Patient = Fixture.Scenario->Patient;
#if WITH_EDITOR
    const FVector PreviousLocation = Patient->LeftChest->GetComponentLocation();
    Patient->LeftChest->AttachToComponent(Patient->Root, FAttachmentTransformRules::KeepWorldTransform);
    Patient->RerunConstructionScripts();
    TestTrue(TEXT("Legacy root attachment is repaired"), Patient->LeftChest->GetAttachParent() == Patient->Body);
    TestTrue(TEXT("Repair preserves the calibrated position"), Patient->LeftChest->GetComponentLocation().Equals(PreviousLocation));
#endif
    USkeletalMesh* Mesh = NewObject<USkeletalMesh>();
    FReferenceSkeletonModifier Skeleton(Mesh->GetRefSkeleton(), nullptr);
    Skeleton.Add(FMeshBoneInfo(TEXT("Root"), TEXT("Root"), INDEX_NONE), FTransform::Identity);
    Skeleton.Add(FMeshBoneInfo(TEXT("Torso"), TEXT("Torso"), 0), FTransform(FVector(0, 0, 100)));
    USkeletalMeshSocket* Socket = NewObject<USkeletalMeshSocket>(Mesh);
    Socket->SocketName = TEXT("ChestSocket");
    Socket->BoneName = TEXT("Torso");
    Mesh->GetMeshOnlySocketList().Add(Socket);
    for (const FClinicExaminationArea& Area : Patient->Areas)
    {
        TestTrue(TEXT("Area follows the skeletal mesh"), Area.Zone->GetAttachParent() == Patient->Body);
        TestTrue(TEXT("Highlight follows its area"), Area.Highlight->GetAttachParent() == Area.Zone);
        TestEqual(TEXT("Default socket name matches the area identifier"), Area.Zone->GetAttachSocketName(), Area.AreaIdentifier);
        Area.Zone->AttachToComponent(Patient->Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("Torso"));
    }
    FText Error;
    TestTrue(TEXT("Existing bones are accepted"), Patient->ValidateAreaBindings(Mesh, Error));
    Patient->LeftChest->AttachToComponent(Patient->Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("ChestSocket"));
    TestTrue(TEXT("Existing sockets are accepted"), Patient->ValidateAreaBindings(Mesh, Error));
    Patient->LeftChest->AttachToComponent(Patient->Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("UnknownBone"));
    TestFalse(TEXT("Missing bones are rejected"), Patient->ValidateAreaBindings(Mesh, Error));
    Patient->LeftChest->AttachToComponent(Patient->Body, FAttachmentTransformRules::KeepRelativeTransform, TEXT("Torso"));
    USkeletalMesh* IncompatibleMesh = NewObject<USkeletalMesh>();
    TestFalse(TEXT("Incompatible replacement meshes are rejected"), Patient->ValidateAreaBindings(IncompatibleMesh, Error));
    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicEditableAreasTest, "YakutSimulator.Clinic.EditableAreas",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicEditableAreasTest::RunTest(const FString& Parameters)
{
    const AClinicPatient* Patient = GetDefault<AClinicPatient>();
    for (const FClinicExaminationArea& Area : Patient->Areas)
    {
        TestNotNull(FString::Format(TEXT("Area {0} is editable in the component editor"), {Area.AreaIdentifier.ToString()}),
            FComponentEditorUtils::GetPropertyForEditableNativeComponent(Area.Zone));
        TestNotNull(FString::Format(TEXT("Highlight {0} is editable in the component editor"), {Area.AreaIdentifier.ToString()}),
            FComponentEditorUtils::GetPropertyForEditableNativeComponent(Area.Highlight));
    }
    return true;
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicPresentationTest, "YakutSimulator.Clinic.PatientPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicPresentationTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    AClinicPatient* Patient = Fixture.Scenario->Patient;
    for (FName Identifier : {FName(TEXT("Idle")), FName(TEXT("ChestFront")), FName(TEXT("ChestBack"))})
    {
        FClinicPatientPoseDefinition Definition;
        Definition.PoseIdentifier = Identifier;
        Definition.TransitionSeconds = 0.5f;
        if (Identifier == TEXT("Idle"))
        {
            Definition.TransitionSeconds = 0;
        }
        if (Identifier == TEXT("ChestBack"))
        {
            Definition.YawOffset = 180;
        }
        Patient->PoseDefinitions.Add(Definition);
    }
    UClinicActionDefinition* Action = NewObject<UClinicActionDefinition>(Fixture.Scenario);
    Action->ActionIdentifier = TEXT("PoseFixture");
    Action->DisplayName = FText::FromString(TEXT("Pose fixture"));
    Action->ActionType = EClinicActionType::Examination;
    Action->bUsesInstrument = true;
    FClinicExaminationStage Front;
    Front.PoseIdentifier = TEXT("ChestFront");
    Front.Areas = {TEXT("LeftChest")};
    FClinicExaminationStage Back;
    Back.PoseIdentifier = TEXT("ChestBack");
    Back.Areas = {TEXT("LeftBack")};
    Action->PresentationStages = {Front, Back};
    FText Error;
    TestTrue(TEXT("Stages define the examination areas"), Action->ValidateDefinition(Error));
    const TArray<FName> ExpectedAreas = {TEXT("LeftChest"), TEXT("LeftBack")};
    TestTrue(TEXT("Area order follows the stage order"), Action->GetAreas() == ExpectedAreas);
    Action->PresentationStages = {Front};
    TestTrue(TEXT("Removing a stage updates the action without a second area list"),
        Action->ValidateDefinition(Error) && Action->GetAreas() == Front.Areas);
    Action->PresentationStages = {Front, Front, Back};
    TestFalse(TEXT("Duplicate stage areas are rejected"), Action->ValidateDefinition(Error));
    Action->PresentationStages = {Front};
    Action->PresentationStages[0].Areas.Add(NAME_None);
    TestFalse(TEXT("Empty area identifiers are rejected"), Action->ValidateDefinition(Error));
    Action->PresentationStages[0].Areas = {TEXT("LeftChest"), TEXT("LeftChest")};
    TestFalse(TEXT("Duplicate areas within one stage are rejected"), Action->ValidateDefinition(Error));
    Action->PresentationStages[0].Areas.Reset();
    TestFalse(TEXT("Stages without areas are rejected"), Action->ValidateDefinition(Error));
    Action->PresentationStages = {Front};
    Action->PresentationStages[0].PoseIdentifier = NAME_None;
    TestFalse(TEXT("Stages without a pose are rejected"), Action->ValidateDefinition(Error));
    Action->PresentationStages.Reset();
    TestTrue(TEXT("Removing every stage clears the derived areas"), Action->GetAreas().IsEmpty());
    TestFalse(TEXT("Instrument examinations require at least one stage"), Action->ValidateDefinition(Error));
    Action->bUsesInstrument = false;
    TestTrue(TEXT("Examinations without body areas can omit stages"), Action->ValidateDefinition(Error));
    Action->PresentationStages = {Front};
    for (EClinicActionType Type : {EClinicActionType::Question, EClinicActionType::Investigation})
    {
        Action->ActionType = Type;
        TestFalse(TEXT("Questions and investigations reject examination stages"), Action->ValidateDefinition(Error));
    }
    Action->ActionType = EClinicActionType::Examination;
    Action->bUsesInstrument = true;
    Action->PresentationStages = {Front, Back};
    Fixture.Scenario->ActionDefinitions.Add(Action);
    const int32 VisitIdentifier = Fixture.Scenario->GetVisitSnapshot().VisitIdentifier;
    Fixture.Scenario->RequestExamination(Action->ActionIdentifier, NAME_None, VisitIdentifier);
    const FClinicPatientPoseState Request = Patient->GetPoseState();
    TestTrue(TEXT("Preparation blocks measurements"), Request.Phase == EClinicPresentationPhase::Transitioning
        && !Patient->IsReadyForExamination(Action->ActionIdentifier, TEXT("LeftChest"), VisitIdentifier));
    TestTrue(TEXT("Preparation hides guidance"), Patient->LeftChestHighlight->bHiddenInGame);
    Patient->BeginInstrumentExamination(Action, VisitIdentifier);
    TestEqual(TEXT("Repeated requests do not restart a pose"), Patient->GetPoseState().RequestIdentifier, Request.RequestIdentifier);
    AActor* PatientActor = Patient;
    PatientActor->Tick(0.5f);
    TestTrue(TEXT("Ready pose enables the front group"), Patient->IsReadyForExamination(Action->ActionIdentifier, TEXT("LeftChest"), VisitIdentifier));
    TestFalse(TEXT("The back group remains unavailable"), Patient->IsReadyForExamination(Action->ActionIdentifier, TEXT("LeftBack"), VisitIdentifier));
    TestTrue(TEXT("Only the front group is highlighted"), !Patient->LeftChestHighlight->bHiddenInGame && Patient->LeftBackHighlight->bHiddenInGame);
    TestFalse(TEXT("An old completion cannot change a ready pose"),
        Patient->CompletePoseRequest(VisitIdentifier, Request.RequestIdentifier, EClinicPresentationResult::Interrupted));
    TestTrue(TEXT("The player can finish during presentation"),
        Fixture.Scenario->FinishExamination(VisitIdentifier).Status == EClinicCommandStatus::Applied);
    TestFalse(TEXT("Finishing disables contact"), Patient->IsReadyForExamination(Action->ActionIdentifier, TEXT("LeftChest"), VisitIdentifier));
    Fixture.Scenario->RestartWorkstation();
    TestFalse(TEXT("Previous-visit animation completion is ignored"),
        Patient->CompletePoseRequest(VisitIdentifier, Request.RequestIdentifier, EClinicPresentationResult::Completed));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicReferenceEvidenceTest, "YakutSimulator.Clinic.ReferenceEvidence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicReferenceEvidenceTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    UClinicDiseaseDefinition* Disease = Fixture.Scenario->DiseaseDefinitions[0];
    FClinicReferenceSymptom Symptom;
    Symptom.SymptomIdentifier = TEXT("NumericSign");
    Symptom.Description = FText::FromString(TEXT("Fixture numeric sign"));
    Symptom.Indicator = Fixture.Scenario->PatientCases[0]->Outcomes[0].Observations[0].Indicator;
    Symptom.bHasMinimum = true;
    Symptom.Minimum = 1;
    Disease->Symptoms.Add(Symptom);
    const int32 Visit = Fixture.Scenario->GetVisitSnapshot().VisitIdentifier;
    EClinicReferenceSymptomStatus Status;
    TestTrue(TEXT("A valid unseen symptom can be queried"), Fixture.Scenario->GetReferenceSymptomStatus(
        Disease->DiseaseIdentifier, Symptom.SymptomIdentifier, Visit, Status));
    TestTrue(TEXT("Unrevealed outcomes stay unknown"), Status == EClinicReferenceSymptomStatus::Unknown);
    Fixture.Scenario->AskQuestion(TEXT("AskFixture"), Visit);
    Fixture.Scenario->GetReferenceSymptomStatus(Disease->DiseaseIdentifier, Symptom.SymptomIdentifier, Visit, Status);
    TestTrue(TEXT("A known value on an inclusive boundary confirms the sign"), Status == EClinicReferenceSymptomStatus::Confirmed);
    Disease->Symptoms[0].bIncludeMinimum = false;
    Fixture.Scenario->GetReferenceSymptomStatus(Disease->DiseaseIdentifier, Symptom.SymptomIdentifier, Visit, Status);
    TestTrue(TEXT("An exclusive boundary is respected"), Status == EClinicReferenceSymptomStatus::NotConfirmed);
    TestEqual(TEXT("Reading a reference does not add clinical actions"), Fixture.Scenario->GetVisitSnapshot().ActionHistory.Num(), 1);
    TestFalse(TEXT("Unknown symptom identifiers are rejected"), Fixture.Scenario->GetReferenceSymptomStatus(
        Disease->DiseaseIdentifier, TEXT("Unknown"), Visit, Status));
    Fixture.Scenario->RestartWorkstation();
    TestFalse(TEXT("Previous-visit queries are rejected"), Fixture.Scenario->GetReferenceSymptomStatus(
        Disease->DiseaseIdentifier, Symptom.SymptomIdentifier, Visit, Status));

    UDataTable* Table = NewObject<UDataTable>(Fixture.Scenario);
    Table->RowStruct = FClinicIndicatorRow::StaticStruct();
    FClinicIndicatorRow Sounds;
    Sounds.DisplayName = FText::FromString(TEXT("Fixture sounds"));
    Sounds.ValueType = EClinicValueType::Choice;
    Sounds.Categories = {TEXT("Normal"), TEXT("Crackles")};
    Table->AddRow(TEXT("Sounds"), Sounds);
    FClinicReferenceSymptom LungSign;
    LungSign.SymptomIdentifier = TEXT("LungSign");
    LungSign.Description = Sounds.DisplayName;
    LungSign.Indicator.DataTable = Table;
    LungSign.Indicator.RowName = TEXT("Sounds");
    LungSign.Areas = {TEXT("LeftChest"), TEXT("RightChest")};
    LungSign.CategoryValues = {TEXT("Crackles")};
    FText Error;
    TestTrue(TEXT("A choice symptom is valid"), LungSign.Validate(Error));
    FClinicObservation Left;
    Left.Indicator = LungSign.Indicator;
    Left.AreaIdentifier = TEXT("LeftChest");
    Left.CategoryValue = TEXT("Normal");
    FClinicObservation Right = Left;
    Right.AreaIdentifier = TEXT("RightChest");
    TestTrue(TEXT("A partially normal exam remains unknown"), LungSign.Evaluate({Left}) == EClinicReferenceSymptomStatus::Unknown);
    TestTrue(TEXT("A completely normal exam does not confirm the sign"),
        LungSign.Evaluate({Left, Right}) == EClinicReferenceSymptomStatus::NotConfirmed);
    Right.CategoryValue = TEXT("Crackles");
    TestTrue(TEXT("One matching area is sufficient even if another is missing"),
        LungSign.Evaluate({Right}) == EClinicReferenceSymptomStatus::Confirmed);
    LungSign.CategoryValues = {TEXT("Invalid")};
    TestFalse(TEXT("Invalid categories are rejected"), LungSign.Validate(Error));
    Symptom.bHasMaximum = true;
    Symptom.Maximum = 0;
    TestFalse(TEXT("Reversed numeric intervals are rejected"), Symptom.Validate(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicReferenceBookTurnTest, "YakutSimulator.Clinic.ReferenceBookTurn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicReferenceBookTurnTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    AClinicReferenceBook* Book = Fixture.World->SpawnActor<AClinicReferenceBook>();
    AVirtualRealityPawn* Pawn = Fixture.World->SpawnActor<AVirtualRealityPawn>();
    Book->Scenario = Fixture.Scenario;
    Book->LeftPageEdge->SetRelativeLocation(FVector(0, -14, 0));
    Book->RightPageEdge->SetRelativeLocation(FVector(0, 14, 0));
    TestFalse(TEXT("The first spread cannot go backwards"), Book->CanTurn(-1));
    TestFalse(TEXT("Only unit directions are allowed"), Book->CanTurn(2));
    Pawn->RightHand->SetWorldLocation(FVector(0, 14, 0));
    TestTrue(TEXT("A free hand can start at the right edge"), Book->TryBeginHandGrip(Pawn, false) == EHandGripResult::Started);
    Pawn->LeftHand->SetWorldLocation(FVector(0, 14, 0));
    TestTrue(TEXT("A second hand is blocked without grabbing a nearby tool"), Book->TryBeginHandGrip(Pawn, true) == EHandGripResult::Blocked);
    Pawn->RightHand->SetWorldLocation(FVector(0, -14, 0));
    Book->Tick(0.1f);
    Book->EndHandGrip(Pawn, false, false);
    TestFalse(TEXT("Another leaf is blocked during settling"), Book->CanTurn(1));
    Book->Tick(1);
    TestEqual(TEXT("One grip advances exactly one spread"), Book->CurrentSpread, 1);
    TestFalse(TEXT("The last spread cannot go forwards"), Book->CanTurn(1));
    Pawn->RightHand->SetWorldLocation(FVector(0, 14, 0));
    TestTrue(TEXT("An unavailable edge consumes Grip without an item fallback"), Book->TryBeginHandGrip(Pawn, false) == EHandGripResult::Blocked);
    Book->Tick(1);
    TestEqual(TEXT("An idle tick cannot advance again"), Book->CurrentSpread, 1);
    Pawn->RightHand->SetWorldLocation(FVector(0, -14, 0));
    TestTrue(TEXT("The left edge starts a backwards turn"), Book->TryBeginHandGrip(Pawn, false) == EHandGripResult::Started);
    Pawn->RightHand->SetWorldLocation(FVector(0, -10, 0));
    Book->Tick(0.1f);
    Book->EndHandGrip(Pawn, false, false);
    Book->Tick(1);
    TestEqual(TEXT("Releasing before halfway returns the leaf"), Book->CurrentSpread, 1);
    Pawn->RightHand->SetWorldLocation(FVector(0, -14, 0));
    Book->TryBeginHandGrip(Pawn, false);
    Pawn->RightHand->SetWorldLocation(FVector(0, 14, 0));
    Book->Tick(0.1f);
    Book->EndHandGrip(Pawn, false, true);
    Book->Tick(1);
    TestEqual(TEXT("Cancellation never commits a page"), Book->CurrentSpread, 1);
    TestTrue(TEXT("Cancellation restores the ready state"), Book->TurnPhase == EClinicBookTurnPhase::Ready);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FClinicReferenceBookHoldingTest, "YakutSimulator.Clinic.ReferenceBookHolding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FClinicReferenceBookHoldingTest::RunTest(const FString& Parameters)
{
    FClinicFixture Fixture;
    AClinicReferenceBook* Book = Fixture.World->SpawnActor<AClinicReferenceBook>();
    AVirtualRealityPawn* Pawn = Fixture.World->SpawnActor<AVirtualRealityPawn>();
    AVirtualRealityPawn* OtherPawn = Fixture.World->SpawnActor<AVirtualRealityPawn>();
    Book->Scenario = Fixture.Scenario;
    Book->SetActorTransform(FTransform(FRotator(10, 25, 5), FVector(50, -80, 90), FVector(1.4)));
    Book->LeftHandGrip->SetRelativeLocation(FVector(-8, -6, -1));
    Book->LeftHandGrip->SetRelativeRotation(FRotator(-35, 0, 0));
    Book->BookGripZone->SetRelativeLocation(FVector(-8, -6, 0));
    Book->BookGripZone->SetBoxExtent(FVector(3, 6, 4));
    Book->LeftPageEdge->SetRelativeLocation(FVector(0, -14, 0));
    Book->RightPageEdge->SetRelativeLocation(FVector(0, 14, 0));
    for (UClinicDiseaseDefinition* Disease : Fixture.Scenario->DiseaseDefinitions)
    {
        FClinicReferenceSymptom Symptom;
        Symptom.SymptomIdentifier = TEXT("Fixture");
        Symptom.Description = FText::FromString(TEXT("Fixture symptom"));
        Symptom.Indicator = Fixture.Scenario->PatientCases[0]->Outcomes[0].Observations[0].Indicator;
        Symptom.bHasMinimum = true;
        Symptom.Minimum = 1;
        Disease->Symptoms.Add(Symptom);
    }
    FText Error;
    TestTrue(TEXT("The holding fixture has valid settings"), Book->ValidateBookSetup(Error));
    Book->DispatchBeginPlay();
    const FTransform Rest = Book->GetActorTransform();
    const FTransform Grip = Book->LeftHandGrip->GetRelativeTransform();
    Pawn->LeftHand->SetWorldLocation(Book->BookGripZone->GetComponentLocation());
    Pawn->LeftHand->SetWorldRotation(FRotator(15, -20, 10));
    Pawn->LeftHand->SetWorldScale3D(FVector(2));
    Pawn->LeftItem = Fixture.World->SpawnActor<AWorkItem>();
    TestTrue(TEXT("A hand occupied by an item cannot pick up the book"), Book->TryBeginHandGrip(Pawn, true) == EHandGripResult::Ignored);
    Pawn->LeftItem = nullptr;
    TestTrue(TEXT("The left grip picks up the cover"), Book->TryBeginHandGrip(Pawn, true) == EHandGripResult::Started);
    TestTrue(TEXT("The book is attached to the left controller"), Book->Root->GetAttachParent() == Pawn->LeftHand);
    TestTrue(TEXT("The authored grip position aligns with the controller"), Book->LeftHandGrip->GetComponentLocation().Equals(Pawn->LeftHand->GetComponentLocation(), 0.001));
    TestTrue(TEXT("The authored grip rotation aligns with the controller"), Book->LeftHandGrip->GetComponentQuat().Equals(Pawn->LeftHand->GetComponentQuat(), 0.001));
    TestTrue(TEXT("The controller scale does not resize the book"), Book->GetActorScale3D().Equals(Rest.GetScale3D(), 0.001));
    Book->EndHandGrip(OtherPawn, true, false);
    TestTrue(TEXT("Another pawn cannot release the book"), Book->Root->GetAttachParent() == Pawn->LeftHand);
    Pawn->RightHand->SetWorldLocation(Book->RightPageEdge->GetComponentLocation());
    TestTrue(TEXT("The right hand can turn a page while the left holds the book"), Book->TryBeginHandGrip(Pawn, false) == EHandGripResult::Started);
    Pawn->RightHand->SetWorldLocation(Book->GetActorTransform().TransformPosition(FVector(0, 7, 0)));
    Book->Tick(0.1f);
    const float Progress = Book->TurnProgress;
    Pawn->LeftHand->SetWorldLocationAndRotation(FVector(75, -30, 120), FRotator(25, 60, -10));
    Pawn->RightHand->SetWorldLocation(Book->GetActorTransform().TransformPosition(FVector(0, 7, 0)));
    Book->Tick(0.1f);
    TestTrue(TEXT("Moving both hands and the book together does not turn a page"), FMath::IsNearlyEqual(Book->TurnProgress, Progress, 0.001f));
    Book->EndHandGrip(Pawn, false, true);
    TestTrue(TEXT("Cancelling the right grip keeps the book held"), Book->Root->GetAttachParent() == Pawn->LeftHand);
    Pawn->RightHand->SetWorldLocation(Book->RightPageEdge->GetComponentLocation());
    Book->TryBeginHandGrip(Pawn, false);
    Pawn->RightHand->SetWorldLocation(Book->LeftPageEdge->GetComponentLocation());
    Book->Tick(0.1f);
    Book->EndHandGrip(Pawn, false, false);
    Book->Tick(1);
    TestEqual(TEXT("A completed right grip commits exactly one spread"), Book->CurrentSpread, 1);
    TestTrue(TEXT("Completing a page does not release the book"), Book->Root->GetAttachParent() == Pawn->LeftHand);
    Pawn->RightHand->SetWorldLocation(Book->LeftPageEdge->GetComponentLocation());
    Book->TryBeginHandGrip(Pawn, false);
    Fixture.Scenario->StartVisit(Fixture.Scenario->PatientCases[0]);
    TestTrue(TEXT("A new visit cancels the leaf without releasing the book"), Book->TurnPhase == EClinicBookTurnPhase::Ready && Book->Root->GetAttachParent() == Pawn->LeftHand);
    Pawn->RightHand->SetWorldLocation(Book->LeftPageEdge->GetComponentLocation());
    Book->TryBeginHandGrip(Pawn, false);
    Pawn->RightHand->SetWorldLocation(Book->RightPageEdge->GetComponentLocation());
    Book->Tick(0.1f);
    Book->EndHandGrip(Pawn, false, false);
    Book->EndHandGrip(Pawn, true, false);
    Book->Tick(1);
    TestEqual(TEXT("Releasing the book cancels even a nearly completed settling turn"), Book->CurrentSpread, 1);
    TestTrue(TEXT("Releasing the book restores the table transform"), Book->GetActorTransform().Equals(Rest, 0.001));
    TestTrue(TEXT("Returning the book removes its hand attachment"), Book->Root->GetAttachParent() == nullptr);
    TestTrue(TEXT("Picking up the book never changes its authored grip"), Book->LeftHandGrip->GetRelativeTransform().Equals(Grip));
    Pawn->LeftHand->SetWorldLocation(Book->BookGripZone->GetComponentLocation());
    Book->TryBeginHandGrip(Pawn, true);
    Pawn->RightHand->SetWorldLocation(Book->LeftPageEdge->GetComponentLocation());
    Book->TryBeginHandGrip(Pawn, false);
    Book->EndHandGrip(Pawn, true, true);
    Book->EndHandGrip(Pawn, false, true);
    TestTrue(TEXT("Losing the holding hand cancels dragging and restores the table"), Book->TurnPhase == EClinicBookTurnPhase::Ready && Book->GetActorTransform().Equals(Rest, 0.001));
    Book->BookGripZone->SetBoxExtent(FVector::ZeroVector);
    TestFalse(TEXT("A zero-size cover grip is rejected"), Book->ValidateBookSetup(Error));
    return true;
}

#endif
