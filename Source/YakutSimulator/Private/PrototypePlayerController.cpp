#include "PrototypePlayerController.h"

#include "Components/WidgetComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "PrototypeGameMode.h"
#include "Blueprint/UserWidget.h"
#include "ScenarioDirector.h"
#include "UObject/ConstructorHelpers.h"
#include "VirtualRealityPawn.h"

APrototypePlayerController::APrototypePlayerController()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultMainMenuClass(TEXT("/Game/Prototype/UI/WBP_MainMenu"));
    MainMenuClass = DefaultMainMenuClass.Class;
    static ConstructorHelpers::FClassFinder<UUserWidget> DefaultScenarioPanelClass(TEXT("/Game/Prototype/UI/WBP_ScenarioPanel"));
    ScenarioPanelClass = DefaultScenarioPanelClass.Class;
}

void APrototypePlayerController::BeginPlay()
{
    Super::BeginPlay();
    if (GetPawn())
    {
        InitializeInterface();
    }
}

void APrototypePlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    if (HasActorBegunPlay())
    {
        InitializeInterface();
    }
}

void APrototypePlayerController::InitializeInterface()
{
    if (IsMainMenuMap())
    {
        SetMenuOpen(true);
    }
    else
    {
        InitializeScenarioPanel();
    }
}

void APrototypePlayerController::InitializeScenarioPanel()
{
    const APrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    AVirtualRealityPawn* PlayerPawn = Cast<AVirtualRealityPawn>(GetPawn());
    if (!GameMode || !PlayerPawn || !ScenarioPanelClass)
    {
        return;
    }
    ScenarioDirector = GameMode->FindScenarioDirector();
    if (!ScenarioDirector)
    {
        return;
    }
    if (!ScenarioPanel)
    {
        ScenarioPanel = CreateWidget<UUserWidget>(this, ScenarioPanelClass);
    }
    if (UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
    {
        PlayerPawn->ScenarioDisplay->SetOwnerPlayer(GetLocalPlayer());
        PlayerPawn->ScenarioDisplay->SetWidget(ScenarioPanel);
        PlayerPawn->ScenarioDisplay->SetVisibility(true);
    }
    else if (!ScenarioPanel->IsInViewport())
    {
        ScenarioPanel->AddToViewport();
        ScenarioPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

bool APrototypePlayerController::IsMainMenuMap() const
{
    const APrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!GameMode || GameMode->MainMenuMap.IsNull())
    {
        return false;
    }
    return UWorld::RemovePIEPrefix(GetWorld()->GetPackage()->GetName()) == GameMode->MainMenuMap.ToSoftObjectPath().GetLongPackageName();
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
        if (!MainMenuClass)
        {
            bMenuOpen = false;
            return;
        }
        Menu = CreateWidget<UUserWidget>(this, MainMenuClass);
    }
    PlayerPawn->SetItemUse(true, false);
    PlayerPawn->SetItemUse(false, false);
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
        OnMenuErrorChanged.Broadcast();
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
    const APrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (!GameMode || !FPackageName::DoesPackageExist(GameMode->MainMenuMap.ToSoftObjectPath().GetLongPackageName()))
    {
        MenuError = FText::FromString(TEXT("Главная карта недоступна. Проверьте настройки игрового режима."));
        OnMenuErrorChanged.Broadcast();
        return;
    }
    if (AVirtualRealityPawn* PlayerPawn = Cast<AVirtualRealityPawn>(GetPawn()))
    {
        PlayerPawn->ReleaseAllItems();
    }
    UGameplayStatics::OpenLevel(this, FName(GameMode->MainMenuMap.ToSoftObjectPath().GetLongPackageName()));
}
