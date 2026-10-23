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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricHullSettings Hull;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Metric")
    FMetricClearanceSettings Clearance;

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
    FMetricRepresentationSettings Representation;
};
