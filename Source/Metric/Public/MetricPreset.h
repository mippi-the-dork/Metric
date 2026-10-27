#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MetricTypes.h"
#include "MetricPreset.generated.h"

UCLASS(BlueprintType)
class METRIC_API UMetricPreset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    virtual void PostLoad() override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricHullSettings Hull;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricVisualizationTheme VisualTheme;

    // Internal migration guard for profile-owned visualization themes.
    UPROPERTY()
    int32 VisualizationThemeVersion = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricClearanceSettings Clearance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricOverheadClearanceSettings OverheadClearance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricSensorSettings Sensors;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricSensorFieldSettings SensorField;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricRangeSettings EngagementRanges;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricLocomotionSettings LocomotionBands;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricTrajectorySettings Trajectory;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricCameraProbeSettings CameraProbe;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricCameraSightlineSettings CameraSightline;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricGroundValidationSettings GroundValidation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricFitAnalysisSettings FitAnalysis;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricThresholdAnalysisSettings ThresholdAnalysis;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricTurnClearanceSettings TurnClearance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricLedgeAnalysisSettings LedgeAnalysis;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricStoppingDistanceSettings StoppingDistance;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricCoverExposureSettings CoverExposure;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricJumpRunwaySettings JumpRunway;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricInteractionReachSettings InteractionReach;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricShoulderClearanceSettings ShoulderClearance;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricCombatDensitySettings CombatDensity;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricAlternateStanceSettings AlternateStance;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricMantleVaultSettings MantleVault;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricRecoverySpaceSettings RecoverySpace;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric") FMetricCrossfireSettings Crossfire;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricRepresentationSettings Representation;
};
