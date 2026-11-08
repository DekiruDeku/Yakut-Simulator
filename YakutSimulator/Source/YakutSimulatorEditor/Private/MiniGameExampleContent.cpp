#include "Common/MiniGameDefinition.h"
#include "Common/MiniGameScenario.h"
#include "Common/MiniGamePanel.h"
#include "Common/GrabbableItem.h"
#include "Fishing/FishingDefinition.h"
#include "Fishing/FishingFish.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "Engine/Font.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "UObject/StrongObjectPtr.h"
#include "GameFramework/GameModeBase.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_EnhancedInputAction.h"
#include "InputAction.h"
#include "StateTree.h"
#include "StateTreeEditorData.h"
#include "StateTreeState.h"
#include "StateTreeCompiler.h"
#include "StateTreeCompilerLog.h"
#include "Components/StateTreeComponentSchema.h"
#include "WidgetBlueprintFactory.h"
#include "WidgetBlueprint.h"
#include "Editor.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace MiniGameContent
{
static const FString Common = TEXT("/Game/Common/");
static const FString Fishing = TEXT("/Game/MiniGames/Fishing/");
static const FString Tests = TEXT("/Game/Tests/MiniGames/");

bool Save(UObject* Asset)
{
    if (!Asset) return false;
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Package, Asset, *FPackageName::LongPackageNameToFilename(Package->GetName(), Asset->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension()), Args);
}
template<typename T> T* NewAsset(const FString& Path, bool& bCreated)
{
    const FString Name = FPackageName::GetLongPackageAssetName(Path);
    if (T* Existing = LoadObject<T>(nullptr, *(Path + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet)) { bCreated = false; return Existing; }
    bCreated = true;
    T* Result = NewObject<T>(CreatePackage(*Path), *Name, RF_Public | RF_Standalone | RF_Transactional);
    FAssetRegistryModule::AssetCreated(Result);
    return Result;
}
UBlueprint* Blueprint(const FString& Path, UClass* Parent, bool& bCreated)
{
    const FString Name = FPackageName::GetLongPackageAssetName(Path);
    if (auto* Existing = LoadObject<UBlueprint>(nullptr, *(Path + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet)) { bCreated = false; return Existing; }
    bCreated = true;
    auto* BP = FKismetEditorUtilities::CreateBlueprint(Parent, CreatePackage(*Path), *Name, BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
    FAssetRegistryModule::AssetCreated(BP);
    return BP;
}
void Finish(UBlueprint* BP)
{
    FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    checkf(BP->Status != BS_Error, TEXT("Blueprint failed: %s"), *BP->GetPathName());
    check(Save(BP));
}
UClass* Panel()
{
    const FString Path = Common + TEXT("UI/WBP_MiniGamePanel");
    if (auto* Existing = LoadObject<UWidgetBlueprint>(nullptr, *(Path + TEXT(".WBP_MiniGamePanel")), nullptr, LOAD_NoWarn | LOAD_Quiet)) return Existing->GeneratedClass;
    auto* Factory = NewObject<UWidgetBlueprintFactory>();
    Factory->ParentClass = UMiniGamePanelWidget::StaticClass();
    auto* BP = Cast<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(), CreatePackage(*Path), TEXT("WBP_MiniGamePanel"), RF_Public | RF_Standalone, nullptr, GWarn));
    check(BP);
    FAssetRegistryModule::AssetCreated(BP);
    UWidgetTree* Tree = BP->WidgetTree;
    auto* Border = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
    Border->SetBrushColor(FLinearColor(.015f, .025f, .04f, .94f));
    Border->SetPadding(FMargin(14));
    auto* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Layout"));
    Border->SetContent(Column);
    auto* Canvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
    Tree->RootWidget = Canvas;
    auto* PanelSlot = Canvas->AddChildToCanvas(Border);
    PanelSlot->SetAnchors(FAnchors(0, 0, 1, 0));
    PanelSlot->SetOffsets(FMargin(0));
    PanelSlot->SetAutoSize(true);
    UFont* Font = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    auto Text = [&](const TCHAR* Name, int32 Size)
    {
        auto* Widget = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        Widget->bIsVariable = true;
        Widget->SetFont(FSlateFontInfo(Font, Size, TEXT("Regular")));
        Widget->SetAutoWrapText(true);
        Column->AddChildToVerticalBox(Widget)->SetPadding(FMargin(0, 3));
        return Widget;
    };
    Text(TEXT("Heading"), 30)->SetText(FText::FromString(TEXT("Задача")));
    Text(TEXT("Description"), 23)->SetText(FText::FromString(TEXT("Подсказка текущего этапа")));
    Text(TEXT("PrimaryLabel"), 20);
    auto* Primary = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("PrimaryGauge"));
    Primary->bIsVariable = true;
    Primary->SetFillColorAndOpacity(FLinearColor(.1f, .6f, 1));
    Column->AddChildToVerticalBox(Primary)->SetPadding(FMargin(0, 3));
    Text(TEXT("SecondaryLabel"), 20);
    auto* Secondary = Tree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("SecondaryGauge"));
    Secondary->bIsVariable = true;
    Secondary->SetFillColorAndOpacity(FLinearColor(1, .7f, .1f));
    Column->AddChildToVerticalBox(Secondary)->SetPadding(FMargin(0, 3));
    Finish(BP);
    return BP->GeneratedClass;
}
UXRInteractionProfile* InputProfile()
{
    bool Created;
    auto* Profile = NewAsset<UXRInteractionProfile>(Common + TEXT("Data/DA_XRInteraction"), Created);
    if (Created)
    {
        const auto Action = [](const TCHAR* Name) { return LoadObject<UInputAction>(nullptr, *FString::Printf(TEXT("/Game/XRFramework/Input/Actions/%s.%s"), Name, Name)); };
        Profile->LeftGrab = Action(TEXT("IA_Grab_Left_Pressed"));
        Profile->LeftRelease = Action(TEXT("IA_Grab_Left_Released"));
        Profile->RightGrab = Action(TEXT("IA_Grab_Right_Pressed"));
        Profile->RightRelease = Action(TEXT("IA_Grab_Right_Released"));
        Profile->LeftUse = Action(TEXT("IA_Shoot_Left"));
        Profile->RightUse = Action(TEXT("IA_Shoot_Right"));
        check(Save(Profile));
    }
    return Profile;
}
UMiniGameUIStyle* Style()
{
    bool Created;
    auto* Result = NewAsset<UMiniGameUIStyle>(Common + TEXT("Data/DA_DefaultUIStyle"), Created);
    if (Created)
    {
        Result->PanelClass = Panel();
        Result->Colors = {{EMiniGameFeedback::Normal, FLinearColor::White}, {EMiniGameFeedback::Success, FLinearColor(.15f, 1, .3f)},
            {EMiniGameFeedback::Warning, FLinearColor(1, .8f, .08f)}, {EMiniGameFeedback::Error, FLinearColor(1, .12f, .08f)}, {EMiniGameFeedback::Disabled, FLinearColor(.55f, .55f, .55f)}};
        check(Save(Result));
    }
    return Result;
}
void DisplayDefaults(UMiniGameDisplayComponent* Display, UClass* Widget)
{
    Display->SetWidgetClass(Widget);
    Display->SetWidgetSpace(EWidgetSpace::World);
    Display->SetDrawSize(FVector2D(640, 420));
    Display->SetPivot(FVector2D(.5f, 0));
    Display->SetBlendMode(EWidgetBlendMode::Transparent);
    Display->SetRelativeScale3D(FVector(.1f));
}
UMaterialInstanceConstant* FeedbackMaterial(const TCHAR* Name, FLinearColor Color)
{
    bool Created;
    auto* Material = NewAsset<UMaterialInstanceConstant>(Common + TEXT("Materials/") + Name, Created);
    if (Created)
    {
        Material->SetParentEditorOnly(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/LevelPrototyping/Materials/M_FlatCol.M_FlatCol")));
        Material->SetVectorParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Base Color")), Color);
        Material->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(TEXT("Roughness")), .6f);
        Material->PostEditChange();
        check(Save(Material));
    }
    return Material;
}
UClass* Station(const FString& Name, int32 Role, UClass* Widget)
{
    bool Created;
    UBlueprint* BP = Blueprint(Common + TEXT("Blueprints/") + Name, AMiniGameStation::StaticClass(), Created);
    if (Created)
    {
        auto* Actor = CastChecked<AMiniGameStation>(BP->GeneratedClass->GetDefaultObject());
        Actor->Zone->SetBoxExtent(FVector(20, 20, 15));
        Actor->PressZone->SetBoxExtent(FVector(17.5f, 17.5f, 2));
        Actor->PressZone->SetRelativeLocation(FVector(0, 0, -15));
        Actor->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Actor->Mesh->SetRelativeScale3D(FVector(.35f, .35f, .04f));
        Actor->Mesh->SetRelativeLocation(FVector(0, 0, -15));
        Actor->Handle->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Actor->Handle->SetRelativeScale3D(FVector(.1f));
        Actor->Handle->SetVisibility(Role == 1);
        Actor->Button->bEnabled = Role == 0;
        Actor->Lever->bEnabled = Role == 1;
        Actor->Receiver->bEnabled = Role == 2;
        if (Role == 3) Actor->Mesh->SetVisibility(false);
        DisplayDefaults(Actor->Display, Widget);
        if (Role == 0 || Role == 2) Actor->Display->SetDrawSize(FVector2D(320, 180));
        Actor->Display->SetRelativeLocation(FVector(0, 0, Role == 1 ? 55 : 25));
        Actor->Display->SetRelativeRotation(FRotator(0, 180, 0));
        Finish(BP);
    }
    return BP->GeneratedClass;
}
UClass* Item(const FString& Name, int32 Kind, UClass* Widget)
{
    bool Created;
    UBlueprint* BP = Blueprint((Kind < 3 ? Fishing : Tests) + TEXT("Blueprints/") + Name, Kind != 1 && Kind != 3 ? AFishingFish::StaticClass() : AGrabbableItem::StaticClass(), Created);
    if (Created)
    {
        auto* Actor = CastChecked<AGrabbableItem>(BP->GeneratedClass->GetDefaultObject());
        Actor->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, Kind == 1 || Kind == 3 ? TEXT("/Engine/BasicShapes/Cube.Cube") : TEXT("/Engine/BasicShapes/Sphere.Sphere")));
        Actor->Body->SetBoxExtent(Kind == 1 ? FVector(16, 3, 3) : FVector(18, 7, 4));
        Actor->Mesh->SetRelativeScale3D(Kind == 1 ? FVector(.3f, .025f, .025f) : FVector(.36f, .12f, .08f));
        Actor->ToolTip->SetRelativeLocation(FVector(15, 0, 0));
        if (Kind == 1)
        {
            Actor->ToolTipVisual->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            Actor->ToolTipVisual->SetRelativeScale3D(FVector(.015f));
            Actor->ToolTipVisual->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, *(Common + TEXT("Materials/MI_WorkingTip"))));
        }
        Actor->Grab->bSnapToHand = Kind == 1;
        Actor->Grab->MaxHandDistance = 40;
        Actor->DisplayName = FText::FromString(Kind == 1 ? TEXT("Нож") : TEXT("Предмет"));
        DisplayDefaults(Actor->Display, Widget);
        Actor->Display->SetRelativeLocation(Kind != 1 && Kind != 3 ? FVector(45, 0, 18) : FVector(0, 0, 12));
        Actor->Display->SetHiddenInGame(Kind == 1);
        Actor->Display->SetRelativeScale3D(FVector(.045f));
        Actor->Display->SetRelativeRotation(FRotator(45, -90, 0));
        if (auto* Fish = Cast<AFishingFish>(Actor))
        {
            Fish->BraceIndicator->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            Fish->BraceIndicator->SetRelativeLocation(FVector(0, -10, 7));
            Fish->BraceIndicator->SetRelativeScale3D(FVector(.04f));
            Fish->BraceIndicator->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, *(Common + TEXT("Materials/MI_GripConfirmation"))));
            const TArray<FVector> Points = Kind == 2
                ? TArray<FVector>{FVector(-8,-7,5),FVector(-8,7,5),FVector(8,7,5),FVector(8,-7,5)}
                : TArray<FVector>{FVector(-12,0,5),FVector(0,0,5),FVector(12,0,5)};
            Fish->CutPath->ClearSplinePoints();
            for (const FVector& Point : Points) Fish->CutPath->AddSplinePoint(Point, ESplineCoordinateSpace::Local);
            if (Kind == 2) Fish->Mesh->SetRelativeScale3D(FVector(.25f, .22f, .06f));
            if (Kind == 4)
            {
                Fish->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
                Fish->Mesh->SetRelativeScale3D(FVector(.22f,.14f,.09f));
                Fish->CutPath->ClearSplinePoints();
                for (const FVector& Point : TArray<FVector>{FVector(-12,-4,5),FVector(-6,4,5),FVector(0,-4,5),FVector(6,4,5),FVector(12,-4,5)})
                    Fish->CutPath->AddSplinePoint(Point, ESplineCoordinateSpace::Local);
            }
        }
        Finish(BP);
    }
    return BP->GeneratedClass;
}
UStateTree* Flow(UMiniGameDefinition* Definition, const FString& Path)
{
    bool Created;
    UStateTree* Tree = NewAsset<UStateTree>(Path, Created);
    if (!Created) return Tree;
    auto* Data = NewObject<UStateTreeEditorData>(Tree, NAME_None, RF_Transactional);
    Tree->EditorData = Data;
    Data->Schema = NewObject<UStateTreeComponentSchema>(Data);
    UStateTreeState& Root = Data->AddRootState();
    TArray<UStateTreeState*> States;
    for (const auto& Objective : Definition->Objectives)
    {
        auto& State = Root.AddChildState(Objective.Id);
        auto& Task = State.AddTask<FMiniGameObjectiveTask>();
        Task.GetInstanceData().ObjectiveId = Objective.Id;
        States.Add(&State);
    }
    for (int32 I = 0; I < States.Num(); ++I)
    {
        if (I + 1 < States.Num()) States[I]->AddTransition(EStateTreeTransitionTrigger::OnEvent, Definition->Objectives[I].CompletionEvent, EStateTreeTransitionType::GotoState, States[I+1]);
        else States[I]->AddTransition(EStateTreeTransitionTrigger::OnStateSucceeded, EStateTreeTransitionType::Succeeded);
    }
    FStateTreeCompilerLog Log;
    FStateTreeCompiler Compiler(Log);
    const bool Compiled = Compiler.Compile(Tree);
    Log.DumpToLog(LogMiniGame);
    checkf(Compiled, TEXT("StateTree compilation failed."));
    check(Save(Tree));
    return Tree;
}
} // namespace MiniGameContent

