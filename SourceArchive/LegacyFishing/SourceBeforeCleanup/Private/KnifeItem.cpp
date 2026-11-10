#include "KnifeItem.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "FishItem.h"
#include "ScenarioDirector.h"
#include "FishingStations.h"
#include "InteractionZoneComponent.h"

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
        bBladeInitialized = false;
        return;
    }
    if (!bBladeInitialized)
    {
        PreviousBladePosition = Position;
        bBladeInitialized = true;
    }
    TArray<FOverlapResult> Contacts;
    FCollisionQueryParams Query;
    Query.AddIgnoredActor(this);
    GetWorld()->OverlapMultiByObjectType(Contacts, Position, FQuat::Identity, FCollisionObjectQueryParams(ECC_WorldDynamic), FCollisionShape::MakeSphere(4), Query);
    for (const FOverlapResult& Contact : Contacts)
    {
        if (AFishItem* Fish = Cast<AFishItem>(Contact.GetActor()))
        {
            if (Fish->ApplyKnifeMovement(this, Position))
            {
                Sharpness = FMath::Max(0.0f, Sharpness - 0.12f);
            }
        }
        else if (AKnifeSharpeningStation* Stone = Cast<AKnifeSharpeningStation>(Contact.GetActor()))
        {
            const FVector Travel = Position - PreviousBladePosition - Stone->GetVelocity() * DeltaSeconds;
            Sharpen(FMath::Min(Travel.Size(), 5.0f));
            if (Sharpness >= 0.95f && Stone->Scenario)
            {
                if (Stone->Scenario->GetRecordedResult(TEXT("PrepareKnife")) == 0)
                {
                    Stone->Scenario->ReportResult(TEXT("PrepareKnife"));
                }
            }
        }
    }
    PreviousBladePosition = Position;
}

void AKnifeItem::Sharpen(float TravelDistance)
{
    Sharpness = FMath::Clamp(Sharpness + TravelDistance / 120, 0.0f, 1.0f);
}
