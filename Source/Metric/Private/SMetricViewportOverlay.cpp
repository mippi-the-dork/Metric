#include "SMetricViewportOverlay.h"

#include "MetricEditorSettings.h"
#include "MetricStyle.h"
#include "MetricSubsystem.h"
#include "MetricTypes.h"
#include "Editor.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SMetricViewportOverlay"

namespace MetricOverlay
{
    enum class ELayer : int32
    {
        Hull = 0,
        Clearance,
        Ground,
        Fit,
        Threshold,
        Turn,
        Sensors,
        SensorField,
        Ranges,
        Locomotion,
        Trajectory,
        Camera,
        Sightline,
        Count
    };

    struct FLayerDesc
    {
        const TCHAR* Name;
        const TCHAR* Prefix;
    };

    static const FLayerDesc Layers[] =
    {
        { TEXT("Hull"), TEXT("HULL") },
        { TEXT("Clearance"), TEXT("CLEARANCE") },
        { TEXT("Ground"), TEXT("GROUND") },
        { TEXT("Fit"), TEXT("FIT") },
        { TEXT("Threshold"), TEXT("THRESHOLD") },
        { TEXT("Turn Clearance"), TEXT("TURN") },
        { TEXT("Sensors"), TEXT("SENSORS") },
        { TEXT("Sensor Field"), TEXT("SENSOR") },
        { TEXT("Engagement Ranges"), TEXT("RANGE") },
        { TEXT("Locomotion"), TEXT("MOVE") },
        { TEXT("Trajectory"), TEXT("TRAJECTORY") },
        { TEXT("Camera"), TEXT("CAMERA") },
        { TEXT("Sightline"), TEXT("VIEW") }
    };

    static UMetricSubsystem* GetSubsystem()
    {
        return GEditor ? GEditor->GetEditorSubsystem<UMetricSubsystem>() : nullptr;
    }

    static bool GetVisibilityFlag(const FMetricLayerVisibility& V, int32 LayerIndex, bool bVisual)
    {
        switch (static_cast<ELayer>(LayerIndex))
        {
        case ELayer::Hull:       return bVisual ? V.bHullVisual : V.bHullResults;
        case ELayer::Clearance:  return bVisual ? V.bClearanceVisual : V.bClearanceResults;
        case ELayer::Ground:     return bVisual ? V.bGroundVisual : V.bGroundResults;
        case ELayer::Fit:        return bVisual ? V.bFitVisual : V.bFitResults;
        case ELayer::Threshold:  return bVisual ? V.bThresholdVisual : V.bThresholdResults;
        case ELayer::Turn:       return bVisual ? V.bTurnVisual : V.bTurnResults;
        case ELayer::Sensors:    return bVisual ? V.bSensorsVisual : V.bSensorsResults;
        case ELayer::SensorField:return bVisual ? V.bSensorFieldVisual : V.bSensorFieldResults;
        case ELayer::Ranges:     return bVisual ? V.bRangesVisual : V.bRangesResults;
        case ELayer::Locomotion: return bVisual ? V.bLocomotionVisual : V.bLocomotionResults;
        case ELayer::Trajectory: return bVisual ? V.bTrajectoryVisual : V.bTrajectoryResults;
        case ELayer::Camera:     return bVisual ? V.bCameraVisual : V.bCameraResults;
        case ELayer::Sightline:  return bVisual ? V.bSightlineVisual : V.bSightlineResults;
        default: return true;
        }
    }

    static void SetVisibilityFlag(FMetricLayerVisibility& V, int32 LayerIndex, bool bVisual, bool bValue)
    {
        switch (static_cast<ELayer>(LayerIndex))
        {
        case ELayer::Hull:       if (bVisual) V.bHullVisual = bValue; else V.bHullResults = bValue; break;
        case ELayer::Clearance:  if (bVisual) V.bClearanceVisual = bValue; else V.bClearanceResults = bValue; break;
        case ELayer::Ground:     if (bVisual) V.bGroundVisual = bValue; else V.bGroundResults = bValue; break;
        case ELayer::Fit:        if (bVisual) V.bFitVisual = bValue; else V.bFitResults = bValue; break;
        case ELayer::Threshold:  if (bVisual) V.bThresholdVisual = bValue; else V.bThresholdResults = bValue; break;
        case ELayer::Turn:       if (bVisual) V.bTurnVisual = bValue; else V.bTurnResults = bValue; break;
        case ELayer::Sensors:    if (bVisual) V.bSensorsVisual = bValue; else V.bSensorsResults = bValue; break;
        case ELayer::SensorField:if (bVisual) V.bSensorFieldVisual = bValue; else V.bSensorFieldResults = bValue; break;
        case ELayer::Ranges:     if (bVisual) V.bRangesVisual = bValue; else V.bRangesResults = bValue; break;
        case ELayer::Locomotion: if (bVisual) V.bLocomotionVisual = bValue; else V.bLocomotionResults = bValue; break;
        case ELayer::Trajectory: if (bVisual) V.bTrajectoryVisual = bValue; else V.bTrajectoryResults = bValue; break;
        case ELayer::Camera:     if (bVisual) V.bCameraVisual = bValue; else V.bCameraResults = bValue; break;
        case ELayer::Sightline:  if (bVisual) V.bSightlineVisual = bValue; else V.bSightlineResults = bValue; break;
        default: break;
        }
    }

