#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Font.h"
#include "Factories/TrueTypeFontFactory.h"
#include "FileHelpers.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/Package.h"

namespace
{
void PrepareFishingTextAssets()
{
    const TCHAR* FontPath = TEXT("/Game/Fishing/UI/F_FishingCyrillic.F_FishingCyrillic");
    const TCHAR* MaterialPath = TEXT("/Game/Fishing/UI/MI_FishingText.MI_FishingText");
    UFont* Font = LoadObject<UFont>(nullptr, FontPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!Font)
    {
        UTrueTypeFontFactory* Factory = NewObject<UTrueTypeFontFactory>();
        Factory->bPropertiesConfigured = true;
        Factory->bFontSelected = true;
        FFontImportOptionsData& Options = Factory->ImportOptions->Data;
        Options.FontName = TEXT("Arial");
        Options.Height = 32;
        Options.UnicodeRange = TEXT("0400-04FF,2190-2193");
        Options.bIncludeASCIIRange = true;
        Options.bUseDistanceFieldAlpha = true;
        Options.DistanceFieldScaleFactor = 4;
        Options.TexturePageWidth = Options.TexturePageMaxHeight = 2048;
        UPackage* Package = CreatePackage(TEXT("/Game/Fishing/UI/F_FishingCyrillic"));
        Font = Cast<UFont>(Factory->FactoryCreateNew(UFont::StaticClass(), Package, TEXT("F_FishingCyrillic"), RF_Public | RF_Standalone, nullptr, GWarn));
        if (!ensureAlways(Font)) return;
        FAssetRegistryModule::AssetCreated(Font);
        Package->MarkPackageDirty();
    }
    if (!ensureAlways(Font->Textures.Num() == 1 && Font->RemapChar(TCHAR(0x0410)) != UFont::NULLCHARACTER &&
        Font->RemapChar(TCHAR(0x044F)) != UFont::NULLCHARACTER && Font->RemapChar(TCHAR(0x0451)) != UFont::NULLCHARACTER)) return;

    UMaterialInstanceConstant* Material = LoadObject<UMaterialInstanceConstant>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!Material)
    {
        UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultTextMaterialOpaque.DefaultTextMaterialOpaque"));
        if (!ensureAlways(Parent)) return;
        TArray<FMaterialParameterInfo> Parameters;
        TArray<FGuid> Ids;
        Parent->GetAllFontParameterInfo(Parameters, Ids);
        if (!ensureAlways(!Parameters.IsEmpty())) return;
        UPackage* Package = CreatePackage(TEXT("/Game/Fishing/UI/MI_FishingText"));
        Material = NewObject<UMaterialInstanceConstant>(Package, TEXT("MI_FishingText"), RF_Public | RF_Standalone);
        Material->SetParentEditorOnly(Parent);
        for (const FMaterialParameterInfo& Parameter : Parameters) Material->SetFontParameterValueEditorOnly(Parameter, Font, 0);
        Material->PostEditChange();
        FAssetRegistryModule::AssetCreated(Material);
        Package->MarkPackageDirty();
    }
    const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages({Font->GetOutermost(), Material->GetOutermost()}, false);
    if (ensureAlways(bSaved)) UE_LOG(LogTemp, Display, TEXT("FISHING_TEXT_READY: %s; Cyrillic verified; one atlas page"), FontPath);
}

FAutoConsoleCommand PrepareTextCommand(TEXT("Fishing.PrepareTextAssets"),
    TEXT("Create the prototype Cyrillic font and text material without modifying existing assets."),
    FConsoleCommandDelegate::CreateStatic(&PrepareFishingTextAssets));
}

#endif
