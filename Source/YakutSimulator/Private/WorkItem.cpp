#include "WorkItem.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "InteractionZoneComponent.h"
#include "UObject/ConstructorHelpers.h"

AWorkItem::AWorkItem()
{
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    SetRootComponent(Body);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultShape(TEXT("/Engine/BasicShapes/Cube.Cube"));
    Body->SetStaticMesh(DefaultShape.Object);
    Body->SetRelativeScale3D(FVector(0.12));
    Body->SetCollisionProfileName(TEXT("PhysicsActor"));
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Body->SetGenerateOverlapEvents(true);
    Body->SetSimulatePhysics(true);
    Body->SetUseCCD(true);
    Body->SetLinearDamping(0.1f);
    Body->SetAngularDamping(0.2f);

    GripPoint = CreateDefaultSubobject<USceneComponent>(TEXT("GripPoint"));
    GripPoint->SetupAttachment(Body);
    UseIndicator = CreateDefaultSubobject<UPointLightComponent>(TEXT("UseIndicator"));
    UseIndicator->SetupAttachment(Body);
    UseIndicator->SetRelativeLocation(FVector(0, 0, 80));
    UseIndicator->SetLightColor(FLinearColor(0.15f, 0.7f, 1.0f));
    UseIndicator->SetIntensity(1200);
    UseIndicator->SetAttenuationRadius(120);
    UseIndicator->SetVisibility(false);
    DisplayName = FText::FromString(TEXT("Учебный предмет"));
}

bool AWorkItem::TryGrab(USceneComponent* Hand)
{
    if (bIsHeld || !Hand)
    {
        return false;
    }
    EndUse();
    Body->SetSimulatePhysics(false);
    AttachToComponent(Hand, FAttachmentTransformRules::KeepWorldTransform);
    const FTransform GripLocal = GripPoint->GetComponentTransform().GetRelativeTransform(GetActorTransform());
    SetActorRelativeRotation(GripLocal.GetRotation().Inverse());
    SetActorRelativeLocation(-GripLocal.GetLocation());
    bIsHeld = true;
    OnItemAction.Broadcast(this, EWorkAction::Grabbed);
    return true;
}

void AWorkItem::Release(const FVector& LinearVelocity, const FVector& AngularVelocity)
{
    if (!bIsHeld)
    {
        return;
    }
    EndUse();
    DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    bIsHeld = false;
    Body->SetSimulatePhysics(true);
    Body->SetPhysicsLinearVelocity(LinearVelocity.GetClampedToMaxSize(1800));
    Body->SetPhysicsAngularVelocityInRadians(AngularVelocity.GetClampedToMaxSize(30));
    OnItemAction.Broadcast(this, EWorkAction::Released);

    TArray<UPrimitiveComponent*> OverlappingComponents;
    Body->GetOverlappingComponents(OverlappingComponents);
    for (UPrimitiveComponent* OverlappingComponent : OverlappingComponents)
    {
        if (UInteractionZoneComponent* Zone = Cast<UInteractionZoneComponent>(OverlappingComponent))
        {
            if (Zone->TryAcceptItem(this))
            {
                break;
            }
        }
    }
}

void AWorkItem::BeginUse()
{
    if (!bIsHeld || bIsUsing)
    {
        return;
    }
    bIsUsing = true;
    ++UseCount;
    ReceiveUseStarted();
    OnItemAction.Broadcast(this, EWorkAction::Used);
}

void AWorkItem::EndUse()
{
    if (!bIsUsing)
    {
        return;
    }
    bIsUsing = false;
    ReceiveUseEnded();
}

void AWorkItem::ReceiveUseStarted_Implementation()
{
    UseIndicator->SetVisibility(true);
}

void AWorkItem::ReceiveUseEnded_Implementation()
{
    UseIndicator->SetVisibility(false);
}

void AWorkItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndUse();
    Super::EndPlay(EndPlayReason);
}
