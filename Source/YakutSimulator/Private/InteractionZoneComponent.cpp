#include "InteractionZoneComponent.h"

#include "Components/StaticMeshComponent.h"
#include "WorkItem.h"

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
    if (!Item || Item->bIsHeld)
    {
        return false;
    }
    if (!AcceptedItemType.IsNone() && Item->ItemType != AcceptedItemType)
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
    if (bSnapOnPlacement)
    {
        Item->Body->SetSimulatePhysics(false);
        Item->AttachToComponent(this, FAttachmentTransformRules::KeepWorldTransform);
        Item->SetActorLocationAndRotation(GetComponentLocation(), GetComponentRotation());
    }
    Item->OnItemAction.Broadcast(Item, EWorkAction::Placed);
    return true;
}
