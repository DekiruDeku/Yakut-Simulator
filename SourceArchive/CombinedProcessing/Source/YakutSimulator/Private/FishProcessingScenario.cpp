#include "FishProcessingScenario.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "FishItem.h"
#include "InteractionZoneComponent.h"
#include "PhysicalLever.h"
#include "WorkOperationComponent.h"
#include "KnifeItem.h"

AFishReceivingBin::AFishReceivingBin()
{
    Intake->AcceptedItemType = NAME_None;
    Intake->SetBoxExtent(FVector(28, 22, 16));
    Intake->SetRelativeLocation(FVector(0, 0, 8));
}

bool AFishReceivingBin::IsFull() const
{
    return Contents.Num() >= Capacity;
}

bool AFishReceivingBin::CanReceiveItem(AWorkItem* Item)
{
    if (IsFull() || Contents.Contains(Item))
    {
        return false;
    }
    if (bReceiveWaste)
    {
        return Item->ItemType == TEXT("Debris") || Item->ItemType == TEXT("Offcut");
    }
    const AFishItem* Fish = Cast<AFishItem>(Item);
    return Fish && Fish->ProductState == EFishProductState::Processed && Fish->bWashed && !Fish->bDefective
        && (!AcceptedSpecies || Fish->Species == AcceptedSpecies);
}

void AFishReceivingBin::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    Item->SetActorRelativeLocation(FVector(0, 0, Contents.Num() * 6));
    Contents.Add(Item);
    Item->Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (AFishItem* Fish = Cast<AFishItem>(Item))
    {
        Fish->ProductState = EFishProductState::Packed;
    }
    if (Scenario)
    {
        FName Result = TEXT("CollectProducts");
        if (Item->ItemType == TEXT("Debris"))
        {
            Result = TEXT("RemoveDebris");
        }
        else if (Item->ItemType == TEXT("Offcut"))
        {
            Result = TEXT("ClearWaste");
        }
        Scenario->ReportResult(Result);
        if (IsFull())
        {
            Scenario->SetStatus(FText::FromString(TEXT("Контейнер заполнен. Подтвердите замену контейнеров.")));
        }
    }
}

void AFishReceivingBin::ReplaceContainer()
{
    if (!IsFull())
    {
        return;
    }
    for (AWorkItem* Item : Contents)
    {
        if (IsValid(Item))
        {
            Item->Destroy();
        }
    }
    Contents.Reset();
    if (Scenario)
    {
        Scenario->ReportResult(TEXT("ChangeContainer"));
    }
}

AFishProcessingScenario::AFishProcessingScenario()
{
    ScenarioTitle = FText::FromString(TEXT("Обработка улова"));
    ConveyorZone = CreateDefaultSubobject<UBoxComponent>(TEXT("ConveyorZone"));
    ConveyorZone->SetupAttachment(Root);
    ConveyorZone->SetBoxExtent(FVector(34, 130, 20));
    ConveyorZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ConveyorZone->SetCollisionResponseToAllChannels(ECR_Ignore);
    ConveyorZone->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
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
    SortOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("SortOperation"));
    DebrisOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("DebrisOperation"));
    ProcessOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("ProcessOperation"));
    WashOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("WashOperation"));
    ProductOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("ProductOperation"));
    WasteOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("WasteOperation"));
    ContainerOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("ContainerOperation"));
    AddOperationStep(TEXT("StartLine"), FText::FromString(TEXT("Включите конвейер и подачу рыбы.")), StartOperation);
    AddOperationStep(TEXT("SortFish"), FText::FromString(TEXT("Разложите рыбу по лоткам видов; посторонние предметы отправьте в отходы.")), SortOperation, { TEXT("StartLine") });
    AddOperationStep(TEXT("RemoveDebris"), FText::FromString(TEXT("Удалите посторонние предметы с ленты.")), DebrisOperation, { TEXT("StartLine") });
    AddOperationStep(TEXT("ProcessFish"), FText::FromString(TEXT("Обработайте отсортированную рыбу ножом на доске.")), ProcessOperation, { TEXT("StartLine") });
    AddOperationStep(TEXT("WashFish"), FText::FromString(TEXT("Промойте рыбу на рабочей доске, включив воду.")), WashOperation, { TEXT("StartLine") });
    AddOperationStep(TEXT("ClearWaste"), FText::FromString(TEXT("Соберите отходы обработки в красный контейнер.")), WasteOperation, { TEXT("StartLine") });
    AddOperationStep(TEXT("CollectProducts"), FText::FromString(TEXT("Поместите обработанную и промытую рыбу в синий контейнер.")), ProductOperation, { TEXT("SortFish"), TEXT("ProcessFish"), TEXT("WashFish") });
    AddOperationStep(TEXT("ChangeContainer"), FText::FromString(TEXT("Замените заполненный контейнер подтверждением команды.")), ContainerOperation, { TEXT("StartLine") });
}

void AFishProcessingScenario::BeginPlay()
{
    SortOperation->RequiredCount = BatchFishCount;
    ProcessOperation->RequiredCount = BatchFishCount;
    WashOperation->RequiredCount = BatchFishCount;
    ProductOperation->RequiredCount = BatchFishCount;
    WasteOperation->RequiredCount = BatchFishCount;
    DebrisOperation->RequiredCount = FMath::Max(1, BatchDebrisCount);
    Super::BeginPlay();
    if (BatchDebrisCount == 0)
    {
        ReportResult(TEXT("RemoveDebris"));
    }
    if (!FishItemClass || FishSpecies.IsEmpty() || (BatchDebrisCount > 0 && !DebrisItemClass) || !ConveyorControl || !FeedControl || !WaterControl || !ProcessingBench || !ProcessingKnife)
    {
        SetStatus(FText::FromString(TEXT("Рабочая линия не настроена.")));
        SetActorTickEnabled(false);
    }
}

