#include "Fishing/FishingItem.h"
#include "Fishing/FishingScenario.h"
#include "Fishing/FishingLog.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFishingItem::AFishingItem()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
    SetRootComponent(Body);
    Body->SetBoxExtent(FVector(18, 7, 4));
    Body->SetMobility(EComponentMobility::Movable);
    Body->SetCollisionObjectType(ECC_WorldDynamic);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionResponseToAllChannels(ECR_Block);
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Body->SetGenerateOverlapEvents(true);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Body);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Mesh->SetStaticMesh(Sphere.Object);
    BladeTip = CreateDefaultSubobject<USceneComponent>(TEXT("BladeTip"));
    BladeTip->SetupAttachment(Body);
    BladeTip->SetRelativeLocation(FVector(15, 0, 0));
    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(Body);
    Label->SetRelativeLocation(FVector(0, 0, 10));
    Label->SetWorldSize(6);
    Label->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Label->SetTextRenderColor(FColor::White);
    static ConstructorHelpers::FObjectFinder<UFont> TextFont(TEXT("/Game/Fishing/UI/F_FishingCyrillic.F_FishingCyrillic"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> TextMaterial(TEXT("/Game/Fishing/UI/MI_FishingText.MI_FishingText"));
    if (TextFont.Succeeded()) Label->SetFont(TextFont.Object);
    if (TextMaterial.Succeeded()) Label->SetTextMaterial(TextMaterial.Object);
}

void AFishingItem::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshAppearance();
}

void AFishingItem::RefreshAppearance()
{
    const bool bPrimitive = Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Engine/BasicShapes/"));
    if (bPrimitive)
    {
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Kind == EFishingItemKind::Knife
            ? TEXT("/Engine/BasicShapes/Cube.Cube") : TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Mesh->SetRelativeScale3D(Kind == EFishingItemKind::Knife ? FVector(.30, .025, .025)
            : (Species == EFishSpecies::A ? FVector(.36, .12, .08) : FVector(.25, .22, .06)));
    }
    Label->SetText(FText::FromString(Kind == EFishingItemKind::Knife ? TEXT("Нож")
        : FString::Printf(TEXT("%s%s"), Species == EFishSpecies::A ? TEXT("A") : TEXT("B"),
            State == EFishingItemState::Processed ? TEXT(" / готово") : TEXT(""))));
}

bool AFishingItem::TryGrab(USceneComponent* Hand)
{
    if (!IsValid(Hand) || IsHeld() || !IsValid(Scenario) ||
        FVector::DistSquared(Hand->GetComponentLocation(), GetActorLocation()) > FMath::Square(40.f))
    {
        UE_LOG(LogFishing, Log, TEXT("[Item.GrabRejected] item=%s hand=%s reason=%s distanceCm=%.1f state=%s"), *GetName(), *GetNameSafe(Hand),
            !IsValid(Hand) ? TEXT("MissingHand") : IsHeld() ? TEXT("AlreadyHeld") : !IsValid(Scenario) ? TEXT("MissingScenario") : TEXT("HandTooFar"),
            IsValid(Hand) ? FVector::Distance(Hand->GetComponentLocation(), GetActorLocation()) : -1.f, *UEnum::GetValueAsString(State));
        return false;
    }
    if (Kind == EFishingItemKind::Knife)
    {
        if (Scenario->Stage != EFishingStage::Processing)
        {
            UE_LOG(LogFishing, Log, TEXT("[Item.GrabRejected] item=%s reason=KnifeOutsideProcessing stage=%s"), *GetName(), *UEnum::GetValueAsString(Scenario->Stage));
            return false;
        }
    }
    else if ((Scenario->Stage != EFishingStage::Sorting && Scenario->Stage != EFishingStage::Processing) ||
        State == EFishingItemState::Queued || State == EFishingItemState::Sorted || State == EFishingItemState::Delivered)
    {
        UE_LOG(LogFishing, Log, TEXT("[Item.GrabRejected] item=%s reason=FishNotAvailable stage=%s state=%s"),
            *GetName(), *UEnum::GetValueAsString(Scenario->Stage), *UEnum::GetValueAsString(State));
        return false;
    }

    HoldingHand = Hand;
    LooseSeconds = 0;
    UE_LOG(LogFishing, Log, TEXT("[Item.Grabbed] item=%s kind=%s state=%s hand=%s position=%s bracing=%d"), *GetName(), *UEnum::GetValueAsString(Kind),
        *UEnum::GetValueAsString(State), *GetNameSafe(Hand), *GetActorLocation().ToCompactString(), State == EFishingItemState::OnBoard);
    if (State == EFishingItemState::OnBoard)
    {
        bBracing = true;
        return true;
    }
    Body->SetSimulatePhysics(false);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    AttachToComponent(Hand, Kind == EFishingItemKind::Knife
        ? FAttachmentTransformRules::SnapToTargetNotIncludingScale : FAttachmentTransformRules::KeepWorldTransform);
    if (Kind == EFishingItemKind::Fish && State == EFishingItemState::Conveyor) State = EFishingItemState::Loose;
    return true;
}

