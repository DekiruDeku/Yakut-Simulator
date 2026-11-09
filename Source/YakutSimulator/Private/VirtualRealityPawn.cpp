#include "VirtualRealityPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/MovementComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "MotionControllerComponent.h"
#include "PrototypePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "WorkItem.h"

AVirtualRealityPawn::AVirtualRealityPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    TrackingOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("TrackingOrigin"));
    SetRootComponent(TrackingOrigin);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(TrackingOrigin);
    LeftHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftHand"));
    LeftHand->SetupAttachment(TrackingOrigin);
    LeftHand->SetTrackingMotionSource(TEXT("LeftGrip"));
    RightHand = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightHand"));
    RightHand->SetupAttachment(TrackingOrigin);
    RightHand->SetTrackingMotionSource(TEXT("RightGrip"));
    LeftAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftAim"));
    LeftAim->SetupAttachment(TrackingOrigin);
    LeftAim->SetTrackingMotionSource(TEXT("LeftAim"));
    RightAim = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightAim"));
    RightAim->SetupAttachment(TrackingOrigin);
    RightAim->SetTrackingMotionSource(TEXT("RightAim"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> HandShape(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UStaticMeshComponent* LeftVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHandVisual"));
    LeftVisual->SetupAttachment(LeftHand);
    LeftVisual->SetStaticMesh(HandShape.Object);
    LeftVisual->SetRelativeScale3D(FVector(0.065));
    LeftVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UStaticMeshComponent* RightVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHandVisual"));
    RightVisual->SetupAttachment(RightHand);
    RightVisual->SetStaticMesh(HandShape.Object);
    RightVisual->SetRelativeScale3D(FVector(0.065));
    RightVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    LeftPointer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("LeftPointer"));
    LeftPointer->SetupAttachment(LeftAim);
    LeftPointer->PointerIndex = 0;
    LeftPointer->InteractionDistance = 500;
    RightPointer = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("RightPointer"));
    RightPointer->SetupAttachment(RightAim);
    RightPointer->PointerIndex = 1;
    RightPointer->InteractionDistance = 500;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> RayShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    LeftRay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftRay"));
    LeftRay->SetupAttachment(TrackingOrigin);
    LeftRay->SetStaticMesh(RayShape.Object);
    LeftRay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightRay = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightRay"));
    RightRay->SetupAttachment(TrackingOrigin);
    RightRay->SetStaticMesh(RayShape.Object);
    RightRay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TeleportMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeleportMarker"));
    TeleportMarker->SetupAttachment(TrackingOrigin);
    TeleportMarker->SetStaticMesh(RayShape.Object);
    TeleportMarker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TeleportMarker->SetWorldScale3D(FVector(0.35f, 0.35f, 0.015f));
    TeleportMarker->SetVisibility(false);

    MenuDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("MenuDisplay"));
    MenuDisplay->SetupAttachment(TrackingOrigin);
    MenuDisplay->SetDrawSize(FVector2D(1000, 800));
    MenuDisplay->SetRelativeScale3D(FVector(0.09));
    MenuDisplay->SetVisibility(false);
    MenuDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AVirtualRealityPawn::IsVirtualRealityActive() const
{
    return UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled();
}

bool AVirtualRealityPawn::IsMenuOpen() const
{
    const APrototypePlayerController* Player = Cast<APrototypePlayerController>(Controller);
    return Player && Player->bMenuOpen;
}

void AVirtualRealityPawn::BeginPlay()
{
    Super::BeginPlay();
    PrimaryActorTick.AddPrerequisite(LeftHand, LeftHand->PrimaryComponentTick);
    PrimaryActorTick.AddPrerequisite(RightHand, RightHand->PrimaryComponentTick);
    if (IsVirtualRealityActive())
    {
        UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::LocalFloor);
    }
    else
    {
        Camera->SetRelativeLocation(FVector(0, 0, 165));
        LeftPointer->bEnableHitTesting = true;
        RightPointer->bEnableHitTesting = true;
    }
}

void AVirtualRealityPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateMovementBase();
    if (!IsValid(LeftItem) || !LeftItem->bIsHeld)
    {
        LeftItem = nullptr;
    }
    if (!IsValid(RightItem) || !RightItem->bIsHeld)
    {
        RightItem = nullptr;
    }
    if (!IsVirtualRealityActive())
    {
        if (!IsMenuOpen() && !DesktopMovement.IsNearlyZero())
        {
            const FVector Movement = GetActorForwardVector() * DesktopMovement.Y + GetActorRightVector() * DesktopMovement.X;
            const FVector Destination = GetActorLocation() + Movement.GetClampedToMaxSize(1) * 180 * DeltaSeconds;
            FHitResult FloorHit;
            FCollisionQueryParams Query;
            Query.AddIgnoredActor(this);
            if (GetWorld()->LineTraceSingleByChannel(FloorHit, Destination + FVector(0, 0, 40), Destination - FVector(0, 0, 80), ECC_WorldStatic, Query))
            {
                SetActorLocation(Destination);
            }
        }
        const FTransform UpdatedView = Camera->GetComponentTransform();
        LeftAim->SetWorldLocationAndRotation(UpdatedView.GetLocation(), UpdatedView.GetRotation());
        RightAim->SetWorldLocationAndRotation(UpdatedView.GetLocation(), UpdatedView.GetRotation());
        LeftHand->SetWorldLocationAndRotation(UpdatedView.TransformPosition(FVector(65, -18, -40)), UpdatedView.GetRotation());
        RightHand->SetWorldLocationAndRotation(UpdatedView.TransformPosition(FVector(65, 18, -40)), UpdatedView.GetRotation());
    }
    UpdateMotion(LeftHand, LeftMotion, DeltaSeconds);
    UpdateMotion(RightHand, RightMotion, DeltaSeconds);
    if (IsVirtualRealityActive())
    {
        if (!LeftHand->IsTracked() && LeftItem)
        {
            LeftItem->Release(FVector::ZeroVector, FVector::ZeroVector);
            LeftItem = nullptr;
        }
        if (!RightHand->IsTracked() && RightItem)
        {
            RightItem->Release(FVector::ZeroVector, FVector::ZeroVector);
            RightItem = nullptr;
        }
    }
    UpdatePointer(LeftPointer, LeftRay);
    UpdatePointer(RightPointer, RightRay);
    if (bTeleportAiming)
    {
        UpdateTeleport();
    }
}

void AVirtualRealityPawn::UpdateMotion(USceneComponent* Hand, FHandMotionState& Motion, float DeltaSeconds)
{
    if (Motion.bInitialized && DeltaSeconds > SMALL_NUMBER)
    {
        Motion.LinearVelocity = FMath::VInterpTo(Motion.LinearVelocity, (Hand->GetComponentLocation() - Motion.PreviousLocation) / DeltaSeconds, DeltaSeconds, 25);
        FQuat Difference = Hand->GetComponentQuat() * Motion.PreviousRotation.Inverse();
        Difference.Normalize();
        FVector RotationAxis;
        float RotationAngle;
        Difference.ToAxisAndAngle(RotationAxis, RotationAngle);
        if (RotationAngle > PI)
        {
            RotationAngle -= 2 * PI;
        }
        Motion.AngularVelocity = FMath::VInterpTo(Motion.AngularVelocity, RotationAxis * RotationAngle / DeltaSeconds, DeltaSeconds, 25);
    }
    Motion.PreviousLocation = Hand->GetComponentLocation();
    Motion.PreviousRotation = Hand->GetComponentQuat();
    Motion.bInitialized = true;
}

void AVirtualRealityPawn::SetMovementBase(UPrimitiveComponent* Base)
{
    if (MovementBase == Base)
    {
        return;
    }
    if (IsValid(MovementBase))
    {
        RemoveTickPrerequisiteActor(MovementBase->GetAttachmentRootActor());
        TInlineComponentArray<UMovementComponent*> Movements(MovementBase->GetAttachmentRootActor());
        for (UMovementComponent* Movement : Movements)
        {
            RemoveTickPrerequisiteComponent(Movement);
        }
    }
    MovementBase = Base;
    if (MovementBase)
    {
        PreviousBaseTransform = MovementBase->GetComponentTransform();
        AddTickPrerequisiteActor(MovementBase->GetAttachmentRootActor());
        TInlineComponentArray<UMovementComponent*> Movements(MovementBase->GetAttachmentRootActor());
        for (UMovementComponent* Movement : Movements)
        {
            AddTickPrerequisiteComponent(Movement);
        }
    }
}