namespace MiniGameContent
{
void ProcessingMessages(UFishingDefinition* Def)
{
    const TMap<FName, FString> Messages = {
        {TEXT("ProcessingInactive"), TEXT("РАЗДЕЛКА НЕ АКТИВНА")},
        {TEXT("ProcessingNeedBoard"), TEXT("ПОЛОЖИТЕ РЫБУ НА ДОСКУ И ОТПУСТИТЕ")},
        {TEXT("ProcessingNeedFishGrip"), TEXT("ЗАЖМИТЕ GRIP У РЫБЫ НА ДОСКЕ")},
        {TEXT("ProcessingBraceTooFar"), TEXT("ПРИДЕРЖИВАЮЩАЯ РУКА ДАЛЕКО ОТ РЫБЫ")},
        {TEXT("ProcessingNeedKnifeGrip"), TEXT("ВОЗЬМИТЕ НОЖ ДРУГОЙ РУКОЙ")},
        {TEXT("ProcessingDifferentHands"), TEXT("РЫБУ И НОЖ НУЖНО ДЕРЖАТЬ РАЗНЫМИ РУКАМИ")},
        {TEXT("ProcessingInvalidPath"), TEXT("НЕ НАСТРОЕНА ТРАЕКТОРИЯ РАЗДЕЛКИ")},
        {TEXT("ProcessingInvalidTip"), TEXT("НЕКОРРЕКТНАЯ ПОЗА КОНЧИКА НОЖА")},
        {TEXT("ProcessingMoveTooFast"), TEXT("ДВИГАЙТЕ НОЖ ПЛАВНЕЕ — СКАЧОК НЕ ЗАСЧИТАН")},
        {TEXT("ProcessingAim"), TEXT("ВЕДИТЕ КОНЧИК НОЖА К ЖЁЛТОЙ МЕТКЕ")},
        {TEXT("ProcessingReadyToDeliver"), TEXT("РАЗДЕЛКА ГОТОВА — СДАЙТЕ РЫБУ")},
        {TEXT("NotOnBoard"), TEXT("ещё не на доске")}, {TEXT("NotHeld"), TEXT("нет захвата")},
        {TEXT("LeftHand"), TEXT("левая рука")}, {TEXT("RightHand"), TEXT("правая рука")},
        {TEXT("ItemHeld"), TEXT("В РУКЕ: {0}")}, {TEXT("FishBraced"), TEXT("ЗАФИКСИРОВАНА: {0}")},
        {TEXT("ProcessingHands"), TEXT("Рыба: {0}\nНож: {1}")},
        {TEXT("ProcessingTarget"), TEXT("Метка {0}/{1}. Кончик: {2} см; допуск: {3} см.")},
        {TEXT("ProcessingDetails"), TEXT("{0}\n{1}")}, {TEXT("CurrentFishCut"), TEXT("Разделка текущей рыбы")}
    };
    bool Changed = false;
    for (const auto& Pair : Messages) if (!Def->Messages.Contains(Pair.Key))
    {
        Def->Messages.Add(Pair.Key, FText::FromString(Pair.Value));
        Changed = true;
    }
    if (Changed) check(Save(Def));
}
UFishingDefinition* FishingProfile(UXRInteractionProfile* Input, UMiniGameUIStyle* UI, UFishSpeciesDefinition* A, UFishSpeciesDefinition* B, UClass* Knife)
{
    bool Created;
    auto* Def = NewAsset<UFishingDefinition>(Fishing + TEXT("Data/DA_Fishing_Default"), Created);
    if (!Created) return Def;
    Def->Id = TEXT("Fishing");
    Def->DisplayName = FText::FromString(TEXT("Рыболовство"));
    Def->Interaction = Input;
    Def->Style = UI;
    Def->RestartAction = TEXT("Restart");
    Def->StopAction = TEXT("Stop");
    Def->ResumeAction = Def->StartAction = TEXT("Start");
    Def->EquipAction = TEXT("Equip");
    Def->SortingAction = TEXT("GoSorting");
    Def->ProcessingAction = TEXT("GoProcessing");
    Def->WinchAction = TEXT("Winch");
    Def->WinchPanel = TEXT("Winch");
    Def->SpeciesCycle = {A, B};
    Def->KnifeClass = Knife;
    Def->Operations = {{TEXT("Safety"), EFishingOperation::Safety}, {TEXT("Deploy"), EFishingOperation::Deploy},
        {TEXT("Trawl"), EFishingOperation::Trawl}, {TEXT("Haul"), EFishingOperation::Haul}, {TEXT("Sorting"), EFishingOperation::Sorting},
        {TEXT("Processing"), EFishingOperation::Processing}, {TEXT("Results"), EFishingOperation::Results}};
    auto Objective = [&](const TCHAR* Id, const TCHAR* Title, const TCHAR* Hint, const TCHAR* Operation, const TCHAR* Workplace, TArray<FName> Actions, bool Final = false)
    {
        FMiniGameObjective Obj;
        Obj.Id = Id;
        Obj.Title = FText::FromString(Title);
        Obj.Instruction = FText::FromString(Hint);
        Obj.Operation = Operation;
        Obj.Workplace = Workplace;
        Obj.AllowedActions = Actions;
        Obj.bFinal = Final;
        if (!Final) Obj.CompletionEvent = FGameplayTag::RequestGameplayTag(FName(*FString::Printf(TEXT("MiniGame.Fishing.%s.Complete"), Id)));
        Def->Objectives.Add(Obj);
    };
    Objective(TEXT("Equip"), TEXT("ПОДГОТОВКА: НАДЕНЬТЕ ЗАЩИТУ"), TEXT("Поднесите контроллер к плашке НАДЕТЬ ЗАЩИТУ. Нажмите Grip или Trigger один раз."), TEXT("Safety"), TEXT("Deck"), {TEXT("Equip")});
    Objective(TEXT("TestStop"), TEXT("ПРОВЕРЬТЕ КНОПКУ СТОП"), TEXT("Защита надета. Один раз нажмите СТОП для проверки безопасности."), TEXT("Safety"), TEXT(""), {TEXT("Stop")});
    Objective(TEXT("Start"), TEXT("НАЧНИТЕ ЛОВЛЮ"), TEXT("Проверка завершена. Нажмите НАЧАТЬ ЛОВЛЮ."), TEXT("Safety"), TEXT(""), {TEXT("Start")});
    Objective(TEXT("Deploy"), TEXT("ВЫПУСК ТРАЛА"), TEXT("Удерживайте Grip у рукоятки. Плавно сдвиньте руку вперёд до скорости около 50%. Отпускание Grip даёт паузу."), TEXT("Deploy"), TEXT(""), {TEXT("Winch"),TEXT("Stop"),TEXT("Start")});
    Objective(TEXT("Trawl"), TEXT("ТРАЛ В ВОДЕ"), TEXT("Отпустите Grip. Дождитесь команды ПОДЪЁМ ТРАЛА, затем снова возьмите рукоятку."), TEXT("Trawl"), TEXT(""), {});
    Objective(TEXT("Haul"), TEXT("ПОДЪЁМ ТРАЛА"), TEXT("Снова удерживайте Grip у рукоятки и плавно подайте руку вперёд до скорости около 50%."), TEXT("Haul"), TEXT(""), {TEXT("Winch"),TEXT("Stop"),TEXT("Start")});
    Objective(TEXT("GoSorting"), TEXT("УЛОВ ПОДНЯТ"), TEXT("Отпустите рычаг. Нажмите В ЦЕХ."), TEXT(""), TEXT(""), {TEXT("GoSorting")});
    Objective(TEXT("Sorting"), TEXT("СОРТИРОВКА УЛОВА"), TEXT("Берите рыбу с ленты и отпускайте в лоток её вида. Ошибки и пропуски возвращаются в лоток пересортировки."), TEXT("Sorting"), TEXT("Sorting"), {});
    Objective(TEXT("GoProcessing"), TEXT("СОРТИРОВКА ГОТОВА"), TEXT("Нажмите К ОБРАБОТКЕ."), TEXT(""), TEXT(""), {TEXT("GoProcessing")});
    Objective(TEXT("Processing"), TEXT("ОБРАБОТКА РЫБЫ"), TEXT("Рыбу из подающего лотка положите на стол. Придерживайте одной рукой, другой ведите нож по меткам. Готовую рыбу сдайте в лоток."), TEXT("Processing"), TEXT("Processing"), {});
    Objective(TEXT("Results"), TEXT("СМЕНА ЗАВЕРШЕНА"), TEXT("Нажмите НОВАЯ СМЕНА для повтора."), TEXT("Results"), TEXT(""), {TEXT("Restart")}, true);
    const TMap<FName, FString> Messages = {
        {TEXT("Equip"),TEXT("НАДЕТЬ\nЗАЩИТУ")}, {TEXT("Equipped"),TEXT("ЗАЩИТА НАДЕТА")}, {TEXT("Stop"),TEXT("СТОП")},
        {TEXT("StartLabel"),TEXT("НАЧАТЬ\nЛОВЛЮ")}, {TEXT("Reset"),TEXT("СБРОС\nОСТАНОВКИ")}, {TEXT("ReadyLabel"),TEXT("ЛЕБЁДКА\nГОТОВА")},
        {TEXT("GoSorting"),TEXT("В ЦЕХ")}, {TEXT("GoProcessing"),TEXT("К ОБРАБОТКЕ")}, {TEXT("Restart"),TEXT("НОВАЯ\nСМЕНА")},
        {TEXT("PressHint"),TEXT("Grip / Trigger")},
        {TEXT("WinchReady"),TEXT("Лебёдка готова. Возьмите рукоятку и плавно подайте руку вперёд.")},
        {TEXT("AlreadyRunning"),TEXT("Лебёдка уже готова. Повторный пуск не требуется.")},
        {TEXT("StopTested"),TEXT("Проверка СТОП завершена.")}, {TEXT("NeedEquipment"),TEXT("Сначала наденьте защиту.")},
        {TEXT("NeedStop"),TEXT("Сначала проверьте СТОП.")}, {TEXT("SpawnFailed"),TEXT("Не удалось создать предметы. Проверьте настройки контента.")},
        {TEXT("OverloadStopped"),TEXT("СТОП: ПЕРЕГРУЗКА")}, {TEXT("ButtonStopped"),TEXT("СТОП: НАЖАТА КНОПКА СТОП")},
        {TEXT("ResetHint"),TEXT("Отпустите Grip. Нажмите СБРОС ОСТАНОВКИ, затем снова возьмите рукоятку.")},
        {TEXT("Lever"),TEXT("ЗАЖМИТЕ GRIP У РУКОЯТКИ")}, {TEXT("LeverHeld"),TEXT("РЫЧАГ В РУКЕ")},
        {TEXT("LoadWarning"),TEXT("Нагрузка растёт. Сдвиньте рычаг назад.")},
        {TEXT("OverloadWarning"),TEXT("РЫЧАГ НАЗАД! До остановки: {0} с.")},
        {TEXT("Speed"),TEXT("Скорость")}, {TEXT("Load"),TEXT("Нагрузка")}, {TEXT("Deployed"),TEXT("Трал выпущен")}, {TEXT("Hauled"),TEXT("Трал поднят")},
        {TEXT("Sorted"),TEXT("Отсортировано")}, {TEXT("Processed"),TEXT("Обработано")}, {TEXT("Delivered"),TEXT("Сдано")},
        {TEXT("SortBin"),TEXT("Лоток для этого вида")}, {TEXT("Board"),TEXT("СТОЛ ОБРАБОТКИ")}, {TEXT("Delivery"),TEXT("ГОТОВЫЙ УЛОВ")},
        {TEXT("WrongBin"),TEXT("Другой вид. Рыба возвращена в пересортировку.")}, {TEXT("SortedAccepted"),TEXT("Рыба принята.")},
        {TEXT("HoldFish"),TEXT("Придерживайте рыбу одной рукой, другой ведите нож по меткам.")},
        {TEXT("ProcessingComplete"),TEXT("Рыба готова. Отпустите придерживающую руку, возьмите рыбу снова и сдайте в ГОТОВЫЙ УЛОВ.")},
        {TEXT("DeliveredAccepted"),TEXT("Готовая рыба принята. Возьмите следующую.")},
        {TEXT("Results"),TEXT("Отсортировано: {Sorted}\nОбработано: {Processed}   Сдано: {Delivered}\nОшибки: {Mistakes}   Пропуски: {Missed}\nПерегрузки: {Overloads}   Время: {Seconds} с\nОчки: {Score}\nНОВАЯ СМЕНА — повтор")}
    };
    for (const auto& Pair : Messages) Def->Messages.Add(Pair.Key, FText::FromString(Pair.Value));
    Def->Flow = Flow(Def, Fishing + TEXT("Flow/ST_Fishing_Default"));
    check(Save(Def));
    return Def;
}
UClass* Pawn(UXRInteractionProfile* Profile)
{
    const FString Path = Common + TEXT("Blueprints/BP_MiniGamePawn");
    if (auto* Existing = LoadObject<UBlueprint>(nullptr, *(Path + TEXT(".BP_MiniGamePawn")), nullptr, LOAD_NoWarn | LOAD_Quiet)) return Existing->GeneratedClass;
    auto* Source = LoadObject<UBlueprint>(nullptr, TEXT("/Game/XRFramework/Blueprints/BP_XRPawn.BP_XRPawn"));
    auto& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    auto* BP = CastChecked<UBlueprint>(AssetTools.DuplicateAsset(TEXT("BP_MiniGamePawn"), Common + TEXT("Blueprints"), Source));
    const TSet<const UInputAction*> Actions{Profile->LeftGrab, Profile->LeftRelease, Profile->RightGrab, Profile->RightRelease, Profile->LeftUse, Profile->RightUse};
    int32 Removed = 0;
    for (UEdGraph* Graph : BP->UbergraphPages)
    {
        const auto Nodes = Graph->Nodes;
        for (UEdGraphNode* Node : Nodes) if (auto* Input = Cast<UK2Node_EnhancedInputAction>(Node)) if (Actions.Contains(Input->InputAction))
        {
            FBlueprintEditorUtils::RemoveNode(BP, Node, false);
            ++Removed;
        }
    }
    auto* Node = BP->SimpleConstructionScript->CreateNode(UXRInteractorComponent::StaticClass(), TEXT("MiniGameInteractor"));
    BP->SimpleConstructionScript->AddNode(Node);
    auto* Interactor = CastChecked<UXRInteractorComponent>(Node->ComponentTemplate);
    Interactor->Profile = Profile;
    Interactor->LeftHand.ComponentProperty = TEXT("MotionControllerLeftGrip");
    Interactor->RightHand.ComponentProperty = TEXT("MotionControllerRightGrip");
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    Finish(BP);
    UE_LOG(LogMiniGame, Display, TEXT("[Migration.Pawn] removedLegacyInputEvents=%d"), Removed);
    return BP->GeneratedClass;
}
UClass* GameMode(UClass* Player)
{
    const FString Path = Common + TEXT("Blueprints/BP_MiniGameGameMode");
    if (auto* Existing = LoadObject<UBlueprint>(nullptr, *(Path + TEXT(".BP_MiniGameGameMode")), nullptr, LOAD_NoWarn | LOAD_Quiet)) return Existing->GeneratedClass;
    auto* Source = LoadObject<UBlueprint>(nullptr, TEXT("/Game/XRFramework/Blueprints/BP_XRGameMode.BP_XRGameMode"));
    auto& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    auto* BP = CastChecked<UBlueprint>(Tools.DuplicateAsset(TEXT("BP_MiniGameGameMode"), Common + TEXT("Blueprints"), Source));
    CastChecked<AGameModeBase>(BP->GeneratedClass->GetDefaultObject())->DefaultPawnClass = Player;
    Finish(BP);
    return BP->GeneratedClass;
}
}

