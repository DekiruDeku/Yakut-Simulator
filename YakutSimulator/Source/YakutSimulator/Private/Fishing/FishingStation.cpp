#include "Fishing/FishingStation.h"
#include "Fishing/FishingScenario.h"
#include "Fishing/FishingItem.h"
#include "Fishing/FishingLog.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AFishingStation::AFishingStation()
{
    PrimaryActorTick.bCanEverTick = true;
    Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
    SetRootComponent(Zone);
    Zone->SetBoxExtent(FVector(20, 20, 15));
    Zone->SetMobility(EComponentMobility::Movable);
    Zone->SetCollisionObjectType(ECC_WorldDynamic);
    Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Zone->SetCollisionResponseToAllChannels(ECR_Overlap);
    Zone->SetGenerateOverlapEvents(true);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Zone);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Mesh->SetStaticMesh(Cube.Object);
    Mesh->SetRelativeScale3D(FVector(.35, .35, .04));
    Mesh->SetRelativeLocation(FVector(0, 0, -15));
    Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
    Handle->SetupAttachment(Zone);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Handle->SetStaticMesh(Sphere.Object);
    Handle->SetRelativeScale3D(FVector(.10));
    Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Zone);
    Label->SetRelativeLocation(FVector(0, 0, 20));
    Label->SetWorldSize(6);
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetTextRenderColor(FColor::White);
    static ConstructorHelpers::FObjectFinder<UFont> TextFont(TEXT("/Game/Fishing/UI/F_FishingCyrillic.F_FishingCyrillic"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TextMaterial(TEXT("/Game/Fishing/UI/MI_FishingText.MI_FishingText"));
    if (TextFont.Succeeded()) Label->SetFont(TextFont.Object);
    if (TextMaterial.Succeeded()) Label->SetTextMaterial(TextMaterial.Object);
}

void AFishingStation::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Handle->SetVisibility(Kind == EFishingStationKind::WinchLever);
    Mesh->SetVisibility(Kind != EFishingStationKind::StatusBoard);
    UpdateLabel();
}

void AFishingStation::BeginPlay()
{
    Super::BeginPlay();
    if (!Scenario)
    {
        for (TActorIterator<AFishingScenario> It(GetWorld()); It; ++It)
        {
            if (Scenario) { Scenario = nullptr; break; }
            Scenario = *It;
        }
    }
    Zone->OnComponentBeginOverlap.AddDynamic(this, &AFishingStation::OnOverlap);
    UpdateLabel();
    if (Kind == EFishingStationKind::StatusBoard || IsButton()) PrimaryActorTick.TickInterval = .1f;
    UE_LOG(LogFishing, Log, TEXT("[Station.Ready] station=%s kind=%s scenario=%s position=%s zoneExtent=%s collision=%d button=%d"),
        *GetName(), *UEnum::GetValueAsString(Kind), *GetNameSafe(Scenario), *GetActorLocation().ToCompactString(),
        *Zone->GetScaledBoxExtent().ToCompactString(), static_cast<int32>(Zone->GetCollisionEnabled()), IsButton());
}

bool AFishingStation::IsButton() const
{
    return Kind == EFishingStationKind::SafetyEquipment || Kind == EFishingStationKind::EmergencyStop ||
        Kind == EFishingStationKind::StartOrResume || Kind == EFishingStationKind::GoToSorting ||
        Kind == EFishingStationKind::GoToProcessing || Kind == EFishingStationKind::Restart;
}

void AFishingStation::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndLever(LeverHand.Get());
    Super::EndPlay(EndPlayReason);
}