void AVirtualRealityPawn::UpdateMovementBase()
{
    if (IsValid(MovementBase))
    {
        const FTransform CurrentBaseTransform = MovementBase->GetComponentTransform();
        SetActorLocation(CurrentBaseTransform.TransformPosition(PreviousBaseTransform.InverseTransformPosition(GetActorLocation())));
        AddActorWorldRotation(FRotator(0, (CurrentBaseTransform.GetRotation() * PreviousBaseTransform.GetRotation().Inverse()).Rotator().Yaw, 0));
        PreviousBaseTransform = CurrentBaseTransform;
    }
    FHitResult Floor;
    FCollisionQueryParams Query;
    Query.AddIgnoredActor(this);
    const FVector Feet(Camera->GetComponentLocation().X, Camera->GetComponentLocation().Y, GetActorLocation().Z);
    if (GetWorld()->LineTraceSingleByChannel(Floor, Feet + FVector(0, 0, 35), Feet - FVector(0, 0, 60), ECC_Visibility, Query)
        && Floor.ImpactNormal.Z > 0.9f
        && Floor.GetComponent()->Mobility == EComponentMobility::Movable)
    {
        SetMovementBase(Floor.GetComponent());
    }
    else
    {
        SetMovementBase(nullptr);
    }
}

void AVirtualRealityPawn::MoveHeldItemsWithPlayer(const FTransform& PreviousPlayerTransform)
{
    for (AWorkItem* Item : { LeftItem.Get(), RightItem.Get() })
    {
        if (!IsValid(Item))
        {
            continue;
        }
        if (Item->bIsAnchored)
        {
            Item->Release(FVector::ZeroVector, FVector::ZeroVector);
            continue;
        }
        const FTransform RelativeBody = Item->Body->GetComponentTransform().GetRelativeTransform(PreviousPlayerTransform);
        const FTransform Destination = RelativeBody * GetActorTransform();
        Item->Body->SetWorldLocationAndRotation(Destination.GetLocation(), Destination.GetRotation(), false, nullptr, ETeleportType::TeleportPhysics);
    }
}

bool AVirtualRealityPawn::TryGrabItem(bool bLeftHand)
{
    if (IsMenuOpen())
    {
        return false;
    }
    AWorkItem* HeldItem = RightItem;
    USceneComponent* Hand = RightHand;
    if (bLeftHand)
    {
        HeldItem = LeftItem;
        Hand = LeftHand;
    }
    if (HeldItem)
    {
        return false;
    }
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Query;
    Query.AddIgnoredActor(this);
    GetWorld()->OverlapMultiByObjectType(Overlaps, Hand->GetComponentLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_PhysicsBody), FCollisionShape::MakeSphere(GrabRadius), Query);
    AWorkItem* ClosestItem = nullptr;
    float ClosestDistance = MAX_flt;
    for (const FOverlapResult& Overlap : Overlaps)
    {
        AWorkItem* Candidate = Cast<AWorkItem>(Overlap.GetActor());
        if (Candidate && Candidate->CanBeGrabbed())
        {
            const float Distance = FVector::DistSquared(Candidate->GripPoint->GetComponentLocation(), Hand->GetComponentLocation());
            if (Distance < ClosestDistance)
            {
                ClosestItem = Candidate;
                ClosestDistance = Distance;
            }
        }
    }
    if (!ClosestItem || !ClosestItem->TryGrab(Hand))
    {
        return false;
    }
    if (bLeftHand)
    {
        LeftItem = ClosestItem;
    }
    else
    {
        RightItem = ClosestItem;
    }
    return true;
}

void AVirtualRealityPawn::ReleaseItem(bool bLeftHand)
{
    if (bLeftHand)
    {
        if (IsValid(LeftItem))
        {
            LeftItem->Release(LeftMotion.LinearVelocity, LeftMotion.AngularVelocity);
        }
        LeftItem = nullptr;
    }
    else
    {
        if (IsValid(RightItem))
        {
            RightItem->Release(RightMotion.LinearVelocity, RightMotion.AngularVelocity);
        }
        RightItem = nullptr;
    }
}

