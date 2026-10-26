#include "SMetricViewportOverlay.h"

#include "MetricEditorSettings.h"
#include "MetricStyle.h"
#include "MetricSubsystem.h"
#include "MetricPreset.h"
#include "MetricTypes.h"
#include "MetricViewPreset.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Modules/ModuleManager.h"
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
        Overhead,
        Ground,
        Fit,
        Threshold,
        Turn,
        Ledge,
        Stopping,
        Sensors,
        SensorField,
        Ranges,
        Locomotion,
        Trajectory,
        Camera,
        Cover,
        JumpRunway,
        Interaction,
        Shoulder,
        CombatDensity,
        AlternateStance,
        Mantle,
        Recovery,
        Crossfire,
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
        { TEXT("Overhead"), TEXT("OVERHEAD") },
        { TEXT("Ground"), TEXT("GROUND") },
        { TEXT("Fit"), TEXT("FIT") },
        { TEXT("Threshold"), TEXT("THRESHOLD") },
        { TEXT("Turn Clearance"), TEXT("TURN") },
        { TEXT("Ledge Analysis"), TEXT("LEDGE") },
        { TEXT("Stopping Distance"), TEXT("STOPPING") },
        { TEXT("Sensors"), TEXT("SENSORS") },
        { TEXT("Sensor Field"), TEXT("SENSOR") },
        { TEXT("Engagement Ranges"), TEXT("RANGE") },
        { TEXT("Locomotion"), TEXT("MOVE") },
        { TEXT("Trajectory"), TEXT("TRAJECTORY") },
        { TEXT("Camera"), TEXT("CAMERA") },
        { TEXT("Cover / Exposure"), TEXT("COVER") },
        { TEXT("Jump Runway"), TEXT("RUNWAY") },
        { TEXT("Interaction Reach"), TEXT("REACH") },
        { TEXT("Shoulder Camera"), TEXT("SHOULDER") },
        { TEXT("Combat Density"), TEXT("DENSITY") },
        { TEXT("Alternate Stance"), TEXT("STANCE") },
        { TEXT("Mantle / Vault"), TEXT("MANTLE") },
        { TEXT("Recovery Space"), TEXT("RECOVERY") },
        { TEXT("Crossfire"), TEXT("CROSSFIRE") },
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
        case ELayer::Overhead:   return bVisual ? V.bOverheadVisual : V.bOverheadResults;
        case ELayer::Ground:     return bVisual ? V.bGroundVisual : V.bGroundResults;
        case ELayer::Fit:        return bVisual ? V.bFitVisual : V.bFitResults;
        case ELayer::Threshold:  return bVisual ? V.bThresholdVisual : V.bThresholdResults;
        case ELayer::Turn:       return bVisual ? V.bTurnVisual : V.bTurnResults;
        case ELayer::Ledge:      return bVisual ? V.bLedgeVisual : V.bLedgeResults;
        case ELayer::Stopping:   return bVisual ? V.bStoppingVisual : V.bStoppingResults;
        case ELayer::Sensors:    return bVisual ? V.bSensorsVisual : V.bSensorsResults;
        case ELayer::SensorField:return bVisual ? V.bSensorFieldVisual : V.bSensorFieldResults;
        case ELayer::Ranges:     return bVisual ? V.bRangesVisual : V.bRangesResults;
        case ELayer::Locomotion: return bVisual ? V.bLocomotionVisual : V.bLocomotionResults;
        case ELayer::Trajectory: return bVisual ? V.bTrajectoryVisual : V.bTrajectoryResults;
        case ELayer::Camera:     return bVisual ? V.bCameraVisual : V.bCameraResults;
        case ELayer::Cover:      return bVisual ? V.bCoverVisual : V.bCoverResults;
        case ELayer::JumpRunway: return bVisual ? V.bJumpRunwayVisual : V.bJumpRunwayResults;
        case ELayer::Interaction:return bVisual ? V.bInteractionVisual : V.bInteractionResults;
        case ELayer::Shoulder:   return bVisual ? V.bShoulderVisual : V.bShoulderResults;
        case ELayer::CombatDensity:return bVisual ? V.bCombatDensityVisual : V.bCombatDensityResults;
        case ELayer::AlternateStance:return bVisual ? V.bAlternateStanceVisual : V.bAlternateStanceResults;
        case ELayer::Mantle:     return bVisual ? V.bMantleVisual : V.bMantleResults;
        case ELayer::Recovery:   return bVisual ? V.bRecoveryVisual : V.bRecoveryResults;
        case ELayer::Crossfire:  return bVisual ? V.bCrossfireVisual : V.bCrossfireResults;
        case ELayer::Sightline:  return bVisual ? V.bSightlineVisual : V.bSightlineResults;
        default: return true;
        }
    }

    static bool IsLayerEnabledByProfile(const FMetricLiveState& State, int32 LayerIndex)
    {
        switch (static_cast<ELayer>(LayerIndex))
        {
        case ELayer::Hull: return State.bShowHull;
        case ELayer::Clearance: return State.ClearanceSettings.bShowClearanceProbe;
        case ELayer::Overhead: return State.OverheadClearanceSettings.bShowOverheadClearance;
        case ELayer::Ground: return State.GroundValidationSettings.bShowGroundValidation;
        case ELayer::Fit: return State.FitAnalysisSettings.bShowFitAnalysis;
        case ELayer::Threshold: return State.ThresholdAnalysisSettings.bShowThresholdAnalysis;
        case ELayer::Turn: return State.TurnClearanceSettings.bShowTurnClearance;
        case ELayer::Ledge: return State.LedgeAnalysisSettings.bShowLedgeAnalysis;
        case ELayer::Stopping: return State.StoppingDistanceSettings.bShowStoppingDistance;
        case ELayer::Sensors: return State.Sensors.bShowSensorHeights;
        case ELayer::SensorField: return State.SensorFieldSettings.bShowSensorField;
        case ELayer::Ranges: return State.Ranges.bShowEngagementRanges;
        case ELayer::Locomotion: return State.Locomotion.bShowLocomotionBands;
        case ELayer::Trajectory: return State.TrajectorySettings.bShowTrajectory;
        case ELayer::Camera: return State.CameraProbeSettings.bShowCameraProbe;
        case ELayer::Cover: return State.CoverExposureSettings.bShowCoverExposure;
        case ELayer::JumpRunway: return State.JumpRunwaySettings.bShowJumpRunway;
        case ELayer::Interaction: return State.InteractionReachSettings.bShowInteractionReach;
        case ELayer::Shoulder: return State.ShoulderClearanceSettings.bShowShoulderClearance;
        case ELayer::CombatDensity: return State.CombatDensitySettings.bShowCombatDensity;
        case ELayer::AlternateStance: return State.AlternateStanceSettings.bShowAlternateStance;
        case ELayer::Mantle: return State.MantleVaultSettings.bShowMantleVault;
        case ELayer::Recovery: return State.RecoverySpaceSettings.bShowRecoverySpace;
        case ELayer::Crossfire: return State.CrossfireSettings.bShowCrossfire;
        case ELayer::Sightline: return State.CameraSightlineSettings.bShowCameraSightline;
        default: return true;
        }
    }

    static void SetVisibilityFlag(FMetricLayerVisibility& V, int32 LayerIndex, bool bVisual, bool bValue)
    {
        switch (static_cast<ELayer>(LayerIndex))
        {
        case ELayer::Hull:       if (bVisual) V.bHullVisual = bValue; else V.bHullResults = bValue; break;
        case ELayer::Clearance:  if (bVisual) V.bClearanceVisual = bValue; else V.bClearanceResults = bValue; break;
        case ELayer::Overhead:   if (bVisual) V.bOverheadVisual = bValue; else V.bOverheadResults = bValue; break;
        case ELayer::Ground:     if (bVisual) V.bGroundVisual = bValue; else V.bGroundResults = bValue; break;
        case ELayer::Fit:        if (bVisual) V.bFitVisual = bValue; else V.bFitResults = bValue; break;
        case ELayer::Threshold:  if (bVisual) V.bThresholdVisual = bValue; else V.bThresholdResults = bValue; break;
        case ELayer::Turn:       if (bVisual) V.bTurnVisual = bValue; else V.bTurnResults = bValue; break;
        case ELayer::Ledge:      if (bVisual) V.bLedgeVisual = bValue; else V.bLedgeResults = bValue; break;
        case ELayer::Stopping:   if (bVisual) V.bStoppingVisual = bValue; else V.bStoppingResults = bValue; break;
        case ELayer::Sensors:    if (bVisual) V.bSensorsVisual = bValue; else V.bSensorsResults = bValue; break;
        case ELayer::SensorField:if (bVisual) V.bSensorFieldVisual = bValue; else V.bSensorFieldResults = bValue; break;
        case ELayer::Ranges:     if (bVisual) V.bRangesVisual = bValue; else V.bRangesResults = bValue; break;
        case ELayer::Locomotion: if (bVisual) V.bLocomotionVisual = bValue; else V.bLocomotionResults = bValue; break;
        case ELayer::Trajectory: if (bVisual) V.bTrajectoryVisual = bValue; else V.bTrajectoryResults = bValue; break;
        case ELayer::Camera:     if (bVisual) V.bCameraVisual = bValue; else V.bCameraResults = bValue; break;
        case ELayer::Cover:      if (bVisual) V.bCoverVisual = bValue; else V.bCoverResults = bValue; break;
        case ELayer::JumpRunway: if (bVisual) V.bJumpRunwayVisual = bValue; else V.bJumpRunwayResults = bValue; break;
        case ELayer::Interaction:if (bVisual) V.bInteractionVisual = bValue; else V.bInteractionResults = bValue; break;
        case ELayer::Shoulder:   if (bVisual) V.bShoulderVisual = bValue; else V.bShoulderResults = bValue; break;
        case ELayer::CombatDensity:if (bVisual) V.bCombatDensityVisual = bValue; else V.bCombatDensityResults = bValue; break;
        case ELayer::AlternateStance:if (bVisual) V.bAlternateStanceVisual = bValue; else V.bAlternateStanceResults = bValue; break;
        case ELayer::Mantle:     if (bVisual) V.bMantleVisual = bValue; else V.bMantleResults = bValue; break;
        case ELayer::Recovery:   if (bVisual) V.bRecoveryVisual = bValue; else V.bRecoveryResults = bValue; break;
        case ELayer::Crossfire:  if (bVisual) V.bCrossfireVisual = bValue; else V.bCrossfireResults = bValue; break;
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
        if (V.bOverheadResults && State.OverheadClearance.bVisible)
        {
            if (State.OverheadClearance.bBlocked) Lines.Add(FString::Printf(TEXT("OVERHEAD    FAIL   %.0fu / %.0fu (-%.0fu)"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight, State.OverheadClearance.HeightDeficit));
            else if (State.OverheadClearance.bNearThreshold) Lines.Add(FString::Printf(TEXT("OVERHEAD    TIGHT  %.0fu / %.0fu"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight));
            else Lines.Add(FString::Printf(TEXT("OVERHEAD    PASS   %.0fu+"), State.OverheadClearance.AvailableHeight));
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
        if (V.bLedgeResults && State.LedgeAnalysis.bVisible)
        {
            if (!State.LedgeAnalysis.bHasEdge) Lines.Add(FString::Printf(TEXT("LEDGE       NONE   %.0fu"), State.LedgeAnalysis.EdgeDistance));
            else if (!State.LedgeAnalysis.bHasLanding) Lines.Add(FString::Printf(TEXT("LEDGE       FAIL   edge %.0fu | drop >%.0fu"), State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth));
            else Lines.Add(FString::Printf(TEXT("LEDGE       %s   edge %.0fu | drop %.0fu"), State.LedgeAnalysis.bSafeDrop ? (State.LedgeAnalysis.bWarning ? TEXT("WARN") : TEXT("SAFE")) : TEXT("FAIL"), State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth));
        }
        if (V.bSensorsResults && State.Sensors.bShowSensorHeights)
            Lines.Add(FString::Printf(TEXT("SENSORS     primary %.0fu   low %.0fu"), State.Sensors.PrimarySensorZOffset, State.Sensors.LowProfileSensorZOffset));
        if (V.bStoppingResults && State.StoppingDistance.bVisible)
        {
            const TCHAR* StopStatus = State.StoppingDistance.bBlocked ? TEXT("FAIL") : (State.StoppingDistance.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS"));
            Lines.Add(FString::Printf(TEXT("STOPPING    %s   need %.0fu | avail %.0fu"), StopStatus, State.StoppingDistance.RequiredDistance, State.StoppingDistance.AvailableDistance));
        }
        if (V.bCoverResults && State.CoverExposure.bVisible)
            Lines.Add(FString::Printf(TEXT("COVER       %.0f%% exposed"), State.CoverExposure.ExposurePercent));
        if (V.bJumpRunwayResults && State.JumpRunway.bVisible)
            Lines.Add(FString::Printf(TEXT("RUNWAY      %s   takeoff %.0fu landing %.0fu"), (State.JumpRunway.bTakeoffPass&&State.JumpRunway.bLandingPass)?TEXT("PASS"):TEXT("FAIL"), State.JumpRunway.TakeoffAvailable, State.JumpRunway.LandingAvailable));
        if (V.bInteractionResults && State.InteractionReach.bVisible)
            Lines.Add(FString::Printf(TEXT("REACH       %s   %.0fu"), State.InteractionReach.bHasTarget?TEXT("TARGET"):TEXT("OPEN"), State.InteractionReach.TargetDistance));
        if (V.bShoulderResults && State.ShoulderClearance.bVisible)
            Lines.Add(FString::Printf(TEXT("SHOULDER    L %s | R %s"), State.ShoulderClearance.bLeftBlocked?TEXT("BLOCKED"):TEXT("CLEAR"), State.ShoulderClearance.bRightBlocked?TEXT("BLOCKED"):TEXT("CLEAR")));
        if (V.bCombatDensityResults && State.CombatDensity.bVisible)
            Lines.Add(FString::Printf(TEXT("DENSITY     ~%d units"), State.CombatDensity.ApproxCapacity));
        if (V.bAlternateStanceResults && State.AlternateStance.bVisible)
            Lines.Add(FString::Printf(TEXT("STANCE      stand %s | alt %s"), State.AlternateStance.bStandingFits?TEXT("PASS"):TEXT("FAIL"), State.AlternateStance.bAlternateFits?TEXT("PASS"):TEXT("FAIL")));
        if (V.bMantleResults && State.MantleVault.bVisible)
            Lines.Add(FString::Printf(TEXT("MANTLE      %s   H %.0fu D %.0fu"), (State.MantleVault.bHasObstacle&&State.MantleVault.bHeightValid&&State.MantleVault.bDepthValid)?TEXT("PASS"):(State.MantleVault.bHasObstacle?TEXT("FAIL"):TEXT("NONE")), State.MantleVault.ObstacleHeight, State.MantleVault.ObstacleDepth));
        if (V.bRecoveryResults && State.RecoverySpace.bVisible)
            Lines.Add(FString::Printf(TEXT("RECOVERY    F %.0f B %.0f L %.0f R %.0fu"), State.RecoverySpace.ForwardClear, State.RecoverySpace.BackwardClear, State.RecoverySpace.LeftClear, State.RecoverySpace.RightClear));
        if (V.bCrossfireResults && State.Crossfire.bVisible)
            Lines.Add(FString::Printf(TEXT("CROSSFIRE   %.0f%% exposed"), State.Crossfire.ExposurePercent));

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
        if (Value.Contains(TEXT("TIGHT")) || Value.Contains(TEXT("WARN")) || Value.Contains(TEXT("COMPRESSED")) || Value.Contains(TEXT("FRAMING FAIL")))
        {
            return FSlateColor(Settings->ClearanceWarningColor);
        }
        if (Value.Contains(TEXT("PASS")) || Value.Contains(TEXT("SAFE")) || Value.Contains(TEXT("CLEAR")) || Value.Contains(TEXT("LANDING OK")) || Value.Contains(TEXT("OPEN")) || Value.Contains(TEXT("NONE")))
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

    static FString LiveLayerValue(int32 LayerIndex)
    {
        UMetricSubsystem* Subsystem = GetSubsystem();
        const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
        if (!Subsystem || !Settings || !Subsystem->IsMetricActive()) return FString();
        const FMetricLiveState& State = Subsystem->GetLiveState();
        const FMetricLayerVisibility& V = Settings->LiveVisibility;
        if (State.EntryType == EMetricCycleEntryType::NativePreviewMesh) return FString();
        switch (static_cast<ELayer>(LayerIndex))
        {
        case ELayer::Hull:
            return (V.bHullResults && State.bShowHull) ? FString::Printf(TEXT("%.0f W x %.0f H x %.0f L"), State.Hull.Width, State.Hull.Height, State.Hull.Length) : FString();
        case ELayer::Clearance:
            if (!V.bClearanceResults || !State.Clearance.bVisible) return FString();
            if (State.Clearance.bBlocked) return FString::Printf(TEXT("FAIL  %.0fu / %.0fu"), State.Clearance.ClearDistance, State.Clearance.ProbeDistance);
            if (State.Clearance.bNearThreshold) return FString::Printf(TEXT("TIGHT  %.0fu"), State.Clearance.ProbeDistance);
            return FString::Printf(TEXT("PASS  %.0fu+"), State.Clearance.ProbeDistance);
        case ELayer::Overhead:
            if (!V.bOverheadResults || !State.OverheadClearance.bVisible) return FString();
            if (State.OverheadClearance.bBlocked) return FString::Printf(TEXT("FAIL  %.0fu / %.0fu  -%.0fu"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight, State.OverheadClearance.HeightDeficit);
            if (State.OverheadClearance.bNearThreshold) return FString::Printf(TEXT("TIGHT  %.0fu / %.0fu"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight);
            return FString::Printf(TEXT("PASS  %.0fu+"), State.OverheadClearance.AvailableHeight);
        case ELayer::Ground:
            return (V.bGroundResults && State.GroundValidation.bVisible) ? FString::Printf(TEXT("%s  %d/%d"), State.GroundValidation.bPass ? TEXT("PASS") : TEXT("FAIL"), State.GroundValidation.TotalSamples - State.GroundValidation.FailedSamples, State.GroundValidation.TotalSamples) : FString();
        case ELayer::Fit:
            return (V.bFitResults && State.FitAnalysis.bVisible) ? FString::Printf(TEXT("%s  W %.0fu  H %.0fu"), State.FitAnalysis.bPass ? (State.FitAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL"), State.FitAnalysis.MinimumAvailableWidth, State.FitAnalysis.MinimumAvailableHeight) : FString();
        case ELayer::Threshold:
            return (V.bThresholdResults && State.ThresholdAnalysis.bVisible) ? FString::Printf(TEXT("%s  %d/%d"), State.ThresholdAnalysis.bPass ? (State.ThresholdAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL"), State.ThresholdAnalysis.TotalSamples - State.ThresholdAnalysis.FailedSamples, State.ThresholdAnalysis.TotalSamples) : FString();
        case ELayer::Turn:
            if (!V.bTurnResults || !State.TurnClearance.bVisible) return FString();
            {
                auto T=[](const FMetricTurnPathResult& P){ if(!P.bTested) return FString(TEXT("OFF")); return P.bBlocked ? FString::Printf(TEXT("BLOCKED %.0f deg"),P.ClearAngle) : FString::Printf(TEXT("PASS %.0f deg"),P.ClearAngle); };
                return FString::Printf(TEXT("L %s | R %s"), *T(State.TurnClearance.Left), *T(State.TurnClearance.Right));
            }
        case ELayer::Ledge:
            if (!V.bLedgeResults || !State.LedgeAnalysis.bVisible) return FString();
            if (!State.LedgeAnalysis.bHasEdge) return FString::Printf(TEXT("NONE  %.0fu"), State.LedgeAnalysis.EdgeDistance);
            if (!State.LedgeAnalysis.bHasLanding) return FString::Printf(TEXT("FAIL  edge %.0fu | drop >%.0fu"), State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth);
            return FString::Printf(TEXT("%s  edge %.0fu | drop %.0fu"), State.LedgeAnalysis.bSafeDrop ? (State.LedgeAnalysis.bWarning ? TEXT("WARN") : TEXT("SAFE")) : TEXT("FAIL"), State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth);
        case ELayer::Stopping:
            if (!V.bStoppingResults || !State.StoppingDistance.bVisible) return FString();
            if (State.StoppingDistance.bBlocked) return FString::Printf(TEXT("FAIL  need %.0fu | avail %.0fu"), State.StoppingDistance.RequiredDistance, State.StoppingDistance.AvailableDistance);
            if (State.StoppingDistance.bNearThreshold) return FString::Printf(TEXT("TIGHT  need %.0fu | avail %.0fu"), State.StoppingDistance.RequiredDistance, State.StoppingDistance.AvailableDistance);
            return FString::Printf(TEXT("PASS  need %.0fu | avail %.0fu+"), State.StoppingDistance.RequiredDistance, State.StoppingDistance.AvailableDistance);
        case ELayer::Sensors:
            return (V.bSensorsResults && State.Sensors.bShowSensorHeights) ? FString::Printf(TEXT("primary %.0fu  low %.0fu"), State.Sensors.PrimarySensorZOffset, State.Sensors.LowProfileSensorZOffset) : FString();
        case ELayer::SensorField:
            return (V.bSensorFieldResults && State.SensorField.bVisible) ? FString::Printf(TEXT("%.0f%% CLEAR"), State.SensorField.ClearPercent) : FString();
        case ELayer::Ranges:
            return (V.bRangesResults && State.Ranges.bShowEngagementRanges) ? FString::Printf(TEXT("%.0f / %.0f / %.0fu"), State.Ranges.ShortRange, State.Ranges.MediumRange, State.Ranges.LongRange) : FString();
        case ELayer::Locomotion:
            return (V.bLocomotionResults && State.Locomotion.bShowLocomotionBands) ? FString::Printf(TEXT("%.0fu/s  %.1f / %.1f / %.1fs"), State.Locomotion.Speed, State.Locomotion.NearTime, State.Locomotion.MidTime, State.Locomotion.FarTime) : FString();
        case ELayer::Trajectory:
            if (!V.bTrajectoryResults || !State.Trajectory.bVisible) return FString();
            {
                const TCHAR* Status = State.Trajectory.bBlocked ? TEXT("BLOCKED") : (State.Trajectory.bLandingValid ? TEXT("LANDING OK") : TEXT("NO LANDING"));
                if (State.Trajectory.ApexSampleIndex != INDEX_NONE)
                {
                    if (State.Trajectory.bApexHasObstacle)
                    {
                        return FString::Printf(TEXT("%s  %.0fu | apex %.0fu | headroom %.0fu%s"), Status, State.Trajectory.HorizontalDistance, State.Trajectory.ApexHeight, State.Trajectory.ApexHeadroom, State.Trajectory.bApexTight ? TEXT(" TIGHT") : TEXT(""));
                    }
                    return FString::Printf(TEXT("%s  %.0fu | apex %.0fu | headroom %.0fu+"), Status, State.Trajectory.HorizontalDistance, State.Trajectory.ApexHeight, State.Trajectory.ApexHeadroom);
                }
                return FString::Printf(TEXT("%s  %.0fu"), Status, State.Trajectory.HorizontalDistance);
            }
        case ELayer::Camera:
            if (!V.bCameraResults || !State.CameraProbe.bVisible) return FString();
            return FString::Printf(TEXT("%s  %.0fu / %.0fu"), State.CameraProbe.bBelowMinimum ? TEXT("TOO CLOSE") : (State.CameraProbe.bCompressed ? TEXT("COMPRESSED") : TEXT("CLEAR")), State.CameraProbe.ResolvedDistance, State.CameraProbe.TargetDistance);
        case ELayer::Cover:
            if (!V.bCoverResults || !State.CoverExposure.bVisible) return FString();
            return FString::Printf(TEXT("%.0f%% exposed | low %s torso %s head %s"), State.CoverExposure.ExposurePercent, State.CoverExposure.bLowCovered?TEXT("covered"):TEXT("open"), State.CoverExposure.bTorsoCovered?TEXT("covered"):TEXT("open"), State.CoverExposure.bHeadCovered?TEXT("covered"):TEXT("open"));
        case ELayer::JumpRunway:
            if (!V.bJumpRunwayResults || !State.JumpRunway.bVisible) return FString();
            return FString::Printf(TEXT("%s | takeoff %.0fu landing %.0fu"), (State.JumpRunway.bTakeoffPass&&State.JumpRunway.bLandingPass)?TEXT("PASS"):TEXT("FAIL"), State.JumpRunway.TakeoffAvailable, State.JumpRunway.LandingAvailable);
        case ELayer::Interaction:
            if (!V.bInteractionResults || !State.InteractionReach.bVisible) return FString();
            return FString::Printf(TEXT("%s | %.0fu"), State.InteractionReach.bHasTarget?TEXT("TARGET"):TEXT("OPEN"), State.InteractionReach.TargetDistance);
        case ELayer::Shoulder:
            if (!V.bShoulderResults || !State.ShoulderClearance.bVisible) return FString();
            return FString::Printf(TEXT("L %s %.0fu | R %s %.0fu"), State.ShoulderClearance.bLeftBlocked?TEXT("BLOCKED"):TEXT("CLEAR"), State.ShoulderClearance.LeftDistance, State.ShoulderClearance.bRightBlocked?TEXT("BLOCKED"):TEXT("CLEAR"), State.ShoulderClearance.RightDistance);
        case ELayer::CombatDensity:
            if (!V.bCombatDensityResults || !State.CombatDensity.bVisible) return FString();
            return FString::Printf(TEXT("~%d units | %.0fu personal"), State.CombatDensity.ApproxCapacity, State.CombatDensity.PersonalDiameter);
        case ELayer::AlternateStance:
            if (!V.bAlternateStanceResults || !State.AlternateStance.bVisible) return FString();
            return FString::Printf(TEXT("stand %s | alt %s | %.0fu"), State.AlternateStance.bStandingFits?TEXT("PASS"):TEXT("FAIL"), State.AlternateStance.bAlternateFits?TEXT("PASS"):TEXT("FAIL"), State.AlternateStance.AvailableHeight);
        case ELayer::Mantle:
            if (!V.bMantleResults || !State.MantleVault.bVisible) return FString();
            return FString::Printf(TEXT("%s | H %.0fu D %.0fu"), (State.MantleVault.bHasObstacle&&State.MantleVault.bHeightValid&&State.MantleVault.bDepthValid)?TEXT("PASS"):(State.MantleVault.bHasObstacle?TEXT("FAIL"):TEXT("NONE")), State.MantleVault.ObstacleHeight, State.MantleVault.ObstacleDepth);
        case ELayer::Recovery:
            if (!V.bRecoveryResults || !State.RecoverySpace.bVisible) return FString();
            return FString::Printf(TEXT("F %.0f B %.0f L %.0f R %.0fu"), State.RecoverySpace.ForwardClear, State.RecoverySpace.BackwardClear, State.RecoverySpace.LeftClear, State.RecoverySpace.RightClear);
        case ELayer::Crossfire:
            if (!V.bCrossfireResults || !State.Crossfire.bVisible) return FString();
            return FString::Printf(TEXT("%.0f%% exposed | %d/%d dirs"), State.Crossfire.ExposurePercent, State.Crossfire.ExposedDirections, State.Crossfire.TotalDirections);
        case ELayer::Sightline:
            if (!V.bSightlineResults || !State.CameraSightline.bVisible) return FString();
            return State.CameraSightline.bOccluded ? TEXT("OCCLUDED") : (!State.CameraSightline.bFramingFail ? TEXT("PASS") : TEXT("FRAMING FAIL"));
        default: return FString();
        }
    }

    static FString PinnedLayerValue(int32 LayerIndex)
    {
        UMetricSubsystem* Subsystem = GetSubsystem();
        if (!Subsystem) return FString();
        const int32 Selected = Subsystem->GetSelectedPinnedBenchmarkIndex();
        const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(Selected);
        const FMetricLiveState* StatePtr = Subsystem->GetSelectedPinnedEvaluatedState();
        if (!Pin || !StatePtr) return FString();
        const FMetricLiveState& State = *StatePtr;
        const FMetricLayerVisibility& V = Pin->Visibility;
        switch (static_cast<ELayer>(LayerIndex))
        {
        case ELayer::Hull: return V.bHullResults ? FString::Printf(TEXT("%.0f W x %.0f H x %.0f L"), State.Hull.Width, State.Hull.Height, State.Hull.Length) : FString();
        case ELayer::Clearance:
            if (!V.bClearanceResults || !State.Clearance.bVisible) return FString();
            if (State.Clearance.bBlocked) return FString::Printf(TEXT("FAIL  %.0fu / %.0fu"), State.Clearance.ClearDistance, State.Clearance.ProbeDistance);
            if (State.Clearance.bNearThreshold) return FString::Printf(TEXT("TIGHT  %.0fu"), State.Clearance.ProbeDistance);
            return FString::Printf(TEXT("PASS  %.0fu+"), State.Clearance.ProbeDistance);
        case ELayer::Overhead:
            if (!V.bOverheadResults || !State.OverheadClearance.bVisible) return FString();
            if (State.OverheadClearance.bBlocked) return FString::Printf(TEXT("FAIL  %.0fu / %.0fu  -%.0fu"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight, State.OverheadClearance.HeightDeficit);
            if (State.OverheadClearance.bNearThreshold) return FString::Printf(TEXT("TIGHT  %.0fu / %.0fu"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight);
            return FString::Printf(TEXT("PASS  %.0fu+"), State.OverheadClearance.AvailableHeight);
        case ELayer::Ground: return (V.bGroundResults && State.GroundValidation.bVisible) ? FString::Printf(TEXT("%s  %d/%d"), State.GroundValidation.bPass ? TEXT("PASS") : TEXT("FAIL"), State.GroundValidation.TotalSamples - State.GroundValidation.FailedSamples, State.GroundValidation.TotalSamples) : FString();
        case ELayer::Fit: return (V.bFitResults && State.FitAnalysis.bVisible) ? FString::Printf(TEXT("%s  W %.0fu  H %.0fu"), State.FitAnalysis.bPass ? (State.FitAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL"), State.FitAnalysis.MinimumAvailableWidth, State.FitAnalysis.MinimumAvailableHeight) : FString();
        case ELayer::Threshold: return (V.bThresholdResults && State.ThresholdAnalysis.bVisible) ? FString::Printf(TEXT("%s  %d/%d"), State.ThresholdAnalysis.bPass ? (State.ThresholdAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL"), State.ThresholdAnalysis.TotalSamples - State.ThresholdAnalysis.FailedSamples, State.ThresholdAnalysis.TotalSamples) : FString();
        case ELayer::Turn:
            if (!V.bTurnResults || !State.TurnClearance.bVisible) return FString();
            { auto T=[](const FMetricTurnPathResult& P){ if(!P.bTested) return FString(TEXT("OFF")); return P.bBlocked ? FString::Printf(TEXT("BLOCKED %.0f deg"),P.ClearAngle) : FString::Printf(TEXT("PASS %.0f deg"),P.ClearAngle); }; return FString::Printf(TEXT("L %s | R %s"), *T(State.TurnClearance.Left), *T(State.TurnClearance.Right)); }
        case ELayer::Ledge:
            if (!V.bLedgeResults || !State.LedgeAnalysis.bVisible) return FString();
            if (!State.LedgeAnalysis.bHasEdge) return FString::Printf(TEXT("NONE  %.0fu"), State.LedgeAnalysis.EdgeDistance);
            if (!State.LedgeAnalysis.bHasLanding) return FString::Printf(TEXT("FAIL  edge %.0fu | drop >%.0fu"), State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth);
            return FString::Printf(TEXT("%s  edge %.0fu | drop %.0fu"), State.LedgeAnalysis.bSafeDrop ? (State.LedgeAnalysis.bWarning ? TEXT("WARN") : TEXT("SAFE")) : TEXT("FAIL"), State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth);
        case ELayer::Stopping:
            if (!V.bStoppingResults || !State.StoppingDistance.bVisible) return FString();
            if (State.StoppingDistance.bBlocked) return FString::Printf(TEXT("FAIL  need %.0fu | avail %.0fu"), State.StoppingDistance.RequiredDistance, State.StoppingDistance.AvailableDistance);
            if (State.StoppingDistance.bNearThreshold) return FString::Printf(TEXT("TIGHT  need %.0fu | avail %.0fu"), State.StoppingDistance.RequiredDistance, State.StoppingDistance.AvailableDistance);
            return FString::Printf(TEXT("PASS  need %.0fu | avail %.0fu+"), State.StoppingDistance.RequiredDistance, State.StoppingDistance.AvailableDistance);
        case ELayer::Sensors: return (V.bSensorsResults && State.Sensors.bShowSensorHeights) ? FString::Printf(TEXT("primary %.0fu  low %.0fu"), State.Sensors.PrimarySensorZOffset, State.Sensors.LowProfileSensorZOffset) : FString();
        case ELayer::SensorField: return (V.bSensorFieldResults && State.SensorField.bVisible) ? FString::Printf(TEXT("%.0f%% CLEAR"), State.SensorField.ClearPercent) : FString();
        case ELayer::Ranges: return (V.bRangesResults && State.Ranges.bShowEngagementRanges) ? FString::Printf(TEXT("%.0f / %.0f / %.0fu"), State.Ranges.ShortRange, State.Ranges.MediumRange, State.Ranges.LongRange) : FString();
        case ELayer::Locomotion: return (V.bLocomotionResults && State.Locomotion.bShowLocomotionBands) ? FString::Printf(TEXT("%.0fu/s  %.1f / %.1f / %.1fs"), State.Locomotion.Speed, State.Locomotion.NearTime, State.Locomotion.MidTime, State.Locomotion.FarTime) : FString();
        case ELayer::Trajectory:
            if (!V.bTrajectoryResults || !State.Trajectory.bVisible) return FString();
            {
                const TCHAR* Status = State.Trajectory.bBlocked ? TEXT("BLOCKED") : (State.Trajectory.bLandingValid ? TEXT("LANDING OK") : TEXT("NO LANDING"));
                if (State.Trajectory.ApexSampleIndex != INDEX_NONE)
                {
                    if (State.Trajectory.bApexHasObstacle)
                    {
                        return FString::Printf(TEXT("%s  %.0fu | apex %.0fu | headroom %.0fu%s"), Status, State.Trajectory.HorizontalDistance, State.Trajectory.ApexHeight, State.Trajectory.ApexHeadroom, State.Trajectory.bApexTight ? TEXT(" TIGHT") : TEXT(""));
                    }
                    return FString::Printf(TEXT("%s  %.0fu | apex %.0fu | headroom %.0fu+"), Status, State.Trajectory.HorizontalDistance, State.Trajectory.ApexHeight, State.Trajectory.ApexHeadroom);
                }
                return FString::Printf(TEXT("%s  %.0fu"), Status, State.Trajectory.HorizontalDistance);
            }
        case ELayer::Camera: if (!V.bCameraResults || !State.CameraProbe.bVisible) return FString(); return FString::Printf(TEXT("%s  %.0fu / %.0fu"), State.CameraProbe.bBelowMinimum ? TEXT("TOO CLOSE") : (State.CameraProbe.bCompressed ? TEXT("COMPRESSED") : TEXT("CLEAR")), State.CameraProbe.ResolvedDistance, State.CameraProbe.TargetDistance);
        case ELayer::Cover:
            if (!V.bCoverResults || !State.CoverExposure.bVisible) return FString();
            return FString::Printf(TEXT("%.0f%% exposed | low %s torso %s head %s"), State.CoverExposure.ExposurePercent, State.CoverExposure.bLowCovered?TEXT("covered"):TEXT("open"), State.CoverExposure.bTorsoCovered?TEXT("covered"):TEXT("open"), State.CoverExposure.bHeadCovered?TEXT("covered"):TEXT("open"));
        case ELayer::JumpRunway:
            if (!V.bJumpRunwayResults || !State.JumpRunway.bVisible) return FString();
            return FString::Printf(TEXT("%s | takeoff %.0fu landing %.0fu"), (State.JumpRunway.bTakeoffPass&&State.JumpRunway.bLandingPass)?TEXT("PASS"):TEXT("FAIL"), State.JumpRunway.TakeoffAvailable, State.JumpRunway.LandingAvailable);
        case ELayer::Interaction:
            if (!V.bInteractionResults || !State.InteractionReach.bVisible) return FString();
            return FString::Printf(TEXT("%s | %.0fu"), State.InteractionReach.bHasTarget?TEXT("TARGET"):TEXT("OPEN"), State.InteractionReach.TargetDistance);
        case ELayer::Shoulder:
            if (!V.bShoulderResults || !State.ShoulderClearance.bVisible) return FString();
            return FString::Printf(TEXT("L %s %.0fu | R %s %.0fu"), State.ShoulderClearance.bLeftBlocked?TEXT("BLOCKED"):TEXT("CLEAR"), State.ShoulderClearance.LeftDistance, State.ShoulderClearance.bRightBlocked?TEXT("BLOCKED"):TEXT("CLEAR"), State.ShoulderClearance.RightDistance);
        case ELayer::CombatDensity:
            if (!V.bCombatDensityResults || !State.CombatDensity.bVisible) return FString();
            return FString::Printf(TEXT("~%d units | %.0fu personal"), State.CombatDensity.ApproxCapacity, State.CombatDensity.PersonalDiameter);
        case ELayer::AlternateStance:
            if (!V.bAlternateStanceResults || !State.AlternateStance.bVisible) return FString();
            return FString::Printf(TEXT("stand %s | alt %s | %.0fu"), State.AlternateStance.bStandingFits?TEXT("PASS"):TEXT("FAIL"), State.AlternateStance.bAlternateFits?TEXT("PASS"):TEXT("FAIL"), State.AlternateStance.AvailableHeight);
        case ELayer::Mantle:
            if (!V.bMantleResults || !State.MantleVault.bVisible) return FString();
            return FString::Printf(TEXT("%s | H %.0fu D %.0fu"), (State.MantleVault.bHasObstacle&&State.MantleVault.bHeightValid&&State.MantleVault.bDepthValid)?TEXT("PASS"):(State.MantleVault.bHasObstacle?TEXT("FAIL"):TEXT("NONE")), State.MantleVault.ObstacleHeight, State.MantleVault.ObstacleDepth);
        case ELayer::Recovery:
            if (!V.bRecoveryResults || !State.RecoverySpace.bVisible) return FString();
            return FString::Printf(TEXT("F %.0f B %.0f L %.0f R %.0fu"), State.RecoverySpace.ForwardClear, State.RecoverySpace.BackwardClear, State.RecoverySpace.LeftClear, State.RecoverySpace.RightClear);
        case ELayer::Crossfire:
            if (!V.bCrossfireResults || !State.Crossfire.bVisible) return FString();
            return FString::Printf(TEXT("%.0f%% exposed | %d/%d dirs"), State.Crossfire.ExposurePercent, State.Crossfire.ExposedDirections, State.Crossfire.TotalDirections);
        case ELayer::Sightline: if (!V.bSightlineResults || !State.CameraSightline.bVisible) return FString(); return State.CameraSightline.bOccluded ? TEXT("OCCLUDED") : (!State.CameraSightline.bFramingFail ? TEXT("PASS") : TEXT("FRAMING FAIL"));
        default: return FString();
        }
    }

    static TSharedRef<SWidget> BuildPersistentPinnedRowsWidget()
    {
        TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
        for (int32 LayerIndex = 0; LayerIndex < static_cast<int32>(ELayer::Count); ++LayerIndex)
        {
            Rows->AddSlot().AutoHeight().Padding(4,1)
            [
                SNew(SHorizontalBox)
                .Visibility_Lambda([LayerIndex]() { return PinnedLayerValue(LayerIndex).IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
                + SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center)
                [ SNew(STextBlock).Text(FText::FromString(Layers[LayerIndex].Name)).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground()) ]
                + SHorizontalBox::Slot().FillWidth(0.58f).HAlign(HAlign_Right).VAlign(VAlign_Center)
                [ SNew(STextBlock).Text_Lambda([LayerIndex]() { return FText::FromString(PinnedLayerValue(LayerIndex)); }).ColorAndOpacity_Lambda([LayerIndex]() { return ResultValueColor(PinnedLayerValue(LayerIndex), GetDefault<UMetricEditorSettings>()); }).Font(FAppStyle::GetFontStyle("SmallFont")).Justification(ETextJustify::Right) ]
            ];
        }
        return Rows;
    }

    static TSharedRef<SWidget> BuildPersistentLiveRowsWidget()
    {
        TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
        for (int32 LayerIndex = 0; LayerIndex < static_cast<int32>(ELayer::Count); ++LayerIndex)
        {
            Rows->AddSlot().AutoHeight().Padding(4,1)
            [
                SNew(SHorizontalBox)
                .Visibility_Lambda([LayerIndex]() { return LiveLayerValue(LayerIndex).IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
                + SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(Layers[LayerIndex].Name))
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
                + SHorizontalBox::Slot().FillWidth(0.58f).HAlign(HAlign_Right).VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text_Lambda([LayerIndex]() { return FText::FromString(LiveLayerValue(LayerIndex)); })
                    .ColorAndOpacity_Lambda([LayerIndex]() { const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>(); return ResultValueColor(LiveLayerValue(LayerIndex), Settings); })
                    .Font(FAppStyle::GetFontStyle("SmallFont"))
                    .Justification(ETextJustify::Right)
                ]
            ];
        }
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
    const bool bLiveVisualizerEnabled = Settings ? Settings->bShowLiveVisualizer : true;
    FString PinnedSummary;
    if (Subsystem)
    {
        if (const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(Selected))
        {
            if (const FMetricLiveState* PinnedState = Subsystem->GetSelectedPinnedEvaluatedState()) PinnedSummary = MetricOverlay::ResultSummary(*PinnedState, Pin->Visibility);
            else PinnedSummary = MetricOverlay::FilterCapturedResults(Pin->CapturedResults, Pin->Visibility);
        }
    }
    const FString ProfileLabel = Subsystem ? Subsystem->GetLiveState().Label.ToString() : FString();
    const bool bStructureChanged =
        bActive != bLastActive ||
        PinCount != LastPinCount ||
        Selected != LastSelectedPin ||
        bLegend != bLastLegendVisible ||
        bMinimized != bLastHUDMinimized ||
        bLiveVisualizerEnabled != bLastLiveVisualizerEnabled ||
        ProfileLabel != LastProfileLabel;

    if (bStructureChanged)
    {
        Rebuild();
    }
    else if (PinnedSummary != LastPinnedSummary)
    {
        RefreshDynamicResultContent(LastLiveSummary, PinnedSummary);
    }
}

void SMetricViewportOverlay::RefreshDynamicResultContent(const FString& LiveSummary, const FString& PinnedSummary)
{
    LastLiveSummary = LiveSummary;

    if (PinnedSummary != LastPinnedSummary)
    {
        LastPinnedSummary = PinnedSummary;
        // Pinned result rows are persistent widgets with bound values.
        // Keep only the cached summary for legacy/fallback text paths.
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
    bLastLiveVisualizerEnabled = Settings ? Settings->bShowLiveVisualizer : true;
    LastLiveSummary = (Subsystem && Settings) ? MetricOverlay::ResultSummary(Subsystem->GetLiveState(), Settings->LiveVisibility) : FString();
    LastProfileLabel = Subsystem ? Subsystem->GetLiveState().Label.ToString() : FString();
    LastPinnedSummary.Reset();
    if (Subsystem)
    {
        if (const FMetricPinnedBenchmark* Pin = Subsystem->GetPinnedBenchmark(LastSelectedPin))
        {
            if (const FMetricLiveState* PinnedState = Subsystem->GetSelectedPinnedEvaluatedState()) LastPinnedSummary = MetricOverlay::ResultSummary(*PinnedState, Pin->Visibility);
            else LastPinnedSummary = MetricOverlay::FilterCapturedResults(Pin->CapturedResults, Pin->Visibility);
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
            SNew(SBox)
            .WidthOverride(36.0f)
            .HeightOverride(36.0f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SNew(SButton)
                    .ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
                    .ToolTipText(LOCTEXT("RestoreHUDTip", "Restore the Metric viewport HUD."))
                    .ContentPadding(FMargin(0.0f))
                    .OnClicked(this, &SMetricViewportOverlay::ToggleHUDMinimized)
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Center)
                .VAlign(VAlign_Center)
                [
                    SNew(SBox)
                    .Visibility(EVisibility::HitTestInvisible)
                    .WidthOverride(28.0f)
                    .HeightOverride(28.0f)
                    [
                        SNew(SImage)
                        .Image(MetricIcon)
                        .ColorAndOpacity(FLinearColor::White)
                    ]
                ]
            ]
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
            .ToolTipText(LOCTEXT("EditLiveTip", "Display Options: control visibility for analyses enabled by the active Metric Profile. Profile-disabled analyses are shown as Off and cannot be displayed until enabled in the profile."))
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
                .Text(LOCTEXT("LiveVisualizerLabel", "Live Analysis"))
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
                .ToolTipText(LOCTEXT("LiveVisualizerTip", "Enable or pause the cursor-driven Live Metric analyzer. Pinned benchmark analysis remains active."))
                .OnClicked(this, &SMetricViewportOverlay::ToggleLiveVisualizer)
            ]
        ];

        if (bLastLiveVisualizerEnabled)
        {
            RootBox->AddSlot().AutoHeight().Padding(0,0,0,4)
            [
                MetricOverlay::BuildPersistentLiveRowsWidget()
            ];
        }
        else
        {
            RootBox->AddSlot().AutoHeight().Padding(4,2,2,4)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("LiveVisualizerDisabled", "Live Analysis Disabled"))
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
            ];
        }
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
            RootBox->AddSlot().AutoHeight().Padding(0,0,0,4)
            [
                MetricOverlay::BuildPersistentPinnedRowsWidget()
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
        .Text(bForBenchmark ? LOCTEXT("BenchmarkVisibility", "BENCHMARK DISPLAY") : LOCTEXT("LiveVisibility", "LIVE DISPLAY VISIBILITY"))
        .Font(FAppStyle::GetFontStyle("SmallFont"))
    ];

    if (!bForBenchmark)
    {
        Box->AddSlot().AutoHeight().Padding(8,0,8,5)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("LiveVisibilityExplanation", "Profile controls whether an analysis runs. Visual and Results only control how enabled analyses are displayed."))
            .AutoWrapText(true)
            .Font(FAppStyle::GetFontStyle("SmallFont"))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];

        Box->AddSlot().AutoHeight().Padding(8,0,8,6)
        [
            SNew(SButton)
            .HAlign(HAlign_Center)
            .Text(LOCTEXT("EditActiveProfile", "Edit Active Profile"))
            .ToolTipText_Lambda([]()
            {
                if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem())
                {
                    if (const FMetricCycleEntry* Entry = Subsystem->GetActiveEntry())
                    {
                        if (Entry->Type == EMetricCycleEntryType::MetricPreset && !Entry->Preset.IsNull())
                        {
                            return LOCTEXT("EditActiveProfileTip", "Open the active Metric Profile in the Metric Profile Editor.");
                        }
                    }
                }
                return LOCTEXT("EditBuiltInProfileTip", "The current entry is not a project Metric Profile asset. Select a Metric Profile to edit it.");
            })
            .IsEnabled_Lambda([]()
            {
                if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem())
                {
                    if (const FMetricCycleEntry* Entry = Subsystem->GetActiveEntry())
                    {
                        return Entry->Type == EMetricCycleEntryType::MetricPreset && !Entry->Preset.IsNull();
                    }
                }
                return false;
            })
            .OnClicked(this, &SMetricViewportOverlay::OpenActiveMetricProfile)
        ];
    }

    Box->AddSlot().AutoHeight().Padding(8,0,8,2)
    [
        SNew(STextBlock).Text(LOCTEXT("ViewPresetHeader", "VIEW PRESETS")).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())
    ];
    Box->AddSlot().AutoHeight().Padding(6,0,6,2)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("ViewFullAudit", "Full Audit")).OnClicked_Lambda([this,bForBenchmark](){ return ApplyBuiltInViewPreset(0,bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("ViewTraversal", "Traversal")).OnClicked_Lambda([this,bForBenchmark](){ return ApplyBuiltInViewPreset(1,bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("ViewCamera", "Camera")).OnClicked_Lambda([this,bForBenchmark](){ return ApplyBuiltInViewPreset(2,bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("ViewCombat", "Combat")).OnClicked_Lambda([this,bForBenchmark](){ return ApplyBuiltInViewPreset(3,bForBenchmark); })]
    ];
    Box->AddSlot().AutoHeight().Padding(6,0,6,4)
    [
        SNew(SHorizontalBox)
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("ViewSpace", "Space & Nav")).OnClicked_Lambda([this,bForBenchmark](){ return ApplyBuiltInViewPreset(4,bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("ViewInteraction", "Interaction")).OnClicked_Lambda([this,bForBenchmark](){ return ApplyBuiltInViewPreset(5,bForBenchmark); })]
        + SHorizontalBox::Slot().AutoWidth().Padding(1,0)[SNew(SButton).Text(LOCTEXT("ViewMinimal", "Minimal")).OnClicked_Lambda([this,bForBenchmark](){ return ApplyBuiltInViewPreset(6,bForBenchmark); })]
    ];

    {
        FARFilter Filter;
        Filter.ClassPaths.Add(UMetricViewPreset::StaticClass()->GetClassPathName());
        Filter.bRecursiveClasses = true;
        TArray<FAssetData> Assets;
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssets(Filter, Assets);
        for (const FAssetData& Asset : Assets)
        {
            const FSoftObjectPath AssetPath = Asset.GetSoftObjectPath();
            FText Label = FText::FromName(Asset.AssetName);
            if (const UMetricViewPreset* ViewPreset = Cast<UMetricViewPreset>(AssetPath.TryLoad()))
            {
                if (!ViewPreset->DisplayName.IsEmpty())
                {
                    Label = ViewPreset->DisplayName;
                }
            }
            Box->AddSlot().AutoHeight().Padding(8,1)
            [
                SNew(SButton).HAlign(HAlign_Left).Text(Label).ToolTipText(FText::FromString(AssetPath.ToString()))
                .OnClicked_Lambda([this,bForBenchmark,AssetPath](){ return ApplyViewPresetAsset(AssetPath,bForBenchmark); })
            ];
        }
    }

    Box->AddSlot().AutoHeight().Padding(8,3,8,2)[SNew(SSeparator)];
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
        + SHorizontalBox::Slot().AutoWidth().Padding(8,0)
        [
            SNew(STextBlock)
            .Visibility(bForBenchmark ? EVisibility::Collapsed : EVisibility::Visible)
            .Text(LOCTEXT("ProfileCol", "Profile"))
            .Font(FAppStyle::GetFontStyle("SmallFont"))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SHorizontalBox::Slot().AutoWidth().Padding(8,0)[SNew(STextBlock).Text(LOCTEXT("VisualCol", "Visual")).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
        + SHorizontalBox::Slot().AutoWidth().Padding(12,0,2,0)[SNew(STextBlock).Text(LOCTEXT("ResultsCol", "Results")).Font(FAppStyle::GetFontStyle("SmallFont")).ColorAndOpacity(FSlateColor::UseSubduedForeground())]
    ];

    for (int32 Index = 0; Index < static_cast<int32>(MetricOverlay::ELayer::Count); ++Index)
    {
        Box->AddSlot().AutoHeight().Padding(8,1)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Text(FText::FromString(MetricOverlay::Layers[Index].Name))
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity_Lambda([Index, bForBenchmark]()
                {
                    if (bForBenchmark)
                    {
                        return FSlateColor::UseForeground();
                    }
                    if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem())
                    {
                        return MetricOverlay::IsLayerEnabledByProfile(Subsystem->GetLiveState(), Index)
                            ? FSlateColor::UseForeground()
                            : FSlateColor::UseSubduedForeground();
                    }
                    return FSlateColor::UseSubduedForeground();
                })
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(8,0).VAlign(VAlign_Center)
            [
                SNew(STextBlock)
                .Visibility(bForBenchmark ? EVisibility::Collapsed : EVisibility::Visible)
                .Text_Lambda([Index]()
                {
                    if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem())
                    {
                        return MetricOverlay::IsLayerEnabledByProfile(Subsystem->GetLiveState(), Index) ? LOCTEXT("ProfileOn", "On") : LOCTEXT("ProfileOff", "Off");
                    }
                    return LOCTEXT("ProfileUnknown", "--");
                })
                .Font(FAppStyle::GetFontStyle("SmallFont"))
                .ColorAndOpacity_Lambda([Index]()
                {
                    if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem())
                    {
                        return MetricOverlay::IsLayerEnabledByProfile(Subsystem->GetLiveState(), Index)
                            ? FSlateColor(FLinearColor(0.35f, 0.90f, 0.48f, 1.0f))
                            : FSlateColor::UseSubduedForeground();
                    }
                    return FSlateColor::UseSubduedForeground();
                })
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(12,0).VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsEnabled_Lambda([Index, bForBenchmark]()
                {
                    if (bForBenchmark) return true;
                    if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem()) return MetricOverlay::IsLayerEnabledByProfile(Subsystem->GetLiveState(), Index);
                    return false;
                })
                .ToolTipText_Lambda([Index, bForBenchmark]()
                {
                    if (!bForBenchmark)
                    {
                        if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem())
                        {
                            if (!MetricOverlay::IsLayerEnabledByProfile(Subsystem->GetLiveState(), Index))
                            {
                                return LOCTEXT("ProfileDisabledVisualTip", "This analysis is Off in the active Metric Profile. Enable it in the Metric Profile Editor before controlling its viewport visibility.");
                            }
                        }
                    }
                    return LOCTEXT("VisualVisibilityTip", "Show or hide this layer's world-space visualization.");
                })
                .IsChecked(this, &SMetricViewportOverlay::GetVisibilityCheckedState, Index, true, bForBenchmark)
                .OnCheckStateChanged(this, &SMetricViewportOverlay::OnVisibilityChanged, Index, true, bForBenchmark)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(17,0,6,0).VAlign(VAlign_Center)
            [
                SNew(SCheckBox)
                .IsEnabled_Lambda([Index, bForBenchmark]()
                {
                    if (bForBenchmark) return true;
                    if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem()) return MetricOverlay::IsLayerEnabledByProfile(Subsystem->GetLiveState(), Index);
                    return false;
                })
                .ToolTipText_Lambda([Index, bForBenchmark]()
                {
                    if (!bForBenchmark)
                    {
                        if (UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem())
                        {
                            if (!MetricOverlay::IsLayerEnabledByProfile(Subsystem->GetLiveState(), Index))
                            {
                                return LOCTEXT("ProfileDisabledResultsTip", "This analysis is Off in the active Metric Profile. Enable it in the Metric Profile Editor before controlling its HUD results.");
                            }
                        }
                    }
                    return LOCTEXT("ResultsVisibilityTip", "Show or hide this layer's result row in the HUD.");
                })
                .IsChecked(this, &SMetricViewportOverlay::GetVisibilityCheckedState, Index, false, bForBenchmark)
                .OnCheckStateChanged(this, &SMetricViewportOverlay::OnVisibilityChanged, Index, false, bForBenchmark)
            ]
        ];
    }
    return SNew(SBox).WidthOverride(bForBenchmark ? 330.0f : 390.0f)[Box];
}

FReply SMetricViewportOverlay::OpenActiveMetricProfile()
{
    UMetricSubsystem* Subsystem = MetricOverlay::GetSubsystem();
    if (!Subsystem || !GEditor)
    {
        return FReply::Handled();
    }

    const FMetricCycleEntry* Entry = Subsystem->GetActiveEntry();
    if (!Entry || !Entry->IsMetricEntry())
    {
        return FReply::Handled();
    }

    if (UMetricPreset* Preset = Entry->Preset.LoadSynchronous())
    {
        if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            AssetEditorSubsystem->OpenEditorForAsset(Preset);
        }
    }

    return FReply::Handled();
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
    if (State.OverheadClearance.bVisible && V.bOverheadVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendOverhead", "Overhead: pass / tight / fail"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.GroundValidation.bVisible && V.bGroundVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendGround", "Ground: pass / step / slope"), { Settings->GroundPassColor, Settings->GroundStepFailColor, Settings->GroundSlopeFailColor });
    if (State.FitAnalysis.bVisible && V.bFitVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendFit", "Fit: pass / tight / fail"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.ThresholdAnalysis.bVisible && V.bThresholdVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendThreshold", "Threshold: pass / tight / fail"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.TurnClearance.bVisible && V.bTurnVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendTurn", "Turn clearance"), { Settings->ClearancePassColor, Settings->ClearanceFailColor });
    if (State.LedgeAnalysis.bVisible && V.bLedgeVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendLedge", "Ledge / drop"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.StoppingDistance.bVisible && V.bStoppingVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendStopping", "Stopping distance"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.CoverExposure.bVisible && V.bCoverVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendCover", "Cover / exposure"), { Settings->ClearancePassColor, Settings->ClearanceFailColor });
    if (State.JumpRunway.bVisible && V.bJumpRunwayVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendRunway", "Jump runway"), { Settings->ClearancePassColor, Settings->ClearanceFailColor });
    if (State.InteractionReach.bVisible && V.bInteractionVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendReach", "Interaction reach"), { Settings->SensorColor, Settings->ClearancePassColor });
    if (State.ShoulderClearance.bVisible && V.bShoulderVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendShoulder", "Shoulder camera"), { Settings->CameraProbeColor, Settings->CameraCompressedColor });
    if (State.CombatDensity.bVisible && V.bCombatDensityVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendDensity", "Combat density"), { Settings->EngagementRangeColor });
    if (State.AlternateStance.bVisible && V.bAlternateStanceVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendStance", "Alternate stance"), { Settings->ClearancePassColor, Settings->ClearanceWarningColor, Settings->ClearanceFailColor });
    if (State.MantleVault.bVisible && V.bMantleVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendMantle", "Mantle / vault"), { Settings->ClearancePassColor, Settings->ClearanceFailColor });
    if (State.RecoverySpace.bVisible && V.bRecoveryVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendRecovery", "Recovery space"), { Settings->ClearancePassColor, Settings->ClearanceFailColor });
    if (State.Crossfire.bVisible && V.bCrossfireVisual) MetricOverlay::AddLegendRow(Box, LOCTEXT("LegendCrossfire", "Crossfire exposure"), { Settings->ClearancePassColor, Settings->ClearanceFailColor });
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
    if (const FMetricLiveState* PinnedState = Subsystem->GetSelectedPinnedEvaluatedState()) return FText::FromString(MetricOverlay::ResultSummary(*PinnedState, Pin->Visibility));
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
    return FText::Format(LOCTEXT("PinnedSource", "Profile: {0}  |  results update against current geometry"), Source);
}

FText SMetricViewportOverlay::BuildComparisonText() const
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    if (Settings && !Settings->bShowLiveVisualizer)
    {
        return LOCTEXT("LiveComparisonDisabled", "Live Metric disabled");
    }
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


FReply SMetricViewportOverlay::ApplyBuiltInViewPreset(int32 Preset, bool bForBenchmark)
{
    FMetricLayerVisibility V;
    for (int32 I=0; I<static_cast<int32>(MetricOverlay::ELayer::Count); ++I)
    {
        MetricOverlay::SetVisibilityFlag(V, I, true, false);
        MetricOverlay::SetVisibilityFlag(V, I, false, false);
    }
    auto Enable=[&](MetricOverlay::ELayer L){ const int32 I=static_cast<int32>(L); MetricOverlay::SetVisibilityFlag(V,I,true,true); MetricOverlay::SetVisibilityFlag(V,I,false,true); };
    if (Preset == 0)
    {
        for (int32 I=0; I<static_cast<int32>(MetricOverlay::ELayer::Count); ++I) { MetricOverlay::SetVisibilityFlag(V,I,true,true); MetricOverlay::SetVisibilityFlag(V,I,false,true); }
    }
    else if (Preset == 1)
    {
        Enable(MetricOverlay::ELayer::Hull); Enable(MetricOverlay::ELayer::Clearance); Enable(MetricOverlay::ELayer::Overhead); Enable(MetricOverlay::ELayer::Ground); Enable(MetricOverlay::ELayer::Fit); Enable(MetricOverlay::ELayer::Threshold); Enable(MetricOverlay::ELayer::Turn); Enable(MetricOverlay::ELayer::Ledge); Enable(MetricOverlay::ELayer::Stopping); Enable(MetricOverlay::ELayer::Locomotion); Enable(MetricOverlay::ELayer::Trajectory); Enable(MetricOverlay::ELayer::JumpRunway); Enable(MetricOverlay::ELayer::AlternateStance); Enable(MetricOverlay::ELayer::Mantle); Enable(MetricOverlay::ELayer::Recovery);
    }
    else if (Preset == 2)
    {
        Enable(MetricOverlay::ELayer::Hull); Enable(MetricOverlay::ELayer::Camera); Enable(MetricOverlay::ELayer::Sightline); Enable(MetricOverlay::ELayer::Shoulder); Enable(MetricOverlay::ELayer::Overhead);
    }
    else if (Preset == 3)
    {
        Enable(MetricOverlay::ELayer::Hull); Enable(MetricOverlay::ELayer::Sensors); Enable(MetricOverlay::ELayer::SensorField); Enable(MetricOverlay::ELayer::Ranges); Enable(MetricOverlay::ELayer::Cover); Enable(MetricOverlay::ELayer::CombatDensity); Enable(MetricOverlay::ELayer::Recovery); Enable(MetricOverlay::ELayer::Crossfire); Enable(MetricOverlay::ELayer::Stopping);
    }
    else if (Preset == 4)
    {
        Enable(MetricOverlay::ELayer::Hull); Enable(MetricOverlay::ELayer::Clearance); Enable(MetricOverlay::ELayer::Overhead); Enable(MetricOverlay::ELayer::Ground); Enable(MetricOverlay::ELayer::Fit); Enable(MetricOverlay::ELayer::Threshold); Enable(MetricOverlay::ELayer::Turn); Enable(MetricOverlay::ELayer::Stopping); Enable(MetricOverlay::ELayer::CombatDensity);
    }
    else if (Preset == 5)
    {
        Enable(MetricOverlay::ELayer::Hull); Enable(MetricOverlay::ELayer::Interaction); Enable(MetricOverlay::ELayer::Mantle); Enable(MetricOverlay::ELayer::AlternateStance); Enable(MetricOverlay::ELayer::Overhead); Enable(MetricOverlay::ELayer::Threshold);
    }
    else
    {
        Enable(MetricOverlay::ELayer::Hull); Enable(MetricOverlay::ELayer::Clearance); Enable(MetricOverlay::ELayer::Ground);
    }

    if (bForBenchmark)
    {
        if (UMetricSubsystem* S=MetricOverlay::GetSubsystem()) { const int32 Index=S->GetSelectedPinnedBenchmarkIndex(); if(Index!=INDEX_NONE) S->SetPinnedBenchmarkVisibility(Index,V); }
    }
    else if (UMetricEditorSettings* Settings=GetMutableDefault<UMetricEditorSettings>())
    {
        Settings->LiveVisibility=V; Settings->SaveConfig();
    }
    Rebuild();
    return FReply::Handled();
}

FReply SMetricViewportOverlay::ApplyViewPresetAsset(FSoftObjectPath AssetPath, bool bForBenchmark)
{
    const UMetricViewPreset* Preset = Cast<UMetricViewPreset>(AssetPath.TryLoad());
    if (!Preset) return FReply::Handled();
    const FMetricLayerVisibility V = Preset->Visibility;
    if (bForBenchmark)
    {
        if (UMetricSubsystem* S=MetricOverlay::GetSubsystem()) { const int32 Index=S->GetSelectedPinnedBenchmarkIndex(); if(Index!=INDEX_NONE) S->SetPinnedBenchmarkVisibility(Index,V); }
    }
    else if (UMetricEditorSettings* Settings=GetMutableDefault<UMetricEditorSettings>())
    {
        Settings->LiveVisibility=V; Settings->SaveConfig();
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