    static FString ResultSummary(const FMetricLiveState& State, const FMetricLayerVisibility& V)
    {
        TArray<FString> Lines;
        if (State.EntryType == EMetricCycleEntryType::NativePreviewMesh)
        {
            return TEXT("Visual scale reference only");
        }

        if (V.bHullResults && State.bShowHull)
            Lines.Add(FString::Printf(TEXT("HULL        %.0f W x %.0f H x %.0f L"), State.Hull.Width, State.Hull.Height, State.Hull.Length));
        if (V.bClearanceResults && State.Clearance.bVisible)
        {
            if (State.Clearance.bBlocked) Lines.Add(FString::Printf(TEXT("CLEARANCE   FAIL   %.0fu / %.0fu"), State.Clearance.ClearDistance, State.Clearance.ProbeDistance));
            else if (State.Clearance.bNearThreshold) Lines.Add(FString::Printf(TEXT("CLEARANCE   TIGHT  %.0fu"), State.Clearance.ProbeDistance));
            else Lines.Add(FString::Printf(TEXT("CLEARANCE   PASS   %.0fu+"), State.Clearance.ProbeDistance));
        }
        if (V.bGroundResults && State.GroundValidation.bVisible)
            Lines.Add(FString::Printf(TEXT("GROUND      %s   %d/%d"), State.GroundValidation.bPass ? TEXT("PASS") : TEXT("FAIL"), State.GroundValidation.TotalSamples - State.GroundValidation.FailedSamples, State.GroundValidation.TotalSamples));
        if (V.bFitResults && State.FitAnalysis.bVisible)
            Lines.Add(FString::Printf(TEXT("FIT         %s   W %.0fu  H %.0fu"), State.FitAnalysis.bPass ? (State.FitAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL"), State.FitAnalysis.MinimumAvailableWidth, State.FitAnalysis.MinimumAvailableHeight));
        if (V.bThresholdResults && State.ThresholdAnalysis.bVisible)
            Lines.Add(FString::Printf(TEXT("THRESHOLD   %s   %d/%d"), State.ThresholdAnalysis.bPass ? (State.ThresholdAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL"), State.ThresholdAnalysis.TotalSamples - State.ThresholdAnalysis.FailedSamples, State.ThresholdAnalysis.TotalSamples));
        if (V.bTurnResults && State.TurnClearance.bVisible)
        {
            auto TurnText = [](const FMetricTurnPathResult& Path) -> FString
            {
                if (!Path.bTested) return TEXT("OFF");
                if (Path.bBlocked)
                {
                    return FString::Printf(TEXT("BLOCKED %.0f deg"), Path.ClearAngle);
                }
                return FString::Printf(TEXT("PASS %.0f deg"), Path.ClearAngle);
            };
            Lines.Add(FString::Printf(TEXT("TURN        L %s | R %s"), *TurnText(State.TurnClearance.Left), *TurnText(State.TurnClearance.Right)));
        }
        if (V.bSensorsResults && State.Sensors.bShowSensorHeights)
            Lines.Add(FString::Printf(TEXT("SENSORS     primary %.0fu   low %.0fu"), State.Sensors.PrimarySensorZOffset, State.Sensors.LowProfileSensorZOffset));
        if (V.bCameraResults && State.CameraProbe.bVisible)
        {
            const TCHAR* Status = State.CameraProbe.bBelowMinimum ? TEXT("TOO CLOSE") : (State.CameraProbe.bCompressed ? TEXT("COMPRESSED") : TEXT("CLEAR"));
            Lines.Add(FString::Printf(TEXT("CAMERA      %s   %.0fu / %.0fu"), Status, State.CameraProbe.ResolvedDistance, State.CameraProbe.TargetDistance));
        }
        if (V.bSightlineResults && State.CameraSightline.bVisible)
        {
            const TCHAR* Status = State.CameraSightline.bOccluded ? TEXT("OCCLUDED") : (!State.CameraSightline.bFramingFail ? TEXT("PASS") : TEXT("FRAMING FAIL"));
            Lines.Add(FString::Printf(TEXT("VIEW        %s"), Status));
        }
        if (V.bTrajectoryResults && State.Trajectory.bVisible)
        {
            const TCHAR* Status = State.Trajectory.bBlocked ? TEXT("BLOCKED") : (State.Trajectory.bLandingValid ? TEXT("LANDING OK") : TEXT("NO LANDING"));
            Lines.Add(FString::Printf(TEXT("TRAJECTORY  %s   %.0fu"), Status, State.Trajectory.HorizontalDistance));
        }
        if (V.bSensorFieldResults && State.SensorField.bVisible) Lines.Add(FString::Printf(TEXT("SENSOR      %.0f%% CLEAR"), State.SensorField.ClearPercent));
        if (V.bLocomotionResults && State.Locomotion.bShowLocomotionBands) Lines.Add(FString::Printf(TEXT("MOVE        %.0fu/s   %.1f / %.1f / %.1fs"), State.Locomotion.Speed, State.Locomotion.NearTime, State.Locomotion.MidTime, State.Locomotion.FarTime));
        if (V.bRangesResults && State.Ranges.bShowEngagementRanges) Lines.Add(FString::Printf(TEXT("RANGE       %.0f / %.0f / %.0fu"), State.Ranges.ShortRange, State.Ranges.MediumRange, State.Ranges.LongRange));
        return Lines.IsEmpty() ? TEXT("No metric results enabled") : FString::Join(Lines, TEXT("\n"));
    }

    static FString FilterCapturedResults(const FString& CapturedResults, const FMetricLayerVisibility& V)
    {
        TArray<FString> InputLines;
        CapturedResults.ParseIntoArrayLines(InputLines, false);
        TArray<FString> OutputLines;
        for (const FString& Line : InputLines)
        {
            FString Trimmed = Line;
            Trimmed.TrimStartInline();
            bool bKeep = true;
            for (int32 LayerIndex = 0; LayerIndex < static_cast<int32>(ELayer::Count); ++LayerIndex)
            {
                if (Trimmed.StartsWith(Layers[LayerIndex].Prefix))
                {
                    bKeep = GetVisibilityFlag(V, LayerIndex, false);
                    break;
                }
            }
            if (bKeep) OutputLines.Add(Line);
        }
        return OutputLines.IsEmpty() ? TEXT("No pinned metric results enabled") : FString::Join(OutputLines, TEXT("\n"));
    }

    static void ResolveAnchor(EMetricHUDAnchor Anchor, EHorizontalAlignment& H, EVerticalAlignment& V, FMargin& Padding, bool bToolbarAware)
    {
        H = HAlign_Left;
        V = VAlign_Top;
        const float Top = bToolbarAware ? 42.0f : 12.0f;
        Padding = FMargin(12.0f, Top, 0.0f, 0.0f);
        switch (Anchor)
        {
        case EMetricHUDAnchor::MiddleLeft: H = HAlign_Left; V = VAlign_Center; Padding = FMargin(12,0,0,0); break;
        case EMetricHUDAnchor::BottomLeft: H = HAlign_Left; V = VAlign_Bottom; Padding = FMargin(12,0,0,12); break;
        case EMetricHUDAnchor::TopMiddle: H = HAlign_Center; V = VAlign_Top; Padding = FMargin(0,Top,0,0); break;
        case EMetricHUDAnchor::BottomMiddle: H = HAlign_Center; V = VAlign_Bottom; Padding = FMargin(0,0,0,12); break;
        case EMetricHUDAnchor::TopRight: H = HAlign_Right; V = VAlign_Top; Padding = FMargin(0,Top,12,0); break;
        case EMetricHUDAnchor::MiddleRight: H = HAlign_Right; V = VAlign_Center; Padding = FMargin(0,0,12,0); break;
        case EMetricHUDAnchor::BottomRight: H = HAlign_Right; V = VAlign_Bottom; Padding = FMargin(0,0,12,12); break;
        case EMetricHUDAnchor::TopLeft:
        default: break;
        }
    }

    static bool IsSelectedViewportEnabled(const UMetricEditorSettings* Settings, int32 ViewportNumber)
    {
        if (!Settings || ViewportNumber == INDEX_NONE)
        {
            return false;
        }
        switch (ViewportNumber)
        {
        case 1: return Settings->bShowHUDInViewport1;
        case 2: return Settings->bShowHUDInViewport2;
        case 3: return Settings->bShowHUDInViewport3;
        case 4: return Settings->bShowHUDInViewport4;
        default: return false;
        }
    }

    static FString CleanProfileLabel(const FText& Label)
    {
        FString Value = Label.ToString();
        Value.RemoveFromStart(TEXT("METRIC - "));
        Value.RemoveFromStart(TEXT("Metric - "));
        return Value;
    }

    static TSharedRef<SWidget> MakeSwatch(const FLinearColor& Color)
    {
        return SNew(SBox).WidthOverride(10.0f).HeightOverride(10.0f)
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::GetBrush("WhiteBrush"))
            .BorderBackgroundColor(Color)
            .Padding(0.0f)
        ];
    }

    static void AddLegendRow(const TSharedRef<SVerticalBox>& Box, const FText& Label, const TArray<FLinearColor>& Colors)
    {
        TSharedRef<SHorizontalBox> Row = SNew(SHorizontalBox);
        for (const FLinearColor& Color : Colors)
        {
            Row->AddSlot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,3,0)[MakeSwatch(Color)];
        }
        Row->AddSlot().FillWidth(1.0f).VAlign(VAlign_Center)
        [
            SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle("SmallFont"))
        ];
        Box->AddSlot().AutoHeight().Padding(0,1)[Row];
    }

    static FSlateColor ResultValueColor(const FString& Value, const UMetricEditorSettings* Settings)
    {
        if (!Settings)
        {
            return FSlateColor::UseForeground();
        }
        if (Value.Contains(TEXT("FAIL")) || Value.Contains(TEXT("BLOCKED")) || Value.Contains(TEXT("TOO CLOSE")) || Value.Contains(TEXT("OCCLUDED")) || Value.Contains(TEXT("NO LANDING")))
        {
            return FSlateColor(Settings->ClearanceFailColor);
        }
        if (Value.Contains(TEXT("TIGHT")) || Value.Contains(TEXT("COMPRESSED")) || Value.Contains(TEXT("FRAMING FAIL")))
        {
            return FSlateColor(Settings->ClearanceWarningColor);
        }
        if (Value.Contains(TEXT("PASS")) || Value.Contains(TEXT("CLEAR")) || Value.Contains(TEXT("LANDING OK")) || Value.Contains(TEXT("OPEN")))
        {
            return FSlateColor(Settings->ClearancePassColor);
        }
        return FSlateColor::UseForeground();
    }

    static void AddResultRows(const TSharedRef<SVerticalBox>& Box, const FString& Summary, const UMetricEditorSettings* Settings)
    {
        TArray<FString> Lines;
        Summary.ParseIntoArrayLines(Lines, false);
        for (const FString& RawLine : Lines)
        {
            FString Line = RawLine;
            Line.TrimStartAndEndInline();
            if (Line.IsEmpty())
            {
                continue;
            }

            FString Label;
            FString Value = Line;
            for (int32 LayerIndex = 0; LayerIndex < static_cast<int32>(ELayer::Count); ++LayerIndex)
            {
                const FString Prefix(Layers[LayerIndex].Prefix);
                if (Line.StartsWith(Prefix))
                {
                    Label = Layers[LayerIndex].Name;
                    Value = Line.Mid(Prefix.Len());
                    Value.TrimStartInline();
                    break;
                }
            }

            if (Label.IsEmpty())
            {
                Box->AddSlot().AutoHeight().Padding(4,1)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Value))
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                    .AutoWrapText(true)
                ];
                continue;
            }

            Box->AddSlot().AutoHeight().Padding(4,1)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Label))
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
                + SHorizontalBox::Slot().FillWidth(0.58f).HAlign(HAlign_Right).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Value))
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ColorAndOpacity(ResultValueColor(Value, Settings))
                    .Justification(ETextJustify::Right)
                ]
            ];
        }
    }

    static TSharedRef<SWidget> BuildResultRowsWidget(const FString& Summary, const UMetricEditorSettings* Settings)
    {
        TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
        AddResultRows(Rows, Summary, Settings);
        return Rows;
    }

}

