#include "PrototypeWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "PrototypeGameMode.h"
#include "PrototypePlayerController.h"
#include "ScenarioDirector.h"
#include "WorkOperationComponent.h"

void UMapSelectionButton::SelectMap()
{
    if (Player)
    {
        Player->OpenScenarioMap(Destination);
    }
}

void UMainMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (WidgetTree->RootWidget)
    {
        RefreshMenu();
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
    WidgetTree->RootWidget = Canvas;
    UBorder* Background = WidgetTree->ConstructWidget<UBorder>();
    Background->SetBrushColor(FLinearColor(0.018f, 0.028f, 0.045f, 0.97f));
    UCanvasPanelSlot* BackgroundLayout = Canvas->AddChildToCanvas(Background);
    BackgroundLayout->SetAnchors(FAnchors(0, 0, 1, 1));
    BackgroundLayout->SetOffsets(FMargin(0));

    USizeBox* Card = WidgetTree->ConstructWidget<USizeBox>();
    Card->SetWidthOverride(560);
    UCanvasPanelSlot* CardLayout = Canvas->AddChildToCanvas(Card);
    CardLayout->SetAnchors(FAnchors(0.5f, 0.5f));
    CardLayout->SetAlignment(FVector2D(0.5f, 0.5f));
    CardLayout->SetAutoSize(true);
    UBorder* Surface = WidgetTree->ConstructWidget<UBorder>();
    Surface->SetBrushColor(FLinearColor(0.045f, 0.065f, 0.095f, 1));
    Surface->SetPadding(FMargin(32));
    Card->SetContent(Surface);
    UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
    Surface->SetContent(Content);

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("Выбор карты")));
    FSlateFontInfo TitleFont = Title->GetFont();
    TitleFont.Size = 30;
    Title->SetFont(TitleFont);
    Content->AddChildToVerticalBox(Title)->SetPadding(FMargin(0, 0, 0, 12));

    UTextBlock* Description = WidgetTree->ConstructWidget<UTextBlock>();
    Description->SetText(FText::FromString(TEXT("Откройте сцену для проверки взаимодействия с предметами.")));
    Description->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(Description)->SetPadding(FMargin(0, 0, 0, 24));

    APrototypePlayerController* Player = Cast<APrototypePlayerController>(GetOwningPlayer());
    const APrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<APrototypeGameMode>();
    if (GameMode)
    {
        for (const FScenarioMapEntry& Entry : GameMode->ScenarioMaps)
        {
            UMapSelectionButton* Button = WidgetTree->ConstructWidget<UMapSelectionButton>();
            Button->Destination = Entry.Map;
            Button->Player = Player;
            Button->SetBackgroundColor(FLinearColor(0.14f, 0.32f, 0.46f, 1));
            UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
            Label->SetText(Entry.DisplayName);
            Label->SetMargin(FMargin(18, 16));
            Button->SetContent(Label);
            Button->OnClicked.AddDynamic(Button, &UMapSelectionButton::SelectMap);
            Content->AddChildToVerticalBox(Button)->SetPadding(FMargin(0, 0, 0, 12));
        }
    }
    if (Player && !Player->IsMainMenuMap())
    {
        UButton* Resume = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* ResumeLabel = WidgetTree->ConstructWidget<UTextBlock>();
        ResumeLabel->SetText(FText::FromString(TEXT("Продолжить")));
        ResumeLabel->SetMargin(FMargin(18, 12));
        Resume->SetContent(ResumeLabel);
        Resume->OnClicked.AddDynamic(this, &UMainMenuWidget::ResumeScenario);
        Content->AddChildToVerticalBox(Resume)->SetPadding(FMargin(0, 8, 0, 12));

        UButton* MainMenu = WidgetTree->ConstructWidget<UButton>();
        UTextBlock* MainMenuLabel = WidgetTree->ConstructWidget<UTextBlock>();
        MainMenuLabel->SetText(FText::FromString(TEXT("Главное меню")));
        MainMenuLabel->SetMargin(FMargin(18, 12));
        MainMenu->SetContent(MainMenuLabel);
        MainMenu->OnClicked.AddDynamic(this, &UMainMenuWidget::OpenMainMenu);
        Content->AddChildToVerticalBox(MainMenu);
    }
    ErrorMessage = WidgetTree->ConstructWidget<UTextBlock>();
    ErrorMessage->SetColorAndOpacity(FSlateColor(FLinearColor(1, 0.45f, 0.3f)));
    ErrorMessage->SetAutoWrapText(true);
    Content->AddChildToVerticalBox(ErrorMessage)->SetPadding(FMargin(0, 16, 0, 0));
    RefreshMenu();
}

