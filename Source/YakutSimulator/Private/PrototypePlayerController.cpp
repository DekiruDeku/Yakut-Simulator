#include "PrototypePlayerController.h"

#include "Components/WidgetComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "PrototypeGameMode.h"
#include "PrototypeWidgets.h"
#include "TimerManager.h"
#include "VirtualRealityPawn.h"

void APrototypePlayerController::BeginPlay()
{
    Super::BeginPlay();
    GetWorldTimerManager().SetTimerForNextTick(this, &APrototypePlayerController::InitializeInterface);
}

void APrototypePlayerController::InitializeInterface()
{
    if (IsMainMenuMap())
    {
        SetMenuOpen(true);
    }
    else if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
    {
        TSubclassOf<UScenarioPanelWidget> PanelClass = LoadClass<UScenarioPanelWidget>(nullptr, TEXT("/Game/Prototype/UI/WBP_ScenarioPanel.WBP_ScenarioPanel_C"));
        if (!PanelClass)
        {
            PanelClass = UScenarioPanelWidget::StaticClass();
        }
        ScenarioPanel = CreateWidget<UScenarioPanelWidget>(this, PanelClass);
        ScenarioPanel->AddToViewport();
        ScenarioPanel->SetPositionInViewport(FVector2D(24, 24));
        ScenarioPanel->SetDesiredSizeInViewport(FVector2D(480, 310));
        ScenarioPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

bool APrototypePlayerController::IsMainMenuMap() const
{
    return UGameplayStatics::GetCurrentLevelName(this, true) == TEXT("L_MainMenu");
}

void APrototypePlayerController::ToggleMenu()
{
    if (IsMainMenuMap() && bMenuOpen)
    {
        return;
    }
    SetMenuOpen(!bMenuOpen);
}

void APrototypePlayerController::SetMenuOpen(bool bOpen)
{
    AVirtualRealityPawn* PlayerPawn = Cast<AVirtualRealityPawn>(GetPawn());
    if (!PlayerPawn)
    {
        return;
    }
    bMenuOpen = bOpen;
    PlayerPawn->CancelTeleport();
    if (bOpen && !Menu)
    {
        TSubclassOf<UMainMenuWidget> MenuClass = LoadClass<UMainMenuWidget>(nullptr, TEXT("/Game/Prototype/UI/WBP_MainMenu.WBP_MainMenu_C"));
        if (!MenuClass)
        {
            MenuClass = UMainMenuWidget::StaticClass();
        }
        Menu = CreateWidget<UMainMenuWidget>(this, MenuClass);
    }
    if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
    {
        PlayerPawn->ShowMenu(Menu, bOpen);
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
    }
    else if (bOpen)
    {
        Menu->AddToViewport(10);
        bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }
    else
    {
        Menu->RemoveFromParent();
        bShowMouseCursor = false;
        SetInputMode(FInputModeGameOnly());
    }
}

void APrototypePlayerController::CancelInteraction()
{
    if (bMenuOpen)
    {
        if (!IsMainMenuMap())
        {
            SetMenuOpen(false);
        }
        return;
    }
    if (AVirtualRealityPawn* PlayerPawn = Cast<AVirtualRealityPawn>(GetPawn()))
    {
        PlayerPawn->CancelTeleport();
    }
}

bool APrototypePlayerController::OpenScenarioMap(TSoftObjectPtr<UWorld> Map)
{
    const APrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    const bool bListed = GameMode && GameMode->ScenarioMaps.ContainsByPredicate([Map](const FScenarioMapEntry& Entry)
    {
        return Entry.Map == Map;
    });
    if (!bListed || !FPackageName::DoesPackageExist(Map.ToSoftObjectPath().GetLongPackageName()))
    {
        MenuError = FText::FromString(TEXT("Карта недоступна. Проверьте настройки списка карт."));
        if (Menu)
        {
            Menu->RefreshMenu();
        }
        return false;
    }
    if (AVirtualRealityPawn* PlayerPawn = Cast<AVirtualRealityPawn>(GetPawn()))
    {
        PlayerPawn->ReleaseAllItems();
    }
    UGameplayStatics::OpenLevel(this, FName(Map.ToSoftObjectPath().GetLongPackageName()));
    return true;
}

void APrototypePlayerController::ReturnToMainMenu()
{
    if (AVirtualRealityPawn* PlayerPawn = Cast<AVirtualRealityPawn>(GetPawn()))
    {
        PlayerPawn->ReleaseAllItems();
    }
    UGameplayStatics::OpenLevel(this, TEXT("/Game/Prototype/Maps/L_MainMenu"));
}