void SMetricViewportOverlay::Construct(const FArguments& InArgs)
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    const EMetricHUDAnchor HUDAnchor = Settings ? Settings->HUDAnchor : EMetricHUDAnchor::TopLeft;
    const EMetricHUDAnchor LegendAnchor = Settings ? Settings->LegendAnchor : EMetricHUDAnchor::BottomLeft;

    EHorizontalAlignment HudH, LegendH;
    EVerticalAlignment HudV, LegendV;
    FMargin HudPadding, LegendPadding;
    MetricOverlay::ResolveAnchor(HUDAnchor, HudH, HudV, HudPadding, true);
    MetricOverlay::ResolveAnchor(LegendAnchor, LegendH, LegendV, LegendPadding, true);

    ChildSlot
    [
        SNew(SOverlay)
        + SOverlay::Slot().HAlign(HudH).VAlign(HudV).Padding(HudPadding)
        [
            SAssignNew(HUDSizeBox, SBox).WidthOverride(390.0f)
            .Visibility(this, &SMetricViewportOverlay::GetHUDVisibility)
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
                .Padding(FMargin(8.0f, 6.0f))
                [ SAssignNew(RootBox, SVerticalBox) ]
            ]
        ]
        + SOverlay::Slot().HAlign(LegendH).VAlign(LegendV).Padding(LegendPadding)
        [
            SAssignNew(LegendBorder, SBorder)
            .Visibility(this, &SMetricViewportOverlay::GetLegendVisibility)
            .BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
            .Padding(FMargin(8.0f, 6.0f))
            [ BuildLegendContent() ]
        ]
    ];
    Rebuild();
}


void SMetricViewportOverlay::SetTargetEnabled(bool bEnabled)
{
    bTargetEnabled = bEnabled;

    // Keep the lightweight overlay container alive even when this viewport is
    // not the current target. Collapsed Slate widgets do not tick, which meant
    // an inactive/untargeted Metric HUD could never reliably notice that it
    // should become visible again. HUD and Legend children own visibility.
    SetVisibility(EVisibility::SelfHitTestInvisible);
}

