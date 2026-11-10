#include "Base/MainGameInstance.h"

#include "Base/MainSettingsSave.h"
#include "Base/VirtualRealityPawn.h"
#include "Kismet/GameplayStatics.h"

void UMainGameInstance::Init()
{
    Super::Init();
    if (UGameplayStatics::DoesSaveGameExist(TEXT("MainSettings"), 0))
    {
        SettingsSave = Cast<UMainSettingsSave>(UGameplayStatics::LoadGameFromSlot(TEXT("MainSettings"), 0));
    }
    if (!SettingsSave)
    {
        SettingsSave = Cast<UMainSettingsSave>(UGameplayStatics::CreateSaveGameObject(SettingsSaveClass));
    }
    if (SettingsSave)
    {
        if (!FMath::IsFinite(SettingsSave->CameraHeightOffset))
        {
            SettingsSave->CameraHeightOffset = 0.0f;
        }
        SettingsSave->CameraHeightOffset = FMath::Clamp(SettingsSave->CameraHeightOffset, -100.0f, 100.0f);
    }
}

bool UMainGameInstance::SetCameraHeightOffset(float OffsetCentimeters)
{
    if (!SettingsSave || !FMath::IsFinite(OffsetCentimeters))
    {
        return false;
    }
    SettingsSave->CameraHeightOffset = FMath::Clamp(OffsetCentimeters, -100.0f, 100.0f);
    if (AVirtualRealityPawn* Player = Cast<AVirtualRealityPawn>(UGameplayStatics::GetPlayerPawn(this, 0)))
    {
        Player->SetCameraHeightOffset(SettingsSave->CameraHeightOffset);
    }
    return UGameplayStatics::SaveGameToSlot(SettingsSave, TEXT("MainSettings"), 0);
}
