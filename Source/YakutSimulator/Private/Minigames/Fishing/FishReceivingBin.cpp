#include "Minigames/Fishing/FishReceivingBin.h"

#include "Components/StaticMeshComponent.h"
#include "Minigames/Fishing/FishItem.h"
#include "Base/InteractionZoneComponent.h"

AFishReceivingBin::AFishReceivingBin()
{
    Intake->AcceptedItemType = TEXT("Fish");
    Intake->SetBoxExtent(FVector(28, 22, 16));
    Intake->SetRelativeLocation(FVector(0, 0, 8));
}

bool AFishReceivingBin::CanReceiveItem(AWorkItem* Item)
{
    const AFishItem* Fish = Cast<AFishItem>(Item);
    return Fish && Fish->ProductState == EFishProductState::Processed && Contents.Num() < Capacity && !Contents.Contains(Item);
}

void AFishReceivingBin::ReceiveItem(AWorkItem* Item, UInteractionZoneComponent* Zone)
{
    AFishItem* Fish = CastChecked<AFishItem>(Item);
    Fish->ProductState = EFishProductState::Packed;
    Fish->bOnProcessingBench = false;
    Item->SetActorRelativeLocation(FVector(0, 0, Contents.Num() * 4));
    Item->Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Contents.Add(Item);
}