void SMetricViewportOverlay::SetViewportNumber(int32 InViewportNumber)
{
    ViewportNumber = InViewportNumber;
}

EVisibility SMetricViewportOverlay::GetHUDVisibility() const
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    const bool bShowHUD = !Settings || Settings->bShowViewportHUD;
    const bool bSelectedMode = Settings && Settings->HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports;
    const bool bViewportEnabled = bSelectedMode ? MetricOverlay::IsSelectedViewportEnabled(Settings, ViewportNumber) : bTargetEnabled;
    return (bViewportEnabled && bLastActive && bShowHUD)
        ? EVisibility::SelfHitTestInvisible
        : EVisibility::Collapsed;
}

void SMetricViewportOverlay::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
    SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    const bool bActive = Subsystem && Subsystem->IsMetricActive();
    const int32 PinCount = Subsystem ? Subsystem->GetPinnedBenchmarkCount() : 0;
    const int32 Selected = Subsystem ? Subsystem->GetSelectedPinnedBenchmarkIndex() : INDEX_NONE;
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    const bool bLegend = Settings ? Settings->bShowLegend : true;
    const bool bMinimized = Settings ? Settings->bHUDMinimized : false;
    const FString LiveSummary = (Subsystem && Settings) ? MetricOverlay::ResultSummary(Subsystem->GetLiveState(), Settings->LiveVisibility) : FString();
    FString PinnedSummary;
    if (Subsystem)
    {
        if (const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(Selected))
        {
            PinnedSummary = MetricOverlay::FilterCapturedResults(Pin->CapturedResults, Pin->Visibility);
        }
    }
    const FString ProfileLabel = Subsystem ? Subsystem->GetLiveState().Label.ToString() : FString();
    const bool bStructureChanged =
        bActive != bLastActive ||
        PinCount != LastPinCount ||
        Selected != LastSelectedPin ||
        bLegend != bLastLegendVisible ||
        bMinimized != bLastHUDMinimized ||
        ProfileLabel != LastProfileLabel;

    if (bStructureChanged)
    {
        Rebuild();
    }
    else if (LiveSummary != LastLiveSummary || PinnedSummary != LastPinnedSummary)
    {
        RefreshDynamicResultContent(LiveSummary, PinnedSummary);
    }
}

void SMetricViewportOverlay::RefreshDynamicResultContent(const FString& LiveSummary, const FString& PinnedSummary)
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();

    if (LiveSummary != LastLiveSummary)
    {
        LastLiveSummary = LiveSummary;
        if (LiveResultsBox.IsValid())
        {
            LiveResultsBox->SetContent(MetricOverlay::BuildResultRowsWidget(LastLiveSummary, Settings));
        }
    }

    if (PinnedSummary != LastPinnedSummary)
    {
        LastPinnedSummary = PinnedSummary;
        if (PinnedResultsBox.IsValid())
        {
            PinnedResultsBox->SetContent(MetricOverlay::BuildResultRowsWidget(LastPinnedSummary, Settings));
        }
    }
}

