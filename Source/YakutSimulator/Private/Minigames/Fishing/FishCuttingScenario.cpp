#include "Minigames/Fishing/FishCuttingScenario.h"

#include "EngineUtils.h"
#include "Minigames/Fishing/FishItem.h"
#include "Minigames/Fishing/FishReceivingBin.h"
#include "Minigames/Fishing/FishingStations.h"
#include "Base/InteractionZoneComponent.h"
#include "Minigames/Fishing/KnifeItem.h"
#include "Base/WorkOperationComponent.h"

AFishCuttingScenario::AFishCuttingScenario()
{
    ScenarioTitle = FText::FromString(TEXT("Разделка рыбы"));
    StartOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("StartOperation"));
    CutOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("CutOperation"));
    CutOperation->RequiredCount = BatchFishCount;
    AddOperationStep(TEXT("StartCutting"), FText::FromString(TEXT("Подтвердите подачу первой рыбы на разделочную доску.")), StartOperation);
    AddOperationStep(TEXT("ProcessFish"), FText::FromString(TEXT("Выполните разрезы для каждого вида рыбы. Ведите нож от зелёной метки к оранжевой.")), CutOperation, { TEXT("StartCutting") });
}

void AFishCuttingScenario::BeginPlay()
{
    CutOperation->RequiredCount = BatchFishCount;
    Super::BeginPlay();
    if (!CuttingBench || !CuttingKnife || !ProductBin || !FishItemClass || FishSpecies.IsEmpty())
    {
        SetStatus(FText::FromString(TEXT("Разделочное рабочее место не настроено.")));
        SetActorTickEnabled(false);
        return;
    }
    for (const UFishSpeciesDefinition* Species : FishSpecies)
    {
        if (!Species || Species->CuttingSteps.IsEmpty())
        {
            SetStatus(FText::FromString(TEXT("Для каждого вида рыбы нужно настроить правила разрезов.")));
            SetActorTickEnabled(false);
            return;
        }
        TSet<FName> Identifiers;
        for (const FFishCutDefinition& Cut : Species->CuttingSteps)
        {
            if (Cut.CutIdentifier.IsNone() || Identifiers.Contains(Cut.CutIdentifier) || FVector::DistSquared(Cut.Start, Cut.End) < 1)
            {
                SetStatus(FText::FromString(TEXT("Правила разрезов содержат пустое имя, повтор или нулевую длину.")));
                SetActorTickEnabled(false);
                return;
            }
            Identifiers.Add(Cut.CutIdentifier);
        }
    }
    CuttingBench->Scenario = this;
    ProductBin->Scenario = this;
    ProductBin->Capacity = BatchFishCount;
}

void AFishCuttingScenario::FeedFish()
{
    if (SpawnedFish >= BatchFishCount || (IsValid(CuttingBench->CurrentFish) && (CuttingBench->CurrentFish->bOnProcessingBench || CuttingBench->CurrentFish->ProductState == EFishProductState::Whole)))
    {
        return;
    }
    const FTransform SpawnTransform(CuttingBench->GetActorRotation(), CuttingBench->Intake->GetComponentLocation());
    AFishItem* Fish = GetWorld()->SpawnActorDeferred<AFishItem>(FishItemClass, SpawnTransform, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (Fish)
    {
        Fish->InitializeFish(FishSpecies[SpawnedFish % FishSpecies.Num()], this);
        Fish->FinishSpawning(SpawnTransform);
        bool bAccepted = CuttingBench->CurrentFish == Fish && Fish->bOnProcessingBench;
        if (!bAccepted)
        {
            bAccepted = CuttingBench->Intake->TryAcceptItem(Fish);
        }
        if (bAccepted)
        {
            ++SpawnedFish;
            SetStatus(Fish->GetCutInstruction());
        }
        else
        {
            Fish->Destroy();
        }
    }
}

void AFishCuttingScenario::ConfirmCommand()
{
    if (!CuttingBench || !CuttingKnife || !ProductBin || FishSpecies.IsEmpty())
    {
        return;
    }
    if (!bBatchStarted)
    {
        bBatchStarted = true;
        ReportResult(TEXT("StartCutting"));
    }
    FeedFish();
}

void AFishCuttingScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bBatchStarted && GetRecordedResult(TEXT("ProcessFish")) < BatchFishCount)
    {
        FeedFish();
    }
}

FText AFishCuttingScenario::GetReadout() const
{
    const AFishItem* Fish = nullptr;
    if (CuttingBench)
    {
        Fish = CuttingBench->CurrentFish;
    }
    if (IsValid(Fish) && Fish->Species && Fish->bOnProcessingBench)
    {
        return FText::FromString(FString::Format(TEXT("Вид: {0}\n{1}\nЗавершено разрезов: {2} / {3}\nТекущий разрез: {4}%\nОбработано рыб: {5} / {6}"), {
            Fish->Species->DisplayName.ToString(), Fish->GetCutInstruction().ToString(), Fish->CurrentCutIndex, Fish->Species->CuttingSteps.Num(),
            FMath::RoundToInt(Fish->CurrentCutProgress * 100), GetRecordedResult(TEXT("ProcessFish")), BatchFishCount }));
    }
    return FText::FromString(FString::Format(TEXT("Обработано рыб: {0} / {1}\nДля подачи используйте подтверждение команды."), { GetRecordedResult(TEXT("ProcessFish")), BatchFishCount }));
}

void AFishCuttingScenario::RestartWorkstation()
{
    if (!CuttingBench || !CuttingKnife || !ProductBin)
    {
        return;
    }
    for (TActorIterator<AFishItem> Fish(GetWorld()); Fish; ++Fish)
    {
        if (Fish->GetOwner() == this)
        {
            Fish->Destroy();
        }
    }
    CuttingBench->CurrentFish = nullptr;
    CuttingKnife->Sharpness = 1;
    ProductBin->Contents.Reset();
    SpawnedFish = 0;
    bBatchStarted = false;
    Super::RestartWorkstation();
}
