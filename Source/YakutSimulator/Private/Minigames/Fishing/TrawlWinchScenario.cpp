#include "Minigames/Fishing/TrawlWinchScenario.h"

#include "Minigames/Fishing/FishingWinch.h"
#include "Base/PhysicalLever.h"
#include "Base/WorkOperationComponent.h"
#include "Components/StaticMeshComponent.h"

ATrawlWinchScenario::ATrawlWinchScenario()
{
    TrawlVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TrawlVisual"));
    TrawlVisual->SetupAttachment(Root);
    TrawlVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PortCableVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortCableVisual"));
    PortCableVisual->SetupAttachment(Root);
    PortCableVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    StarboardCableVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StarboardCableVisual"));
    StarboardCableVisual->SetupAttachment(Root);
    StarboardCableVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PortFairlead = CreateDefaultSubobject<USceneComponent>(TEXT("PortFairlead"));
    PortFairlead->SetupAttachment(Root);
    StarboardFairlead = CreateDefaultSubobject<USceneComponent>(TEXT("StarboardFairlead"));
    StarboardFairlead->SetupAttachment(Root);
    ScenarioTitle = FText::FromString(TEXT("Оператор траловых лебёдок"));
    PreparationOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("PreparationOperation"));
    DeploymentOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("DeploymentOperation"));
    DepthOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("DepthOperation"));
    TowingOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("TowingOperation"));
    HaulingOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("HaulingOperation"));
    SecuringOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("SecuringOperation"));
    AddOperationStep(TEXT("PrepareWinches"), FText::FromString(TEXT("Проверьте нейтраль обоих рычагов и подтвердите готовность.")), PreparationOperation);
    AddOperationStep(TEXT("DeployCables"), FText::FromString(TEXT("Команда мостика: выпустить оба ваера до первой отметки и остановить барабаны.")), DeploymentOperation, { TEXT("PrepareWinches") });
    AddOperationStep(TEXT("ReachDepth"), FText::FromString(TEXT("Команда мостика: довести оба ваера до рабочей длины.")), DepthOperation, { TEXT("DeployCables") });
    AddOperationStep(TEXT("MaintainTow"), FText::FromString(TEXT("Удерживайте рабочую длину и безопасное натяжение обоих ваеров.")), TowingOperation, { TEXT("ReachDepth") });
    AddOperationStep(TEXT("HaulCables"), FText::FromString(TEXT("Команда мостика: выбрать оба ваера, синхронно остановить барабаны.")), HaulingOperation, { TEXT("MaintainTow") });
    AddOperationStep(TEXT("SecureWinches"), FText::FromString(TEXT("Проверьте остановку и подтвердите фиксацию оборудования.")), SecuringOperation, { TEXT("HaulCables") });
}

void ATrawlWinchScenario::BeginPlay()
{
    Super::BeginPlay();
    if (!PortWinch || !StarboardWinch || !PortWinch->ControlLever || !StarboardWinch->ControlLever)
    {
        SetStatus(FText::FromString(TEXT("Не назначены обе лебёдки и их рычаги.")));
        SetActorTickEnabled(false);
        return;
    }
    AddTickPrerequisiteActor(PortWinch);
    AddTickPrerequisiteActor(StarboardWinch);
    if (EmergencyControl)
    {
        EmergencyControl->OnItemAction.AddUniqueDynamic(this, &ATrawlWinchScenario::ReceiveEmergency);
    }
}

bool ATrawlWinchScenario::AreControlsNeutral() const
{
    return PortWinch && StarboardWinch && PortWinch->ControlLever && StarboardWinch->ControlLever
        && FMath::Abs(PortWinch->ControlLever->Value) < 0.15f && FMath::Abs(StarboardWinch->ControlLever->Value) < 0.15f
        && FMath::Abs(PortWinch->DrumSpeed) < 2 && FMath::Abs(StarboardWinch->DrumSpeed) < 2;
}

bool ATrawlWinchScenario::IsLoadSafe() const
{
    return PortWinch->Tension < MaximumTension && StarboardWinch->Tension < MaximumTension
        && FMath::Abs(PortWinch->Tension - StarboardWinch->Tension) < MaximumTensionDifference
        && FMath::Abs(PortWinch->CableLength - StarboardWinch->CableLength) < MaximumLengthDifference;
}

