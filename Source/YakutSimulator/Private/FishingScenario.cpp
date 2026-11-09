#include "FishingScenario.h"

#include "Components/WidgetComponent.h"
#include "FishingSonar.h"
#include "FishingWinch.h"
#include "Trawl.h"
#include "TrawlerVessel.h"
#include "VesselMovementComponent.h"
#include "WorkOperationComponent.h"

AFishingScenario::AFishingScenario()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    ConsoleDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("ConsoleDisplay"));
    ConsoleDisplay->SetupAttachment(Root);
    ConsoleDisplay->SetDrawSize(FVector2D(1000, 680));
    ConsoleDisplay->SetRelativeScale3D(FVector(0.075));
    ConsoleDisplay->SetCollisionProfileName(TEXT("UI"));
    SearchOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("SearchOperation"));
    DeployOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("DeployOperation"));
    CatchOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("CatchOperation"));
    RecoverOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("RecoverOperation"));
    UnloadOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("UnloadOperation"));
    SortOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("SortOperation"));
    PrepareKnifeOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("PrepareKnifeOperation"));
    ProcessOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("ProcessOperation"));
    PackOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("PackOperation"));
    FreezeOperation = CreateDefaultSubobject<UWorkOperationComponent>(TEXT("FreezeOperation"));
    ScenarioTitle = FText::FromString(TEXT("Рыболовная смена"));
    AddFishingStep(TEXT("FindFish"), FText::FromString(TEXT("Найти скопление рыбы сонаром")), SearchOperation);
    AddFishingStep(TEXT("DeployTrawl"), FText::FromString(TEXT("Выпустить трал")), DeployOperation, TEXT("FindFish"));
    AddFishingStep(TEXT("CatchFish"), FText::FromString(TEXT("Провести трал через скопление рыбы")), CatchOperation, TEXT("DeployTrawl"));
    AddFishingStep(TEXT("RecoverTrawl"), FText::FromString(TEXT("Поднять трал")), RecoverOperation, TEXT("CatchFish"));
    AddFishingStep(TEXT("UnloadCatch"), FText::FromString(TEXT("Остановить судно и выгрузить улов")), UnloadOperation, TEXT("RecoverTrawl"));
    AddFishingStep(TEXT("SortFish"), FText::FromString(TEXT("Рассортировать три рыбы по видам")), SortOperation, TEXT("UnloadCatch"));
    AddFishingStep(TEXT("PrepareKnife"), FText::FromString(TEXT("Заточить нож на камне")), PrepareKnifeOperation, TEXT("SortFish"));
    AddFishingStep(TEXT("ProcessFish"), FText::FromString(TEXT("Обработать три рыбы на рабочем столе")), ProcessOperation, TEXT("PrepareKnife"));
    AddFishingStep(TEXT("PackFish"), FText::FromString(TEXT("Упаковать три обработанные рыбы")), PackOperation, TEXT("ProcessFish"));
    AddFishingStep(TEXT("FreezeFish"), FText::FromString(TEXT("Поместить ящик с рыбой в морозильник")), FreezeOperation, TEXT("PackFish"));
    SortOperation->RequiredCount = 3;
    ProcessOperation->RequiredCount = 3;
    PackOperation->RequiredCount = 3;
    FreezeOperation->RequiredCount = 3;
}

void AFishingScenario::AddFishingStep(FName Identifier, const FText& Instruction, UWorkOperationComponent* Operation, FName Dependency)
{
    Operation->bUseExplicitResults = true;
    FScenarioStepDefinition Step;
    Step.StepIdentifier = Identifier;
    Step.Instruction = Instruction;
    Step.Operation = Operation;
    if (!Dependency.IsNone())
    {
        Step.RequiredSteps.Add(Dependency);
    }
    Steps.Add(Step);
}

void AFishingScenario::BeginPlay()
{
    Super::BeginPlay();
    InitializeConsole(GetWorld()->GetFirstPlayerController());
}

bool AFishingScenario::UpdateOperation(UWorkOperationComponent* Operation, int32 Total)
{
    if (Operation->State == EWorkOperationState::Completed)
    {
        return false;
    }
    Operation->AdvanceProgress(Total - Operation->CurrentCount);
    return true;
}

void AFishingScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ReadoutAccumulator += DeltaSeconds;
    if (ReadoutAccumulator >= 0.5f)
    {
        ReadoutAccumulator = 0;
        OnScenarioChanged.Broadcast();
    }
    if (UpdateOperation(SearchOperation, Sonar && Sonar->SelectedSchool))
    {
        return;
    }
    if (UpdateOperation(DeployOperation, bTrawlDeployed))
    {
        return;
    }
    if (UpdateOperation(CatchOperation, bFishCaught))
    {
        return;
    }
    if (UpdateOperation(RecoverOperation, bTrawlRecovered))
    {
        return;
    }
    if (UpdateOperation(UnloadOperation, UnloadedFish))
    {
        return;
    }
    if (UpdateOperation(SortOperation, SortedFish))
    {
        return;
    }
    if (UpdateOperation(PrepareKnifeOperation, bKnifePrepared))
    {
        return;
    }
    if (UpdateOperation(ProcessOperation, ProcessedFish))
    {
        return;
    }
    if (UpdateOperation(PackOperation, PackedFish))
    {
        return;
    }
    UpdateOperation(FreezeOperation, FrozenFish);
}

void AFishingScenario::SetStatus(const FText& Message)
{
    LastAction = Message;
    OnScenarioChanged.Broadcast();
}

FText AFishingScenario::GetConsoleReadout() const
{
    FString StateName = TEXT("На борту");
    if (Trawl)
    {
        switch (Trawl->State)
        {
            case ETrawlState::Deploying:
            {
                StateName = TEXT("Выпуск");
                break;
            }
            case ETrawlState::Towing:
            {
                StateName = TEXT("Буксировка");
                break;
            }
            case ETrawlState::Hauling:
            {
                StateName = TEXT("Подъём");
                break;
            }
            case ETrawlState::ReadyToUnload:
            {
                StateName = TEXT("Готов к выгрузке");
                break;
            }
            default:
            {
                break;
            }
        }
    }
    int32 CableMeters = 0;
    int32 CatchCount = 0;
    if (Trawl)
    {
        CableMeters = FMath::RoundToInt(Trawl->CableLength / 100);
        CatchCount = Trawl->GetCatchCount();
    }
    return FText::FromString(FString::Format(TEXT("{0}\nТрос: {1} м   Улов: {2}\nСортировка: {3}   Обработка: {4}\nУпаковано: {5}   Заморожено: {6}"), {
        StateName, CableMeters, CatchCount, SortedFish, ProcessedFish, PackedFish, FrozenFish }));
}