void AFishProcessingScenario::FeedItem()
{
    FActorSpawnParameters Parameters;
    Parameters.Owner = this;
    Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (bNextFeedIsDebris && SpawnedDebris < BatchDebrisCount && DebrisItemClass)
    {
        GetWorld()->SpawnActor<AWorkItem>(DebrisItemClass, FeedPoint->GetComponentLocation(), FeedPoint->GetComponentRotation(), Parameters);
        ++SpawnedDebris;
        bNextFeedIsDebris = false;
    }
    else if (SpawnedFish < BatchFishCount)
    {
        AFishItem* Fish = GetWorld()->SpawnActor<AFishItem>(FishItemClass, FeedPoint->GetComponentLocation(), FeedPoint->GetComponentRotation(), Parameters);
        if (Fish)
        {
            Fish->InitializeFish(FishSpecies[SpawnedFish % FishSpecies.Num()], this);
            Fish->bDefective = SpawnedFish % 3 == 2;
            if (Fish->bDefective)
            {
                Fish->Body->SetVectorParameterValueOnMaterials(TEXT("ProductTint"), FVector(0.6, 0.7, 0.4));
            }
            ++SpawnedFish;
            bNextFeedIsDebris = true;
        }
    }
    else if (SpawnedDebris < BatchDebrisCount && DebrisItemClass)
    {
        bNextFeedIsDebris = true;
        FeedItem();
    }
}

void AFishProcessingScenario::Tick(float DeltaSeconds)
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
    if (CurrentBeltSpeed > 2 && FeedRatio > 0.15f)
    {
        if (!bProductionStarted)
        {
            bProductionStarted = true;
            ReportResult(TEXT("StartLine"));
        }
        FeedAccumulator += DeltaSeconds * FeedRatio;
        if (FeedAccumulator >= FeedInterval)
        {
            FeedAccumulator = 0;
            FeedItem();
        }
    }
    TArray<AActor*> Items;
    ConveyorZone->GetOverlappingActors(Items, AWorkItem::StaticClass());
    for (AActor* Actor : Items)
    {
        AWorkItem* Item = CastChecked<AWorkItem>(Actor);
        if (Item->bIsHeld || !Item->Body->IsSimulatingPhysics())
        {
            continue;
        }
        const FVector DesiredVelocity = GetActorRightVector() * CurrentBeltSpeed;
        const FVector Velocity = Item->Body->GetPhysicsLinearVelocity();
        Item->Body->AddForce((DesiredVelocity - FVector(Velocity.X, Velocity.Y, 0)).GetClampedToMaxSize(90) * Item->Body->GetMass() * 5);
    }
    AFishItem* Fish = ProcessingBench->CurrentFish;
    if (WaterControl->Value > 0.2f && IsValid(Fish) && Fish->bOnProcessingBench && !Fish->bWashed)
    {
        Fish->WashingProgress += DeltaSeconds * WaterControl->Value / WashingSeconds;
        if (Fish->WashingProgress >= 1)
        {
            Fish->WashingProgress = 1;
            Fish->bWashed = true;
            ReportResult(TEXT("WashFish"));
            SetStatus(FText::FromString(TEXT("Рыба промыта.")));
        }
    }
}

void AFishProcessingScenario::ConfirmCommand()
{
    bool bReplaced = false;
    for (AFishReceivingBin* Bin : ReceivingBins)
    {
        if (Bin && Bin->IsFull())
        {
            Bin->ReplaceContainer();
            bReplaced = true;
        }
    }
    if (bReplaced)
    {
        SetStatus(FText::FromString(TEXT("Заполненные контейнеры заменены.")));
    }
    else
    {
        SetStatus(FText::FromString(TEXT("Заполненных контейнеров нет.")));
    }
}

FText AFishProcessingScenario::GetReadout() const
{
    return FText::FromString(FString::Format(TEXT("Лента: {0} см/с   Подано рыбы: {1} / {2}\nОтсортировано: {3}   Обработано: {4}   Промыто: {5}\nПродукция: {6}   Отходы: {7}   Посторонние объекты: {8}\nЗамены контейнеров: {9}"), {
        FMath::RoundToInt(CurrentBeltSpeed), SpawnedFish, BatchFishCount, GetRecordedResult(TEXT("SortFish")), GetRecordedResult(TEXT("ProcessFish")), GetRecordedResult(TEXT("WashFish")),
        GetRecordedResult(TEXT("CollectProducts")), GetRecordedResult(TEXT("ClearWaste")), GetRecordedResult(TEXT("RemoveDebris")), GetRecordedResult(TEXT("ChangeContainer")) }));
}

void AFishProcessingScenario::RestartWorkstation()
{
    if (!ProcessingBench || !ProcessingKnife)
    {
        return;
    }
    for (TActorIterator<AWorkItem> Item(GetWorld()); Item; ++Item)
    {
        if (Item->GetOwner() == this)
        {
            Item->Destroy();
        }
    }
    for (AFishReceivingBin* Bin : ReceivingBins)
    {
        if (Bin)
        {
            Bin->Contents.Reset();
        }
    }
    ProcessingBench->CurrentFish = nullptr;
    ProcessingKnife->Sharpness = 1;
    SpawnedFish = 0;
    SpawnedDebris = 0;
    FeedAccumulator = 0;
    bNextFeedIsDebris = false;
    bProductionStarted = false;
    Super::RestartWorkstation();
}
