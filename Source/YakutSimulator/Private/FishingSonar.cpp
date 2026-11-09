#include "FishingSonar.h"

#include "EngineUtils.h"
#include "FishSchool.h"
#include "PhysicalLever.h"
#include "TrawlerVessel.h"

AFishingSonar::AFishingSonar()
{
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AFishingSonar::BeginPlay()
{
    Super::BeginPlay();
    if (ScanLever)
    {
        ScanLever->OnValueChanged.AddUniqueDynamic(this, &AFishingSonar::ProcessScanLever);
    }
}

void AFishingSonar::Scan()
{
    FoundSchools.Reset();
    SelectedSchool = nullptr;
    if (!Vessel)
    {
        return;
    }
    for (TActorIterator<AFishSchool> SchoolIterator(GetWorld()); SchoolIterator; ++SchoolIterator)
    {
        if (SchoolIterator->Species && SchoolIterator->RemainingFish > 0 && FVector::DistSquared2D(Vessel->GetActorLocation(), SchoolIterator->GetActorLocation()) <= FMath::Square(DetectionRange))
        {
            FoundSchools.Add(*SchoolIterator);
        }
    }
    FoundSchools.Sort([this](const AFishSchool& First, const AFishSchool& Second)
    {
        return FVector::DistSquared2D(Vessel->GetActorLocation(), First.GetActorLocation()) < FVector::DistSquared2D(Vessel->GetActorLocation(), Second.GetActorLocation());
    });
    if (!FoundSchools.IsEmpty())
    {
        SelectedSchool = FoundSchools[0];
    }
    OnSonarChanged.Broadcast();
}

void AFishingSonar::SelectNextSchool()
{
    if (FoundSchools.IsEmpty())
    {
        Scan();
        return;
    }
    SelectedSchool = FoundSchools[(FoundSchools.Find(SelectedSchool) + 1) % FoundSchools.Num()];
    OnSonarChanged.Broadcast();
}

FText AFishingSonar::GetReadout() const
{
    if (!Vessel || !IsValid(SelectedSchool))
    {
        return FText::FromString(TEXT("Сонар: цель не выбрана. Выполните сканирование."));
    }
    const FVector Direction = SelectedSchool->GetActorLocation() - Vessel->GetActorLocation();
    const float Bearing = FMath::FindDeltaAngleDegrees(Vessel->GetActorRotation().Yaw, Direction.Rotation().Yaw);
    return FText::FromString(FString::Format(TEXT("{0} | {1} м | пеленг {2}° | глубина {3} м"), {
        SelectedSchool->Species->DisplayName.ToString(), FMath::RoundToInt(Direction.Size2D() / 100), FMath::RoundToInt(Bearing), FMath::RoundToInt((WaterHeight - SelectedSchool->GetActorLocation().Z) / 100) }));
}

void AFishingSonar::ProcessScanLever(float Value)
{
    if (FMath::Abs(Value) < 0.2f)
    {
        bScanArmed = true;
    }
    if (bScanArmed && Value > 0.6f)
    {
        bScanArmed = false;
        Scan();
    }
}

void AFishingSonar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (IsValid(ScanLever))
    {
        ScanLever->OnValueChanged.RemoveDynamic(this, &AFishingSonar::ProcessScanLever);
    }
    Super::EndPlay(EndPlayReason);
}
