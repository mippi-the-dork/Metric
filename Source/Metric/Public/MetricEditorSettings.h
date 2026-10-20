#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MetricEditorSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, meta = (DisplayName = "Metric - User"))
class METRIC_API UMetricEditorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Preview Mesh Compatibility")
    bool bIncludeUnrealPreviewMeshes = true;

    UPROPERTY(Config, EditAnywhere, Category = "Display")
    FLinearColor HullColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.5", ClampMax = "8.0"))
    float HullLineThickness = 1.5f;

    UPROPERTY(Config, EditAnywhere, Category = "Display")
    FLinearColor ForwardColor = FLinearColor::White;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "cm"))
    float AnchorLift = 10.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "4.0", ClampMax = "100.0", Units = "cm"))
    float AnchorRadius = 16.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.5", ClampMax = "8.0"))
    float AnchorLineThickness = 2.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "8.0", ClampMax = "100.0", Units = "cm"))
    float LabelWorldSize = 18.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.0", ClampMax = "300.0", Units = "cm"))
    float LabelHeightPadding = 35.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Clearance Probe")
    FLinearColor ClearancePassColor = FLinearColor(0.1f, 1.0f, 0.2f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Clearance Probe")
    FLinearColor ClearanceWarningColor = FLinearColor(1.0f, 0.65f, 0.05f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Clearance Probe")
    FLinearColor ClearanceFailColor = FLinearColor(1.0f, 0.08f, 0.05f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Sensors and Ranges")
    FLinearColor SensorColor = FLinearColor(0.15f, 0.55f, 1.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Sensors and Ranges")
    FLinearColor EngagementRangeColor = FLinearColor(0.1f, 0.45f, 1.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Sensors and Ranges", meta = (ClampMin = "0.5", ClampMax = "6.0"))
    float SecondaryLineThickness = 1.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Locomotion Bands")
    FLinearColor LocomotionBandColor = FLinearColor(0.72f, 0.32f, 1.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Trajectory")
    FLinearColor TrajectoryColor = FLinearColor(1.0f, 0.42f, 0.08f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Trajectory")
    FLinearColor TrajectoryLandingColor = FLinearColor(0.2f, 1.0f, 0.35f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Trajectory")
    FLinearColor TrajectoryBlockedColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Camera Probe")
    FLinearColor CameraProbeColor = FLinearColor(1.0f, 0.15f, 0.85f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Camera Probe")
    FLinearColor CameraCompressedColor = FLinearColor(1.0f, 0.08f, 0.22f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Camera Sightline")
    FLinearColor CameraSightlineColor = FLinearColor(0.95f, 0.45f, 1.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Ground Validation")
    FLinearColor GroundPassColor = FLinearColor(0.12f, 0.95f, 0.25f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Ground Validation")
    FLinearColor GroundStepFailColor = FLinearColor(1.0f, 0.62f, 0.08f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Ground Validation")
    FLinearColor GroundSlopeFailColor = FLinearColor(1.0f, 0.10f, 0.08f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Ground Validation")
    FLinearColor GroundGridColor = FLinearColor(0.25f, 0.85f, 0.85f, 0.55f);

    UPROPERTY(Config, EditAnywhere, Category = "Legend")
    bool bShowLegend = true;

    UPROPERTY(Config, EditAnywhere, Category = "Legend", meta = (EditCondition = "bShowLegend"))
    FLinearColor LegendTextColor = FLinearColor(0.82f, 0.86f, 0.92f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Legend", meta = (EditCondition = "bShowLegend", ClampMin = "8.0", ClampMax = "40.0", Units = "cm"))
    float LegendWorldSize = 11.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Legend", meta = (EditCondition = "bShowLegend", ClampMin = "10.0", ClampMax = "200.0", Units = "cm"))
    float LegendHeightOffset = 55.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Pinned Benchmarks")
    FLinearColor PinnedBenchmarkColor = FLinearColor(0.95f, 0.95f, 0.35f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Pinned Benchmarks", meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaxPinnedBenchmarks = 8;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FText GetSectionText() const override;
};