namespace MiniGameContent
{
UFishSpeciesDefinition* Species(const FString& Path, const TCHAR* Id, const TCHAR* Label, UClass* ActorClass)
{
    bool Created;
    auto* Def = NewAsset<UFishSpeciesDefinition>(Path, Created);
    if (Created) { Def->Id = Id; Def->DisplayName = FText::FromString(Label); Def->ActorClass = ActorClass; check(Save(Def)); }
    return Def;
}
void CommonTestContent(UXRInteractionProfile* Input, UMiniGameUIStyle* UI)
{
    bool Created;
    auto* Def = NewAsset<UMiniGameDefinition>(Tests + TEXT("DA_CommonScenario"), Created);
    if (Created)
    {
        Def->Id = TEXT("CommonTest");
        Def->Interaction = Input;
        Def->Style = UI;
        Def->RestartAction = TEXT("Restart");
        FMiniGameObjective First;
        First.Id = TEXT("Interact");
        First.Title = FText::FromString(TEXT("Общее взаимодействие"));
        First.AllowedActions = {TEXT("Continue"), TEXT("Lever")};
        First.CompletionEvent = FGameplayTag::RequestGameplayTag(TEXT("MiniGame.Test.Continue"));
        FMiniGameObjective Final;
        Final.Id = TEXT("Complete");
        Final.Title = FText::FromString(TEXT("Готово"));
        Final.bFinal = true;
        Final.AllowedActions = {TEXT("Restart")};
        Def->Objectives = {First, Final};
        Def->ActionEvents.Add(TEXT("Continue"), First.CompletionEvent);
        Def->Flow = Flow(Def, Tests + TEXT("ST_CommonScenario"));
        check(Save(Def));
    }
    Item(TEXT("BP_TestItem"), 3, UI->PanelClass);
}
void Create()
{
    if (!GEditor || GEditor->PlayWorld) { UE_LOG(LogMiniGame, Error, TEXT("Stop Play before creating example content.")); return; }
    auto* GripMaterial = FeedbackMaterial(TEXT("MI_GripConfirmation"), FLinearColor(.05f, 1, .1f));
    auto* TipMaterial = FeedbackMaterial(TEXT("MI_WorkingTip"), FLinearColor(.02f, .6f, 1));
    FeedbackMaterial(TEXT("MI_CutTarget"), FLinearColor(1, .7f, .02f));
    TStrongObjectPtr<UXRInteractionProfile> Input(InputProfile());
    TStrongObjectPtr<UMiniGameUIStyle> UI(Style());
    auto* A = Species(Fishing + TEXT("Data/DA_Fish_A"), TEXT("A"), TEXT("РЫБА A"), Item(TEXT("BP_Fish_A"),0,UI->PanelClass));
    auto* B = Species(Fishing + TEXT("Data/DA_Fish_B"), TEXT("B"), TEXT("РЫБА B"), Item(TEXT("BP_Fish_B"),2,UI->PanelClass));
    UClass* Knife = Item(TEXT("BP_Knife"),1,UI->PanelClass);
    TStrongObjectPtr<UFishingDefinition> Def(FishingProfile(Input.Get(), UI.Get(), A, B, Knife));
    CommonTestContent(Input.Get(), UI.Get());
    auto* C = Species(Tests + TEXT("DA_Fish_C"), TEXT("C"), TEXT("РЫБА C"), Item(TEXT("BP_Fish_C"),4,UI->PanelClass));
    bool Created;
    auto* Alternative = NewAsset<UFishingDefinition>(Tests + TEXT("DA_Fishing_ThreeSpecies"),Created);
    if (Created)
    {
        UEngine::CopyPropertiesForUnrelatedObjects(Def.Get(), Alternative);
        Alternative->Id = TEXT("Fishing_ThreeSpecies");
        Alternative->SpeciesCycle.Add(C);
        Alternative->Catch.FishCount = 3;
        Alternative->Winch.bEnableOverloads = false;
        Alternative->Winch.DeploySeconds = Alternative->Winch.HaulSeconds = 1;
        Alternative->Catch.FeedInterval = .1f;
        check(Save(Alternative));
    }
    ProcessingMessages(Def.Get());
    ProcessingMessages(Alternative);
    for (UClass* ItemClass : {A->ActorClass.Get(), B->ActorClass.Get(), C->ActorClass.Get(), Knife})
    {
        auto* ItemDefaults = CastChecked<AGrabbableItem>(ItemClass->GetDefaultObject());
        auto* FishDefaults = Cast<AFishingFish>(ItemDefaults);
        auto* Indicator = FishDefaults ? FishDefaults->BraceIndicator.Get() : ItemDefaults->ToolTipVisual.Get();
        if (!Indicator->GetStaticMesh())
        {
            Indicator->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            Indicator->SetRelativeScale3D(FVector(FishDefaults ? .04f : .015f));
            if (FishDefaults) Indicator->SetRelativeLocation(FVector(0, -10, 7));
            Indicator->SetMaterial(0, FishDefaults ? GripMaterial : TipMaterial);
            Finish(CastChecked<UBlueprint>(ItemClass->ClassGeneratedBy));
        }
    }
    for (UMiniGameDefinition* Profile : {static_cast<UMiniGameDefinition*>(Def.Get()), LoadObject<UMiniGameDefinition>(nullptr, *(Tests+TEXT("DA_CommonScenario"))), static_cast<UMiniGameDefinition*>(Alternative)})
        if (Profile && !Profile->Messages.Contains(TEXT("ActionUnavailable")))
        {
            Profile->Messages.Add(TEXT("ActionUnavailable"), FText::FromString(TEXT("Сейчас это действие недоступно. Следуйте заданию на табло.")));
            check(Save(Profile));
        }
    GameMode(Pawn(Input.Get()));
    Station(TEXT("BP_InteractionButton"),0,UI->PanelClass);
    Station(TEXT("BP_InteractionLever"),1,UI->PanelClass);
    Station(TEXT("BP_ItemReceiver"),2,UI->PanelClass);
    Station(TEXT("BP_StatusPanel"),3,UI->PanelClass);
    UE_LOG(LogMiniGame, Display, TEXT("[Migration.ContentReady] definitions and assets saved; designer-owned visuals preserved."));
}
FAutoConsoleCommand Command(TEXT("MiniGame.CreateExampleContent"), TEXT("Create missing shared/fishing example assets; existing assets and maps are preserved."),
    FConsoleCommandDelegate::CreateStatic(&Create));
}
