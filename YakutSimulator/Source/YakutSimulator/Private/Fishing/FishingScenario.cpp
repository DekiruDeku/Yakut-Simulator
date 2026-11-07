#include "Fishing/FishingScenario.h"
#include "Fishing/FishingItem.h"
#include "Fishing/FishingStation.h"
#include "Fishing/FishingInteractionComponent.h"
#include "Fishing/FishingLog.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AFishingScenario::AFishingScenario()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    ConveyorPath = CreateDefaultSubobject<USplineComponent>(TEXT("ConveyorPath"));
    ConveyorPath->SetupAttachment(RootComponent);
    ConveyorPath->ClearSplinePoints(false);
    ConveyorPath->AddSplinePoint(FVector(0, -100, 95), ESplineCoordinateSpace::Local, false);
    ConveyorPath->AddSplinePoint(FVector(0, 100, 95), ESplineCoordinateSpace::Local, true);
    RecoveryPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RecoveryPoint"));
    RecoveryPoint->SetupAttachment(RootComponent);
    RecoveryPoint->SetRelativeLocation(FVector(-10, 0, 95));
    ProcessingPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProcessingPoint"));
    ProcessingPoint->SetupAttachment(RootComponent);
    ProcessingPoint->SetRelativeLocation(FVector(240, 0, 95));
    ProcessingSupplyPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProcessingSupplyPoint"));
    ProcessingSupplyPoint->SetupAttachment(RootComponent);
    ProcessingSupplyPoint->SetRelativeLocation(FVector(200, 0, 95));
    KnifeHome = CreateDefaultSubobject<USceneComponent>(TEXT("KnifeHome"));
    KnifeHome->SetupAttachment(RootComponent);
    KnifeHome->SetRelativeLocation(FVector(280, 0, 95));
    FinishedPoint = CreateDefaultSubobject<USceneComponent>(TEXT("FinishedPoint"));
    FinishedPoint->SetupAttachment(RootComponent);
    FinishedPoint->SetRelativeLocation(FVector(240, 60, 95));
    DeckPlayerPoint = CreateDefaultSubobject<USceneComponent>(TEXT("DeckPlayerPoint"));
    DeckPlayerPoint->SetupAttachment(RootComponent);
    DeckPlayerPoint->SetRelativeLocation(FVector(-700, -65, 300));
    DeckPlayerPoint->SetRelativeRotation(FRotator(0, 90, 0));
    SortingPlayerPoint = CreateDefaultSubobject<USceneComponent>(TEXT("SortingPlayerPoint"));
    SortingPlayerPoint->SetupAttachment(RootComponent);
    SortingPlayerPoint->SetRelativeLocation(FVector(-60, 0, 0));
    ProcessingPlayerPoint = CreateDefaultSubobject<USceneComponent>(TEXT("ProcessingPlayerPoint"));
    ProcessingPlayerPoint->SetupAttachment(RootComponent);
    ProcessingPlayerPoint->SetRelativeLocation(FVector(240, -65, 0));
    ProcessingPlayerPoint->SetRelativeRotation(FRotator(0, 90, 0));
    NetPosition = CreateDefaultSubobject<USceneComponent>(TEXT("NetPosition"));
    NetPosition->SetupAttachment(RootComponent);
    NetPosition->SetRelativeLocation(FVector(-700, 150, 400));
    CutGuide = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CutGuide"));
    CutGuide->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    CutGuide->SetStaticMesh(Sphere.Object);
    CutGuide->SetRelativeScale3D(FVector(.045));
    CutGuide->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CutGuide->SetVisibility(false);
    FishClass = AFishingItem::StaticClass();
    KnifeClass = AFishingItem::StaticClass();
}

void AFishingScenario::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogFishing, Display, TEXT("[Scenario.BeginPlay] world=%s scenario=%s fishCount=%d attachVRInput=%d"),
        *GetNameSafe(GetWorld()), *GetName(), FishCount, bAttachVRInput);
    RaisedNetLocation = NetPosition->GetRelativeLocation();
    RestartShift();
}

void AFishingScenario::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearAllTimersForObject(this);
    if (IsValid(VRInteraction)) VRInteraction->DestroyComponent();
    for (AFishingItem* Fish : Catch) if (IsValid(Fish)) Fish->Destroy();
    if (IsValid(Knife)) Knife->Destroy();
    Super::EndPlay(EndPlayReason);
}

void AFishingScenario::Feedback(const TCHAR* Message)
{
    LastFeedback = FText::FromString(Message);
    UE_LOG(LogFishing, Display, TEXT("[Scenario.Feedback] scenario=%s stage=%s message=\"%s\" equipped=%d stopTested=%d"),
        *GetName(), *UEnum::GetValueAsString(Stage), Message, bSafetyEquipped, bStopTested);
    OnFeedback.Broadcast(LastFeedback);
}

