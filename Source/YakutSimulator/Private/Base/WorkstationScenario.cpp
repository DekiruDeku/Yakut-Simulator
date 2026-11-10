#include "Base/WorkstationScenario.h"

#include "Components/WidgetComponent.h"
#include "Base/PhysicalLever.h"

AWorkstationScenario::AWorkstationScenario()
{
    PrimaryActorTick.bCanEverTick = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    ConsoleDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("ConsoleDisplay"));
    ConsoleDisplay->SetupAttachment(Root);
    ConsoleDisplay->SetDrawSize(FVector2D(1100, 850));
    ConsoleDisplay->SetRelativeScale3D(FVector(0.075));
    ConsoleDisplay->SetCollisionProfileName(TEXT("UI"));
}

void AWorkstationScenario::BeginPlay()
{
    Super::BeginPlay();
    if (CommandControl)
    {
        CommandControl->OnItemAction.AddUniqueDynamic(this, &AWorkstationScenario::ReceiveCommand);
    }
    InitializeConsole(GetWorld()->GetFirstPlayerController());
}

void AWorkstationScenario::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    DisplayAccumulator += DeltaSeconds;
    if (DisplayAccumulator >= 0.25f)
    {
        DisplayAccumulator = 0;
        OnScenarioChanged.Broadcast();
    }
}

FText AWorkstationScenario::GetReadout() const
{
    return GetCurrentInstruction();
}

void AWorkstationScenario::ConfirmCommand()
{
}

void AWorkstationScenario::RestartWorkstation()
{
    RestartScenario();
}

void AWorkstationScenario::ReceiveCommand(AWorkItem* Item, EWorkAction Action)
{
    if (Action == EWorkAction::Used)
    {
        ConfirmCommand();
    }
}