bool AFishingStation::Use()
{
    UE_LOG(LogFishing, Log, TEXT("[Station.Use] station=%s kind=%s scenario=%s stage=%s"), *GetName(), *UEnum::GetValueAsString(Kind),
        *GetNameSafe(Scenario), IsValid(Scenario) ? *UEnum::GetValueAsString(Scenario->Stage) : TEXT("None"));
    if (!IsValid(Scenario))
    {
        UE_LOG(LogFishing, Log, TEXT("[Station.Result] station=%s result=Rejected reason=MissingScenario"), *GetName());
        return false;
    }
    bool bAccepted = false;
    switch (Kind)
    {
    case EFishingStationKind::SafetyEquipment: bAccepted = Scenario->EquipSafety(); break;
    case EFishingStationKind::EmergencyStop:
        bAccepted = Scenario->Stage == EFishingStage::Safety || Scenario->Stage == EFishingStage::Deploying || Scenario->Stage == EFishingStage::Hauling;
        Scenario->EmergencyStop();
        break;
    case EFishingStationKind::StartOrResume: bAccepted = Scenario->StartOrResume(); break;
    case EFishingStationKind::GoToSorting: bAccepted = Scenario->BeginSorting(); break;
    case EFishingStationKind::GoToProcessing: bAccepted = Scenario->BeginProcessing(); break;
    case EFishingStationKind::Restart:
        if (Scenario->Stage == EFishingStage::Complete) { Scenario->RestartShift(); bAccepted = true; }
        break;
    default: break;
    }
    UpdateLabel();
    UE_LOG(LogFishing, Log, TEXT("[Station.Result] station=%s kind=%s result=%s stage=%s equipped=%d stopTested=%d feedback=\"%s\""),
        *GetName(), *UEnum::GetValueAsString(Kind), bAccepted ? TEXT("Accepted") : TEXT("Rejected"), *UEnum::GetValueAsString(Scenario->Stage),
        Scenario->bSafetyEquipped, Scenario->bStopTested, *Scenario->LastFeedback.ToString());
    return bAccepted;
}

bool AFishingStation::BeginLever(USceneComponent* Hand)
{
    if (Kind != EFishingStationKind::WinchLever || !IsValid(Scenario) || !IsValid(Hand) || !LeverHand.IsExplicitlyNull() ||
        Scenario->bEmergencyStopped || (Scenario->Stage != EFishingStage::Deploying && Scenario->Stage != EFishingStage::Hauling))
    {
        UE_LOG(LogFishing, Log, TEXT("[Lever.Rejected] station=%s hand=%s kind=%s scenario=%s holdingHand=%s stopped=%d stage=%s"),
            *GetName(), *GetNameSafe(Hand), *UEnum::GetValueAsString(Kind), *GetNameSafe(Scenario), *GetNameSafe(LeverHand.Get()),
            IsValid(Scenario) && Scenario->bEmergencyStopped, IsValid(Scenario) ? *UEnum::GetValueAsString(Scenario->Stage) : TEXT("None"));
        return false;
    }
    HandleHomeTransform = Handle->GetRelativeTransform();
    // shortcut: рукоятка следует контроллеру без физического шарнира; ограничить механику при замене макета рычага.
    if (!Handle->AttachToComponent(Hand, FAttachmentTransformRules::SnapToTargetNotIncludingScale))
    {
        UE_LOG(LogFishing, Warning, TEXT("[Lever.Rejected] station=%s hand=%s reason=AttachmentFailed"), *GetName(), *GetNameSafe(Hand));
        return false;
    }
    LeverHand = Hand;
    LeverGrabLocation = GetActorTransform().InverseTransformPosition(Hand->GetComponentLocation());
    AddTickPrerequisiteComponent(Hand);
    Scenario->SetWinchInput(0);
    UpdateLabel();
    UE_LOG(LogFishing, Log, TEXT("[Lever.Grabbed] station=%s hand=%s handPosition=%s attachedTo=%s origin=%s"), *GetName(), *GetNameSafe(Hand),
        *Hand->GetComponentLocation().ToCompactString(), *GetNameSafe(Handle->GetAttachParent()), *LeverGrabLocation.ToCompactString());
    return true;
}

void AFishingStation::EndLever(USceneComponent* Hand)
{
    if (LeverHand.IsExplicitlyNull() || LeverHand.Get() != Hand) return;
    UE_LOG(LogFishing, Log, TEXT("[Lever.Released] station=%s hand=%s"), *GetName(), *GetNameSafe(Hand));
    if (IsValid(Hand)) RemoveTickPrerequisiteComponent(Hand);
    LeverHand.Reset();
    if (IsValid(Scenario)) Scenario->SetWinchInput(0);
    Handle->AttachToComponent(Zone, FAttachmentTransformRules::KeepRelativeTransform);
    Handle->SetRelativeTransform(HandleHomeTransform);
    UpdateLabel();
}

