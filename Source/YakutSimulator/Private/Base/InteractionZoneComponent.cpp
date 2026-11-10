#include "Base/InteractionZoneComponent.h"

#include "Components/StaticMeshComponent.h"
#include "Base/WorkItem.h"

UInteractionZoneComponent::UInteractionZoneComponent()
{
    InitBoxExtent(FVector(25, 25, 20));
    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    SetGenerateOverlapEvents(true);
}

bool UInteractionZoneComponent::CanAcceptItem(AWorkItem* Item) const
{
    if (!Item || Item->bIsHeld || Item->bIsAnchored || bAcceptingItem)
    {
        return false;
    }
    if (!AcceptedItemType.IsNone() && Item->ItemType != AcceptedItemType)
    {
        return false;
    }
    if (AcceptanceRule.IsBound() && !AcceptanceRule.Execute(Item))
    {
        return false;
    }
    return FMath::PointBoxIntersection(GetComponentTransform().InverseTransformPosition(Item->GetActorLocation()), FBox(-GetUnscaledBoxExtent(), GetUnscaledBoxExtent()));
}

bool UInteractionZoneComponent::TryAcceptItem(AWorkItem* Item)
{
    if (!CanAcceptItem(Item))
    {
        return false;
    }
    TGuardValue<bool> AcceptanceGuard(bAcceptingItem, true);
    if (bSnapOnPlacement)
    {
        Item->Body->SetSimulatePhysics(false);
        Item->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
        Item->SetActorLocationAndRotation(GetComponentLocation(), GetComponentRotation());
    }
    Item->OnItemAction.Broadcast(Item, EWorkAction::Placed);
    OnItemAccepted.Broadcast(Item, this);
    return true;
}

void UInteractionZoneComponent::BeginPlay()
{
    Super::BeginPlay();
    OnComponentBeginOverlap.AddUniqueDynamic(this, &UInteractionZoneComponent::ReceiveOverlap);
}

void UInteractionZoneComponent::ReceiveOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (bAcceptReleasedItemsOnOverlap)
    {
        TryAcceptItem(Cast<AWorkItem>(OtherActor));
    }
}
