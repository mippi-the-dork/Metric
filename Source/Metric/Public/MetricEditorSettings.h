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
    TopMiddle UMETA(Hidden),
    BottomMiddle UMETA(Hidden),
    TopRight UMETA(DisplayName = "Top Right"),
    MiddleRight UMETA(DisplayName = "Middle Right"),
    BottomRight UMETA(DisplayName = "Bottom Right")
};

UENUM()
enum class EMetricEnvironmentSource : uint8
{
    Collision UMETA(DisplayName = "Collision", ToolTip = "Use authored Unreal collision only. Best for final gameplay validation when collision is production-ready."),
    Hybrid UMETA(DisplayName = "Hybrid", ToolTip = "Use authored collision and supplement collision-disabled visible primitives with approximate editor bounds."),
    VisualGeometry UMETA(DisplayName = "Visual Geometry (Approximate)", ToolTip = "Ignore authored collision and evaluate editor-visible primitive bounds. Useful for blockout and incomplete collision, but intentionally approximate.")
};

UENUM()
enum class EMetricAnalysisFrameMode : uint8
{
    World UMETA(DisplayName = "World Up", ToolTip = "Use conventional world +Z as Metric Up."),
    SurfaceNormal UMETA(DisplayName = "Surface Normal", ToolTip = "Use the placement surface normal as Metric Up for non-horizontal traversal and curved spaces."),
    CustomUp UMETA(DisplayName = "Custom Up", ToolTip = "Use the configured custom world-space direction as Metric Up.")
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
    FVector AnalysisUp = FVector::UpVector;

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
    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (ToolTip = "Show Metric's viewport HUD. This controls presentation only and does not disable Metric analysis or world visualizations."))
    bool bShowViewportHUD = true;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (ToolTip = "Choose which viewport corner or side the Metric HUD is anchored to."))
    EMetricHUDAnchor HUDAnchor = EMetricHUDAnchor::TopLeft;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (ToolTip = "Start the Metric HUD in its compact minimized state."))
    bool bHUDMinimized = false;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (DisplayName = "Enable Live Analysis", ToolTip = "Enable cursor-driven live Metric evaluation. Turning this off pauses new live analysis while preserving Metric, pins, and other editor workflow state."))
    bool bShowLiveVisualizer = true;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (DisplayName = "Live Analysis Update Rate", ToolTip = "Maximum rate for expensive cursor-driven Metric analysis while the cursor or camera is moving. The viewport may render faster than this. Set to 0 for uncapped analysis.", ClampMin = "0.0", ClampMax = "120.0", Units = "Hz"))
    float LiveAnalysisUpdateRateHz = 30.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (DisplayName = "Live Analysis Time Budget", ToolTip = "Approximate CPU time Metric may spend advancing live analysis during one editor frame. Metric yields between whole analysis jobs, so a single expensive job may exceed this value. Set to 0 for unlimited work per frame.", ClampMin = "0.0", ClampMax = "16.0", Units = "ms"))
    float LiveAnalysisTimeBudgetMs = 2.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (DisplayName = "Environment Source", ToolTip = "Collision uses authored collision only. Hybrid supplements collision-disabled visible primitives with approximate editor bounds. Visual Geometry uses approximate editor-visible primitive bounds and does not require authored collision."))
    EMetricEnvironmentSource EnvironmentSource = EMetricEnvironmentSource::Collision;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (DisplayName = "Analysis Frame", ToolTip = "Controls the Up direction used by spatial analyses. World Up preserves conventional horizon-locked behavior. Surface Normal aligns Metric to the placement surface. Custom Up uses the configured direction."))
    EMetricAnalysisFrameMode AnalysisFrame = EMetricAnalysisFrameMode::World;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (DisplayName = "Custom Up Direction", EditCondition = "AnalysisFrame == EMetricAnalysisFrameMode::CustomUp", ToolTip = "World-space Up direction used by Live Metric when Analysis Frame is Custom Up. Zero-length values fall back to World Up."))
    FVector CustomAnalysisUp = FVector::UpVector;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (DisplayName = "Show Analysis Frame", ToolTip = "Draw the current analysis frame axes so the active Up and Forward directions are visible in the viewport."))
    bool bShowAnalysisFrame = true;


    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (DisplayName = "Query Inspection", ToolTip = "Focus the viewport on one analysis and draw its actual query spans, contacts, normals and failure evidence."))
    bool bQueryInspectionEnabled = false;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (DisplayName = "Inspection Target", EditCondition = "bQueryInspectionEnabled", ToolTip = "Choose which analysis Query Inspection isolates. Auto selects the most relevant failing or warning analysis when possible."))
    EMetricInspectionTarget QueryInspectionTarget = EMetricInspectionTarget::Auto;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (ToolTip = "Choose whether the HUD follows the most recently interacted level viewport or appears only in explicitly selected viewports."))
    EMetricHUDViewportTargeting HUDViewportTargeting = EMetricHUDViewportTargeting::FollowLastInteracted;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (ToolTip = "Controls which Live Metric analysis layers are presented. These visibility options do not change profile thresholds or evaluation logic."))
    FMetricLayerVisibility LiveVisibility;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport1 = true;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport2 = false;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport3 = false;

    UPROPERTY(Config, EditAnywhere, Category = "Viewport HUD", meta = (EditCondition = "HUDViewportTargeting == EMetricHUDViewportTargeting::SelectedViewports"))
    bool bShowHUDInViewport4 = false;

    UPROPERTY(Config, EditAnywhere, Category = "Preview Mesh Compatibility", meta = (ToolTip = "Include Unreal Editor preview meshes in Metric's cycle list for compatibility with the native viewport preview workflow."))
    bool bIncludeUnrealPreviewMeshes = true;

    UPROPERTY(Config)
    FLinearColor HullColor = FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.5", ClampMax = "8.0", ToolTip = "Base line thickness used to draw the Metric hull in the editor viewport."))
    float HullLineThickness = 1.5f;

    UPROPERTY(Config)
    FLinearColor ForwardColor = FLinearColor::White;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.0", ClampMax = "100.0", Units = "cm", ToolTip = "Vertical offset used to lift the placement anchor above the sampled surface for readability."))
    float AnchorLift = 10.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "4.0", ClampMax = "100.0", Units = "cm", ToolTip = "World-space radius of the placement anchor marker."))
    float AnchorRadius = 16.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ClampMin = "0.5", ClampMax = "8.0"))
    float AnchorLineThickness = 2.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Display", meta = (ToolTip = "Increase line thickness for warning and failure evidence so problem geometry reads more clearly in the viewport."))
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
