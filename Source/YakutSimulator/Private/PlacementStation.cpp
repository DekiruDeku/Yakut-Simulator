#include "PlacementStation.h"

#include "Components/StaticMeshComponent.h"
#include "InteractionZoneComponent.h"
#include "UObject/ConstructorHelpers.h"

APlacementStation::APlacementStation()
{
    Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Surface"));
    SetRootComponent(Surface);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    Surface->SetStaticMesh(DefaultShape.Object);
    Surface->SetWorldScale3D(FVector(0.55f, 0.55f, 0.04f));
    AcceptanceZone = CreateDefaultSubobject<UInteractionZoneComponent>(TEXT("AcceptanceZone"));
    AcceptanceZone->SetupAttachment(Surface);
    AcceptanceZone->SetAbsolute(false, false, true);
    AcceptanceZone->SetRelativeLocation(FVector(0, 0, 230));
}