void AFishingScenario::SetStage(EFishingStage NewStage)
{
    UE_LOG(LogFishing, Display, TEXT("[Scenario.StageChanged] scenario=%s from=%s to=%s elapsed=%.2f sorted=%d processed=%d delivered=%d"),
        *GetName(), *UEnum::GetValueAsString(Stage), *UEnum::GetValueAsString(NewStage), Results.ElapsedSeconds,
        Results.SortedFish, Results.ProcessedFish, Results.DeliveredFish);
    Stage = NewStage;
    StageSeconds = 0;
    LastFeedback = FText::GetEmpty();
    bHasPreviousKnifeTip = false;
    LastCuttingReadiness.Empty();
    OverloadSeconds = 0;
    if (Stage != EFishingStage::Deploying && Stage != EFishingStage::Hauling) WinchInput = WinchLoad = 0;
    OnStageChanged.Broadcast(Stage);
}

void AFishingScenario::RestartShift()
{
    UE_LOG(LogFishing, Display, TEXT("[Scenario.Restart] scenario=%s previousStage=%s catchCount=%d"), *GetName(), *UEnum::GetValueAsString(Stage), Catch.Num());
    GetWorldTimerManager().ClearTimer(TravelTimer);
    if (IsValid(VRInteraction))
    {
        VRInteraction->ReleaseGrip(false);
        VRInteraction->ReleaseGrip(true);
    }
    for (AFishingItem* Fish : Catch) if (IsValid(Fish)) Fish->Destroy();
    Catch.Empty();
    if (IsValid(Knife)) Knife->Destroy();
    Knife = nullptr;
    BoardFish = nullptr;
    Results = FFishingResults();
    Results.TotalFish = FMath::Clamp(FishCount, 2, 32);
    const auto Positive = [](float Value, float Fallback, float Minimum) { return FMath::IsFinite(Value) ? FMath::Max(Minimum, Value) : Fallback; };
    DeploySeconds = Positive(DeploySeconds, 8, 1);
    TrawlSeconds = Positive(TrawlSeconds, 4, .1f);
    HaulSeconds = Positive(HaulSeconds, 20, 1);
    ConveyorSpeed = Positive(ConveyorSpeed, 12, 1);
    FeedInterval = Positive(FeedInterval, 3, .5f);
    CutTolerance = FMath::IsFinite(CutTolerance) ? FMath::Clamp(CutTolerance, 1.f, 8.f) : 3.f;
    bSafetyEquipped = bStopTested = bEmergencyStopped = bStoppedByOverload = false;
    WinchInput = NetDepth = WinchLoad = OverloadSeconds = FeedSeconds = 0;
    NetPosition->SetRelativeLocation(RaisedNetLocation);
    NextFishToFeed = 0;
    CutGuide->SetVisibility(false);
    SetStage(EFishingStage::Safety);
    MovePlayer(DeckPlayerPoint);
}

bool AFishingScenario::EquipSafety()
{
    if (Stage != EFishingStage::Safety)
    {
        UE_LOG(LogFishing, Log, TEXT("[Safety.EquipRejected] scenario=%s reason=WrongStage expected=Safety actual=%s"), *GetName(), *UEnum::GetValueAsString(Stage));
        return false;
    }
    UE_LOG(LogFishing, Display, TEXT("[Safety.Equipped] scenario=%s previous=%d current=1 next=EmergencyStop"), *GetName(), bSafetyEquipped);
    bSafetyEquipped = true;
    Feedback(TEXT("Защита надета. Теперь проверьте аварийную остановку."));
    return true;
}

void AFishingScenario::EmergencyStop()
{
    StopWinch(false);
}

void AFishingScenario::StopWinch(bool bOverload)
{
    if (Stage != EFishingStage::Safety && Stage != EFishingStage::Deploying && Stage != EFishingStage::Hauling)
    {
        UE_LOG(LogFishing, Log, TEXT("[Safety.StopRejected] reason=WrongStage stage=%s"), *UEnum::GetValueAsString(Stage));
        return;
    }
    bEmergencyStopped = true;
    bStoppedByOverload = bOverload;
    WinchInput = WinchLoad = OverloadSeconds = 0;
    if (IsValid(VRInteraction))
    {
        VRInteraction->ReleaseGrip(false);
        VRInteraction->ReleaseGrip(true);
    }
    if (Stage == EFishingStage::Safety && bSafetyEquipped) bStopTested = true;
    UE_LOG(LogFishing, Display, TEXT("[Safety.EmergencyStop] cause=%s stage=%s equipped=%d stopTested=%d overloads=%d"),
        bOverload ? TEXT("Overload") : TEXT("StopButton"), *UEnum::GetValueAsString(Stage), bSafetyEquipped, bStopTested, Results.Overloads);
    Feedback(bOverload ? TEXT("ПЕРЕГРУЗКА. Отпустите Grip, нажмите СБРОС ОСТАНОВКИ и используйте меньший ход рычага.")
        : Stage == EFishingStage::Safety ? (bSafetyEquipped ? TEXT("Кнопка СТОП проверена. Нажмите НАЧАТЬ ЛОВЛЮ.") : TEXT("Сначала нажмите НАДЕТЬ ЗАЩИТУ, затем снова проверьте СТОП."))
        : TEXT("Нажата кнопка СТОП. Отпустите Grip и нажмите СБРОС ОСТАНОВКИ."));
}