void AFishingStation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!LeverHand.IsExplicitlyNull() && (!LeverHand.IsValid() || !IsValid(Scenario) || Scenario->bEmergencyStopped ||
        (Scenario->Stage != EFishingStage::Deploying && Scenario->Stage != EFishingStage::Hauling)))
    {
        UE_LOG(LogFishing, Log, TEXT("[Lever.AutoRelease] station=%s reason=%s"), *GetName(), !LeverHand.IsValid() ? TEXT("MissingHand") :
            !IsValid(Scenario) ? TEXT("MissingScenario") : Scenario->bEmergencyStopped ? TEXT("EmergencyStopped") : TEXT("StageChanged"));
        EndLever(LeverHand.Get());
    }
    if (!IsValid(Scenario)) return;
    if (Kind == EFishingStationKind::WinchLever && LeverHand.IsValid())
    {
        const float Travel = FMath::IsFinite(LeverTravel) ? FMath::Max(5.f, LeverTravel) : 20.f;
        const float Displacement = GetActorTransform().InverseTransformPosition(LeverHand->GetComponentLocation()).X - LeverGrabLocation.X;
        const float Target = Displacement > 2.f ? FMath::Clamp(Displacement / Travel, 0.f, 1.f) : 0.f;
        // Accelerate gently; slowing down and releasing must stop immediately.
        const float Input = FMath::Min(Target, Scenario->WinchInput + FMath::Max(0.f, DeltaSeconds) * .75f);
        Scenario->SetWinchInput(Input);
    }
    if (Kind == EFishingStationKind::StatusBoard || Kind == EFishingStationKind::WinchLever || IsButton()) UpdateLabel();
}

bool AFishingStation::ContainsItem(const AFishingItem* Item) const
{
    if (!IsValid(Item)) return false;
    const FVector Local = Zone->GetComponentTransform().InverseTransformPosition(Item->GetActorLocation());
    const FVector Extent = Zone->GetUnscaledBoxExtent();
    return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool AFishingStation::AcceptItem(AFishingItem* Item)
{
    if (!IsValid(Scenario) || !IsValid(Item) || Item->Scenario != Scenario || Item->IsHeld())
    {
        UE_LOG(LogFishing, Verbose, TEXT("[Receiver.Rejected] station=%s item=%s reason=%s"), *GetName(), *GetNameSafe(Item),
            !IsValid(Scenario) ? TEXT("MissingScenario") : !IsValid(Item) ? TEXT("MissingItem") : Item->Scenario != Scenario ? TEXT("DifferentScenario") : TEXT("ItemStillHeld"));
        return false;
    }
    UE_LOG(LogFishing, Log, TEXT("[Receiver.Attempt] station=%s kind=%s item=%s state=%s stage=%s"), *GetName(), *UEnum::GetValueAsString(Kind),
        *Item->GetName(), *UEnum::GetValueAsString(Item->State), *UEnum::GetValueAsString(Scenario->Stage));
    switch (Kind)
    {
    case EFishingStationKind::SortingBin:
        if (Scenario->Stage != EFishingStage::Sorting || Item->Kind != EFishingItemKind::Fish || Item->bSorted ||
            (Item->State != EFishingItemState::Loose && Item->State != EFishingItemState::Conveyor))
        {
            UE_LOG(LogFishing, Log, TEXT("[Receiver.Rejected] station=%s item=%s reason=WrongStageOrItemState sorted=%d"), *GetName(), *Item->GetName(), Item->bSorted);
            return false;
        }
        Scenario->TrySortFish(Item, Species);
        return true; // Неверный вид тоже обработан: рыба уже возвращена в лоток пересортировки.
    case EFishingStationKind::ProcessingBoard: return Scenario->TryLoadFish(Item);
    case EFishingStationKind::DeliveryBin: return Scenario->TryDeliverFish(Item);
    default: return false;
    }
}

void AFishingStation::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep)
{
    if (AFishingItem* Item = Cast<AFishingItem>(OtherActor))
        if (ContainsItem(Item)) AcceptItem(Item);
}

