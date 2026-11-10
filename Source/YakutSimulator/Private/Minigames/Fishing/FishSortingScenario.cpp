#include "Minigames/Fishing/FishSortingScenario.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Minigames/Fishing/FishItem.h"
#include "Minigames/Fishing/FishingStations.h"
#include "Base/PhysicalLever.h"
#include "Base/WorkOperationComponent.h"

AFishSortingScenario::AFishSortingScenario()
{
    ScenarioTitle = FText::FromString(TEXT("Сортировка рыбы"));
    ConveyorZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ConveyorZone"));
    ConveyorZone->SetupAttachment(Root);
    ConveyorZone->SetBoxExtent(FVector(40, 100, 25));
    ConveyorZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ConveyorZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    ConveyorZone->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    ConveyorZone->SetGenerateOverlapEvents(true);
    FeedPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FeedPoint"));
    FeedPoint->SetupAttachment(Root);
    ConveyorSurface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ConveyorSurface"));
    ConveyorSurface->SetupAttachment(Root);
    ConveyorSurface->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    WaterStream = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterStream"));
    WaterStream->SetupAttachment(Root);
    WaterStream->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaterStream->SetVisibility(false);
    StartOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("StartOperation"));
    WashOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("WashOperation"));
    SortOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("SortOperation"));
    WashOperation->RequiredCount = BatchFishCount;
    SortOperation->RequiredCount = BatchFishCount;
    AddOperationStep(TEXT("StartLine"), FText::FromString(TEXT("Включите ленту, подачу рыбы и воду.")), StartOperation);
    AddOperationStep(TEXT("WashFish"), FText::FromString(TEXT("Промойте поступающую рыбу в приёмном лотке.")), WashOperation, { TEXT("StartLine") });
    AddOperationStep(TEXT("SortFish"), FText::FromString(TEXT("Разложите рыбу в лотки соответствующих видов.")), SortOperation, { TEXT("StartLine") });
}

void AFishSortingScenario::BeginPlay()
{
    WashOperation->RequiredCount = BatchFishCount;
    SortOperation->RequiredCount = BatchFishCount;
    Super::BeginPlay();
    if (!FishItemClass || FishSpecies.IsEmpty() || !ConveyorControl || !FeedControl || !WaterControl || SortingTrays.IsEmpty())
    {
        SetStatus(FText::FromString(TEXT("Не настроены подача, вода или сортировочные лотки.")));
        SetActorTickEnabled(false);
        return;
    }
    for (AFishSortingConveyor* Tray : SortingTrays)
    {
        Tray->Scenario = this;
    }
}

void AFishSortingScenario::ConfirmCommand()
{
    if (ConveyorControl && FeedControl && WaterControl && ConveyorControl->Value > 0.15f && FeedControl->Value > 0.15f && WaterControl->Value > 0.2f)
    {
        if (!bProductionStarted)
        {
            bProductionStarted = true;
            ReportResult(TEXT("StartLine"));
        }
    }
    else
    {
        SetStatus(FText::FromString(TEXT("Для подачи включите ленту, подачу рыбы и воду.")));
    }
}

void AFishSortingScenario::FeedFish()
{
    if (SpawnedFish >= BatchFishCount)
    {
        return;
    }
    FActorSpawnParameters Parameters;
    Parameters.Owner = this;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AFishItem* Fish = GetWorld()->SpawnActor<AFishItem>(FishItemClass, FeedPoint->GetComponentLocation(), FeedPoint->GetComponentRotation(), Parameters);
    if (Fish)
    {
        Fish->InitializeFish(FishSpecies[SpawnedFish % FishSpecies.Num()], this);
        ++SpawnedFish;
    }
}

void AFishSortingScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetCompletedStepCount() == Steps.Num())
    {
        CurrentBeltSpeed = 0;
        ConveyorSurface->SetScalarParameterValueOnMaterials(TEXT("BeltSpeed"), 0);
        WaterStream->SetVisibility(false);
        return;
    }
    CurrentBeltSpeed = FMath::Max(0.0f, ConveyorControl->Value) * MaximumBeltSpeed;
    ConveyorSurface->SetScalarParameterValueOnMaterials(TEXT("BeltSpeed"), CurrentBeltSpeed / 100);
    WaterStream->SetVisibility(WaterControl->Value > 0.2f);
    const float FeedRatio = FMath::Max(0.0f, FeedControl->Value);
    if (CurrentBeltSpeed > 1 && FeedRatio > 0.15f)
    {
        if (!bProductionStarted && WaterControl->Value > 0.2f)
        {
            ConfirmCommand();
        }
        if (bProductionStarted)
        {
            FeedAccumulator += DeltaSeconds * FeedRatio;
            if (FeedAccumulator >= FeedInterval)
            {
                FeedAccumulator = 0;
                FeedFish();
            }
        }
    }
    TArray<AActor*> Items;
    ConveyorZone->GetOverlappingActors(Items, AFishItem::StaticClass());
    for (AActor* Actor : Items)
    {
        AFishItem* Fish = CastChecked<AFishItem>(Actor);
        if (Fish->Scenario != this || Fish->bSorted)
        {
            continue;
        }
        if (!Fish->bIsHeld && Fish->Body->IsSimulatingPhysics())
        {
            const FVector Velocity = Fish->Body->GetPhysicsLinearVelocity();
            Fish->Body->AddForce((GetActorRightVector() * CurrentBeltSpeed - FVector(Velocity.X, Velocity.Y, 0)).GetClampedToMaxSize(90) * Fish->Body->GetMass() * 5);
        }
        if (WaterControl->Value > 0.2f && !Fish->bWashed)
        {
            Fish->WashingProgress += DeltaSeconds * WaterControl->Value / WashingSeconds;
            if (Fish->WashingProgress >= 1)
            {
                Fish->WashingProgress = 1;
                Fish->bWashed = true;
                ReportResult(TEXT("WashFish"));
            }
        }
    }
}

FText AFishSortingScenario::GetReadout() const
{
    return FText::FromString(FString::Format(TEXT("Подано: {0} / {1}\nПромыто: {2}   Отсортировано: {3}\nСкорость ленты: {4} см/с\nБерите рыбу из приёмного лотка и переносите в лоток своего вида."), {
        SpawnedFish, BatchFishCount, GetRecordedResult(TEXT("WashFish")), GetRecordedResult(TEXT("SortFish")), FMath::RoundToInt(CurrentBeltSpeed) }));
}

void AFishSortingScenario::RestartWorkstation()
{
    for (TActorIterator<AFishItem> Fish(GetWorld()); Fish; ++Fish)
    {
        if (Fish->GetOwner() == this)
        {
            Fish->Destroy();
        }
    }
    SpawnedFish = 0;
    FeedAccumulator = 0;
    bProductionStarted = false;
    Super::RestartWorkstation();
}