void UMainMenuWidget::RefreshMenu()
{
    if (ErrorMessage)
    {
        if (const APrototypePlayerController* Player = Cast<APrototypePlayerController>(GetOwningPlayer()))
        {
            ErrorMessage->SetText(Player->MenuError);
        }
    }
}

void UMainMenuWidget::ResumeScenario()
{
    if (APrototypePlayerController* Player = Cast<APrototypePlayerController>(GetOwningPlayer()))
    {
        Player->ToggleMenu();
    }
}

void UMainMenuWidget::OpenMainMenu()
{
    if (APrototypePlayerController* Player = Cast<APrototypePlayerController>(GetOwningPlayer()))
    {
        Player->ReturnToMainMenu();
    }
}

void UScenarioPanelWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    if (!WidgetTree->RootWidget)
    {
        UBorder* Surface = WidgetTree->ConstructWidget<UBorder>();
        Surface->SetBrushColor(FLinearColor(0.025f, 0.04f, 0.065f, 0.94f));
        Surface->SetPadding(FMargin(22));
        WidgetTree->RootWidget = Surface;
        UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>();
        Surface->SetContent(Content);
        Heading = WidgetTree->ConstructWidget<UTextBlock>();
        FSlateFontInfo HeadingFont = Heading->GetFont();
        HeadingFont.Size = 24;
        Heading->SetFont(HeadingFont);
        Content->AddChildToVerticalBox(Heading)->SetPadding(FMargin(0, 0, 0, 12));
        Instruction = WidgetTree->ConstructWidget<UTextBlock>();
        Instruction->SetAutoWrapText(true);
        Content->AddChildToVerticalBox(Instruction)->SetPadding(FMargin(0, 0, 0, 10));
        Progress = WidgetTree->ConstructWidget<UTextBlock>();
        Content->AddChildToVerticalBox(Progress)->SetPadding(FMargin(0, 0, 0, 10));
        Feedback = WidgetTree->ConstructWidget<UTextBlock>();
        Feedback->SetAutoWrapText(true);
        Content->AddChildToVerticalBox(Feedback)->SetPadding(FMargin(0, 0, 0, 18));
        UTextBlock* Controls = WidgetTree->ConstructWidget<UTextBlock>();
        Controls->SetAutoWrapText(true);
        Controls->SetText(FText::FromString(TEXT("VR: Grip — взять, Trigger — использовать, Y — меню.\nПК: E / Q — правая / левая рука, ЛКМ / F — использовать.\nWASD — движение, ПКМ + мышь — обзор, Esc — меню.")));
        FSlateFontInfo ControlsFont = Controls->GetFont();
        ControlsFont.Size = 15;
        Controls->SetFont(ControlsFont);
        Content->AddChildToVerticalBox(Controls);
    }
}

void UScenarioPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    for (TActorIterator<AScenarioDirector> DirectorIterator(GetWorld()); DirectorIterator; ++DirectorIterator)
    {
        Director = *DirectorIterator;
        break;
    }
    if (Director)
    {
        Director->OnScenarioChanged.AddUniqueDynamic(this, &UScenarioPanelWidget::RefreshScenario);
    }
    RefreshScenario();
}

void UScenarioPanelWidget::NativeDestruct()
{
    if (Director)
    {
        Director->OnScenarioChanged.RemoveDynamic(this, &UScenarioPanelWidget::RefreshScenario);
    }
    Super::NativeDestruct();
}

void UScenarioPanelWidget::RefreshScenario()
{
    if (!Director || !Heading)
    {
        return;
    }
    Heading->SetText(Director->ScenarioTitle);
    Instruction->SetText(Director->GetCurrentInstruction());
    Progress->SetText(FText::Format(FText::FromString(TEXT("Выполнено шагов: {0} из {1}")), Director->GetCompletedStepCount(), Director->Steps.Num()));
    Feedback->SetText(Director->LastAction);
}