bool ATrawlWinchScenario::IsAtLength(float Length) const
{
    return FMath::Abs(PortWinch->CableLength - Length) <= LengthTolerance && FMath::Abs(StarboardWinch->CableLength - Length) <= LengthTolerance;
}

void ATrawlWinchScenario::EmergencyStop()
{
    bEmergencyLatched = true;
    PortWinch->bEmergencyStopped = true;
    StarboardWinch->bEmergencyStopped = true;
    PortWinch->DrumSpeed = 0;
    StarboardWinch->DrumSpeed = 0;
    StableSeconds = 0;
    SetStatus(FText::FromString(TEXT("Аварийная остановка. Верните оба рычага в нейтраль и подтвердите сброс.")));
}

void ATrawlWinchScenario::ReceiveEmergency(AWorkItem* Item, EWorkAction Action)
{
    if (Action == EWorkAction::Used && PortWinch && StarboardWinch)
    {
        EmergencyStop();
    }
}

void ATrawlWinchScenario::ConfirmCommand()
{
    if (!AreControlsNeutral())
    {
        SetStatus(FText::FromString(TEXT("Остановите оба барабана и установите рычаги в нейтраль.")));
        return;
    }
    if (bEmergencyLatched)
    {
        bEmergencyLatched = false;
        PortWinch->bEmergencyStopped = false;
        StarboardWinch->bEmergencyStopped = false;
        OverloadSeconds = 0;
        SetStatus(FText::FromString(TEXT("Аварийная остановка снята. Восстановите длину и баланс натяжения.")));
    }
    else if (Stage == EWinchCycleStage::Preparation)
    {
        ReportResult(TEXT("PrepareWinches"));
        Stage = EWinchCycleStage::PayingOut;
        PortWinch->bMotorEnabled = true;
        StarboardWinch->bMotorEnabled = true;
    }
    else if (Stage == EWinchCycleStage::Securing && IsAtLength(0))
    {
        ReportResult(TEXT("SecureWinches"));
        Stage = EWinchCycleStage::Completed;
        PortWinch->bMotorEnabled = false;
        StarboardWinch->bMotorEnabled = false;
        SetStatus(FText::FromString(TEXT("Промысловый цикл завершён. Оборудование зафиксировано.")));
    }
}

void ATrawlWinchScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    CycleSeconds += DeltaSeconds;
    const float AverageLength = (PortWinch->CableLength + StarboardWinch->CableLength) * 0.5f;
    TrawlVisual->SetRelativeLocation(FVector(850 + AverageLength * 0.25f, 0, -30 - AverageLength * 0.15f));
    for (const TPair<UStaticMeshComponent*, USceneComponent*>& Cable : { TPair<UStaticMeshComponent*, USceneComponent*>(PortCableVisual, PortFairlead), TPair<UStaticMeshComponent*, USceneComponent*>(StarboardCableVisual, StarboardFairlead) })
    {
        const FVector Start = Cable.Value->GetComponentLocation();
        const FVector End = TrawlVisual->GetComponentLocation();
        Cable.Key->SetWorldLocation((Start + End) * 0.5f);
        Cable.Key->SetWorldRotation(FRotationMatrix::MakeFromZ(End - Start).Rotator());
        Cable.Key->SetWorldScale3D(FVector(0.025, 0.025, FVector::Distance(Start, End) / 100));
    }
    const float LengthDifference = PortWinch->CableLength - StarboardWinch->CableLength;
    float PortLoad = 8;
    float StarboardLoad = 8;
    if (Stage == EWinchCycleStage::Towing)
    {
        PortLoad = 42 + FMath::Sin(CycleSeconds * 0.65f) * 14;
        StarboardLoad = 42 + FMath::Sin(CycleSeconds * 0.65f + 1.4f) * 14;
        StarboardLoad += LoadSurge * FMath::Max(0.0f, FMath::Sin(StableTowingSeconds * 2 * PI / LoadVariationPeriod - PI * 0.5f));
    }
    PortWinch->ExternalLoad = PortLoad + FMath::Max(0.0f, -LengthDifference) * 0.05f;
    StarboardWinch->ExternalLoad = StarboardLoad + FMath::Max(0.0f, LengthDifference) * 0.05f;
    if (PortTensionControl)
    {
        PortWinch->TensionSetting = (PortTensionControl->Value + 1) * 0.5f;
    }
    if (StarboardTensionControl)
    {
        StarboardWinch->TensionSetting = (StarboardTensionControl->Value + 1) * 0.5f;
    }
    if (bEmergencyLatched)
    {
        return;
    }
    if (!IsLoadSafe())
    {
        if (OverloadSeconds == 0)
        {
            SetStatus(FText::FromString(TEXT("Опасная нагрузка или рассогласование ваеров. Скорректируйте обе стороны.")));
        }
        OverloadSeconds += DeltaSeconds;
        if (OverloadSeconds > 3)
        {
            EmergencyStop();
        }
        StableSeconds = 0;
        return;
    }
    OverloadSeconds = 0;
    float TargetLength = 0;
    if (Stage == EWinchCycleStage::PayingOut)
    {
        TargetLength = DeploymentLength;
    }
    else if (Stage == EWinchCycleStage::WorkingDepth || Stage == EWinchCycleStage::Towing)
    {
        TargetLength = WorkingLength;
    }
    if (IsAtLength(TargetLength) && AreControlsNeutral())
    {
        StableSeconds += DeltaSeconds;
    }
    else
    {
        StableSeconds = 0;
    }
    if (Stage == EWinchCycleStage::Towing)
    {
        if (StableSeconds > 0)
        {
            StableTowingSeconds += DeltaSeconds;
        }
        if (StableTowingSeconds >= TowingSeconds)
        {
            ReportResult(TEXT("MaintainTow"));
            Stage = EWinchCycleStage::Hauling;
            StableSeconds = 0;
        }
    }
    else if (StableSeconds >= 1.5f)
    {
        if (Stage == EWinchCycleStage::PayingOut)
        {
            ReportResult(TEXT("DeployCables"));
            Stage = EWinchCycleStage::WorkingDepth;
        }
        else if (Stage == EWinchCycleStage::WorkingDepth)
        {
            ReportResult(TEXT("ReachDepth"));
            Stage = EWinchCycleStage::Towing;
        }
        else if (Stage == EWinchCycleStage::Hauling)
        {
            ReportResult(TEXT("HaulCables"));
            Stage = EWinchCycleStage::Securing;
        }
        StableSeconds = 0;
    }
}