void AVirtualRealityPawn::SetItemUse(bool bLeftHand, bool bUse)
{
    AWorkItem* Item = RightItem;
    UWidgetInteractionComponent* Pointer = RightPointer;
    bool* PointerPressed = &bRightPointerPressed;
    if (bLeftHand)
    {
        Item = LeftItem;
        Pointer = LeftPointer;
        PointerPressed = &bLeftPointerPressed;
    }
    if (!bUse)
    {
        if (*PointerPressed)
        {
            Pointer->ReleasePointerKey(EKeys::LeftMouseButton);
            *PointerPressed = false;
        }
        if (IsValid(Item))
        {
            Item->EndUse();
        }
        return;
    }
    if (IsMenuOpen())
    {
        if (IsVirtualRealityActive())
        {
            Pointer->PressPointerKey(EKeys::LeftMouseButton);
            *PointerPressed = true;
        }
    }
    else if (IsValid(Item))
    {
        Item->BeginUse();
    }
    else if (Pointer->IsOverHitTestVisibleWidget())
    {
        Pointer->PressPointerKey(EKeys::LeftMouseButton);
        *PointerPressed = true;
    }
}

void AVirtualRealityPawn::ReleaseAllItems()
{
    SetItemUse(true, false);
    SetItemUse(false, false);
    ReleaseItem(true);
    ReleaseItem(false);
}

void AVirtualRealityPawn::ShowMenu(UUserWidget* Menu, bool bShow)
{
    MenuDisplay->SetWidget(Menu);
    MenuDisplay->SetVisibility(bShow);
    MenuDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (bShow)
    {
        const FRotator Facing(0, Camera->GetComponentRotation().Yaw, 0);
        MenuDisplay->SetWorldLocation(Camera->GetComponentLocation() + Facing.Vector() * 130);
        MenuDisplay->SetWorldRotation(FRotator(0, Facing.Yaw + 180, 0));
        MenuDisplay->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        MenuDisplay->SetCollisionResponseToAllChannels(ECR_Ignore);
        MenuDisplay->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    }
}

void AVirtualRealityPawn::UpdatePointer(UWidgetInteractionComponent* Pointer, UStaticMeshComponent* Ray)
{
    Ray->SetVisibility(IsVirtualRealityActive() && (IsMenuOpen() || Pointer->IsOverHitTestVisibleWidget()));
    if (!Ray->IsVisible())
    {
        return;
    }
    const FVector Start = Pointer->GetComponentLocation();
    FVector End = Start + Pointer->GetForwardVector() * 180;
    if (Pointer->IsOverHitTestVisibleWidget())
    {
        End = Pointer->GetLastHitResult().ImpactPoint;
    }
    Ray->SetWorldLocation((Start + End) * 0.5);
    Ray->SetWorldRotation(FRotationMatrix::MakeFromZ(End - Start).Rotator());
    Ray->SetWorldScale3D(FVector(0.004f, 0.004f, FVector::Distance(Start, End) / 100));
}

void AVirtualRealityPawn::ProcessGrip(bool bLeftHand, float Value)
{
    bool* Pressed = &bRightGripPressed;
    if (bLeftHand)
    {
        Pressed = &bLeftGripPressed;
    }
    if (!*Pressed && Value >= 0.65f)
    {
        *Pressed = true;
        TryGrabItem(bLeftHand);
    }
    else if (*Pressed && Value <= 0.35f)
    {
        *Pressed = false;
        ReleaseItem(bLeftHand);
    }
}

void AVirtualRealityPawn::ProcessUse(bool bLeftHand, float Value)
{
    bool* Pressed = &bRightUsePressed;
    if (bLeftHand)
    {
        Pressed = &bLeftUsePressed;
    }
    if (!*Pressed && Value >= 0.65f)
    {
        *Pressed = true;
        SetItemUse(bLeftHand, true);
    }
    else if (*Pressed && Value <= 0.35f)
    {
        *Pressed = false;
        SetItemUse(bLeftHand, false);
    }
}

void AVirtualRealityPawn::GrabLeft(const FInputActionValue& Value) { ProcessGrip(true, Value.Get<float>()); }
void AVirtualRealityPawn::GrabRight(const FInputActionValue& Value) { ProcessGrip(false, Value.Get<float>()); }
void AVirtualRealityPawn::StopGrabLeft(const FInputActionValue& Value) { ProcessGrip(true, 0); }
void AVirtualRealityPawn::StopGrabRight(const FInputActionValue& Value) { ProcessGrip(false, 0); }
void AVirtualRealityPawn::UseLeft(const FInputActionValue& Value) { ProcessUse(true, Value.Get<float>()); }
void AVirtualRealityPawn::UseRight(const FInputActionValue& Value) { ProcessUse(false, Value.Get<float>()); }
void AVirtualRealityPawn::StopUseLeft(const FInputActionValue& Value) { ProcessUse(true, 0); }
void AVirtualRealityPawn::StopUseRight(const FInputActionValue& Value) { ProcessUse(false, 0); }

