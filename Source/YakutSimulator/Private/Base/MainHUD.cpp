#include "Base/MainHUD.h"

#include "Components/WidgetComponent.h"

AMainHUD::AMainHUD()
{
    SetActorHiddenInGame(false);
    SettingsDisplay = CreateDefaultSubobject<UWidgetComponent>(TEXT("SettingsDisplay"));
    SetRootComponent(SettingsDisplay);
    SettingsDisplay->SetDrawSize(FVector2D(720, 520));
    SettingsDisplay->SetRelativeScale3D(FVector(0.12f));
    SettingsDisplay->SetCollisionProfileName(TEXT("UI"));
    SettingsDisplay->SetVisibility(false);
    SettingsDisplay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
