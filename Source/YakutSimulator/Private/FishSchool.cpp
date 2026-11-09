#include "FishSchool.h"

#include "Components/InstancedStaticMeshComponent.h"

AFishSchool::AFishSchool()
{
    FishVisuals = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FishVisuals"));
    SetRootComponent(FishVisuals);
    FishVisuals->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AFishSchool::BeginPlay()
{
    Super::BeginPlay();
    if (!Species || !Species->Mesh)
    {
        return;
    }
    FishVisuals->SetStaticMesh(Species->Mesh);
    FishVisuals->SetMaterial(0, Species->Material);
    FRandomStream Distribution(731);
    for (int32 FishIndex = 0; FishIndex < 24; ++FishIndex)
    {
        FishVisuals->AddInstance(FTransform(FRotator(0, Distribution.FRandRange(-25, 25), 0), FVector(Distribution.FRandRange(-Radius, Radius), Distribution.FRandRange(-Radius, Radius), Distribution.FRandRange(-60, 60))));
    }
}

int32 AFishSchool::ConsumeFish(int32 RequestedCount)
{
    const int32 Taken = FMath::Clamp(RequestedCount, 0, RemainingFish);
    RemainingFish -= Taken;
    return Taken;
}