void AVirtualRealityPawn::AimTeleport(const FInputActionValue& Value)
{
    if (IsMenuOpen() || bTeleportCancelled)
    {
        return;
    }
    if (Value.Get<FVector2D>().Y > 0.65f)
    {
        bTeleportAiming = true;
    }
    TeleportMarker->SetVisibility(bTeleportAiming && bTeleportValid);
}

void AVirtualRealityPawn::FinishTeleport(const FInputActionValue& Value)
{
    if (!bTeleportCancelled && bTeleportAiming && !IsMenuOpen())
    {
        CommitTeleport();
    }
    bTeleportAiming = false;
    bTeleportCancelled = false;
    bTeleportValid = false;
    TeleportMarker->SetVisibility(false);
}

void AVirtualRealityPawn::HoldTeleport(const FInputActionValue& Value)
{
    if (!IsMenuOpen() && !bTeleportCancelled)
    {
        bTeleportAiming = true;
    }
}

void AVirtualRealityPawn::UpdateTeleport()
{
    FVector Start = LeftAim->GetComponentLocation();
    FVector Direction = LeftAim->GetForwardVector();
    if (!IsVirtualRealityActive())
    {
        Start = Camera->GetComponentLocation();
        Direction = Camera->GetForwardVector();
    }
    FPredictProjectilePathParams Parameters(8, Start, Direction * 650, 2, ECC_Visibility, this);
    Parameters.bTraceWithCollision = true;
    Parameters.OverrideGravityZ = -980;
    FPredictProjectilePathResult Result;
    UGameplayStatics::PredictProjectilePath(this, Parameters, Result);
    bTeleportValid = Result.HitResult.bBlockingHit && Result.HitResult.ImpactNormal.Z > 0.95f && !Result.HitResult.GetComponent()->IsSimulatingPhysics() && FMath::Abs(Result.HitResult.ImpactPoint.Z - GetActorLocation().Z) < 15;
    if (bTeleportValid)
    {
        TeleportDestination = Result.HitResult.ImpactPoint;
        TeleportMarker->SetWorldLocation(TeleportDestination + FVector(0, 0, 1));
    }
    TeleportMarker->SetVisibility(bTeleportValid);
}

void AVirtualRealityPawn::CommitTeleport()
{
    if (!bTeleportValid)
    {
        return;
    }
    FVector HeadOffset = Camera->GetComponentLocation() - GetActorLocation();
    HeadOffset.Z = 0;
    const FTransform PreviousPlayerTransform = GetActorTransform();
    SetActorLocation(TeleportDestination - HeadOffset);
    MoveHeldItemsWithPlayer(PreviousPlayerTransform);
    SetMovementBase(nullptr);
    LeftMotion = FHandMotionState();
    RightMotion = FHandMotionState();
}

void AVirtualRealityPawn::CancelTeleport()
{
    if (bTeleportAiming)
    {
        bTeleportCancelled = true;
    }
    bTeleportAiming = false;
    bTeleportValid = false;
    TeleportMarker->SetVisibility(false);
}

void AVirtualRealityPawn::Turn(const FInputActionValue& Value)
{
    const float Amount = Value.Get<float>();
    if (FMath::Abs(Amount) < 0.35f)
    {
        bTurnReady = true;
    }
    if (IsMenuOpen() || !bTurnReady || FMath::Abs(Amount) < 0.7f)
    {
        return;
    }
    const FTransform PreviousPlayerTransform = GetActorTransform();
    const FVector PreviousHeadPosition = Camera->GetComponentLocation();
    AddActorWorldRotation(FRotator(0, FMath::Sign(Amount) * 30, 0));
    FVector HeadCorrection = PreviousHeadPosition - Camera->GetComponentLocation();
    HeadCorrection.Z = 0;
    AddActorWorldOffset(HeadCorrection);
    MoveHeldItemsWithPlayer(PreviousPlayerTransform);
    bTurnReady = false;
    LeftMotion = FHandMotionState();
    RightMotion = FHandMotionState();
}

void AVirtualRealityPawn::FinishTurn(const FInputActionValue& Value) { bTurnReady = true; }
void AVirtualRealityPawn::MoveDesktop(const FInputActionValue& Value) { DesktopMovement = Value.Get<FVector2D>(); }
void AVirtualRealityPawn::StopMoveDesktop(const FInputActionValue& Value) { DesktopMovement = FVector2D::ZeroVector; }