FText ATrawlWinchScenario::GetReadout() const
{
    if (!PortWinch || !StarboardWinch)
    {
        return FText::FromString(TEXT("Оборудование не подключено."));
    }
    float TargetLength = 0;
    if (Stage == EWinchCycleStage::PayingOut)
    {
        TargetLength = DeploymentLength;
    }
    else if (Stage == EWinchCycleStage::WorkingDepth || Stage == EWinchCycleStage::Towing)
    {
        TargetLength = WorkingLength;
    }
    return FText::FromString(FString::Format(TEXT("ЛЕВЫЙ: {0} м   {1} кН   {2} м/с\nПРАВЫЙ: {3} м   {4} кН   {5} м/с\nЗаданная длина: {6} м\nДопуск длины: {7} м   Разность: до {8} м\nНагрузка: до {9} кН   Разность: до {10} кН\nБуксировка в норме: {11} / {12} с"), {
        FMath::RoundToFloat(PortWinch->CableLength / 10) / 10, FMath::RoundToInt(PortWinch->Tension), FMath::RoundToFloat(PortWinch->DrumSpeed / 10) / 10,
        FMath::RoundToFloat(StarboardWinch->CableLength / 10) / 10, FMath::RoundToInt(StarboardWinch->Tension), FMath::RoundToFloat(StarboardWinch->DrumSpeed / 10) / 10,
        TargetLength / 100, LengthTolerance / 100, MaximumLengthDifference / 100, MaximumTension, MaximumTensionDifference, FMath::FloorToInt(StableTowingSeconds), FMath::RoundToInt(TowingSeconds) }));
}

void ATrawlWinchScenario::RestartWorkstation()
{
    if (!AreControlsNeutral())
    {
        SetStatus(FText::FromString(TEXT("Перед новым циклом установите оба рычага в нейтраль.")));
        return;
    }
    Super::RestartWorkstation();
    Stage = EWinchCycleStage::Preparation;
    bEmergencyLatched = false;
    StableSeconds = 0;
    StableTowingSeconds = 0;
    OverloadSeconds = 0;
    CycleSeconds = 0;
    for (AFishingWinch* Winch : { PortWinch.Get(), StarboardWinch.Get() })
    {
        Winch->CableLength = 0;
        Winch->DrumSpeed = 0;
        Winch->bMotorEnabled = false;
        Winch->bEmergencyStopped = false;
        Winch->bFaulted = false;
    }
}
