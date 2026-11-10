#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MetricTypes.h"
#include "MetricProfile.generated.h"

UCLASS(BlueprintType)
class METRIC_API UMetricProfile : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual void Serialize(FArchive& Ar) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Optional designer-facing name shown in Metric UI. If empty, Metric falls back to the asset name."))
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Reference hull used by Metric occupancy, clearance, movement, and presentation systems."))
    FMetricHullSettings Hull;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Semantic colors used by this profile in viewport visualization, HUD results, legend swatches, and captured benchmarks."))
    FMetricVisualizationTheme VisualTheme;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Profile-owned presentation defaults. HUD and Visualizer visibility are independent from whether an analysis is enabled and are combined with temporary View Preset / Display Options filtering."))
    FMetricLayerVisibility Presentation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Forward hull-clearance settings for usable space ahead of the profile."))
    FMetricClearanceSettings Clearance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Vertical headroom settings above the active hull."))
    FMetricOverheadClearanceSettings OverheadClearance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Reference sensor and eye-height markers used by related analyses and visual guides."))
    FMetricSensorSettings Sensors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Field-of-view sampling settings for blocked versus clear sensing directions."))
    FMetricSensorFieldSettings SensorField;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Authored short, medium, and long gameplay range references. These do not query geometry."))
    FMetricRangeSettings EngagementRanges;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Movement-distance reference bands derived from authored speed and time horizons."))
    FMetricLocomotionSettings LocomotionBands;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Ballistic trajectory, Jump Envelope, landing, and apex-clearance settings."))
    FMetricTrajectorySettings Trajectory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Third-person camera-boom clearance and compression settings."))
    FMetricCameraProbeSettings CameraProbe;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Camera-to-subject visibility and framing settings."))
    FMetricCameraSightlineSettings CameraSightline;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Floor continuity, walkable slope, and discrete step validation settings."))
    FMetricGroundValidationSettings GroundValidation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Sampled corridor occupancy, width, and headroom settings for continuous fit checks."))
    FMetricFitAnalysisSettings FitAnalysis;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Short transition occupancy, width, height, floor, and step validation settings."))
    FMetricThresholdAnalysisSettings ThresholdAnalysis;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Swept turning-room settings for left and/or right maneuver checks."))
    FMetricTurnClearanceSettings TurnClearance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Forward ledge detection, landing search, and safe-drop settings."))
    FMetricLedgeAnalysisSettings LedgeAnalysis;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Modeled reaction, braking, and available stopping-runway settings."))
    FMetricStoppingDistanceSettings StoppingDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Three-height cover and exposure sampling settings against a forward threat.")) FMetricCoverExposureSettings CoverExposure;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Supported takeoff and post-landing runway settings tied to the primary trajectory landing.")) FMetricJumpRunwaySettings JumpRunway;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Forward interaction-reach sweep settings.")) FMetricInteractionReachSettings InteractionReach;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Independent left/right third-person shoulder camera clearance settings.")) FMetricShoulderClearanceSettings ShoulderClearance;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Approximate combat-area capacity reference settings based on profile personal space.")) FMetricCombatDensitySettings CombatDensity;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Alternate-height hull occupancy settings for crouch or other low-profile stances.")) FMetricAlternateStanceSettings AlternateStance;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Approximate obstacle height, depth, and reach envelope for mantle/vault candidates.")) FMetricMantleVaultSettings MantleVault;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Directional dodge/recovery clearance plus supporting-ground settings.")) FMetricRecoverySpaceSettings RecoverySpace;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Multi-direction exposure sampling settings in the active Analysis Frame.")) FMetricCrossfireSettings Crossfire;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric", meta = (ToolTip = "Optional visual-only representation for this profile. Representation settings never change query geometry or analysis results."))
    FMetricRepresentationSettings Representation;
};
