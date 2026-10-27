#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MetricTypes.h"
#include "MetricEditorSettings.generated.h"

UENUM()
enum class EMetricHUDAnchor : uint8
{
    TopLeft UMETA(DisplayName = "Top Left"),
    MiddleLeft UMETA(DisplayName = "Middle Left"),
    BottomLeft UMETA(DisplayName = "Bottom Left"),
    TopMiddle UMETA(DisplayName = "Top Middle"),
    BottomMiddle UMETA(DisplayName = "Bottom Middle"),
    TopRight UMETA(DisplayName = "Top Right"),
    MiddleRight UMETA(DisplayName = "Middle Right"),
    BottomRight UMETA(DisplayName = "Bottom Right")
};

UENUM()
enum class EMetricHUDViewportTargeting : uint8
{
    FollowLastInteracted UMETA(DisplayName = "Follow Last Interacted Viewport"),
    SelectedViewports UMETA(DisplayName = "Selected Viewports")
};


USTRUCT()
struct FMetricPersistentBenchmark
{
    GENERATED_BODY()

    UPROPERTY()
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY()
    FVector Forward = FVector::ForwardVector;

    UPROPERTY()
    FMetricHullSettings Hull;

    UPROPERTY()
    FMetricVisualizationTheme VisualTheme;

    UPROPERTY()
    FString Label;

    UPROPERTY()
    FString SourceProfileLabel;

    UPROPERTY()
    FString CapturedResults;

    UPROPERTY()
    bool bHasVisualSnapshot = false;

    UPROPERTY()
    FMetricLayerVisibility Visibility;

    UPROPERTY()
    FVector SurfaceNormal = FVector::UpVector;

    UPROPERTY()
    FMetricClearanceSettings ClearanceSettings;

    UPROPERTY()
    FMetricOverheadClearanceSettings OverheadClearanceSettings;

    UPROPERTY()
    FMetricSensorSettings Sensors;

    UPROPERTY()
    FMetricSensorFieldSettings SensorFieldSettings;

    UPROPERTY()
    FMetricRangeSettings Ranges;

    UPROPERTY()
    FMetricLocomotionSettings Locomotion;

    UPROPERTY()
    FMetricTrajectorySettings TrajectorySettings;

    UPROPERTY()
    FMetricCameraProbeSettings CameraProbeSettings;

    UPROPERTY()
    FMetricCameraSightlineSettings CameraSightlineSettings;

    UPROPERTY()
    FMetricGroundValidationSettings GroundValidationSettings;

    UPROPERTY()
    FMetricFitAnalysisSettings FitAnalysisSettings;

    UPROPERTY()
    FMetricThresholdAnalysisSettings ThresholdAnalysisSettings;

    UPROPERTY()
    FMetricTurnClearanceSettings TurnClearanceSettings;

    UPROPERTY()
    FMetricLedgeAnalysisSettings LedgeAnalysisSettings;

    UPROPERTY()
    FMetricStoppingDistanceSettings StoppingDistanceSettings;

    UPROPERTY() FMetricCoverExposureSettings CoverExposureSettings;
    UPROPERTY() FMetricJumpRunwaySettings JumpRunwaySettings;
    UPROPERTY() FMetricInteractionReachSettings InteractionReachSettings;
    UPROPERTY() FMetricShoulderClearanceSettings ShoulderClearanceSettings;
    UPROPERTY() FMetricCombatDensitySettings CombatDensitySettings;
    UPROPERTY() FMetricAlternateStanceSettings AlternateStanceSettings;
    UPROPERTY() FMetricMantleVaultSettings MantleVaultSettings;
    UPROPERTY() FMetricRecoverySpaceSettings RecoverySpaceSettings;
    UPROPERTY() FMetricCrossfireSettings CrossfireSettings;
};

USTRUCT()
struct FMetricPersistentBenchmarkMap
{
    GENERATED_BODY()

    UPROPERTY()
    FString MapPackageName;

    UPROPERTY()
    TArray<FMetricPersistentBenchmark> Benchmarks;
};

UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, meta = (DisplayName = "Metric - User"))
class METRIC_API UMetricEditorSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD")
    bool bShowViewportHUD = true;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD")
    EMetricHUDAnchor HUDAnchor = EMetricHUDAnchor::TopLeft;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD")
    bool bHUDMinimized = false;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (DisplayName = "Enable Live Analysis"))
    bool bShowLiveVisualizer = true;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD")
    EMetricHUDViewportTargeting HUDViewportTargeting = EMetricHUDViewportTargeting::FollowLastInteracted;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD")
    FMetricLayerVisibility LiveVisibility;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport1 = true;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport2 = false;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport3 = false;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport4 = false;

    UPROPERTY(Config, EditAnywhere, Category = "Preview Mesh Compatibility")
    bool bIncludeUnrealPreviewMeshes = true;

    UPROPERTY(Config)
    FLinearColor HullColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.5", ClampMax = "8.0"))
    float HullLineThickness = 1.5f;

    UPROPERTY(Config)
    FLinearColor ForwardColor = FLinearColor::White;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "cm"))
    float AnchorLift = 10.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "4.0", ClampMax = "100.0", Units = "cm"))
    float AnchorRadius = 16.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.5", ClampMax = "8.0"))
    float AnchorLineThickness = 2.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display")
    bool bEmphasizeProblemLines = true;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (EditCondition = "bEmphasizeProblemLines", ClampMin = "1.0", ClampMax = "4.0"))
    float WarningLineThicknessMultiplier = 1.75f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (EditCondition = "bEmphasizeProblemLines", ClampMin = "1.0", ClampMax = "6.0"))
    float FailureLineThicknessMultiplier = 2.5f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "8.0", ClampMax = "100.0", Units = "cm"))
    float LabelWorldSize = 18.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.0", ClampMax = "300.0", Units = "cm"))
    float LabelHeightPadding = 35.0f;

    UPROPERTY(Config)
    FLinearColor ClearancePassColor = FLinearColor(0.1f, 1.0f, 0.2f, 1.0f);

    UPROPERTY(Config)
    FLinearColor ClearanceWarningColor = FLinearColor(1.0f, 0.65f, 0.05f, 1.0f);

    UPROPERTY(Config)
    FLinearColor ClearanceFailColor = FLinearColor(1.0f, 0.08f, 0.05f, 1.0f);

    UPROPERTY(Config)
    FLinearColor SensorColor = FLinearColor(0.15f, 0.55f, 1.0f, 1.0f);

    UPROPERTY(Config)
    FLinearColor EngagementRangeColor = FLinearColor(0.1f, 0.45f, 1.0f, 1.0f);

    UPROPERTY(Config)
    FLinearColor SensorFieldColor = FLinearColor(0.12f, 0.72f, 1.0f, 1.0f);

    UPROPERTY(Config)
    FLinearColor SensorFieldBlockedColor = FLinearColor(1.0f, 0.18f, 0.08f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Sensors and Ranges", meta = (ClampMin = "0.5", ClampMax = "6.0"))
    float SecondaryLineThickness = 1.0f;

    UPROPERTY(Config)
    FLinearColor LocomotionBandColor = FLinearColor(0.72f, 0.32f, 1.0f, 1.0f);

    UPROPERTY(Config)
    FLinearColor TrajectoryColor = FLinearColor(1.0f, 0.42f, 0.08f, 1.0f);

    UPROPERTY(Config)
    FLinearColor TrajectoryLandingColor = FLinearColor(0.2f, 1.0f, 0.35f, 1.0f);

    UPROPERTY(Config)
    FLinearColor TrajectoryBlockedColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

    UPROPERTY(Config)
    FLinearColor CameraProbeColor = FLinearColor(1.0f, 0.15f, 0.85f, 1.0f);

    UPROPERTY(Config)
    FLinearColor CameraCompressedColor = FLinearColor(1.0f, 0.08f, 0.22f, 1.0f);

    UPROPERTY(Config)
    FLinearColor CameraSightlineColor = FLinearColor(0.95f, 0.45f, 1.0f, 1.0f);

    UPROPERTY(Config)
    FLinearColor GroundPassColor = FLinearColor(0.12f, 0.95f, 0.25f, 1.0f);

    UPROPERTY(Config)
    FLinearColor GroundStepFailColor = FLinearColor(1.0f, 0.62f, 0.08f, 1.0f);

    UPROPERTY(Config)
    FLinearColor GroundSlopeFailColor = FLinearColor(1.0f, 0.10f, 0.08f, 1.0f);

    UPROPERTY(Config)
    FLinearColor GroundGridColor = FLinearColor(0.25f, 0.85f, 0.85f, 0.55f);

    UPROPERTY(Config, EditAnywhere, Category = "Legend")
    bool bShowLegend = true;

    UPROPERTY(Config, EditAnywhere, Category = "Legend")
    EMetricHUDAnchor LegendAnchor = EMetricHUDAnchor::BottomLeft;

    UPROPERTY(Config, EditAnywhere, Category = "Legend", meta = (EditCondition = "bShowLegend"))
    FLinearColor LegendTextColor = FLinearColor(0.82f, 0.86f, 0.92f, 1.0f);

    UPROPERTY(Config)
    FLinearColor PinnedBenchmarkColor = FLinearColor(0.95f, 0.95f, 0.35f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Pinned Benchmarks", meta = (ClampMin = "1", ClampMax = "32"))
    int32 MaxPinnedBenchmarks = 8;

    UPROPERTY(Config, EditAnywhere, Category = "Pinned Benchmarks")
    bool bPersistPinnedBenchmarksPerMap = true;

    // Hidden editor-user data. Metric stores benchmark snapshots by saved map package.
    // This never writes Actors or objects into the level itself.
    UPROPERTY(Config)
    TArray<FMetricPersistentBenchmarkMap> PersistentBenchmarkMaps;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FText GetSectionText() const override;
};