bool AFishingScenario::StartOrResume()
{
    UE_LOG(LogFishing, Log, TEXT("[Scenario.StartRequested] stage=%s equipped=%d stopTested=%d stopped=%d"),
        *UEnum::GetValueAsString(Stage), bSafetyEquipped, bStopTested, bEmergencyStopped);
    if (Stage == EFishingStage::Safety)
    {
        if (!bSafetyEquipped || !bStopTested)
        {
            UE_LOG(LogFishing, Log, TEXT("[Scenario.StartRejected] reason=%s"), !bSafetyEquipped ? TEXT("SafetyNotEquipped") : TEXT("EmergencyStopNotTested"));
            Feedback(!bSafetyEquipped ? TEXT("Сначала нажмите НАДЕТЬ ЗАЩИТУ.") : TEXT("Сначала нажмите СТОП для проверки безопасности."));
            return false;
        }
        bEmergencyStopped = false;
        bStoppedByOverload = false;
        WinchInput = WinchLoad = OverloadSeconds = 0;
        SetStage(EFishingStage::Deploying);
        return true;
    }
    if ((Stage == EFishingStage::Deploying || Stage == EFishingStage::Hauling) && bEmergencyStopped)
    {
        bEmergencyStopped = false;
        bStoppedByOverload = false;
        OverloadSeconds = 0;
        WinchInput = WinchLoad = 0;
        Feedback(TEXT("Лебёдка готова. Зажмите Grip у рукоятки и плавно сдвиньте руку вперёд."));
        return true;
    }
    if (Stage == EFishingStage::Deploying || Stage == EFishingStage::Hauling)
    {
        Feedback(TEXT("Лебёдка уже готова. Управляйте рычагом; повторный пуск не требуется."));
        return true;
    }
    UE_LOG(LogFishing, Log, TEXT("[Scenario.StartRejected] reason=WrongStageOrAlreadyRunning stage=%s"), *UEnum::GetValueAsString(Stage));
    return false;
}

void AFishingScenario::SetWinchInput(float Value)
{
    const float Previous = WinchInput;
    WinchInput = FMath::IsFinite(Value) && !bEmergencyStopped &&
        (Stage == EFishingStage::Deploying || Stage == EFishingStage::Hauling)
        ? FMath::Clamp(Value, 0.f, 1.f) : 0.f;
    if (FMath::RoundToInt(Previous * 10) != FMath::RoundToInt(WinchInput * 10))
        UE_LOG(LogFishing, Log, TEXT("[Winch.InputChanged] requested=%.3f previous=%.3f actual=%.3f stage=%s stopped=%d load=%.3f depth=%.3f"),
            Value, Previous, WinchInput, *UEnum::GetValueAsString(Stage), bEmergencyStopped, WinchLoad, NetDepth);
}

void AFishingScenario::UpdateWinch(float DeltaSeconds)
{
    if (Stage == EFishingStage::Trawling)
    {
        if (StageSeconds >= TrawlSeconds) SetStage(EFishingStage::Hauling);
        return;
    }
    if (Stage != EFishingStage::Deploying && Stage != EFishingStage::Hauling) return;
    // shortcut: условная нагрузка, заменить моделью оборудования при переходе к тренажёру.
    const float Resistance = .15f + .15f * (1.f + FMath::Sin(StageSeconds * 1.7f));
    WinchLoad = bEmergencyStopped ? 0.f : FMath::Clamp(Resistance + .6f * WinchInput, 0.f, 1.f);
    OverloadSeconds = WinchLoad > .9f ? OverloadSeconds + DeltaSeconds : 0.f;
    if (OverloadSeconds >= OverloadStopDelay)
    {
        UE_LOG(LogFishing, Display, TEXT("[Winch.Overload] load=%.3f duration=%.3f input=%.3f"), WinchLoad, OverloadSeconds, WinchInput);
        ++Results.Overloads;
        StopWinch(true);
        return;
    }
    if (bEmergencyStopped) return;
    if (Stage == EFishingStage::Deploying)
    {
        NetDepth = FMath::Min(1.f, NetDepth + WinchInput * DeltaSeconds / DeploySeconds);
        if (NetDepth >= 1.f)
        {
            WinchInput = 0;
            SetStage(EFishingStage::Trawling);
        }
    }
    else
    {
        NetDepth = FMath::Max(0.f, NetDepth - WinchInput * DeltaSeconds / HaulSeconds);
        if (NetDepth <= 0.f)
        {
            WinchInput = 0;
            SetStage(EFishingStage::AwaitSorting);
        }
    }
    NetPosition->SetRelativeLocation(RaisedNetLocation - FVector(0, 0, 500.f * NetDepth));
}