void SMetricViewportOverlay::Rebuild()
{
    if (!RootBox.IsValid()) return;
    RootBox->ClearChildren();
    LiveResultsBox.Reset();
    PinnedResultsBox.Reset();

    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    bLastActive = Subsystem && Subsystem->IsMetricActive();
    LastPinCount = Subsystem ? Subsystem->GetPinnedBenchmarkCount() : 0;
    LastSelectedPin = Subsystem ? Subsystem->GetSelectedPinnedBenchmarkIndex() : INDEX_NONE;
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    bLastLegendVisible = Settings ? Settings->bShowLegend : true;
    bLastHUDMinimized = Settings ? Settings->bHUDMinimized : false;
    LastLiveSummary = (Subsystem && Settings) ? MetricOverlay::ResultSummary(Subsystem->GetLiveState(), Settings->LiveVisibility) : FString();
    LastProfileLabel = Subsystem ? Subsystem->GetLiveState().Label.ToString() : FString();
    LastPinnedSummary.Reset();
    if (Subsystem)
    {
        if (const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(LastSelectedPin))
        {
            LastPinnedSummary = MetricOverlay::FilterCapturedResults(Pin->CapturedResults, Pin->Visibility);
        }
    }
    SetVisibility(EVisibility::SelfHitTestInvisible);
    if (!bLastActive || !Subsystem) return;

    if (HUDSizeBox.IsValid())
    {
        HUDSizeBox->SetWidthOverride(bLastHUDMinimized ? 44.0f : 390.0f);
    }

    const FSlateBrush* MetricIcon = FMetricStyle::GetBrush(TEXT("Metric.HUDIcon"));
    if (!MetricIcon) MetricIcon = FMetricStyle::GetBrush(TEXT("ClassIcon.MetricPreset"));
    if (!MetricIcon) MetricIcon = FAppStyle::GetBrush(TEXT("Icons.Info"));

    if (bLastHUDMinimized)
    {
        RootBox->AddSlot().AutoHeight()
        [
            SNew(SButton)
            .ToolTipText(LOCTEXT("RestoreHUDTip", "Restore the Metric viewport HUD."))
            .ContentPadding(FMargin(2.0f))
            .OnClicked(this, &SMetricViewportOverlay::ToggleHUDMinimized)
            [ SNew(SBox).WidthOverride(28.0f).HeightOverride(28.0f)[SNew(SImage).Image(MetricIcon).ColorAndOpacity(FLinearColor::White)] ]
        ];
        if (LegendBorder.IsValid()) LegendBorder->SetContent(BuildLegendContent());
        return;
    }

    const FString ProfileName = MetricOverlay::CleanProfileLabel(Subsystem->GetLiveState().Label);
    RootBox->AddSlot().AutoHeight().Padding(0,0,0,4)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,6,0)
        [ SNew(SBox).WidthOverride(22.0f).HeightOverride(22.0f)[SNew(SImage).Image(MetricIcon).ColorAndOpacity(FLinearColor::White)] ]
        + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
        [ SNew(STextBlock).Text(LOCTEXT("MetricHeader", "METRIC")).Font(FAppStyle::GetFontStyle("HeadingExtraSmall")) ]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)
        [
            SNew(SButton)
            .ToolTipText(LOCTEXT("MinimizeHUDTip", "Minimize Metric to its viewport icon while keeping world-space visualization active."))
            .ContentPadding(FMargin(3.0f))
            .OnClicked(this, &SMetricViewportOverlay::ToggleHUDMinimized)
            [ SNew(SImage).Image(FAppStyle::GetBrush("Icons.ChevronDown")) ]
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)
        [
            SNew(SComboButton)
            .ToolTipText(LOCTEXT("EditLiveTip", "Display Options: choose which live Metric layers draw in the viewport and which appear in results."))
            .ButtonContent()[ SNew(SImage).Image(FAppStyle::GetBrush("Icons.Edit")) ]
            .OnGetMenuContent(this, &SMetricViewportOverlay::BuildVisibilityMenu, false)
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)
        [
            SNew(SButton)
            .Text(this, &SMetricViewportOverlay::BuildLegendButtonText)
            .ToolTipText(LOCTEXT("LegendToggleTip", "Show or hide the Metric legend in this viewport."))
            .OnClicked(this, &SMetricViewportOverlay::ToggleLegend)
        ]
    ];

    RootBox->AddSlot().AutoHeight().Padding(28,0,2,3)
    [
        SNew(STextBlock)
        .Text(FText::FromString(ProfileName))
        .Font(FAppStyle::GetFontStyle("SmallFont"))
        .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        .ToolTipText(FText::FromString(ProfileName))
    ];

    auto AddSectionHeader = [this](const FText& Title, bool bExpanded, FReply (SMetricViewportOverlay::*Handler)())
    {
        RootBox->AddSlot().AutoHeight().Padding(0,2,0,1)
        [
            SNew(SButton)
            .ContentPadding(FMargin(2.0f, 2.0f))
            .OnClicked(this, Handler)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,4,0)
                [ SNew(SImage).Image(FAppStyle::GetBrush(bExpanded ? TEXT("Icons.ChevronDown") : TEXT("Icons.ChevronRight"))) ]
                + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
                [ SNew(STextBlock).Text(Title).Font(FAppStyle::GetFontStyle("SmallFont")) ]
            ]
        ];
    };

    AddSectionHeader(LOCTEXT("LiveSection", "LIVE METRIC"), bLiveSectionExpanded, &SMetricViewportOverlay::ToggleLiveSection);
    if (bLiveSectionExpanded)
    {
        RootBox->AddSlot().AutoHeight().Padding(4,1,2,2)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("LiveVisualizerLabel", "Live Visualizer"))
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                SNew(SButton)
                .Text_Lambda([]()
                {
                    const UMetricEditorSettings* CurrentSettings = GetDefault<UMetricEditorSettings>();
                    return (CurrentSettings && CurrentSettings->bShowLiveVisualizer) ? LOCTEXT("LiveVisualizerOn", "On") : LOCTEXT("LiveVisualizerOff", "Off");
                })
                .ToolTipText(LOCTEXT("LiveVisualizerTip", "Show or hide the live world-space Metric visualization without affecting benchmarks or live result rows."))
                .OnClicked(this, &SMetricViewportOverlay::ToggleLiveVisualizer)
            ]
        ];

        const UMetricEditorSettings* CurrentSettings = GetDefault<UMetricEditorSettings>();
        RootBox->AddSlot().AutoHeight().Padding(0,0,0,4)
        [
            SAssignNew(LiveResultsBox, SBox)
            [
                MetricOverlay::BuildResultRowsWidget(LastLiveSummary, CurrentSettings)
            ]
        ];
    }

    RootBox->AddSlot().AutoHeight().Padding(0,2,0,2)[SNew(SSeparator)];

    AddSectionHeader(FText::Format(LOCTEXT("BenchmarksSection", "BENCHMARKS ({0})"), FText::AsNumber(LastPinCount)), bBenchmarkSectionExpanded, &SMetricViewportOverlay::ToggleBenchmarkSection);
    if (bBenchmarkSectionExpanded)
    {
        RootBox->AddSlot().AutoHeight().Padding(4,2,2,3)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f)
            [
                SNew(SComboButton)
                .ButtonContent()[ SNew(STextBlock).Text(this, &SMetricViewportOverlay::BuildBenchmarkSelectionText) ]
                .OnGetMenuContent(this, &SMetricViewportOverlay::BuildBenchmarkMenu)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(3,0,0,0)
            [
                SNew(SComboButton)
                .IsEnabled(LastSelectedPin != INDEX_NONE)
                .ToolTipText(LOCTEXT("EditBenchmarkTip", "Display Options: choose which layers this benchmark draws and reports."))
                .ButtonContent()[ SNew(SImage).Image(FAppStyle::GetBrush("Icons.Edit")) ]
                .OnGetMenuContent(this, &SMetricViewportOverlay::BuildVisibilityMenu, true)
            ]
        ];

        if (LastSelectedPin != INDEX_NONE)
        {
            const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(LastSelectedPin);
            if (Pin)
            {
                if (bRenameActive)
                {
                    RootBox->AddSlot().AutoHeight().Padding(4,1,2,3)
                    [ SNew(SEditableTextBox).Text(Pin->Label).SelectAllTextWhenFocused(true).OnTextCommitted(this, &SMetricViewportOverlay::CommitRename, LastSelectedPin) ];
                }

                RootBox->AddSlot().AutoHeight().Padding(4,1,2,4)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0,2,0)
                    [ SNew(SButton).ToolTipText(LOCTEXT("FocusTip", "Focus the selected benchmark in the active viewport.")).ContentPadding(FMargin(4.0f)).OnClicked(this, &SMetricViewportOverlay::FocusSelectedPin)[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Search"))] ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0,2,0)
                    [ SNew(SButton).ToolTipText(LOCTEXT("ReplaceTip", "Replace this benchmark with the current live Metric snapshot.")).ContentPadding(FMargin(4.0f)).OnClicked(this, &SMetricViewportOverlay::ReplaceSelectedPin)[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Refresh"))] ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(0,0,2,0)
                    [ SNew(SButton).ToolTipText(LOCTEXT("RenameTip", "Rename the selected benchmark.")).ContentPadding(FMargin(4.0f)).OnClicked(this, &SMetricViewportOverlay::RenameSelectedPin)[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Edit"))] ]
                    + SHorizontalBox::Slot().AutoWidth()
                    [ SNew(SButton).ToolTipText(LOCTEXT("DeleteTip", "Delete the selected benchmark.")).ContentPadding(FMargin(4.0f)).OnClicked(this, &SMetricViewportOverlay::DeleteSelectedPin)[SNew(SImage).Image(FAppStyle::GetBrush("Icons.Delete"))] ]
                ];
            }
        }
    }

    if (LastSelectedPin != INDEX_NONE)
    {
        RootBox->AddSlot().AutoHeight().Padding(0,1,0,1)[SNew(SSeparator)];
        AddSectionHeader(LOCTEXT("PinnedResultsSection", "PINNED RESULTS"), bPinnedSectionExpanded, &SMetricViewportOverlay::TogglePinnedSection);
        if (bPinnedSectionExpanded)
        {
            RootBox->AddSlot().AutoHeight().Padding(4,1,2,1)[SNew(STextBlock).Text(this, &SMetricViewportOverlay::BuildPinnedSourceText).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())];
            const UMetricEditorSettings* CurrentSettings = GetDefault<UMetricEditorSettings>();
            RootBox->AddSlot().AutoHeight().Padding(0,0,0,4)
            [
                SAssignNew(PinnedResultsBox, SBox)
                [
                    MetricOverlay::BuildResultRowsWidget(LastPinnedSummary, CurrentSettings)
                ]
            ];
        }

        AddSectionHeader(LOCTEXT("ComparisonSection", "LIVE vs PIN"), bComparisonSectionExpanded, &SMetricViewportOverlay::ToggleComparisonSection);
        if (bComparisonSectionExpanded)
        {
            RootBox->AddSlot().AutoHeight().Padding(4,1,2,4)
            [ SNew(STextBlock).Text(this, &SMetricViewportOverlay::BuildComparisonText).Font(FAppStyle::GetFontStyle("SmallFont")).AutoWrapText(true).WrapTextAt(322.0f) ];
        }
    }

    RootBox->AddSlot().AutoHeight().Padding(2,5,0,0)
    [ SNew(STextBlock).Text(LOCTEXT("Shortcuts", "Ctrl+\\ Pin   |   Alt+\\ Next   |   Ctrl+Shift+\\ Clear")).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground()) ];

    if (LegendBorder.IsValid())
    {
        LegendBorder->SetContent(BuildLegendContent());
    }
}