void AVirtualRealityPawn::LookDesktop(const FInputActionValue& Value)
{
    if (IsVirtualRealityActive() || IsMenuOpen())
    {
        return;
    }
    if (const APlayerController* Player = Cast<APlayerController>(Controller))
    {
        if (Player->IsInputKeyDown(EKeys::RightMouseButton))
        {
            AddActorWorldRotation(FRotator(0, Value.Get<FVector2D>().X * 0.15f, 0));
            FRotator ViewRotation = Camera->GetRelativeRotation();
            ViewRotation.Pitch = FMath::Clamp(ViewRotation.Pitch + Value.Get<FVector2D>().Y * 0.15f, -75, 75);
            Camera->SetRelativeRotation(ViewRotation);
        }
    }
}

void AVirtualRealityPawn::ToggleMenu()
{
    if (APrototypePlayerController* Player = Cast<APrototypePlayerController>(Controller))
    {
        Player->ToggleMenu();
    }
}

void AVirtualRealityPawn::CancelInteraction()
{
    if (APrototypePlayerController* Player = Cast<APrototypePlayerController>(Controller))
    {
        Player->CancelInteraction();
    }
}

void AVirtualRealityPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    if (!InteractionMapping
        || !GrabLeftAction
        || !GrabRightAction
        || !UseLeftAction
        || !UseRightAction
        || !TeleportAction
        || !SnapTurnAction
        || !ToggleMenuAction
        || !CancelAction
        || !DesktopMoveAction
        || !DesktopLookAction
        || !TeleportHoldAction)
    {
        UKismetSystemLibrary::PrintString(this, TEXT("The pawn input configuration is incomplete"), true, true, FLinearColor::Red);
        return;
    }
    UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    APlayerController* Player = CastChecked<APlayerController>(Controller);
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer());
    InputSubsystem->AddMappingContext(InteractionMapping, 0);

    EnhancedInput->BindAction(GrabLeftAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::GrabLeft);
    EnhancedInput->BindAction(GrabLeftAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopGrabLeft);
    EnhancedInput->BindAction(GrabLeftAction, ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopGrabLeft);
    EnhancedInput->BindAction(GrabRightAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::GrabRight);
    EnhancedInput->BindAction(GrabRightAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopGrabRight);
    EnhancedInput->BindAction(GrabRightAction, ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopGrabRight);
    EnhancedInput->BindAction(UseLeftAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::UseLeft);
    EnhancedInput->BindAction(UseLeftAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopUseLeft);
    EnhancedInput->BindAction(UseLeftAction, ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopUseLeft);
    EnhancedInput->BindAction(UseRightAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::UseRight);
    EnhancedInput->BindAction(UseRightAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopUseRight);
    EnhancedInput->BindAction(UseRightAction, ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopUseRight);
    EnhancedInput->BindAction(TeleportAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::AimTeleport);
    EnhancedInput->BindAction(TeleportAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::FinishTeleport);
    EnhancedInput->BindAction(SnapTurnAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::Turn);
    EnhancedInput->BindAction(SnapTurnAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::FinishTurn);
    EnhancedInput->BindAction(SnapTurnAction, ETriggerEvent::Canceled, this, &AVirtualRealityPawn::FinishTurn);
    EnhancedInput->BindAction(ToggleMenuAction, ETriggerEvent::Started, this, &AVirtualRealityPawn::ToggleMenu);
    EnhancedInput->BindAction(CancelAction, ETriggerEvent::Started, this, &AVirtualRealityPawn::CancelInteraction);
    EnhancedInput->BindAction(DesktopMoveAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::MoveDesktop);
    EnhancedInput->BindAction(DesktopMoveAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopMoveDesktop);
    EnhancedInput->BindAction(DesktopMoveAction, ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopMoveDesktop);
    EnhancedInput->BindAction(DesktopLookAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::LookDesktop);
    EnhancedInput->BindAction(TeleportHoldAction, ETriggerEvent::Triggered, this, &AVirtualRealityPawn::HoldTeleport);
    EnhancedInput->BindAction(TeleportHoldAction, ETriggerEvent::Completed, this, &AVirtualRealityPawn::FinishTeleport);
}

void AVirtualRealityPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseAllItems();
    Super::EndPlay(EndPlayReason);
}
