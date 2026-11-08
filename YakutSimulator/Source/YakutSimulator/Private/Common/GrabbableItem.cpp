#include "Common/GrabbableItem.h"
#include "Common/MiniGameScenario.h"
#include "Common/MiniGamePanel.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/DataValidation.h"
namespace
{
bool HasVisibleGeometry(const AActor* Actor)
{
    TArray<UMeshComponent*> Visuals;
    Actor->GetComponents(Visuals);
    return Visuals.ContainsByPredicate([](const auto* Visual) { return !Visual->IsA<UWidgetComponent>() && Visual->IsVisible() && Visual->GetNumMaterials() > 0; });
}
}

AGrabbableItem::AGrabbableItem()
{
    PrimaryActorTick.bCanEverTick = true;
    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
    SetRootComponent(Body);
    Body->SetMobility(EComponentMobility::Movable);
    Body->SetCollisionObjectType(ECC_WorldDynamic);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionResponseToAllChannels(ECR_Block);
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Body);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ToolTip = CreateDefaultSubobject<USceneComponent>(TEXT("ToolTip"));
    ToolTip->SetupAttachment(Body);
    ToolTipVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ToolTipVisual"));
    ToolTipVisual->SetupAttachment(ToolTip);
    ToolTipVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ToolTipVisual->SetVisibility(false);
    Grab = CreateDefaultSubobject<UGrabInteractionComponent>(TEXT("Grab"));
    Grab->SetupAttachment(Body);
    Grab->Shape.ComponentProperty = TEXT("Body");
    Grab->MoveComponent.ComponentProperty = TEXT("Body");
    Display = CreateDefaultSubobject<UMiniGameDisplayComponent>(TEXT("Display"));
    Display->SetupAttachment(Body);
}
void AGrabbableItem::BeginPlay()
{
    Super::BeginPlay();
    Grab->Scenario = Scenario;
    Display->BindScenario(Scenario);
}
bool AGrabbableItem::IsHeld() const { return IsValid(Grab->GetHoldingHand()); }
bool AGrabbableItem::CanInteract_Implementation(const FInteractionContext& Context, EInteractionAction, FText& Reason) const
{
    const bool Valid = Scenario && Context.Scenario == Scenario && SourceRun == Context.RunId && Scenario->Status == EMiniGameStatus::Running;
    if (!Valid && Scenario && Scenario->ActiveDefinition()) Reason = Scenario->ActiveDefinition()->Text(TEXT("ActionUnavailable"));
    return Valid;
}
void AGrabbableItem::InteractionStarted_Implementation(const FInteractionContext&)
{
    LooseSeconds = 0;
    ToolTipVisual->SetVisibility(true);
    if (IsValid(Scenario)) Scenario->NotifyPresentation();
}
void AGrabbableItem::InteractionEnded_Implementation(const FInteractionContext&)
{
    LooseSeconds = 0;
    ToolTipVisual->SetVisibility(false);
    if (IsValid(Scenario)) Scenario->NotifyPresentation();
}
bool AGrabbableItem::TryReceivers()
{
    if (!Scenario || !GetWorld() || SourceRun != Scenario->RunId) return false;
    auto* Registry = GetWorld()->GetSubsystem<UMiniGameWorldSubsystem>();
    if (!Registry) return false;
    TArray<UItemReceiverComponent*> Candidates;
    for (const auto& Receiver : Registry->Receivers)
        if (Receiver.IsValid() && Receiver->Scenario == Scenario && Receiver->Contains(GetActorLocation())) Candidates.Add(Receiver.Get());
    Candidates.Sort([&](const auto& A, const auto& B) { return FVector::DistSquared(A.GetOwner()->GetActorLocation(), GetActorLocation()) < FVector::DistSquared(B.GetOwner()->GetActorLocation(), GetActorLocation()); });
    for (auto* Receiver : Candidates) if (Receiver->TryReceive(this)) return true;
    return false;
}
void AGrabbableItem::Park(const FTransform& Transform, bool Hidden)
{
    Grab->HoldingContext = {};
    ToolTipVisual->SetVisibility(false);
    Body->SetSimulatePhysics(false);
    Body->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    FTransform Pose = Transform;
    Pose.SetScale3D(GetActorScale3D());
    SetActorTransform(Pose, false, nullptr, ETeleportType::TeleportPhysics);
    SetActorHiddenInGame(Hidden);
    SetActorEnableCollision(!Hidden);
    LooseSeconds = 0;
}
void AGrabbableItem::ReturnHome() { if (IsValid(Home)) Park(Home->GetComponentTransform()); }
void AGrabbableItem::Tick(float Delta)
{
    Super::Tick(Delta);
    if (!FMath::IsFinite(Delta) || Delta <= 0 || IsHeld() || IsHidden() || !Body->IsSimulatingPhysics() || !IsValid(Scenario) || SourceRun != Scenario->RunId) return;
    LooseSeconds += Delta;
    if ((bRecoverOnTimeout && LooseSeconds >= RecoverySeconds) ||
        (bRecoverBelowHome && IsValid(Home) && GetActorLocation().Z < Home->GetComponentLocation().Z - RecoveryDropDistance))
    {
        UE_LOG(LogInteraction, Log, TEXT("[Recovery.Request] run=%s actor=%s"), *SourceRun.ToString(), *GetName());
        LooseSeconds = 0;
        Scenario->RecoverItem(this);
    }
}
bool AGrabbableItem::ValidateItem(TArray<FText>& Errors) const
{
    if (!HasVisibleGeometry(this)) Errors.Add(NSLOCTEXT("MiniGame.Validation", "ItemVisual", "Assign an item mesh in its Blueprint."));
    if (Body->GetUnscaledBoxExtent().GetMin() <= 0) Errors.Add(NSLOCTEXT("MiniGame.Validation", "ItemCollision", "Item collision extents must be positive."));
    if (!FMath::IsFinite(RecoverySeconds) || RecoverySeconds <= 0 || !FMath::IsFinite(RecoveryDropDistance) || RecoveryDropDistance < 0 || !Grab->GetMoveComponent() || !Grab->GetShape())
        Errors.Add(NSLOCTEXT("MiniGame.Validation", "ItemBindings", "Invalid recovery settings or grab component bindings."));
    return Errors.IsEmpty();
}
AMiniGameStation::AMiniGameStation()
{
    Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
    SetRootComponent(Zone);
    Zone->SetMobility(EComponentMobility::Movable);
    Zone->SetCollisionObjectType(ECC_WorldDynamic);
    Zone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Zone->SetCollisionResponseToAllChannels(ECR_Overlap);
    PressZone = CreateDefaultSubobject<UBoxComponent>(TEXT("PressZone"));
    PressZone->SetupAttachment(Zone);
    PressZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PressZone->SetCollisionResponseToAllChannels(ECR_Overlap);
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(Zone);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Handle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Handle"));
    Handle->SetupAttachment(Zone);
    Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Button = CreateDefaultSubobject<UPressableInteractionComponent>(TEXT("Button"));
    Button->SetupAttachment(Zone);
    Button->Shape.ComponentProperty = TEXT("PressZone");
    Button->bEnabled = false;
    Lever = CreateDefaultSubobject<ULeverInteractionComponent>(TEXT("Lever"));
    Lever->SetupAttachment(Zone);
    Lever->Shape.ComponentProperty = TEXT("Zone");
    Lever->Handle.ComponentProperty = TEXT("Handle");
    Lever->bEnabled = false;
    Receiver = CreateDefaultSubobject<UItemReceiverComponent>(TEXT("Receiver"));
    Receiver->Shape.ComponentProperty = TEXT("Zone");
    Receiver->bEnabled = false;
    Display = CreateDefaultSubobject<UMiniGameDisplayComponent>(TEXT("Display"));
    Display->SetupAttachment(Zone);
}
void AMiniGameStation::BeginPlay()
{
    Super::BeginPlay();
    Button->Scenario = Lever->Scenario = Scenario;
    Receiver->Scenario = Scenario;
    Display->BindScenario(Scenario);
}
bool AMiniGameStation::ValidateStation(TArray<FText>& Errors) const
{
    const bool Instance = !HasAnyFlags(RF_ClassDefaultObject);
    if ((Button->bEnabled || Lever->bEnabled || Receiver->bEnabled) && !HasVisibleGeometry(this))
        Errors.Add(NSLOCTEXT("MiniGame.Validation", "StationMesh", "Assign the station visual mesh."));
    if ((Button->bEnabled && ((Instance && Button->ActionId.IsNone()) || !Button->GetShape() || PressZone->GetUnscaledBoxExtent().GetMin() <= 0 || !FMath::IsFinite(Button->MaxHandDistance) || Button->MaxHandDistance < 0)) ||
        (Receiver->bEnabled && (!Receiver->Shape.GetComponent(Receiver->GetOwner()) || Zone->GetUnscaledBoxExtent().GetMin() <= 0)))
        Errors.Add(NSLOCTEXT("MiniGame.Validation", "StationShape", "Assign station actions and positive, independent interaction volumes."));
    if (Lever->bEnabled && (!Lever->GetHandle() || !Lever->GetShape() || (Instance && Lever->ActionId.IsNone()) ||
        !FMath::IsFinite(Lever->Travel) || Lever->Travel <= 0 || !FMath::IsFinite(Lever->DeadZone) || Lever->DeadZone < 0 || Lever->DeadZone >= Lever->Travel ||
        !FMath::IsFinite(Lever->Acceleration) || Lever->Acceleration <= 0 || Lever->LocalAxis.ContainsNaN() || Lever->LocalAxis.IsNearlyZero()))
        Errors.Add(NSLOCTEXT("MiniGame.Validation", "Lever", "Assign the lever handle, axis, action, travel, dead zone and acceleration."));
    return Errors.IsEmpty();
}
#if WITH_EDITOR
EDataValidationResult AGrabbableItem::IsDataValid(FDataValidationContext& Context) const
{
    TArray<FText> Errors; ValidateItem(Errors);
    for (const FText& Error : Errors) Context.AddError(Error);
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
EDataValidationResult AMiniGameStation::IsDataValid(FDataValidationContext& Context) const
{
    TArray<FText> Errors; ValidateStation(Errors);
    for (const FText& Error : Errors) Context.AddError(Error);
    return Errors.IsEmpty() ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
