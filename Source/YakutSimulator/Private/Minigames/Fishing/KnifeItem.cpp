#include "Minigames/Fishing/KnifeItem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Minigames/Fishing/FishItem.h"

AKnifeItem::AKnifeItem()
{
    ItemType = TEXT("Knife");
    DisplayName = FText::FromString(TEXT("Нож"));
    Body->SetRelativeScale3D(FVector::OneVector);
    BladeTip = CreateDefaultSubobject<USceneComponent>(TEXT("BladeTip"));
    BladeTip->SetupAttachment(Body);
    BladeTip->SetRelativeLocation(FVector(24, 0, 0));
}

void AKnifeItem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const FVector Position = BladeTip->GetComponentLocation();
    if (!bIsHeld || !bIsUsing)
    {
        if (IsValid(ContactFish))
        {
            ContactFish->ResetKnifeContact(this);
        }
        ContactFish = nullptr;
        return;
    }
    TArray<FOverlapResult> Contacts;
    FCollisionQueryParams Query;
    Query.AddIgnoredActor(this);
    GetWorld()->OverlapMultiByObjectType(Contacts, Position, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldDynamic), FCollisionShape::MakeSphere(4), Query);
    AFishItem* ClosestFish = nullptr;
    float ClosestDistance = MAX_flt;
    for (const FOverlapResult& Contact : Contacts)
    {
        if (AFishItem* Fish = Cast<AFishItem>(Contact.GetActor()))
        {
            const float Distance = FVector::DistSquared(Fish->GetActorLocation(), Position);
            if (Fish->bOnProcessingBench && Distance < ClosestDistance)
            {
                ClosestFish = Fish;
                ClosestDistance = Distance;
            }
        }
    }
    if (ContactFish != ClosestFish && IsValid(ContactFish))
    {
        ContactFish->ResetKnifeContact(this);
    }
    ContactFish = ClosestFish;
    if (ContactFish && ContactFish->ApplyKnifeMovement(this, Position))
    {
        Sharpness = FMath::Max(0.0f, Sharpness - 0.12f);
    }
}