bool AFishingItem::IsBraced() const
{
    return bBracing && HoldingHand.IsValid() &&
        FVector::DistSquared(HoldingHand->GetComponentLocation(), GetActorLocation()) <= FMath::Square(30.f);
}

void AFishingItem::Release()
{
    if (!IsHeld()) return;
    UE_LOG(LogFishing, Log, TEXT("[Item.Release] item=%s hand=%s state=%s bracing=%d position=%s"), *GetName(), *GetNameSafe(HoldingHand.Get()),
        *UEnum::GetValueAsString(State), bBracing, *GetActorLocation().ToCompactString());
    HoldingHand.Reset();
    if (bBracing)
    {
        bBracing = false;
        return;
    }
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    LooseSeconds = 0;
    if (IsValid(Scenario) && Scenario->TryPlaceReleasedItem(this)) return;
    if (State != EFishingItemState::Processed) State = EFishingItemState::Loose;
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetSimulatePhysics(true);
    UE_LOG(LogFishing, Log, TEXT("[Item.Loose] item=%s state=%s receiver=None physics=1"), *GetName(), *UEnum::GetValueAsString(State));
}

void AFishingItem::Park(const FTransform& Transform, EFishingItemState NewState)
{
    UE_LOG(LogFishing, Log, TEXT("[Item.Parked] item=%s batch=%d kind=%s species=%s from=%s to=%s position=%s"),
        *GetName(), BatchIndex, *UEnum::GetValueAsString(Kind), *UEnum::GetValueAsString(Species),
        *UEnum::GetValueAsString(State), *UEnum::GetValueAsString(NewState), *Transform.GetLocation().ToCompactString());
    HoldingHand.Reset();
    bBracing = false;
    Body->SetSimulatePhysics(false);
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetActorTransform(Transform, false, nullptr, ETeleportType::TeleportPhysics);
    State = NewState;
    LooseSeconds = 0;
    SetActorHiddenInGame(State == EFishingItemState::Queued || State == EFishingItemState::Sorted || State == EFishingItemState::Delivered);
    SetActorEnableCollision(!IsHidden());
    RefreshAppearance();
}

void AFishingItem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsHeld() && !IsHidden() && Body->IsSimulatingPhysics() && IsValid(Scenario))
    {
        LooseSeconds += DeltaSeconds;
        if (LooseSeconds > 5.f || GetActorLocation().Z < Scenario->GetActorLocation().Z - 150.f)
        {
            UE_LOG(LogFishing, Log, TEXT("[Item.RecoveryRequested] item=%s reason=%s looseSeconds=%.2f position=%s"),
                *GetName(), LooseSeconds > 5.f ? TEXT("LooseTimeout") : TEXT("BelowFloor"), LooseSeconds, *GetActorLocation().ToCompactString());
            Scenario->RecoverItem(this);
        }
    }
}

TArray<FVector> AFishingItem::GetCutPoints() const
{
    // shortcut: две условные траектории, заменить после выбора реальных видов и операций обработки.
    return Species == EFishSpecies::A
        ? TArray<FVector>{FVector(-12, 0, 5), FVector(0, 0, 5), FVector(12, 0, 5)}
        : TArray<FVector>{FVector(-8, -7, 5), FVector(-8, 7, 5), FVector(8, 7, 5), FVector(8, -7, 5)};
}