TSharedRef<SWidget> SMetricViewportOverlay::BuildBenchmarkMenu()
{
    TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    List->AddSlot().AutoHeight().Padding(2,1)
    [
        SNew(SButton)
        .HAlign(HAlign_Left)
        .Text(LOCTEXT("BenchmarkNone", "None"))
        .ToolTipText(LOCTEXT("BenchmarkNoneTip", "Show only the live Metric lens and no selected benchmark analysis."))
        .OnClicked_Lambda([this]()
        {
            return SelectPin(INDEX_NONE);
        })
    ];
    if (!Subsystem || Subsystem->GetPinnedBenchmarkCount() == 0)
    {
        List->AddSlot().AutoHeight().Padding(8,6)[SNew(STextBlock).Text(LOCTEXT("NoBenchmarks", "No pinned benchmarks"))];
    }
    else
    {
        for (int32 Index = 0; Index < Subsystem->GetPinnedBenchmarkCount(); ++Index)
        {
            const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(Index);
            if (!Pin) continue;
            const FString Label = FString::Printf(TEXT("P%d  %s"), Index + 1, *Pin->Label.ToString());
            List->AddSlot().AutoHeight().Padding(2,1)[SNew(SButton).HAlign(HAlign_Left).Text(FText::FromString(Label)).OnClicked_Lambda([this, Index]()
            {
                return SelectPin(Index);
            })];
        }
    }
    return SNew(SBox).WidthOverride(350.0f).MaxDesiredHeight(280.0f)[SNew(SScrollBox) + SScrollBox::Slot()[List]];
}

