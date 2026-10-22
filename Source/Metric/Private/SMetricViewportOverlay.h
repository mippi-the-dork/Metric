#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SVerticalBox;
class SWidget;
class SBorder;

enum class EMetricHUDAnchor : uint8;

class SMetricViewportOverlay final : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMetricViewportOverlay) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetTargetEnabled(bool bEnabled);
    virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
    void Rebuild();
    FText BuildSummaryText() const;
    FText BuildPinnedResultsText() const;
    FText BuildPinnedSourceText() const;
    FText BuildComparisonText() const;
    FText BuildBenchmarkSelectionText() const;
    FText BuildLegendButtonText() const;
    EVisibility GetHUDVisibility() const;
    EVisibility GetLegendVisibility() const;
    TSharedRef<SWidget> BuildBenchmarkMenu();
    TSharedRef<SWidget> BuildVisibilityMenu(bool bForBenchmark);
    TSharedRef<SWidget> BuildLegendContent();
    FReply SelectPin(int32 Index);
    FReply FocusSelectedPin();
    FReply DeleteSelectedPin();
    FReply ReplaceSelectedPin();
    FReply ToggleLegend();
    void OnVisibilityChanged(ECheckBoxState NewState, int32 LayerIndex, bool bVisual, bool bForBenchmark);
    ECheckBoxState GetVisibilityCheckedState(int32 LayerIndex, bool bVisual, bool bForBenchmark) const;
    void CommitRename(const FText& NewText, ETextCommit::Type CommitType, int32 Index);

    TSharedPtr<SVerticalBox> RootBox;
    TSharedPtr<SBorder> LegendBorder;
    int32 LastPinCount = INDEX_NONE;
    int32 LastSelectedPin = INDEX_NONE;
    bool bLastActive = false;
    bool bLastLegendVisible = true;
    bool bTargetEnabled = true;
};