bool AFishingScenario::BeginSorting()
{
    if (Stage != EFishingStage::AwaitSorting)
    {
        UE_LOG(LogFishing, Log, TEXT("[Sorting.BeginRejected] reason=WrongStage expected=AwaitSorting actual=%s"), *UEnum::GetValueAsString(Stage));
        return false;
    }
    if (!FishClass || ConveyorPath->GetSplineLength() < 1.f) { Feedback(TEXT("Ошибка настройки: задайте класс рыбы и путь ленты.")); return false; }
    for (int32 Index = 0; Index < Results.TotalFish; ++Index)
    {
        AFishingItem* Fish = GetWorld()->SpawnActorDeferred<AFishingItem>(FishClass, GetActorTransform(), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (!Fish)
        {
            for (AFishingItem* Created : Catch) if (IsValid(Created)) Created->Destroy();
            Catch.Empty();
            Feedback(TEXT("Не удалось создать улов. Проверьте класс рыбы и повторите переход."));
            return false;
        }
        Fish->Scenario = this;
        Fish->Kind = EFishingItemKind::Fish;
        Fish->Species = Index % 2 == 0 ? EFishSpecies::A : EFishSpecies::B;
        Fish->BatchIndex = Index;
        Fish->FinishSpawning(GetActorTransform());
        Fish->Park(GetActorTransform(), EFishingItemState::Queued);
        Catch.Add(Fish);
    }
    FeedSeconds = FeedInterval;
    SetStage(EFishingStage::Sorting);
    MovePlayer(SortingPlayerPoint);
    return true;
}

void AFishingScenario::UpdateConveyor(float DeltaSeconds)
{
    FeedSeconds += DeltaSeconds;
    if (NextFishToFeed < Catch.Num() && FeedSeconds >= FeedInterval)
    {
        AFishingItem* Fish = Catch[NextFishToFeed++];
        Fish->ConveyorDistance = 0;
        UE_LOG(LogFishing, Log, TEXT("[Conveyor.Feed] item=%s batch=%d species=%s remaining=%d"), *GetNameSafe(Fish), Fish->BatchIndex, *UEnum::GetValueAsString(Fish->Species), Catch.Num() - NextFishToFeed);
        Fish->Park(ConveyorPath->GetTransformAtDistanceAlongSpline(0, ESplineCoordinateSpace::World), EFishingItemState::Conveyor);
        FeedSeconds = 0;
    }
    for (AFishingItem* Fish : Catch)
    {
        if (!IsValid(Fish) || Fish->IsHeld() || Fish->State != EFishingItemState::Conveyor) continue;
        Fish->ConveyorDistance += ConveyorSpeed * DeltaSeconds;
        if (Fish->ConveyorDistance >= ConveyorPath->GetSplineLength()) RecoverItem(Fish, true);
        else Fish->SetActorTransform(ConveyorPath->GetTransformAtDistanceAlongSpline(Fish->ConveyorDistance, ESplineCoordinateSpace::World));
    }
}

bool AFishingScenario::OwnsFish(const AFishingItem* Fish) const
{
    return IsValid(Fish) && Fish->Scenario == this && Fish->Kind == EFishingItemKind::Fish && Catch.Contains(Fish);
}

bool AFishingScenario::TrySortFish(AFishingItem* Fish, EFishSpecies BinSpecies)
{
    if (Stage != EFishingStage::Sorting || !OwnsFish(Fish) || Fish->IsHeld() || Fish->bSorted ||
        (Fish->State != EFishingItemState::Conveyor && Fish->State != EFishingItemState::Loose))
    {
        UE_LOG(LogFishing, Log, TEXT("[Sorting.Rejected] item=%s reason=%s stage=%s"), *GetNameSafe(Fish), Stage != EFishingStage::Sorting ? TEXT("WrongStage") :
            !OwnsFish(Fish) ? TEXT("ForeignOrMissingFish") : Fish->IsHeld() ? TEXT("FishStillHeld") : Fish->bSorted ? TEXT("AlreadySorted") : TEXT("FishNotFed"), *UEnum::GetValueAsString(Stage));
        return false;
    }
    if (Fish->Species != BinSpecies)
    {
        UE_LOG(LogFishing, Log, TEXT("[Sorting.WrongBin] item=%s species=%s bin=%s alreadyRecorded=%d"), *Fish->GetName(), *UEnum::GetValueAsString(Fish->Species), *UEnum::GetValueAsString(BinSpecies), Fish->bSortingMistakeRecorded);
        if (!Fish->bSortingMistakeRecorded) { ++Results.SortingMistakes; Fish->bSortingMistakeRecorded = true; }
        RecoverItem(Fish);
        Feedback(TEXT("Другой вид рыбы. Улов возвращён в лоток пересортировки."));
        return false;
    }
    Fish->bSorted = true;
    Fish->Park(Fish->GetActorTransform(), EFishingItemState::Sorted);
    ++Results.SortedFish;
    UE_LOG(LogFishing, Display, TEXT("[Sorting.Accepted] item=%s sorted=%d total=%d"), *Fish->GetName(), Results.SortedFish, Results.TotalFish);
    if (Results.SortedFish == Results.TotalFish) SetStage(EFishingStage::AwaitProcessing);
    else Feedback(TEXT("Верно. Следующая рыба."));
    return true;
}

bool AFishingScenario::BeginProcessing()
{
    if (Stage != EFishingStage::AwaitProcessing)
    {
        UE_LOG(LogFishing, Log, TEXT("[Processing.BeginRejected] reason=WrongStage expected=AwaitProcessing actual=%s"), *UEnum::GetValueAsString(Stage));
        return false;
    }
    if (!KnifeClass) { Feedback(TEXT("Ошибка настройки: задайте класс ножа.")); return false; }
    Knife = GetWorld()->SpawnActorDeferred<AFishingItem>(KnifeClass, KnifeHome->GetComponentTransform(), this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Knife) { Feedback(TEXT("Не удалось создать нож. Повторите переход.")); return false; }
    Knife->Scenario = this;
    Knife->Kind = EFishingItemKind::Knife;
    Knife->FinishSpawning(KnifeHome->GetComponentTransform());
    Knife->Park(KnifeHome->GetComponentTransform(), EFishingItemState::Loose);
    SetStage(EFishingStage::Processing);
    PresentNextFish();
    MovePlayer(ProcessingPlayerPoint);
    return true;
}

void AFishingScenario::PresentNextFish()
{
    for (AFishingItem* Fish : Catch)
    {
        if (IsValid(Fish) && Fish->State == EFishingItemState::Sorted)
        {
            Fish->Park(ProcessingSupplyPoint->GetComponentTransform(), EFishingItemState::Loose);
            return;
        }
    }
}

bool AFishingScenario::TryLoadFish(AFishingItem* Fish)
{
    if (Stage != EFishingStage::Processing || IsValid(BoardFish) || !OwnsFish(Fish) || !Fish->bSorted ||
        Fish->IsHeld() || Fish->State != EFishingItemState::Loose)
    {
        UE_LOG(LogFishing, Log, TEXT("[Processing.LoadRejected] item=%s stage=%s boardFish=%s reason=%s"), *GetNameSafe(Fish), *UEnum::GetValueAsString(Stage), *GetNameSafe(BoardFish),
            Stage != EFishingStage::Processing ? TEXT("WrongStage") : IsValid(BoardFish) ? TEXT("BoardOccupied") : !OwnsFish(Fish) ? TEXT("ForeignOrMissingFish") :
            !Fish->bSorted ? TEXT("NotSorted") : Fish->IsHeld() ? TEXT("FishStillHeld") : TEXT("WrongItemState"));
        return false;
    }
    BoardFish = Fish;
    Fish->NextCutPoint = 0;
    Fish->Park(ProcessingPoint->GetComponentTransform(), EFishingItemState::OnBoard);
    bHasPreviousKnifeTip = false;
    Feedback(TEXT("Придерживайте рыбу одной рукой. Другой ведите кончик ножа по меткам."));
    return true;
}

bool AFishingScenario::TryDeliverFish(AFishingItem* Fish)
{
    if (Stage != EFishingStage::Processing || !OwnsFish(Fish) || Fish != BoardFish || Fish->IsHeld() || Fish->State != EFishingItemState::Processed)
    {
        UE_LOG(LogFishing, Log, TEXT("[Delivery.Rejected] item=%s reason=%s stage=%s"), *GetNameSafe(Fish), Stage != EFishingStage::Processing ? TEXT("WrongStage") :
            !OwnsFish(Fish) ? TEXT("ForeignOrMissingFish") : Fish != BoardFish ? TEXT("NotBoardFish") : Fish->IsHeld() ? TEXT("FishStillHeld") : TEXT("NotProcessed"), *UEnum::GetValueAsString(Stage));
        return false;
    }
    Fish->Park(FinishedPoint->GetComponentTransform(), EFishingItemState::Delivered);
    BoardFish = nullptr;
    ++Results.DeliveredFish;
    UE_LOG(LogFishing, Display, TEXT("[Delivery.Accepted] item=%s delivered=%d total=%d"), *Fish->GetName(), Results.DeliveredFish, Results.TotalFish);
    if (Results.DeliveredFish == Results.TotalFish)
    {
        if (IsValid(Knife)) Knife->Park(KnifeHome->GetComponentTransform(), EFishingItemState::Loose);
        SetStage(EFishingStage::Complete);
    }
    else
    {
        PresentNextFish();
        Feedback(TEXT("Готовая рыба принята. Возьмите следующую из подающего лотка."));
    }
    return true;
}

void AFishingScenario::RecoverItem(AFishingItem* Item, bool bMissed)
{
    if (!IsValid(Item) || Item->Scenario != this || Item->IsHeld())
    {
        UE_LOG(LogFishing, Verbose, TEXT("[Recovery.Rejected] item=%s reason=%s"), *GetNameSafe(Item), !IsValid(Item) ? TEXT("MissingItem") : Item->Scenario != this ? TEXT("DifferentScenario") : TEXT("ItemStillHeld"));
        return;
    }
    UE_LOG(LogFishing, Log, TEXT("[Recovery.Requested] item=%s state=%s missed=%d stage=%s"), *Item->GetName(), *UEnum::GetValueAsString(Item->State), bMissed, *UEnum::GetValueAsString(Stage));
    if (Item == Knife)
    {
        Item->Park(KnifeHome->GetComponentTransform(), EFishingItemState::Loose);
        return;
    }
    if (!OwnsFish(Item) || Item->State == EFishingItemState::Delivered || Item->State == EFishingItemState::Sorted || Item->State == EFishingItemState::Queued) return;
    if (Stage == EFishingStage::Sorting)
    {
        if (bMissed && !Item->bMissRecorded) { ++Results.MissedFish; Item->bMissRecorded = true; }
        FTransform Position = RecoveryPoint->GetComponentTransform();
        Position.AddToTranslation(RecoveryPoint->GetComponentRotation().RotateVector(FVector((Item->BatchIndex % 2) * 22.f, (Item->BatchIndex / 2) * 14.f, 0)));
        Item->Park(Position, EFishingItemState::Loose);
    }
    else if (Stage == EFishingStage::Processing)
        Item->Park(Item == BoardFish ? ProcessingPoint->GetComponentTransform() : ProcessingSupplyPoint->GetComponentTransform(),
            Item->State == EFishingItemState::Processed ? EFishingItemState::Processed : EFishingItemState::Loose);
}

bool AFishingScenario::TryPlaceReleasedItem(AFishingItem* Item)
{
    if (!IsValid(Item) || Item->Scenario != this || Item->Kind != EFishingItemKind::Fish) return false;
    for (TActorIterator<AFishingStation> It(GetWorld()); It; ++It)
        if (It->Scenario == this && It->ContainsItem(Item) && It->AcceptItem(Item)) return true;
    return false;
}

void AFishingScenario::AdvanceKnifeStroke(const FVector& From, const FVector& To)
{
    if (Stage != EFishingStage::Processing || !IsValid(BoardFish) || BoardFish->State != EFishingItemState::OnBoard ||
        !BoardFish->IsBraced() || !IsValid(Knife) || !Knife->IsHeld() || Knife->GetHoldingHand() == BoardFish->GetHoldingHand() ||
        From.ContainsNaN() || To.ContainsNaN() || FVector::DistSquared(From, To) > FMath::Square(40.f)) return;
    const TArray<FVector> Points = BoardFish->GetCutPoints();
    float PreviousT = -1.f;
    const FVector Segment = To - From;
    while (BoardFish->NextCutPoint < Points.Num())
    {
        const FVector Target = BoardFish->GetActorTransform().TransformPosition(Points[BoardFish->NextCutPoint]);
        const float T = Segment.IsNearlyZero() ? 0.f : FMath::Clamp(FVector::DotProduct(Target - From, Segment) / Segment.SizeSquared(), 0.f, 1.f);
        if (T < PreviousT || FVector::DistSquared(From + T * Segment, Target) > FMath::Square(CutTolerance)) break;
        PreviousT = T;
        ++BoardFish->NextCutPoint;
        UE_LOG(LogFishing, Log, TEXT("[Cut.PointReached] item=%s progress=%d/%d tip=%s target=%s"), *BoardFish->GetName(), BoardFish->NextCutPoint, Points.Num(), *To.ToCompactString(), *Target.ToCompactString());
    }
    if (BoardFish->NextCutPoint == Points.Num())
    {
        BoardFish->State = EFishingItemState::Processed;
        BoardFish->RefreshAppearance();
        ++Results.ProcessedFish;
        CutGuide->SetVisibility(false);
        Feedback(TEXT("Обработка завершена. Отпустите захват, возьмите рыбу и положите в готовый улов."));
    }
}

void AFishingScenario::UpdateCutting()
{
    const bool bCanCut = IsValid(BoardFish) && BoardFish->State == EFishingItemState::OnBoard;
    CutGuide->SetVisibility(bCanCut);
    if (bCanCut)
    {
        const TArray<FVector> Points = BoardFish->GetCutPoints();
        if (Points.IsValidIndex(BoardFish->NextCutPoint)) CutGuide->SetWorldLocation(BoardFish->GetActorTransform().TransformPosition(Points[BoardFish->NextCutPoint]));
    }
    const FString Readiness = !bCanCut ? TEXT("NoUnprocessedFishOnBoard") : !IsValid(Knife) ? TEXT("MissingKnife") : !Knife->IsHeld() ? TEXT("KnifeNotHeld") :
        !BoardFish->IsBraced() ? TEXT("FishNotBraced") : Knife->GetHoldingHand() == BoardFish->GetHoldingHand() ? TEXT("SameHandForKnifeAndFish") : TEXT("Ready");
    if (Readiness != LastCuttingReadiness)
    {
        UE_LOG(LogFishing, Log, TEXT("[Cut.ReadinessChanged] reason=%s boardFish=%s knife=%s"), *Readiness, *GetNameSafe(BoardFish), *GetNameSafe(Knife));
        LastCuttingReadiness = Readiness;
    }
    if (!bCanCut || !IsValid(Knife) || !Knife->IsHeld() || !BoardFish->IsBraced())
    {
        bHasPreviousKnifeTip = false;
        return;
    }
    const FVector Tip = Knife->BladeTip->GetComponentLocation();
    AdvanceKnifeStroke(bHasPreviousKnifeTip ? PreviousKnifeTip : Tip, Tip);
    PreviousKnifeTip = Tip;
    bHasPreviousKnifeTip = true;
}

void AFishingScenario::MovePlayer(USceneComponent* Destination)
{
    APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!IsValid(Pawn) || !IsValid(Destination))
    {
        UE_LOG(LogFishing, Log, TEXT("[Travel.Unavailable] pawn=%s destination=%s reason=%s"), *GetNameSafe(Pawn), *GetNameSafe(Destination), !IsValid(Pawn) ? TEXT("MissingPlayer") : TEXT("MissingDestination"));
        return;
    }
    UE_LOG(LogFishing, Log, TEXT("[Travel.Requested] pawn=%s destination=%s location=%s"), *GetNameSafe(Pawn), *GetNameSafe(Destination), *Destination->GetComponentLocation().ToCompactString());
    APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
    if (PC && PC->PlayerCameraManager) PC->PlayerCameraManager->StartCameraFade(0, 1, .12f, FLinearColor::Black, false, true);
    const FTransform Target = Destination->GetComponentTransform();
    const TWeakObjectPtr<APawn> WeakPawn(Pawn);
    GetWorldTimerManager().SetTimer(TravelTimer, FTimerDelegate::CreateWeakLambda(this, [WeakPawn, Target]()
    {
        APawn* Player = WeakPawn.Get();
        if (!Player) return;
        UCameraComponent* Camera = Player->FindComponentByClass<UCameraComponent>();
        const float ViewYaw = Camera ? Camera->GetComponentRotation().Yaw : Player->GetActorRotation().Yaw;
        Player->AddActorWorldRotation(FRotator(0, FMath::FindDeltaAngleDegrees(ViewYaw, Target.Rotator().Yaw), 0));
        FVector Offset = Camera ? Camera->GetComponentLocation() - Player->GetActorLocation() : FVector::ZeroVector;
        Offset.Z = 0;
        Player->SetActorLocation(Target.GetLocation() - Offset, false, nullptr, ETeleportType::TeleportPhysics);
        UE_LOG(LogFishing, Log, TEXT("[Travel.Completed] pawn=%s origin=%s headOffset=%s"), *Player->GetName(), *Player->GetActorLocation().ToCompactString(), *Offset.ToCompactString());
        if (APlayerController* Controller = Cast<APlayerController>(Player->GetController()))
            if (Controller->PlayerCameraManager) Controller->PlayerCameraManager->StartCameraFade(1, 0, .15f, FLinearColor::Black, false, false);
    }), .12f, false);
}

void AFishingScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0) return;
    if (bAttachVRInput && !IsValid(VRInteraction))
    {
        if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            VRInteraction = NewObject<UFishingInteractionComponent>(Pawn);
            VRInteraction->Scenario = this;
            Pawn->AddInstanceComponent(VRInteraction);
            VRInteraction->RegisterComponent();
            UE_LOG(LogFishing, Log, TEXT("[Scenario.InputAttached] scenario=%s pawn=%s component=%s"), *GetName(), *Pawn->GetName(), *VRInteraction->GetName());
        }
    }
    if (Stage != EFishingStage::Complete) Results.ElapsedSeconds += DeltaSeconds;
    StageSeconds += DeltaSeconds;
    UpdateWinch(DeltaSeconds);
    if (Stage == EFishingStage::Sorting) UpdateConveyor(DeltaSeconds);
    if (Stage == EFishingStage::Processing) UpdateCutting();
}

FText AFishingScenario::GetStatusText() const
{
    if (Stage == EFishingStage::Safety)
    {
        const TCHAR* Next = !bSafetyEquipped ? TEXT("НАДЕТЬ ЗАЩИТУ") : !bStopTested ? TEXT("СТОП — проверка безопасности") : TEXT("НАЧАТЬ ЛОВЛЮ");
        return FText::FromString(FString::Printf(TEXT("ШАГ 1/6 · ПОДГОТОВКА\n1. Защита: %s\n2. Проверка СТОП: %s\nСЕЙЧАС: %s\nПоднесите контроллер к самой плашке.\nНажмите Grip или Trigger один раз."),
            bSafetyEquipped ? TEXT("ГОТОВО") : TEXT("не надета"), bStopTested ? TEXT("ГОТОВО") : TEXT("не выполнена"), Next));
    }
    if (Stage == EFishingStage::Deploying || Stage == EFishingStage::Hauling)
        return FText::FromString(FString::Printf(TEXT("ШАГ %d/6\n%s\n%s: %.0f%%"), Stage == EFishingStage::Deploying ? 2 : 3,
            *GetWinchStatusText().ToString(), Stage == EFishingStage::Deploying ? TEXT("Трал выпущен") : TEXT("Трал поднят"),
            (Stage == EFishingStage::Deploying ? NetDepth : 1.f - NetDepth) * 100));
    if (Stage == EFishingStage::Trawling)
        return FText::FromString(FString::Printf(TEXT("ШАГ 2/6 · ТРАЛ В ВОДЕ\nОтпустите Grip. Траление: ещё %.0f с.\nПосле команды ПОДЪЁМ снова возьмите рычаг."), FMath::Max(0.f, TrawlSeconds - StageSeconds)));
    FString Objective;
    switch (Stage)
    {
    case EFishingStage::AwaitSorting: Objective = TEXT("ШАГ 3/6 · УЛОВ ПОДНЯТ\nСЕЙЧАС: нажмите В ЦЕХ."); break;
    case EFishingStage::Sorting: Objective = TEXT("ШАГ 4/6 · СОРТИРОВКА\nРыба A → лоток A; рыба B → лоток B."); break;
    case EFishingStage::AwaitProcessing: Objective = TEXT("ШАГ 4/6 · СОРТИРОВКА ГОТОВА\nСЕЙЧАС: нажмите К ОБРАБОТКЕ."); break;
    case EFishingStage::Processing: Objective = TEXT("ШАГ 5/6 · ОБРАБОТКА\nРыба на стол → держите второй рукой → нож по меткам → готовый улов."); break;
    case EFishingStage::Complete: Objective = TEXT("ШАГ 6/6 · СМЕНА ЗАВЕРШЕНА\nНажмите НОВАЯ СМЕНА для повтора."); break;
    default: break;
    }
    return FText::FromString(FString::Printf(TEXT("%s\n%s\nТрал: %.0f%%   Нагрузка: %.0f%%%s\nСортировка: %d/%d   Обработка: %d/%d   Сдано: %d/%d\nОшибки сортировки: %d   Пропуски: %d   Перегрузки: %d\nВремя: %.0f с"),
        *Objective, *LastFeedback.ToString(), NetDepth * 100, WinchLoad * 100,
        bEmergencyStopped ? TEXT(" / СТОП") : TEXT(""), Results.SortedFish, Results.TotalFish,
        Results.ProcessedFish, Results.TotalFish, Results.DeliveredFish, Results.TotalFish,
        Results.SortingMistakes, Results.MissedFish, Results.Overloads, Results.ElapsedSeconds));
}

