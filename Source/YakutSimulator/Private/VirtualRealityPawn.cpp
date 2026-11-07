#include "VirtualRealityPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/WidgetInteractionComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/OverlapResult.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "PrototypePlayerController.h"
#include "PrototypeWidgets.h"
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
    ScenarioDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("ScenarioDisplay"));
    ScenarioDisplay->SetupAttachment(TrackingOrigin);
    ScenarioDisplay->SetDrawSize(FVector2D(620, 400));
    ScenarioDisplay->SetRelativeLocation(FVector(140, -105, 145));
    ScenarioDisplay->SetRelativeRotation(FRotator(0, 145, 0));
    ScenarioDisplay->SetRelativeScale3D(FVector(0.09));
    ScenarioDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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
        TSubclassOf<UScenarioPanelWidget> PanelClass = LoadClass<UScenarioPanelWidget>(nullptr, TEXT("/Game/Prototype/UI/WBP_ScenarioPanel.WBP_ScenarioPanel_C"));
        ScenarioDisplay->SetWidgetClass(PanelClass);
        ScenarioDisplay->SetOwnerPlayer(GetWorld()->GetFirstLocalPlayerFromController());
        ScenarioDisplay->SetVisibility(UGameplayStatics::GetCurrentLevelName(this, true) != TEXT("L_MainMenu"));
    }
    else
    {
        Camera->SetRelativeLocation(FVector(0, 0, 165));
        ScenarioDisplay->SetVisibility(false);
        LeftPointer->bEnableHitTesting = false;
        RightPointer->bEnableHitTesting = false;
    }
}

void AVirtualRealityPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsVirtualRealityActive())
    {
        const FTransform View = Camera->GetComponentTransform();
        LeftHand->SetWorldLocationAndRotation(View.TransformPosition(FVector(65, -18, -40)), View.GetRotation());
        RightHand->SetWorldLocationAndRotation(View.TransformPosition(FVector(65, 18, -40)), View.GetRotation());
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
        if (Candidate && !Candidate->bIsHeld)
        {
            const float Distance = FVector::DistSquared(Candidate->GetActorLocation(), Hand->GetComponentLocation());
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
}

void AVirtualRealityPawn::ReleaseAllItems()
{
    SetItemUse(true, false);
    SetItemUse(false, false);
    ReleaseItem(true);
    ReleaseItem(false);
}

void AVirtualRealityPawn::ShowMenu(UMainMenuWidget* Menu, bool bShow)
{
    SetItemUse(true, false);
    SetItemUse(false, false);
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
    Ray->SetVisibility(IsMenuOpen() && IsVirtualRealityActive());
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
    bTeleportValid = Result.HitResult.bBlockingHit && Result.HitResult.ImpactNormal.Z > 0.95f && Result.HitResult.GetComponent()->GetCollisionObjectType() == ECC_WorldStatic && FMath::Abs(Result.HitResult.ImpactPoint.Z - GetActorLocation().Z) < 15;
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
    SetActorLocation(TeleportDestination - HeadOffset);
    LeftMotion.bInitialized = false;
    RightMotion.bInitialized = false;
    LeftMotion.LinearVelocity = FVector::ZeroVector;
    RightMotion.LinearVelocity = FVector::ZeroVector;
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
    const FVector PreviousHeadPosition = Camera->GetComponentLocation();
    AddActorWorldRotation(FRotator(0, FMath::Sign(Amount) * 30, 0));
    FVector HeadCorrection = PreviousHeadPosition - Camera->GetComponentLocation();
    HeadCorrection.Z = 0;
    AddActorWorldOffset(HeadCorrection);
    bTurnReady = false;
    LeftMotion.bInitialized = false;
    RightMotion.bInitialized = false;
    LeftMotion.LinearVelocity = FVector::ZeroVector;
    RightMotion.LinearVelocity = FVector::ZeroVector;
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
        SetItemUse(true, false);
        SetItemUse(false, false);
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
    UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
    APlayerController* Player = CastChecked<APlayerController>(Controller);
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(Player->GetLocalPlayer());
    UInputMappingContext* Mapping = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Prototype/Input/IMC_Interaction.IMC_Interaction"));
    if (!Mapping)
    {
        return;
    }
    InputSubsystem->AddMappingContext(Mapping, 0);

    const TArray<FString> ActionNames = { TEXT("IA_GrabLeft"), TEXT("IA_GrabRight"), TEXT("IA_UseLeft"), TEXT("IA_UseRight"), TEXT("IA_Teleport"), TEXT("IA_SnapTurn"), TEXT("IA_ToggleMenu"), TEXT("IA_Cancel"), TEXT("IA_DesktopMove"), TEXT("IA_DesktopLook"), TEXT("IA_TeleportHold") };
    for (const FString& ActionName : ActionNames)
    {
        InputActions.Add(LoadObject<UInputAction>(nullptr, *FString::Format(TEXT("/Game/Prototype/Input/{0}.{0}"), { ActionName })));
    }
    EnhancedInput->BindAction(InputActions[0], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::GrabLeft);
    EnhancedInput->BindAction(InputActions[0], ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopGrabLeft);
    EnhancedInput->BindAction(InputActions[0], ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopGrabLeft);
    EnhancedInput->BindAction(InputActions[1], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::GrabRight);
    EnhancedInput->BindAction(InputActions[1], ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopGrabRight);
    EnhancedInput->BindAction(InputActions[1], ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopGrabRight);
    EnhancedInput->BindAction(InputActions[2], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::UseLeft);
    EnhancedInput->BindAction(InputActions[2], ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopUseLeft);
    EnhancedInput->BindAction(InputActions[2], ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopUseLeft);
    EnhancedInput->BindAction(InputActions[3], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::UseRight);
    EnhancedInput->BindAction(InputActions[3], ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopUseRight);
    EnhancedInput->BindAction(InputActions[3], ETriggerEvent::Canceled, this, &AVirtualRealityPawn::StopUseRight);
    EnhancedInput->BindAction(InputActions[4], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::AimTeleport);
    EnhancedInput->BindAction(InputActions[4], ETriggerEvent::Completed, this, &AVirtualRealityPawn::FinishTeleport);
    EnhancedInput->BindAction(InputActions[5], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::Turn);
    EnhancedInput->BindAction(InputActions[5], ETriggerEvent::Completed, this, &AVirtualRealityPawn::FinishTurn);
    EnhancedInput->BindAction(InputActions[6], ETriggerEvent::Started, this, &AVirtualRealityPawn::ToggleMenu);
    EnhancedInput->BindAction(InputActions[7], ETriggerEvent::Started, this, &AVirtualRealityPawn::CancelInteraction);
    EnhancedInput->BindAction(InputActions[8], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::MoveDesktop);
    EnhancedInput->BindAction(InputActions[8], ETriggerEvent::Completed, this, &AVirtualRealityPawn::StopMoveDesktop);
    EnhancedInput->BindAction(InputActions[9], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::LookDesktop);
    EnhancedInput->BindAction(InputActions[10], ETriggerEvent::Triggered, this, &AVirtualRealityPawn::HoldTeleport);
    EnhancedInput->BindAction(InputActions[10], ETriggerEvent::Completed, this, &AVirtualRealityPawn::FinishTeleport);
}

void AVirtualRealityPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ReleaseAllItems();
    Super::EndPlay(EndPlayReason);
}