FText AFishingStation::GetLabelText() const
{
    if (Kind == EFishingStationKind::StatusBoard && IsValid(Scenario)) return Scenario->GetStatusText();
    if (Kind == EFishingStationKind::WinchLever && IsValid(Scenario))
    {
        const bool bCanGrip = !Scenario->bEmergencyStopped && (Scenario->Stage == EFishingStage::Deploying || Scenario->Stage == EFishingStage::Hauling);
        const FString Grip = LeverHand.IsValid() ? TEXT("РЫЧАГ В РУКЕ") : bCanGrip ? TEXT("ЗАЖМИТЕ GRIP У РУКОЯТКИ") : TEXT("ЛЕБЁДКА");
        return FText::FromString(FString::Printf(TEXT("%s\n%s"), *Grip, *Scenario->GetWinchStatusText().ToString()));
    }
    const TCHAR* Text = TEXT("");
    switch (Kind)
    {
    case EFishingStationKind::SafetyEquipment: Text = IsValid(Scenario) && Scenario->bSafetyEquipped ? TEXT("ЗАЩИТА НАДЕТА") : TEXT("НАДЕТЬ ЗАЩИТУ"); break;
    case EFishingStationKind::EmergencyStop: Text = TEXT("СТОП"); break;
    case EFishingStationKind::StartOrResume:
        Text = IsValid(Scenario) && Scenario->Stage != EFishingStage::Safety
            ? (Scenario->bEmergencyStopped ? TEXT("СБРОС\nОСТАНОВКИ") : TEXT("ЛЕБЁДКА\nГОТОВА")) : TEXT("НАЧАТЬ\nЛОВЛЮ");
        break;
    case EFishingStationKind::WinchLever: Text = LeverHand.IsValid() ? TEXT("РЫЧАГ В РУКЕ\nВПЕРЁД →") : TEXT("ЛЕБЁДКА →\nЗАЖМИТЕ GRIP"); break;
    case EFishingStationKind::GoToSorting: Text = TEXT("В ЦЕХ"); break;
    case EFishingStationKind::SortingBin: Text = Species == EFishSpecies::A ? TEXT("РЫБА A") : TEXT("РЫБА B"); break;
    case EFishingStationKind::GoToProcessing: Text = TEXT("К ОБРАБОТКЕ"); break;
    case EFishingStationKind::ProcessingBoard: Text = TEXT("СТОЛ ОБРАБОТКИ"); break;
    case EFishingStationKind::DeliveryBin: Text = TEXT("ГОТОВЫЙ УЛОВ"); break;
    case EFishingStationKind::Restart: Text = TEXT("НОВАЯ СМЕНА"); break;
    case EFishingStationKind::StatusBoard: Text = TEXT("ЗАДАНИЕ СМЕНЫ"); break;
    }
    return FText::FromString(Text);
}

FColor AFishingStation::GetLabelColor() const
{
    if (!IsValid(Scenario) || Kind == EFishingStationKind::StatusBoard) return FColor::White;
    const FColor Next(255, 220, 40);
    const FColor Disabled(120, 120, 120);
    switch (Kind)
    {
    case EFishingStationKind::WinchLever:
        if (Scenario->bEmergencyStopped) return FColor::Red;
        if (Scenario->Stage != EFishingStage::Deploying && Scenario->Stage != EFishingStage::Hauling) return Disabled;
        if (Scenario->WinchLoad > .8f) return Next;
        return LeverHand.IsValid() ? FColor::Green : FColor::White;
    case EFishingStationKind::SafetyEquipment:
        return Scenario->bSafetyEquipped ? FColor::Green : Scenario->Stage == EFishingStage::Safety ? Next : Disabled;
    case EFishingStationKind::EmergencyStop:
        return Scenario->Stage == EFishingStage::Safety && Scenario->bSafetyEquipped && !Scenario->bStopTested ? Next : FColor::Red;
    case EFishingStationKind::StartOrResume:
        return (Scenario->Stage == EFishingStage::Safety && Scenario->bSafetyEquipped && Scenario->bStopTested) ||
            ((Scenario->Stage == EFishingStage::Deploying || Scenario->Stage == EFishingStage::Hauling) && Scenario->bEmergencyStopped) ? Next : Disabled;
    case EFishingStationKind::GoToSorting: return Scenario->Stage == EFishingStage::AwaitSorting ? Next : Disabled;
    case EFishingStationKind::GoToProcessing: return Scenario->Stage == EFishingStage::AwaitProcessing ? Next : Disabled;
    case EFishingStationKind::Restart: return Scenario->Stage == EFishingStage::Complete ? Next : Disabled;
    default: return FColor::White;
    }
}

void AFishingStation::UpdateLabel()
{
    if (Kind == EFishingStationKind::StatusBoard && Label->WorldSize != 5.5f) Label->SetWorldSize(5.5f);
    if (Kind == EFishingStationKind::WinchLever)
    {
        const FVector Position(0, 0, 55);
        if (!Label->GetRelativeLocation().Equals(Position)) Label->SetRelativeLocation(Position);
        if (Label->WorldSize != 4.5f) Label->SetWorldSize(4.5f);
    }
    const FText Text = GetLabelText();
    if (!Label->Text.EqualTo(Text)) Label->SetText(Text);
    const FColor Color = GetLabelColor();
    if (Label->TextRenderColor != Color) Label->SetTextRenderColor(Color);
}