TSharedRef<SWidget> SMetricViewportOverlay::BuildVisibilityMenu(bool bForBenchmark)
{
    TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
    Box->AddSlot().AutoHeight().Padding(8,6,8,4)
    [
        SNew(STextBlock)
        .Text(bForBenchmark ? LOCTEXT("BenchmarkVisibility", "BENCHMARK DISPLAY") : LOCTEXT("LiveVisibility", "LIVE DISPLAY"))
        .Font(FAppStyle::GetFontStyle("SmallFont"))
    ];

    Box->AddSlot().AutoHeight().Padding(6,0,6,5)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("PresetAll", "All")).ToolTipText(LOCTEXT("PresetAllTip", "Show all visuals and all result rows.")).OnClicked_Lambda([this, bForBenchmark]() { return ApplyVisibilityPreset(0, bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("PresetVisual", "Visuals")).ToolTipText(LOCTEXT("PresetVisualTip", "Show world visuals only.")).OnClicked_Lambda([this, bForBenchmark]() { return ApplyVisibilityPreset(1, bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("PresetResults", "Results")).ToolTipText(LOCTEXT("PresetResultsTip", "Show HUD results only.")).OnClicked_Lambda([this, bForBenchmark]() { return ApplyVisibilityPreset(2, bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("PresetNone", "None")).ToolTipText(LOCTEXT("PresetNoneTip", "Hide all Metric layers for this context.")).OnClicked_Lambda([this, bForBenchmark]() { return ApplyVisibilityPreset(3, bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("PresetReset", "Reset")).ToolTipText(LOCTEXT("PresetResetTip", "Restore the default Metric layer visibility.")).OnClicked_Lambda([this, bForBenchmark]() { return ApplyVisibilityPreset(4, bForBenchmark); })]
    ];

    Box->AddSlot().AutoHeight().Padding(8,0,8,2)[SNew(SSeparator)];
    Box->AddSlot().AutoHeight().Padding(8,2,8,3)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(STextBlock).Text(LOCTEXT("LayerCol", "Layer")).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
        + SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(STextBlock).Text(LOCTEXT("VisualCol", "Visual")).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
        + SHorizontalBox::Slot().AutoWidth().Padding(12,0,2,0)[SNew(STextBlock).Text(LOCTEXT("ResultsCol", "Results")).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
    ];

    for (int32 Index = 0; Index < static_cast<int32>(MetricOverlay::ELayer::Count); ++Index)
    {
        Box->AddSlot().AutoHeight().Padding(8,1)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)[SNew(STextBlock).Text(FText::FromString(MetricOverlay::Layers[Index].Name)).Font(FAppStyle::GetFontStyle("SmallFont"))]
            + SHorizontalBox::Slot().AutoWidth().Padding(12,0).VAlign(VAlign_Center)
            [ SNew(SCheckBox).IsChecked(this, &SMetricViewportOverlay::GetVisibilityCheckedState, Index, true, bForBenchmark).OnCheckStateChanged(this, &SMetricViewportOverlay::OnVisibilityChanged, Index, true, bForBenchmark) ]
            + SHorizontalBox::Slot().AutoWidth().Padding(17,0,6,0).VAlign(VAlign_Center)
            [ SNew(SCheckBox).IsChecked(this, &SMetricViewportOverlay::GetVisibilityCheckedState, Index, false, bForBenchmark).OnCheckStateChanged(this, &SMetricViewportOverlay::OnVisibilityChanged, Index, false, bForBenchmark) ]
        ];
    }
    return SNew(SBox).WidthOverride(330.0f)[Box];
}

TSharedRef<SWidget> SMetricViewportOverlay::BuildLegendContent()
{
    TSharedRef<SVerticalBox> Box = SNew(SVerticalBox);
    Box->AddSlot().AutoHeight().Padding(0,0,0,4)[SNew(STextBlock).Text(LOCTEXT("LegendHeader", "METRIC LEGEND")).Font(FAppStyle::GetFontStyle("SmallFont"))];

    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    if (!Subsystem || !Settings)
    {
        return Box;
    }

    const FMetricLiveState& State = Subsystem->GetLiveState();
    if (State.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        Box->AddSlot().AutoHeight()[SNew(STextBlock).Text(LOCTEXT("NativeLegend", "Visual scale reference only")).Font(FAppStyle::GetFontStyle("SmallFont"))];
        return Box;
    }

    const FMetricLayerVisibility& V = Settings->LiveVisibility;
    if (State.bShowHull && V.bHullVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendHull", "Hull / footprint"), { Settings->HullColor });
    if (State.Clearance.bVisible && V.bClearanceVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendClearance", "Clearance: pass / tight / fail"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.GroundValidation.bVisible && V.bGroundVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendGround", "Ground: pass / step / slope"), { Settings->GroundPassColor, Settings->GroundStepFailColor, Settings->GroundSlopeFailColor });
    if (State.FitAnalysis.bVisible && V.bFitVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendFit", "Fit: pass / tight / fail"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.ThresholdAnalysis.bVisible && V.bThresholdVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendThreshold", "Threshold: pass / tight / fail"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.TurnClearance.bVisible && V.bTurnVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendTurn", "Turn clearance"), { Settings->ClearancePassColor, Settings->ClearanceFailColor });
    if (State.Sensors.bShowSensorHeights && V.bSensorsVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendSensors", "Sensor heights"), { Settings->SensorColor });
    if (State.SensorField.bVisible && V.bSensorFieldVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendSensorField", "Sensor field / blocked"), { Settings->SensorFieldColor, Settings->SensorFieldBlockedColor });
    if (State.Ranges.bShowEngagementRanges && V.bRangesVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendRange", "Engagement ranges"), { Settings->EngagementRangeColor });
    if (State.Locomotion.bShowLocomotionBands && V.bLocomotionVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendMove", "Locomotion bands"), { Settings->LocomotionBandColor });
    if (State.Trajectory.bVisible && V.bTrajectoryVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendTrajectory", "Trajectory / landing / blocked"), { Settings->TrajectoryColor, Settings->TrajectoryLandingColor, Settings->TrajectoryBlockedColor });
    if (State.CameraProbe.bVisible && V.bCameraVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendCamera", "Camera / compressed"), { Settings->CameraProbeColor, Settings->CameraCompressedColor });
    if (State.CameraSightline.bVisible && V.bSightlineVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendView", "Camera sightline"), { Settings->CameraSightlineColor });
    if (Subsystem->GetPinnedBenchmarkCount() > 0) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendPin", "Pinned benchmark"), { Settings->PinnedBenchmarkColor });
    return Box;
}

FText SMetricViewportOverlay::BuildSummaryText() const
{
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    return (Subsystem && Settings) ? FText::FromString(MetricOverlay::ResultSummary(Subsystem->GetLiveState(), Settings->LiveVisibility)) : FText::GetEmpty();
}

FText SMetricViewportOverlay::BuildBenchmarkSelectionText() const
{
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    if (!Subsystem || Subsystem->GetSelectedPinnedBenchmarkIndex() == INDEX_NONE) return LOCTEXT("SelectBenchmark", "None");
    const int32 Index = Subsystem->GetSelectedPinnedBenchmarkIndex();
    const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(Index);
    return Pin ? FText::FromString(FString::Printf(TEXT("P%d  %s"), Index + 1, *Pin->Label.ToString())) : LOCTEXT("SelectBenchmark2", "None");
}

FText SMetricViewportOverlay::BuildPinnedResultsText() const
{
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    if (!Subsystem) return FText::GetEmpty();
    const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(Subsystem->GetSelectedPinnedBenchmarkIndex());
    if (!Pin) return FText::GetEmpty();
    if (Pin->CapturedResults.IsEmpty()) return LOCTEXT("LegacyPinNoResults", "Legacy benchmark: no captured analysis data. Use Replace to capture the current Metric results into this pin.");
    return FText::FromString(MetricOverlay::FilterCapturedResults(Pin->CapturedResults, Pin->Visibility));
}

FText SMetricViewportOverlay::BuildPinnedSourceText() const
{
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    if (!Subsystem) return FText::GetEmpty();
    const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(Subsystem->GetSelectedPinnedBenchmarkIndex());
    if (!Pin) return FText::GetEmpty();
    const FText& Source = Pin->SourceProfileLabel.IsEmpty() ? Pin->Label : Pin->SourceProfileLabel;
    return FText::Format(LOCTEXT("PinnedSource", "Captured from: {0}"), Source);
}

FText SMetricViewportOverlay::BuildComparisonText() const
{
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    if (!Subsystem || !Subsystem->GetPinnedBenchmark(Subsystem->GetSelectedPinnedBenchmarkIndex())) return FText::GetEmpty();
    const FMetricPinnedBenchmark& Pin = *Subsystem->GetPinnedBenchmark(Subsystem->GetSelectedPinnedBenchmarkIndex());
    const FMetricLiveState& Live = Subsystem->GetLiveState();
    if (!Live.bHasHit) return LOCTEXT("NoLiveCompare", "Move the Metric lens onto valid geometry to compare.");
    return FText::FromString(FString::Printf(TEXT("P%d  |  %.0fu away  |  Width %+.0fu  Height %+.0fu"), Subsystem->GetSelectedPinnedBenchmarkIndex()+1, FVector::Distance(Live.Location, Pin.WorldLocation), Live.Hull.Width - Pin.Hull.Width, Live.Hull.Height - Pin.Hull.Height));
}

FText SMetricViewportOverlay::BuildLegendButtonText() const
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    return (Settings && Settings->bShowLegend) ? LOCTEXT("LegendOn", "Legend") : LOCTEXT("LegendOff", "Legend Off");
}

EVisibility SMetricViewportOverlay::GetLegendVisibility() const
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    const bool bShowLegend = !Settings || Settings->bShowLegend;
    const bool bSelectedMode = Settings && Settings->HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports;
    const bool bViewportEnabled = bSelectedMode ? MetricOverlay::IsSelectedViewportEnabled(Settings, ViewportNumber) : bTargetEnabled;
    return (bViewportEnabled && bLastActive && bShowLegend)
        ? EVisibility::HitTestInvisible
        : EVisibility::Collapsed;
}

FReply SMetricViewportOverlay::ToggleLegend()
{
    if (UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>())
    {
        Settings->bShowLegend = !Settings->bShowLegend;
        Settings->SaveConfig();
        bLastLegendVisible = Settings->bShowLegend;
        Rebuild();
    }
    return FReply::Handled();
}

FReply SMetricViewportOverlay::ToggleHUDMinimized()
{
    if (UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>())
    {
        Settings->bHUDMinimized = !Settings->bHUDMinimized;
        Settings->SaveConfig();
        bLastHUDMinimized = Settings->bHUDMinimized;
        Rebuild();
    }
    return FReply::Handled();
}

FReply SMetricViewportOverlay::ToggleLiveVisualizer()
{
    if (UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>())
    {
        Settings->bShowLiveVisualizer = !Settings->bShowLiveVisualizer;
        Settings->SaveConfig();
        Rebuild();
    }
    return FReply::Handled();
}

ECheckBoxState SMetricViewportOverlay::GetVisibilityCheckedState(int32 LayerIndex, bool bVisual, bool bForBenchmark) const
{
    if (bForBenchmark)
    {
        if (UMetricSubsystem* S = MetricOverlay::GetSubsystem())
        {
            if (const FMetricPinnedBenchmark* Pin = S->GetPinnedBenchmark(S->GetSelectedPinnedBenchmarkIndex()))
                return MetricOverlay::GetVisibilityFlag(Pin->Visibility, LayerIndex, bVisual) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
        }
        return ECheckBoxState::Unchecked;
    }
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    return (!Settings || MetricOverlay::GetVisibilityFlag(Settings->LiveVisibility, LayerIndex, bVisual)) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SMetricViewportOverlay::OnVisibilityChanged(ECheckBoxState NewState, int32 LayerIndex, bool bVisual, bool bForBenchmark)
{
    const bool bEnabled = NewState == ECheckBoxState::Checked;
    if (bForBenchmark)
    {
        if (UMetricSubsystem* S = MetricOverlay::GetSubsystem())
        {
            const int32 Index = S->GetSelectedPinnedBenchmarkIndex();
            if (const FMetricPinnedBenchmark* Pin = S->GetPinnedBenchmark(Index))
            {
                FMetricLayerVisibility V = Pin->Visibility;
                MetricOverlay::SetVisibilityFlag(V, LayerIndex, bVisual, bEnabled);
                S->SetPinnedBenchmarkVisibility(Index, V);
            }
        }
    }
    else if (UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>())
    {
        MetricOverlay::SetVisibilityFlag(Settings->LiveVisibility, LayerIndex, bVisual, bEnabled);
        Settings->SaveConfig();
    }
    Rebuild();
}

FReply SMetricViewportOverlay::ToggleLiveSection() { bLiveSectionExpanded = !bLiveSectionExpanded; Rebuild(); return FReply::Handled(); }
FReply SMetricViewportOverlay::ToggleBenchmarkSection() { bBenchmarkSectionExpanded = !bBenchmarkSectionExpanded; Rebuild(); return FReply::Handled(); }
FReply SMetricViewportOverlay::TogglePinnedSection() { bPinnedSectionExpanded = !bPinnedSectionExpanded; Rebuild(); return FReply::Handled(); }
FReply SMetricViewportOverlay::ToggleComparisonSection() { bComparisonSectionExpanded = !bComparisonSectionExpanded; Rebuild(); return FReply::Handled(); }
FReply SMetricViewportOverlay::RenameSelectedPin() { bRenameActive = !bRenameActive; Rebuild(); return FReply::Handled(); }

FReply SMetricViewportOverlay::ApplyVisibilityPreset(int32 Preset, bool bForBenchmark)
{
    FMetricLayerVisibility V;
    for (int32 LayerIndex = 0; LayerIndex < static_cast<int32>(MetricOverlay::ELayer::Count); ++LayerIndex)
    {
        bool bVisual = true;
        bool bResults = true;
        if (Preset == 1) bResults = false;
        else if (Preset == 2) bVisual = false;
        else if (Preset == 3) { bVisual = false; bResults = false; }
        MetricOverlay::SetVisibilityFlag(V, LayerIndex, true, bVisual);
        MetricOverlay::SetVisibilityFlag(V, LayerIndex, false, bResults);
    }

    if (bForBenchmark)
    {
        if (UMetricSubsystem* S = MetricOverlay::GetSubsystem())
        {
            const int32 Index = S->GetSelectedPinnedBenchmarkIndex();
            if (Index != INDEX_NONE) S->SetPinnedBenchmarkVisibility(Index, V);
        }
    }
    else if (UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>())
    {
        Settings->LiveVisibility = V;
        Settings->SaveConfig();
    }
    Rebuild();
    return FReply::Handled();
}

FReply SMetricViewportOverlay::SelectPin(int32 Index) { if (UMetricSubsystem* S = MetricOverlay::GetSubsystem()) S->SelectPinnedBenchmark(Index); Rebuild(); return FReply::Handled(); }
FReply SMetricViewportOverlay::FocusSelectedPin() { if (UMetricSubsystem* S = MetricOverlay::GetSubsystem()) S->FocusPinnedBenchmark(S->GetSelectedPinnedBenchmarkIndex()); return FReply::Handled(); }
FReply SMetricViewportOverlay::DeleteSelectedPin() { if (UMetricSubsystem* S = MetricOverlay::GetSubsystem()) S->DeletePinnedBenchmark(S->GetSelectedPinnedBenchmarkIndex()); Rebuild(); return FReply::Handled(); }
FReply SMetricViewportOverlay::ReplaceSelectedPin() { if (UMetricSubsystem* S = MetricOverlay::GetSubsystem()) S->ReplacePinnedBenchmarkWithLive(S->GetSelectedPinnedBenchmarkIndex()); Rebuild(); return FReply::Handled(); }
void SMetricViewportOverlay::CommitRename(const FText& NewText, ETextCommit::Type CommitType, int32 Index) { if (UMetricSubsystem* S = MetricOverlay::GetSubsystem()) S->RenamePinnedBenchmark(Index, NewText); bRenameActive = false; Rebuild(); }

#undef LOCTEXT_NAMESPACE