FText AFishingScenario::GetWinchStatusText() const
{
    if (Stage == EFishingStage::Safety) return FText::FromString(TEXT("ЗАБЛОКИРОВАНА\nВыполните подготовку на табло."));
    if (Stage == EFishingStage::Trawling) return FText::FromString(TEXT("ТРАЛ В ВОДЕ\nОтпустите Grip. Ждите команды ПОДЪЁМ."));
    if (Stage != EFishingStage::Deploying && Stage != EFishingStage::Hauling)
        return FText::FromString(TEXT("УЛОВ ПОДНЯТ\nНажмите В ЦЕХ."));
    if (bEmergencyStopped)
        return FText::FromString(FString::Printf(TEXT("СТОП: %s\nСкорость: 0%%\nОтпустите Grip → СБРОС ОСТАНОВКИ."),
            bStoppedByOverload ? TEXT("ПЕРЕГРУЗКА") : TEXT("НАЖАТА КНОПКА СТОП")));
    const int32 Bars = FMath::Clamp(FMath::RoundToInt(WinchInput * 10), 0, 10);
    const FString Gauge = FString::ChrN(Bars, TEXT('=')) + FString::ChrN(10 - Bars, TEXT('.'));
    const FString Hint = WinchLoad > .9f
        ? FString::Printf(TEXT("РЫЧАГ НАЗАД! До остановки: %.1f с"), FMath::Max(0.f, OverloadStopDelay - OverloadSeconds))
        : WinchLoad > .8f ? TEXT("Нагрузка растёт — сдвиньте рычаг назад.")
        : TEXT("Плавно вперёд → быстрее; назад → медленнее.\nОтпустите Grip — пауза.");
    return FText::FromString(FString::Printf(TEXT("%s\nСкорость: %.0f%% [%s]\nНагрузка: %.0f%%\n%s"),
        Stage == EFishingStage::Deploying ? TEXT("ВЫПУСК ТРАЛА") : TEXT("ПОДЪЁМ ТРАЛА"), WinchInput * 100, *Gauge, WinchLoad * 100, *Hint));
}
