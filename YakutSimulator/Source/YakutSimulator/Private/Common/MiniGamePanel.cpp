#include "Common/MiniGamePanel.h"
#include "Common/MiniGameScenario.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"

void UMiniGamePanelWidget::Present(const FMiniGamePresentation& Data, UMiniGameUIStyle* Style)
{
    Presentation = Data;
    SetVisibility(Data.Title.IsEmpty() && Data.Body.IsEmpty() && Data.Metrics.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    if (Heading) Heading->SetText(Data.Title);
    if (Description) Description->SetText(Data.Body);
    if (Style) if (const auto* Color = Style->Colors.Find(Data.Feedback))
        if (Heading) Heading->SetColorAndOpacity(FSlateColor(*Color));
    UTextBlock* Labels[] = {PrimaryLabel, SecondaryLabel};
    UProgressBar* Gauges[] = {PrimaryGauge, SecondaryGauge};
    for (int32 I = 0; I < 2; ++I)
    {
        const bool bVisible = Data.Metrics.IsValidIndex(I);
        if (Labels[I])
        {
            Labels[I]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
            if (bVisible && Style) Labels[I]->SetText(FText::Format(Style->MetricFormat, Data.Metrics[I].Label,
                FText::AsNumber(FMath::RoundToInt(Data.Metrics[I].Maximum > 0 ? Data.Metrics[I].Value / Data.Metrics[I].Maximum * 100 : 0))));
        }
        if (Gauges[I])
        {
            Gauges[I]->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
            if (bVisible) Gauges[I]->SetPercent(Data.Metrics[I].Maximum > 0 ? FMath::Clamp(Data.Metrics[I].Value / Data.Metrics[I].Maximum, 0.f, 1.f) : 0);
        }
    }
    OnPresentationChanged(Data);
}
UMiniGameDisplayComponent::UMiniGameDisplayComponent()
{
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void UMiniGameDisplayComponent::BindScenario(AMiniGameScenario* InScenario)
{
    if (IsValid(Scenario)) Scenario->OnPresentationChanged.RemoveDynamic(this, &UMiniGameDisplayComponent::Refresh);
    Scenario = InScenario;
    if (IsValid(Scenario))
    {
        Scenario->OnPresentationChanged.AddUniqueDynamic(this, &UMiniGameDisplayComponent::Refresh);
        Refresh();
    }
}
void UMiniGameDisplayComponent::BeginPlay()
{
    Super::BeginPlay();
    BindScenario(Scenario);
}
void UMiniGameDisplayComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(Scenario)) Scenario->OnPresentationChanged.RemoveDynamic(this, &UMiniGameDisplayComponent::Refresh);
    Super::EndPlay(Reason);
}
void UMiniGameDisplayComponent::Refresh()
{
    if (!IsValid(Scenario) || !Scenario->Definition || !Scenario->Definition->Style) return;
    if (!GetWidgetClass()) SetWidgetClass(Scenario->Definition->Style->PanelClass);
    InitWidget();
    if (auto* Panel = Cast<UMiniGamePanelWidget>(GetUserWidgetObject())) Panel->Present(Scenario->GetPresentation(PanelId, GetOwner()), Scenario->Definition->Style);
}
