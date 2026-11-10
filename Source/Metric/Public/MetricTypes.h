#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "MetricTypes.generated.h"

class UMetricProfile;
class UStaticMesh;
class AActor;


UENUM()
enum class EMetricAnalysisFrameMode : uint8
{
    World UMETA(DisplayName = "World Up", ToolTip = "Use conventional world +Z as Metric Up."),
    SurfaceNormal UMETA(DisplayName = "Surface Normal", ToolTip = "Use the placement surface normal as Metric Up for non-horizontal traversal and curved spaces."),
    CustomUp UMETA(DisplayName = "Custom Up", ToolTip = "Use the configured custom world-space direction as Metric Up."),
    Provider UMETA(DisplayName = "Gravity / Provider", ToolTip = "Ask the configured Metric gravity provider for the active Up direction and gravity magnitude. Falls back to World Up when no provider resolves the location.")
};

UENUM()
enum class EMetricFrameSamplingPolicy : uint8
{
    FixedAtOrigin UMETA(DisplayName = "Fixed at Origin", ToolTip = "Resolve the analysis frame once at the Metric origin and keep it fixed for the full evaluation."),
    ResampleAlongAnalysis UMETA(DisplayName = "Resample Along Analysis", ToolTip = "Re-evaluate the active frame source as Metric moves through space. Surface Normal follows sampled surfaces; Gravity / Provider re-samples the configured gravity provider at each relevant analysis point.")
};

UENUM(BlueprintType)
enum class EMetricHullType : uint8
{
    Capsule UMETA(DisplayName = "Capsule"),
    OrientedBox UMETA(DisplayName = "Oriented Box"),
    Sphere UMETA(DisplayName = "Sphere"),
    Cylinder UMETA(DisplayName = "Cylinder")
};

UENUM()
enum class EMetricCycleEntryType : uint8
{
    BuiltInMetric,
    MetricProfile,
    NativePreviewMesh
};

USTRUCT(BlueprintType)
struct FMetricHullSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (ToolTip = "Shape used for hull visualization and occupancy/sweep analyses. Cylinder uses a capsule approximation where Unreal query shapes do not provide an exact cylinder."))
    EMetricHullType HullType = EMetricHullType::Capsule;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "Total reference height of the gameplay entity."))
    float Height = 180.0f;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "Reference side-to-side width of the gameplay entity."))
    float Width = 88.0f;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "Reference front-to-back length of the gameplay entity. Capsule and sphere queries primarily use Width and Height."))
    float Length = 88.0f;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (Units = "cm", ToolTip = "Local offset from the Metric placement point to the center of the gameplay hull. Entity-relative analyses use this same offset."))
    FVector LocalOffset = FVector::ZeroVector;
};


USTRUCT(BlueprintType)
struct FMetricClearanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Clearance", meta = (ToolTip = "Enable forward hull-clearance analysis for this profile."))
    bool bShowClearanceProbe = true;

    UPROPERTY(EditAnywhere, Category = "Clearance", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance forward from the hull to test for unobstructed usable space."))
    float ProbeDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Clearance", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Remaining clearance at or below this distance is reported as Tight instead of Pass."))
    float WarningMargin = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Clearance", meta = (ToolTip = "Collision channel used by Clearance queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricClearanceResult
{
    bool bVisible = false;
    bool bBlocked = false;
    bool bNearThreshold = false;
    bool bHasObstacle = false;
    bool bApproximateShape = false;
    float ProbeDistance = 0.0f;
    float ClearDistance = 0.0f;
    float ObstacleDistance = 0.0f;
    float WarningMargin = 0.0f;
    FVector SweepStartOffset = FVector::ZeroVector;
    FVector SweepEndOffset = FVector::ZeroVector;
    FVector ImpactPointOffset = FVector::ZeroVector;
    FVector ImpactNormal = FVector::UpVector;
};

USTRUCT(BlueprintType)
struct FMetricSensorSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Sensors", meta = (ToolTip = "Show authored sensor-height reference markers for this profile. These are reference points, not collision queries."))
    bool bShowSensorHeights = true;

    UPROPERTY(EditAnywhere, Category = "Sensors", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Primary sensor or eye reference height above the profile origin along Analysis Up."))
    float PrimarySensorZOffset = 160.0f;

    UPROPERTY(EditAnywhere, Category = "Sensors", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Secondary low-profile sensor or eye reference height above the profile origin along Analysis Up."))
    float LowProfileSensorZOffset = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Sensors", meta = (ClampMin = "5.0", Units = "cm", ToolTip = "World-space radius used to draw the sensor-height reference markers."))
    float MarkerRadius = 24.0f;
};

USTRUCT(BlueprintType)
struct FMetricSensorFieldSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (ToolTip = "Enable sampled field-of-view analysis from the primary sensor origin."))
    bool bShowSensorField = false;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Maximum distance queried by each Sensor Field ray."))
    float Range = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (ClampMin = "1.0", ClampMax = "89.0", Units = "deg", ToolTip = "Half-angle to the left and right of Analysis Forward. The full horizontal field is twice this value."))
    float HorizontalHalfAngle = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (ClampMin = "1.0", ClampMax = "89.0", Units = "deg", ToolTip = "Vertical half-angle of the queried sensor field. Metric traces the lower boundary, center plane, and upper boundary across the horizontal fan."))
    float VerticalHalfAngle = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (ClampMin = "3", ClampMax = "31", ToolTip = "Number of horizontal directions sampled across the field. Each direction is queried at the lower, center, and upper vertical bands."))
    int32 HorizontalRayCount = 9;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (ToolTip = "Collision channel used by Sensor Field rays when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricSensorFieldRay
{
    FVector EndOffset = FVector::ZeroVector;
    bool bBlocked = false;
    float Distance = 0.0f;
};

struct FMetricSensorFieldResult
{
    bool bVisible = false;
    int32 TotalRays = 0;
    int32 BlockedRays = 0;
    float ClearPercent = 100.0f;
    float NearestBlockDistance = 0.0f;
    FVector OriginOffset = FVector::ZeroVector;
    TArray<FMetricSensorFieldRay> Rays;
};

USTRUCT(BlueprintType)
struct FMetricRangeSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges", meta = (ToolTip = "Show authored short, medium, and long gameplay range references. These are visual references and do not query geometry."))
    bool bShowEngagementRanges = false;

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Radius of the authored short-range gameplay reference."))
    float ShortRange = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Radius of the authored medium-range gameplay reference."))
    float MediumRange = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Radius of the authored long-range gameplay reference."))
    float LongRange = 3000.0f;
};

USTRUCT(BlueprintType)
struct FMetricLocomotionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (ToolTip = "Show movement-distance references derived from Speed multiplied by the Near, Mid, and Far times."))
    bool bShowLocomotionBands = false;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (ClampMin = "1.0", Units = "cm/s", ToolTip = "Reference movement speed used to convert the authored time bands into world distances."))
    float Speed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (ClampMin = "0.1", Units = "s", ToolTip = "Time horizon used for the near locomotion distance band."))
    float NearTime = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (ClampMin = "0.1", Units = "s", ToolTip = "Time horizon used for the middle locomotion distance band."))
    float MidTime = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (ClampMin = "0.1", Units = "s", ToolTip = "Time horizon used for the far locomotion distance band."))
    float FarTime = 5.0f;
};


UENUM(BlueprintType)
enum class EMetricTrajectoryDisplayMode : uint8
{
    SingleArc UMETA(DisplayName = "Single Arc"),
    JumpEnvelope UMETA(DisplayName = "Jump Envelope"),
    AnimatedEnvelope UMETA(DisplayName = "Animated Envelope")
};

USTRUCT(BlueprintType)
struct FMetricTrajectorySettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ToolTip = "Enable ballistic trajectory analysis for this profile."))
    bool bShowTrajectory = false;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "1.0", Units = "cm/s", ToolTip = "Primary horizontal launch speed along Analysis Forward."))
    float HorizontalSpeed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "1.0", Units = "cm/s", ToolTip = "Initial launch speed along Analysis Up."))
    float LaunchVerticalSpeed = 550.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "0.05", ClampMax = "5.0", ToolTip = "Multiplier applied to the editor world gravity magnitude along negative Analysis Up."))
    float GravityScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "0.25", ClampMax = "10.0", Units = "s", ToolTip = "Maximum simulated flight time before the trajectory stops if no valid landing is found."))
    float MaxSimulationTime = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "0.01", ClampMax = "0.2", Units = "s", ToolTip = "Time step between trajectory samples. Smaller values increase query density and cost."))
    float SampleInterval = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "0.0", ClampMax = "89.0", Units = "deg", ToolTip = "Maximum surface slope, relative to Analysis Up, accepted as a valid landing."))
    float MaxLandingSlope = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (DisplayName = "Show Trajectory Guide Rails", ToolTip = "Draw guide rails around the primary trajectory path to show the swept hull volume. This is separate from Jump Envelope, which evaluates multiple launch speeds."))
    bool bShowTrajectoryGuideRails = true;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (DisplayName = "Guide Rail Sample Stride", EditCondition = "bShowTrajectoryGuideRails", ClampMin = "1", ClampMax = "12", ToolTip = "Draw guide-rail cross sections every N trajectory samples. Higher values reduce visual density without changing trajectory evaluation."))
    int32 GuideRailSampleStride = 3;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Maximum headroom distance checked above the hull at the primary trajectory apex."))
    float ApexProbeDistance = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Apex headroom at or below this margin is reported as Tight."))
    float ApexWarningMargin = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (ToolTip = "Collision channel used by trajectory, landing, and apex-clearance queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, Category = "Trajectory | Jump Envelope", meta = (ToolTip = "Choose a single primary arc, a family of sampled launch speeds, or an animated sweep through that family."))
    EMetricTrajectoryDisplayMode DisplayMode = EMetricTrajectoryDisplayMode::SingleArc;

    UPROPERTY(EditAnywhere, Category = "Trajectory | Jump Envelope", meta = (EditCondition = "DisplayMode != EMetricTrajectoryDisplayMode::SingleArc", ClampMin = "2", ClampMax = "12", ToolTip = "Number of horizontal-speed variants sampled for Jump Envelope modes, including the primary full-speed arc."))
    int32 JumpEnvelopeArcCount = 7;

    UPROPERTY(EditAnywhere, Category = "Trajectory | Jump Envelope", meta = (EditCondition = "DisplayMode != EMetricTrajectoryDisplayMode::SingleArc", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Slowest horizontal-speed fraction included in the Jump Envelope. 1.0 is the primary Horizontal Speed."))
    float MinimumHorizontalSpeedFraction = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Trajectory | Jump Envelope", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Maximum downward travel from takeoff allowed before an arc stops searching for a landing. Set to 0 for no drop limit."))
    float MaximumDropDistance = 1000.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory | Jump Envelope", meta = (EditCondition = "DisplayMode == EMetricTrajectoryDisplayMode::AnimatedEnvelope", ClampMin = "0.05", ClampMax = "2.0", Units = "Hz", ToolTip = "Rate used to cycle the highlighted arc in Animated Envelope mode. This affects presentation only."))
    float EnvelopeAnimationRate = 0.25f;
};

struct FMetricTrajectoryArcResult
{
    float HorizontalSpeedFraction = 1.0f;
    float FlightTime = 0.0f;
    float HorizontalDistance = 0.0f;
    float LandingDrop = 0.0f;
    bool bBlocked = false;
    bool bHasLanding = false;
    bool bLandingValid = false;
    bool bExceededMaxDrop = false;
    FVector ImpactPointOffset = FVector::ZeroVector;
    FVector ImpactNormal = FVector::UpVector;
    int32 ApexSampleIndex = INDEX_NONE;
    TArray<FVector> SampleOffsets;
    TArray<FVector> SampleUps;
};

struct FMetricTrajectoryResult
{
    bool bVisible = false;
    bool bBlocked = false;
    bool bHasLanding = false;
    bool bLandingValid = false;
    bool bApproximateShape = false;
    float FlightTime = 0.0f;
    float HorizontalDistance = 0.0f;
    FVector ImpactPointOffset = FVector::ZeroVector;
    FVector ImpactNormal = FVector::UpVector;
    int32 ApexSampleIndex = INDEX_NONE;
    FVector ApexOffset = FVector::ZeroVector;
    FVector ApexClearanceImpactOffset = FVector::ZeroVector;
    float ApexHeight = 0.0f;
    float ApexHeadroom = 0.0f;
    bool bApexHasObstacle = false;
    bool bApexTight = false;
    TArray<FVector> SampleOffsets;
    TArray<FVector> SampleUps;
    TArray<FMetricTrajectoryArcResult> EnvelopeArcs;
    int32 ValidEnvelopeLandings = 0;
    int32 TestedEnvelopeArcs = 0;
    int32 ActiveEnvelopeArcIndex = INDEX_NONE;
};


USTRUCT(BlueprintType)
struct FMetricCameraProbeSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (ToolTip = "Enable third-person camera-boom clearance analysis."))
    bool bShowCameraProbe = false;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Camera pivot height above the profile origin along Analysis Up."))
    float PivotZOffset = 160.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Desired camera-boom distance behind the profile."))
    float TargetDistance = 350.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (Units = "cm", ToolTip = "Additional camera endpoint offset along Analysis Up."))
    float VerticalOffset = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (Units = "cm", ToolTip = "Camera endpoint offset along Analysis Right. Positive and negative values choose opposite shoulders."))
    float ShoulderOffset = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (ClampMin = "1.0", Units = "cm", ToolTip = "Radius of the camera sweep used to detect compression against geometry."))
    float ProbeRadius = 12.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Camera distance at or below this value is considered too close after compression."))
    float MinDistance = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (ToolTip = "Collision channel used by Camera Probe sweeps when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricCameraProbeResult
{
    bool bVisible = false;
    bool bCompressed = false;
    bool bBelowMinimum = false;
    float TargetDistance = 0.0f;
    float ResolvedDistance = 0.0f;
    float CompressionPercent = 0.0f;
    FVector PivotOffset = FVector::ZeroVector;
    FVector DesiredCameraOffset = FVector::ZeroVector;
    FVector ResolvedCameraOffset = FVector::ZeroVector;
    FVector ImpactPointOffset = FVector::ZeroVector;
    FVector ImpactNormal = FVector::UpVector;
};


USTRUCT(BlueprintType)
struct FMetricCameraSightlineSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (ToolTip = "Enable camera-to-subject visibility and framing analysis."))
    bool bShowCameraSightline = false;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Subject point height above the profile origin along Analysis Up."))
    float SubjectZOffset = 140.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (ClampMin = "1.0", ClampMax = "80.0", Units = "deg", ToolTip = "Maximum allowed angular deviation between the camera view direction and subject direction before framing fails."))
    float FramingHalfAngle = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Radius of the sightline sweep. Use 0 for a line-like query or a larger value for a thicker visibility corridor."))
    float TraceRadius = 4.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (ToolTip = "Collision channel used by Camera Sightline queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricCameraSightlineResult
{
    bool bVisible = false;
    bool bOccluded = false;
    bool bFramingFail = false;
    float SubjectAngle = 0.0f;
    FVector CameraOffset = FVector::ZeroVector;
    FVector AimOffset = FVector::ZeroVector;
    FVector SubjectOffset = FVector::ZeroVector;
    FVector ImpactPointOffset = FVector::ZeroVector;
};


USTRUCT(BlueprintType)
struct FMetricOverheadClearanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance", meta = (ToolTip = "Enable vertical headroom analysis above the active hull."))
    bool bShowOverheadClearance = false;

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance", meta = (ClampMin = "50.0", Units = "cm", ToolTip = "Maximum distance above the hull top searched for overhead geometry."))
    float MaxProbeHeight = 350.0f;

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Extra headroom at or below this amount is reported as Tight."))
    float WarningMargin = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance", meta = (ToolTip = "Collision channel used by Overhead Clearance queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricOverheadClearanceResult
{
    bool bVisible = false;
    bool bBlocked = false;
    bool bNearThreshold = false;
    float AvailableHeight = 0.0f;
    float RequiredHeight = 0.0f;
    float HeightDeficit = 0.0f;
    FVector ImpactPointOffset = FVector::ZeroVector;
};


USTRUCT(BlueprintType)
struct FMetricGroundValidationSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ToolTip = "Enable sampled floor continuity, slope, and discrete-step validation ahead of the profile."))
    bool bShowGroundValidation = false;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "50.0", Units = "cm", ToolTip = "Distance ahead of the profile covered by Ground Validation samples."))
    float ForwardDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance between successive forward ground sample stations."))
    float SampleSpacing = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Total side-to-side ground coverage sampled around the forward path."))
    float LateralCoverage = 88.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "0.0", ClampMax = "89.0", Units = "deg", ToolTip = "Maximum continuous floor slope accepted as walkable, measured relative to Analysis Up."))
    float MaxWalkableSlope = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Maximum discrete upward surface discontinuity accepted between neighboring samples. Continuous ramps are evaluated by Max Walkable Slope instead."))
    float MaxStepUp = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Maximum discrete downward surface discontinuity accepted between neighboring samples. Larger drops can also become ledges for Ledge Analysis."))
    float MaxStepDown = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance above each sample point from which the floor trace begins."))
    float TraceHeight = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance below the trace start searched for supporting floor."))
    float TraceDepth = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (ToolTip = "Collision channel used by Ground Validation floor queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricGroundSample
{
    FVector Offset = FVector::ZeroVector;
    FVector Normal = FVector::UpVector;
    bool bHit = false;
    bool bSlopeFail = false;
    bool bStepUpFail = false;
    bool bStepDownFail = false;
    float SlopeDegrees = 0.0f;
    float StepDelta = 0.0f;
};

struct FMetricGroundValidationResult
{
    bool bVisible = false;
    bool bPass = true;
    int32 TotalSamples = 0;
    int32 FailedSamples = 0;
    float MaxObservedSlope = 0.0f;
    float MaxObservedStepUp = 0.0f;
    float MaxObservedStepDown = 0.0f;
    TArray<FMetricGroundSample> Samples;
};



USTRUCT(BlueprintType)
struct FMetricFitAnalysisSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ToolTip = "Enable sampled corridor-fit analysis using the configured hull plus width and headroom measurements."))
    bool bShowFitAnalysis = false;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Length of corridor sampled ahead of the profile."))
    float ForwardDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance between corridor sample stations."))
    float SampleSpacing = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Maximum distance searched to each side when measuring available corridor width."))
    float MaxSideProbeDistance = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Maximum distance above the sampled floor searched when measuring available height."))
    float MaxCeilingProbeHeight = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Width or height remaining within this margin of the required hull size is reported as Tight."))
    float WarningMargin = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance above each corridor station from which the supporting-floor trace begins."))
    float FloorTraceHeight = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance below the floor-trace start searched for supporting floor."))
    float FloorTraceDepth = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (ToolTip = "Collision channel used by Fit Analysis occupancy and measurement queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricFitSample
{
    FVector BaseOffset = FVector::ZeroVector;
    FVector LeftOffset = FVector::ZeroVector;
    FVector RightOffset = FVector::ZeroVector;
    FVector CeilingOffset = FVector::ZeroVector;
    bool bHasFloor = false;
    bool bLeftBounded = false;
    bool bRightBounded = false;
    bool bCeilingBounded = false;
    bool bWidthFail = false;
    bool bHeightFail = false;
    bool bHullBlocked = false;
    bool bNearThreshold = false;
    float AvailableWidth = 0.0f;
    float AvailableHeight = 0.0f;
    float WidthDeficit = 0.0f;
    float HeightDeficit = 0.0f;
    FVector HullImpactOffset = FVector::ZeroVector;
};

struct FMetricFitAnalysisResult
{
    bool bVisible = false;
    bool bPass = true;
    bool bNearThreshold = false;
    int32 TotalSamples = 0;
    int32 FailedSamples = 0;
    int32 InvalidSamples = 0;
    int32 HullBlockedSamples = 0;
    float RequiredWidth = 0.0f;
    float RequiredHeight = 0.0f;
    float MinimumAvailableWidth = 0.0f;
    float MinimumAvailableHeight = 0.0f;
    float MaxWidthDeficit = 0.0f;
    float MaxHeightDeficit = 0.0f;
    TArray<FMetricFitSample> Samples;
};



USTRUCT(BlueprintType)
struct FMetricThresholdAnalysisSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ToolTip = "Enable a short transition check for doorways, tunnel mouths, curbs, and similar thresholds."))
    bool bShowThresholdAnalysis = false;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Total transition depth sampled at the entry, midpoint, and far side."))
    float ProbeDepth = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Maximum distance searched to each side when measuring threshold width."))
    float MaxSideProbeDistance = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Maximum distance above the sampled floor searched when measuring threshold height."))
    float MaxCeilingProbeHeight = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Width or height remaining within this margin of the required hull size is reported as Tight."))
    float WarningMargin = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Maximum discrete step up accepted across the threshold. Continuous ramps are not treated as steps."))
    float MaxStepUp = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Maximum discrete step down accepted across the threshold. Continuous ramps are not treated as steps."))
    float MaxStepDown = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance above each threshold station from which the supporting-floor trace begins."))
    float FloorTraceHeight = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance below the floor-trace start searched for supporting floor."))
    float FloorTraceDepth = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (ToolTip = "Collision channel used by Threshold Analysis occupancy, floor, and measurement queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricThresholdSample
{
    FVector BaseOffset = FVector::ZeroVector;
    FVector LeftOffset = FVector::ZeroVector;
    FVector RightOffset = FVector::ZeroVector;
    FVector CeilingOffset = FVector::ZeroVector;
    bool bHasFloor = false;
    bool bWidthFail = false;
    bool bHeightFail = false;
    bool bHullBlocked = false;
    bool bStepUpFail = false;
    bool bStepDownFail = false;
    bool bNearThreshold = false;
    float AvailableWidth = 0.0f;
    float AvailableHeight = 0.0f;
    float WidthDeficit = 0.0f;
    float HeightDeficit = 0.0f;
    float StepDelta = 0.0f;
    FVector FloorNormal = FVector::UpVector;
    FVector HullImpactOffset = FVector::ZeroVector;
};

struct FMetricThresholdAnalysisResult
{
    bool bVisible = false;
    bool bPass = true;
    bool bNearThreshold = false;
    int32 TotalSamples = 0;
    int32 FailedSamples = 0;
    int32 InvalidSamples = 0;
    int32 HullBlockedSamples = 0;
    float RequiredWidth = 0.0f;
    float RequiredHeight = 0.0f;
    float MinimumAvailableWidth = 0.0f;
    float MinimumAvailableHeight = 0.0f;
    float MaxWidthDeficit = 0.0f;
    float MaxHeightDeficit = 0.0f;
    float MaxObservedStepUp = 0.0f;
    float MaxObservedStepDown = 0.0f;
    TArray<FMetricThresholdSample> Samples;
};

UENUM(BlueprintType)
enum class EMetricTurnDirection : uint8
{
    Left UMETA(DisplayName = "Left"),
    Right UMETA(DisplayName = "Right"),
    Both UMETA(DisplayName = "Both")
};

USTRUCT(BlueprintType)
struct FMetricTurnClearanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (ToolTip = "Enable swept turning-room analysis around the configured arc."))
    bool bShowTurnClearance = false;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Radius from the profile origin to the center path followed by the hull during the turn."))
    float TurnRadius = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (ClampMin = "10.0", ClampMax = "180.0", Units = "deg", ToolTip = "Total angle tested from the starting forward direction."))
    float TurnAngle = 90.0f;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (ClampMin = "2.0", ClampMax = "30.0", Units = "deg", ToolTip = "Angular spacing between hull sweep samples. Smaller values increase fidelity and query cost."))
    float SampleAngle = 7.5f;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (ToolTip = "Choose whether to test left turns, right turns, or both."))
    EMetricTurnDirection Direction = EMetricTurnDirection::Both;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (ToolTip = "Collision channel used by Turn Clearance sweeps when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricTurnPathResult
{
    bool bTested = false;
    bool bBlocked = false;
    float ClearAngle = 0.0f;
    float ClearDistance = 0.0f;
    FVector ImpactPointOffset = FVector::ZeroVector;
    FVector ImpactNormal = FVector::UpVector;
    TArray<FVector> SampleOffsets;
};

struct FMetricTurnClearanceResult
{
    bool bVisible = false;
    bool bApproximateShape = false;
    FMetricTurnPathResult Left;
    FMetricTurnPathResult Right;
};


USTRUCT(BlueprintType)
struct FMetricLedgeAnalysisSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (ToolTip = "Enable forward ledge detection. Continuous ramps remain ground; drops beyond the Ground Validation step-down limit become ledge candidates."))
    bool bShowLedgeAnalysis = false;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Maximum distance ahead searched for the first ledge transition."))
    float ForwardDistance = 150.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (ClampMin = "5.0", Units = "cm", ToolTip = "Distance between forward samples used to locate the ledge transition."))
    float SampleSpacing = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Maximum detected drop depth reported as safe. Deeper landings are failures."))
    float MaxSafeDrop = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Maximum distance below a detected edge searched for a landing surface."))
    float MaxProbeDepth = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (ClampMin = "10.0", Units = "cm", ToolTip = "Distance above each ledge sample from which the downward floor trace begins."))
    float TraceHeight = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (ToolTip = "Collision channel used by Ledge Analysis ground and landing queries when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricLedgeAnalysisResult
{
    bool bVisible = false;
    bool bHasEdge = false;
    bool bHasLanding = false;
    bool bSafeDrop = true;
    bool bWarning = false;
    float EdgeDistance = 0.0f;
    float DropDepth = 0.0f;
    FVector EdgeOffset = FVector::ZeroVector;
    FVector LandingOffset = FVector::ZeroVector;
};


USTRUCT(BlueprintType)
struct FMetricStoppingDistanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (ToolTip = "Enable modeled stopping-runway analysis using speed, reaction time, and deceleration."))
    bool bShowStoppingDistance = false;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (ClampMin = "1.0", Units = "cm/s", ToolTip = "Movement speed used to calculate reaction and braking distance."))
    float Speed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (ClampMin = "1.0", Units = "cm/s^2", ToolTip = "Braking deceleration used to calculate modeled braking distance."))
    float Deceleration = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (ClampMin = "0.0", Units = "s", ToolTip = "Delay before braking begins. Speed multiplied by this time becomes reaction distance."))
    float ReactionTime = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (ClampMin = "0.0", Units = "cm", ToolTip = "Available runway within this distance of the required stopping distance is reported as Tight."))
    float WarningMargin = 75.0f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (ToolTip = "Collision channel used by the stopping-runway hull sweep when the Environment Source includes collision."))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};

struct FMetricStoppingDistanceResult
{
    bool bVisible = false;
    bool bBlocked = false;
    bool bNearThreshold = false;
    bool bHasObstacle = false;
    bool bApproximateShape = false;
    float ReactionDistance = 0.0f;
    float BrakingDistance = 0.0f;
    float RequiredDistance = 0.0f;
    float AvailableDistance = 0.0f;
    float ObstacleDistance = 0.0f;
    FVector StartOffset = FVector::ZeroVector;
    FVector EndOffset = FVector::ZeroVector;
    FVector ImpactPointOffset = FVector::ZeroVector;
    FVector ImpactNormal = FVector::UpVector;
};



USTRUCT(BlueprintType)
struct FMetricCoverExposureSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta = (ToolTip = "Enable three-height cover sampling against a threat point ahead of the profile.")) bool bShowCoverExposure = false;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(ClampMin="50.0", Units="cm", ToolTip = "Distance from the profile to the sampled threat point along Analysis Forward.")) float ThreatDistance = 1200.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(ClampMin="0.0", Units="cm", ToolTip = "Threat point height above the profile origin along Analysis Up.")) float ThreatHeight = 160.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(ClampMin="0.0", Units="cm", ToolTip = "Low body sample height used to determine whether low cover blocks the threat.")) float LowSampleHeight = 60.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(ClampMin="0.0", Units="cm", ToolTip = "Torso sample height used for the middle cover/exposure query.")) float TorsoSampleHeight = 120.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(ClampMin="0.0", Units="cm", ToolTip = "Head sample height used for the upper cover/exposure query.")) float HeadSampleHeight = 170.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(ToolTip = "Collision channel used by Cover / Exposure sightlines when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricCoverExposureResult
{
    bool bVisible=false; float ExposurePercent=100.0f; bool bLowCovered=false; bool bTorsoCovered=false; bool bHeadCovered=false;
    FVector ThreatOffset=FVector::ZeroVector; FVector LowOffset=FVector::ZeroVector; FVector TorsoOffset=FVector::ZeroVector; FVector HeadOffset=FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMetricJumpRunwaySettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(ToolTip="Validate continuous walkable support before takeoff and after the primary trajectory landing. Runway support uses the profile Ground Validation slope and step limits.")) bool bShowJumpRunway = false;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(ClampMin="0.0", Units="cm", ToolTip = "Required continuous supported distance behind the takeoff point.")) float TakeoffRunway = 150.0f;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(ClampMin="0.0", Units="cm", ToolTip = "Required continuous supported distance forward from the actual primary trajectory landing.")) float LandingRunway = 180.0f;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(ClampMin="10.0", Units="cm", ToolTip = "Maximum depth searched for walkable support beneath runway samples.")) float GroundProbeDepth = 150.0f;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(ToolTip = "Collision channel used by Jump Runway support queries when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricJumpRunwayResult
{
    bool bVisible=false; bool bTakeoffPass=false; bool bLandingPass=false; float TakeoffAvailable=0.0f; float LandingAvailable=0.0f;
    FVector TakeoffEndOffset=FVector::ZeroVector; FVector LandingStartOffset=FVector::ZeroVector; FVector LandingEndOffset=FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMetricInteractionReachSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta = (ToolTip = "Enable forward interaction-reach probing from the configured reach origin.")) bool bShowInteractionReach = false;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(ClampMin="10.0", Units="cm", ToolTip = "Maximum forward distance searched for an interaction target.")) float ReachDistance = 180.0f;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(ClampMin="0.0", Units="cm", ToolTip = "Interaction origin height above the offset hull origin along Analysis Up.")) float ReachHeight = 130.0f;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(ClampMin="1.0", Units="cm", ToolTip = "Radius of the interaction sweep used to find reachable geometry.")) float ProbeRadius = 18.0f;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(ToolTip = "Collision channel used by Interaction Reach when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricInteractionReachResult
{
    bool bVisible=false; bool bHasTarget=false; float TargetDistance=0.0f; FVector OriginOffset=FVector::ZeroVector; FVector EndOffset=FVector::ZeroVector; FVector HitOffset=FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMetricShoulderClearanceSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta = (ToolTip = "Enable independent left and right third-person shoulder-camera clearance checks.")) bool bShowShoulderClearance = false;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(ClampMin="0.0", Units="cm", ToolTip = "Camera pivot height above the offset hull origin along Analysis Up.")) float PivotHeight = 150.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(ClampMin="0.0", Units="cm", ToolTip = "Sideways distance from the pivot to each shoulder camera path along Analysis Right.")) float ShoulderOffset = 55.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(ClampMin="10.0", Units="cm", ToolTip = "Distance behind the profile tested for each shoulder camera position.")) float CameraDistance = 300.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(ClampMin="1.0", Units="cm", ToolTip = "Radius of each shoulder camera sweep.")) float ProbeRadius = 12.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(ToolTip = "Collision channel used by Shoulder Camera Clearance when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricShoulderClearanceResult
{
    bool bVisible=false; bool bLeftBlocked=false; bool bRightBlocked=false; float LeftDistance=0.0f; float RightDistance=0.0f;
    FVector PivotOffset=FVector::ZeroVector; FVector LeftOffset=FVector::ZeroVector; FVector RightOffset=FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMetricCombatDensitySettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Combat Spacing Density", meta = (ToolTip = "Show an approximate capacity reference for profile-sized personal-space cells inside a circular encounter area. This does not query scene occupancy.")) bool bShowCombatDensity = false;
    UPROPERTY(EditAnywhere, Category="Combat Spacing Density", meta=(ClampMin="50.0", Units="cm", ToolTip = "Radius of the encounter area used for the capacity estimate.")) float AreaRadius = 600.0f;
    UPROPERTY(EditAnywhere, Category="Combat Spacing Density", meta=(ClampMin="0.0", Units="cm", ToolTip = "Additional spacing added around the profile hull when estimating personal-space cell size.")) float PersonalSpacePadding = 50.0f;
};
struct FMetricCombatDensityResult
{
    bool bVisible=false; float PersonalDiameter=0.0f; int32 ApproxCapacity=0; float AreaRadius=0.0f;
};

USTRUCT(BlueprintType)
struct FMetricAlternateStanceSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit", meta=(ToolTip="Check whether the full configured hull fits at standing height and at the alternate stance height. Uses hull occupancy, not only a centerline headroom trace.")) bool bShowAlternateStance = false;
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit", meta=(ClampMin="10.0", Units="cm", ToolTip = "Hull height used for the alternate stance, such as crouch or low-profile traversal.")) float AlternateHeight = 110.0f;
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit", meta=(ClampMin="0.0", Units="cm", ToolTip = "Available headroom within this amount of the required stance height is reported as Tight.")) float WarningMargin = 15.0f;
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit", meta=(ToolTip = "Collision channel used by Alternate Stance hull-occupancy and headroom queries when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricAlternateStanceResult
{
    bool bVisible=false; bool bStandingFits=false; bool bAlternateFits=false; float AvailableHeight=0.0f;
};

USTRUCT(BlueprintType)
struct FMetricMantleVaultSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta = (ToolTip = "Enable approximate obstacle height/depth validation for mantle or vault candidates ahead of the profile.")) bool bShowMantleVault = false;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(ClampMin="10.0", Units="cm", ToolTip = "Maximum forward distance searched for a mantle or vault obstacle.")) float ForwardReach = 120.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(ClampMin="10.0", Units="cm", ToolTip = "Minimum obstacle height considered a valid mantle/vault candidate.")) float MinObstacleHeight = 45.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(ClampMin="10.0", Units="cm", ToolTip = "Maximum obstacle height considered a valid mantle/vault candidate.")) float MaxObstacleHeight = 180.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(ClampMin="10.0", Units="cm", ToolTip = "Maximum top-surface depth accepted for the configured mantle/vault envelope.")) float MaxObstacleDepth = 100.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(ToolTip = "Collision channel used by Mantle / Vault obstacle queries when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricMantleVaultResult
{
    bool bVisible=false; bool bHasObstacle=false; bool bHeightValid=false; bool bDepthValid=false; float ObstacleHeight=0.0f; float ObstacleDepth=0.0f;
    FVector FrontOffset=FVector::ZeroVector; FVector TopOffset=FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMetricRecoverySpaceSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(ToolTip="Validate directional recovery space using the configured hull. A direction passes only when the hull path is unobstructed and supporting ground remains within the profile Ground Validation slope and step limits.")) bool bShowRecoverySpace = false;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(ClampMin="10.0", Units="cm", ToolTip = "Required supported unobstructed recovery distance along Analysis Forward.")) float ForwardDistance = 250.0f;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(ClampMin="10.0", Units="cm", ToolTip = "Required supported unobstructed recovery distance opposite Analysis Forward.")) float BackwardDistance = 180.0f;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(ClampMin="10.0", Units="cm", ToolTip = "Required supported unobstructed recovery distance to both left and right.")) float SideDistance = 220.0f;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(ToolTip = "Collision channel used by Recovery / Dodge hull sweeps and support queries when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricRecoverySpaceResult
{
    bool bVisible=false; bool bForwardClear=false; bool bBackwardClear=false; bool bLeftClear=false; bool bRightClear=false;
    bool bForwardGroundSupported=true; bool bBackwardGroundSupported=true; bool bLeftGroundSupported=true; bool bRightGroundSupported=true;
    float ForwardClear=0.0f; float BackwardClear=0.0f; float LeftClear=0.0f; float RightClear=0.0f;
};

USTRUCT(BlueprintType)
struct FMetricCrossfireSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta = (ToolTip = "Enable exposure sampling from evenly distributed threat directions in the active Analysis Frame.")) bool bShowCrossfire = false;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(ClampMin="100.0", Units="cm", ToolTip = "Radius at which threat origins are placed around the profile.")) float ThreatDistance = 1200.0f;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(ClampMin="4", ClampMax="16", ToolTip = "Number of evenly spaced threat directions sampled around the profile.")) int32 DirectionCount = 8;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(ClampMin="0.0", Units="cm", ToolTip = "Body sample height above the offset hull origin along Analysis Up.")) float SampleHeight = 130.0f;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(ToolTip = "Collision channel used by Crossfire Exposure sightlines when the Environment Source includes collision.")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricCrossfireRay
{
    FVector EndOffset=FVector::ZeroVector; bool bExposed=true;
};
struct FMetricCrossfireResult
{
    bool bVisible=false; int32 ExposedDirections=0; int32 TotalDirections=0; float ExposurePercent=0.0f; TArray<FMetricCrossfireRay> Rays;
};

USTRUCT(BlueprintType)
struct FMetricRepresentationSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Representation", meta = (ToolTip = "Show an optional visual-only Static Mesh representation with this profile. It does not affect Metric queries."))
    bool bShowStaticMesh = false;

    UPROPERTY(EditAnywhere, Category = "Representation", meta = (ToolTip = "Static Mesh used only as a visual representation of the profile."))
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, Category = "Representation", meta = (ToolTip = "Transform applied to the visual-only Static Mesh relative to the Metric profile origin."))
    FTransform RelativeTransform = FTransform::Identity;
};

struct FMetricCycleEntry
{
    EMetricCycleEntryType Type = EMetricCycleEntryType::BuiltInMetric;
    FText DisplayName;
    TSoftObjectPtr<UMetricProfile> Profile;
    FSoftObjectPath NativePreviewMesh;

    bool IsMetricEntry() const
    {
        return Type == EMetricCycleEntryType::BuiltInMetric || Type == EMetricCycleEntryType::MetricProfile;
    }
};


USTRUCT(BlueprintType)
struct FMetricStatusColors
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Colors", meta = (ToolTip = "Color used when the associated analysis passes."))
    FLinearColor Pass = FLinearColor(0.10f, 0.82f, 1.00f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Colors", meta = (ToolTip = "Color used when the associated analysis is close to its configured limit or reports a warning."))
    FLinearColor Tight = FLinearColor(0.72f, 0.42f, 1.00f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Colors", meta = (ToolTip = "Color used when the associated analysis fails."))
    FLinearColor Fail = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
};

USTRUCT(BlueprintType)
struct FMetricVisualizationTheme
{
    GENERATED_BODY()

    // Every color below is profile-owned. The same semantic color is used by
    // the viewport visualization and the matching HUD/result readout.
    UPROPERTY(EditAnywhere, Category = "Core|Hull") FLinearColor Hull = FLinearColor(0.05f, 0.78f, 1.0f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Core|Hull") FLinearColor Forward = FLinearColor(0.68f, 0.70f, 0.74f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Core|Hull") FLinearColor Anchor = FLinearColor(0.92f, 0.94f, 0.98f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Space & Fit|Clearance") FMetricStatusColors Clearance;
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Overhead") FMetricStatusColors Overhead;
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Fit") FMetricStatusColors Fit;
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Threshold") FMetricStatusColors Threshold;
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Stopping Distance") FMetricStatusColors Stopping;

    UPROPERTY(EditAnywhere, Category = "Space & Fit|Ground") FLinearColor GroundPass = FLinearColor(0.10f, 0.82f, 1.00f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Ground") FLinearColor GroundStepFail = FLinearColor(0.96f, 0.66f, 0.20f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Ground") FLinearColor GroundSlopeFail = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Ground") FLinearColor GroundMissingFloor = FLinearColor(0.92f, 0.18f, 0.52f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Space & Fit|Ground") FLinearColor GroundGrid = FLinearColor(0.42f, 0.48f, 0.56f, 0.55f);

    UPROPERTY(EditAnywhere, Category = "Movement|Turn Clearance") FLinearColor TurnClear = FLinearColor(0.05f, 0.92f, 0.70f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Turn Clearance") FLinearColor TurnBlocked = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Ledge") FLinearColor LedgeSafe = FLinearColor(0.05f, 0.92f, 0.70f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Ledge") FLinearColor LedgeWarning = FLinearColor(0.96f, 0.66f, 0.20f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Ledge") FLinearColor LedgeFail = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Ledge") FLinearColor LedgeNone = FLinearColor(0.55f, 0.58f, 0.64f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Sensors|Sensor Heights") FLinearColor SensorPrimary = FLinearColor(0.28f, 0.58f, 1.0f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Sensors|Sensor Heights") FLinearColor SensorLow = FLinearColor(0.50f, 0.72f, 1.0f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Sensors|Sensor Field") FLinearColor SensorFieldClear = FLinearColor(0.28f, 0.58f, 1.0f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Sensors|Sensor Field") FLinearColor SensorFieldBlocked = FLinearColor(0.92f, 0.18f, 0.52f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Ranges|Engagement") FLinearColor EngagementShort = FLinearColor(0.95f, 0.36f, 0.20f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Ranges|Engagement") FLinearColor EngagementMedium = FLinearColor(0.95f, 0.52f, 0.22f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Ranges|Engagement") FLinearColor EngagementLong = FLinearColor(0.95f, 0.70f, 0.28f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Locomotion") FLinearColor LocomotionNear = FLinearColor(0.05f, 0.92f, 0.70f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Locomotion") FLinearColor LocomotionMid = FLinearColor(0.16f, 0.78f, 0.82f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Locomotion") FLinearColor LocomotionFar = FLinearColor(0.30f, 0.64f, 0.92f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Movement|Trajectory") FLinearColor TrajectoryPath = FLinearColor(0.05f, 0.92f, 0.70f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Trajectory") FLinearColor TrajectoryLanding = FLinearColor(0.10f, 0.82f, 1.00f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Trajectory") FLinearColor TrajectoryTight = FLinearColor(0.72f, 0.42f, 1.00f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Trajectory") FLinearColor TrajectoryBlocked = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Trajectory") FLinearColor TrajectoryApex = FLinearColor(0.20f, 0.84f, 0.92f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Camera|Probe") FLinearColor CameraClear = FLinearColor(0.98f, 0.30f, 0.82f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Camera|Probe") FLinearColor CameraCompressed = FLinearColor(0.76f, 0.38f, 1.00f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Camera|Probe") FLinearColor CameraTooClose = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Camera|Sightline") FLinearColor SightlineClear = FLinearColor(0.98f, 0.30f, 0.82f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Camera|Sightline") FLinearColor SightlineFramingFail = FLinearColor(0.96f, 0.66f, 0.20f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Camera|Sightline") FLinearColor SightlineOccluded = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Combat|Cover") FLinearColor CoverCovered = FLinearColor(0.24f, 0.78f, 0.72f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|Cover") FLinearColor CoverExposed = FLinearColor(1.00f, 0.32f, 0.20f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Jump Runway") FLinearColor JumpRunwayPass = FLinearColor(0.05f, 0.92f, 0.70f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Jump Runway") FLinearColor JumpRunwayFail = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Interaction|Reach") FLinearColor InteractionOpen = FLinearColor(0.96f, 0.82f, 0.24f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Interaction|Reach") FLinearColor InteractionTarget = FLinearColor(0.30f, 0.92f, 0.58f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Camera|Shoulder") FLinearColor ShoulderClear = FLinearColor(0.98f, 0.30f, 0.82f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Camera|Shoulder") FLinearColor ShoulderBlocked = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|Density") FLinearColor CombatDensityArea = FLinearColor(1.00f, 0.42f, 0.18f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|Density") FLinearColor CombatDensityPersonal = FLinearColor(1.00f, 0.68f, 0.28f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Alternate Stance") FMetricStatusColors AlternateStance;
    UPROPERTY(EditAnywhere, Category = "Movement|Mantle Vault") FLinearColor MantlePass = FLinearColor(0.05f, 0.92f, 0.70f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Mantle Vault") FLinearColor MantleFail = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Mantle Vault") FLinearColor MantleNone = FLinearColor(0.55f, 0.58f, 0.64f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Recovery") FLinearColor RecoveryClear = FLinearColor(0.05f, 0.92f, 0.70f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Movement|Recovery") FLinearColor RecoveryBlocked = FLinearColor(1.00f, 0.28f, 0.38f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|Crossfire") FLinearColor CrossfireCovered = FLinearColor(0.24f, 0.78f, 0.72f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Combat|Crossfire") FLinearColor CrossfireExposed = FLinearColor(1.00f, 0.32f, 0.20f, 1.0f);

    UPROPERTY(EditAnywhere, Category = "Benchmarks") FLinearColor PinnedBenchmark = FLinearColor(0.96f, 0.88f, 0.34f, 1.0f);
    UPROPERTY(EditAnywhere, Category = "Neutral") FLinearColor Neutral = FLinearColor(0.68f, 0.70f, 0.74f, 1.0f);
};


USTRUCT(BlueprintType)
struct FMetricLayerVisibility
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Hull Visual", ToolTip = "Show or hide the Hull world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bHullVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Hull Results", ToolTip = "Show or hide the Hull HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bHullResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Clearance Visual", ToolTip = "Show or hide the Clearance world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bClearanceVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Clearance Results", ToolTip = "Show or hide the Clearance HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bClearanceResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Overhead Visual", ToolTip = "Show or hide the Overhead world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bOverheadVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Overhead Results", ToolTip = "Show or hide the Overhead HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bOverheadResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Ground Visual", ToolTip = "Show or hide the Ground world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bGroundVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Ground Results", ToolTip = "Show or hide the Ground HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bGroundResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Fit Visual", ToolTip = "Show or hide the Fit world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bFitVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Fit Results", ToolTip = "Show or hide the Fit HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bFitResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Threshold Visual", ToolTip = "Show or hide the Threshold world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bThresholdVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Threshold Results", ToolTip = "Show or hide the Threshold HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bThresholdResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Sensors Visual", ToolTip = "Show or hide the Sensors world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bSensorsVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Sensors Results", ToolTip = "Show or hide the Sensors HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bSensorsResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Sensor Field Visual", ToolTip = "Show or hide the Sensor Field world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bSensorFieldVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Sensor Field Results", ToolTip = "Show or hide the Sensor Field HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bSensorFieldResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Ranges Visual", ToolTip = "Show or hide the Ranges world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bRangesVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Ranges Results", ToolTip = "Show or hide the Ranges HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bRangesResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Locomotion Visual", ToolTip = "Show or hide the Locomotion world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bLocomotionVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Locomotion Results", ToolTip = "Show or hide the Locomotion HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bLocomotionResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Trajectory Visual", ToolTip = "Show or hide the Trajectory world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bTrajectoryVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Trajectory Results", ToolTip = "Show or hide the Trajectory HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bTrajectoryResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Camera Probe Visual", ToolTip = "Show or hide the Camera Probe world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bCameraVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Camera Probe Results", ToolTip = "Show or hide the Camera Probe HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bCameraResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Turn Visual", ToolTip = "Show or hide the Turn world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bTurnVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Turn Results", ToolTip = "Show or hide the Turn HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bTurnResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Ledge Visual", ToolTip = "Show or hide the Ledge world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bLedgeVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Ledge Results", ToolTip = "Show or hide the Ledge HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bLedgeResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Stopping Visual", ToolTip = "Show or hide the Stopping world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bStoppingVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Stopping Results", ToolTip = "Show or hide the Stopping HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bStoppingResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Cover Visual", ToolTip = "Show or hide the Cover world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bCoverVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Cover Results", ToolTip = "Show or hide the Cover HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bCoverResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Jump Runway Visual", ToolTip = "Show or hide the Jump Runway world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bJumpRunwayVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Jump Runway Results", ToolTip = "Show or hide the Jump Runway HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bJumpRunwayResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Interaction Visual", ToolTip = "Show or hide the Interaction world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bInteractionVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Interaction Results", ToolTip = "Show or hide the Interaction HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bInteractionResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Shoulder Visual", ToolTip = "Show or hide the Shoulder world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bShoulderVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Shoulder Results", ToolTip = "Show or hide the Shoulder HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bShoulderResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Combat Density Visual", ToolTip = "Show or hide the Combat Density world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bCombatDensityVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Combat Density Results", ToolTip = "Show or hide the Combat Density HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bCombatDensityResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Alternate Stance Visual", ToolTip = "Show or hide the Alternate Stance world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bAlternateStanceVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Alternate Stance Results", ToolTip = "Show or hide the Alternate Stance HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bAlternateStanceResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Mantle Visual", ToolTip = "Show or hide the Mantle world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bMantleVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Mantle Results", ToolTip = "Show or hide the Mantle HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bMantleResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Recovery Visual", ToolTip = "Show or hide the Recovery world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bRecoveryVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Recovery Results", ToolTip = "Show or hide the Recovery HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bRecoveryResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Crossfire Visual", ToolTip = "Show or hide the Crossfire world-space visualization when this visibility mask is applied. This changes presentation only, not analysis.")) bool bCrossfireVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Crossfire Results", ToolTip = "Show or hide the Crossfire HUD result row when this visibility mask is applied. This changes presentation only, not analysis.")) bool bCrossfireResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Camera Sightline Visual", ToolTip = "Show or hide the Camera Sightline world-space visualization when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bSightlineVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility", meta = (DisplayName = "Camera Sightline Results", ToolTip = "Show or hide the Camera Sightline HUD result row when this visibility mask is applied. This changes presentation only, not analysis."))
    bool bSightlineResults = true;

    void IntersectWith(const FMetricLayerVisibility& Other)
    {
#define METRIC_AND_VISIBILITY(Name) Name = Name && Other.Name
        METRIC_AND_VISIBILITY(bHullVisual); METRIC_AND_VISIBILITY(bHullResults);
        METRIC_AND_VISIBILITY(bClearanceVisual); METRIC_AND_VISIBILITY(bClearanceResults);
        METRIC_AND_VISIBILITY(bOverheadVisual); METRIC_AND_VISIBILITY(bOverheadResults);
        METRIC_AND_VISIBILITY(bGroundVisual); METRIC_AND_VISIBILITY(bGroundResults);
        METRIC_AND_VISIBILITY(bFitVisual); METRIC_AND_VISIBILITY(bFitResults);
        METRIC_AND_VISIBILITY(bThresholdVisual); METRIC_AND_VISIBILITY(bThresholdResults);
        METRIC_AND_VISIBILITY(bSensorsVisual); METRIC_AND_VISIBILITY(bSensorsResults);
        METRIC_AND_VISIBILITY(bSensorFieldVisual); METRIC_AND_VISIBILITY(bSensorFieldResults);
        METRIC_AND_VISIBILITY(bRangesVisual); METRIC_AND_VISIBILITY(bRangesResults);
        METRIC_AND_VISIBILITY(bLocomotionVisual); METRIC_AND_VISIBILITY(bLocomotionResults);
        METRIC_AND_VISIBILITY(bTrajectoryVisual); METRIC_AND_VISIBILITY(bTrajectoryResults);
        METRIC_AND_VISIBILITY(bCameraVisual); METRIC_AND_VISIBILITY(bCameraResults);
        METRIC_AND_VISIBILITY(bSightlineVisual); METRIC_AND_VISIBILITY(bSightlineResults);
        METRIC_AND_VISIBILITY(bTurnVisual); METRIC_AND_VISIBILITY(bTurnResults);
        METRIC_AND_VISIBILITY(bLedgeVisual); METRIC_AND_VISIBILITY(bLedgeResults);
        METRIC_AND_VISIBILITY(bStoppingVisual); METRIC_AND_VISIBILITY(bStoppingResults);
        METRIC_AND_VISIBILITY(bCoverVisual); METRIC_AND_VISIBILITY(bCoverResults);
        METRIC_AND_VISIBILITY(bJumpRunwayVisual); METRIC_AND_VISIBILITY(bJumpRunwayResults);
        METRIC_AND_VISIBILITY(bInteractionVisual); METRIC_AND_VISIBILITY(bInteractionResults);
        METRIC_AND_VISIBILITY(bShoulderVisual); METRIC_AND_VISIBILITY(bShoulderResults);
        METRIC_AND_VISIBILITY(bCombatDensityVisual); METRIC_AND_VISIBILITY(bCombatDensityResults);
        METRIC_AND_VISIBILITY(bAlternateStanceVisual); METRIC_AND_VISIBILITY(bAlternateStanceResults);
        METRIC_AND_VISIBILITY(bMantleVisual); METRIC_AND_VISIBILITY(bMantleResults);
        METRIC_AND_VISIBILITY(bRecoveryVisual); METRIC_AND_VISIBILITY(bRecoveryResults);
        METRIC_AND_VISIBILITY(bCrossfireVisual); METRIC_AND_VISIBILITY(bCrossfireResults);
#undef METRIC_AND_VISIBILITY
    }

};

struct FMetricPinnedBenchmark
{
    FVector WorldLocation = FVector::ZeroVector;
    FVector SurfaceNormal = FVector::UpVector;
    FVector AnalysisUp = FVector::UpVector;
    EMetricAnalysisFrameMode AnalysisFrameMode = EMetricAnalysisFrameMode::World;
    EMetricFrameSamplingPolicy FrameSampling = EMetricFrameSamplingPolicy::FixedAtOrigin;
    float AnalysisGravityMagnitude = 0.0f;
    FVector Forward = FVector::ForwardVector;
    FMetricHullSettings Hull;
    FMetricVisualizationTheme VisualTheme;
    FText Label;
    FText SourceProfileLabel;
    FString CapturedResults;
    bool bHasVisualSnapshot = false;
    FMetricLayerVisibility Visibility;
    FMetricClearanceSettings ClearanceSettings;
    FMetricOverheadClearanceSettings OverheadClearanceSettings;
    FMetricSensorSettings Sensors;
    FMetricSensorFieldSettings SensorFieldSettings;
    FMetricRangeSettings Ranges;
    FMetricLocomotionSettings Locomotion;
    FMetricTrajectorySettings TrajectorySettings;
    FMetricCameraProbeSettings CameraProbeSettings;
    FMetricCameraSightlineSettings CameraSightlineSettings;
    FMetricGroundValidationSettings GroundValidationSettings;
    FMetricFitAnalysisSettings FitAnalysisSettings;
    FMetricThresholdAnalysisSettings ThresholdAnalysisSettings;
    FMetricTurnClearanceSettings TurnClearanceSettings;
    FMetricLedgeAnalysisSettings LedgeAnalysisSettings;
    FMetricStoppingDistanceSettings StoppingDistanceSettings;
    FMetricCoverExposureSettings CoverExposureSettings;
    FMetricJumpRunwaySettings JumpRunwaySettings;
    FMetricInteractionReachSettings InteractionReachSettings;
    FMetricShoulderClearanceSettings ShoulderClearanceSettings;
    FMetricCombatDensitySettings CombatDensitySettings;
    FMetricAlternateStanceSettings AlternateStanceSettings;
    FMetricMantleVaultSettings MantleVaultSettings;
    FMetricRecoverySpaceSettings RecoverySpaceSettings;
    FMetricCrossfireSettings CrossfireSettings;
};


UENUM()
enum class EMetricInspectionTarget : uint8
{
    Auto UMETA(DisplayName = "Auto (Most Relevant)"),
    Clearance UMETA(DisplayName = "Clearance"),
    GroundValidation UMETA(DisplayName = "Ground Validation"),
    FitAnalysis UMETA(DisplayName = "Fit Analysis"),
    ThresholdAnalysis UMETA(DisplayName = "Threshold Analysis"),
    LedgeAnalysis UMETA(DisplayName = "Ledge Analysis"),
    StoppingDistance UMETA(DisplayName = "Stopping Distance"),
    MantleVault UMETA(DisplayName = "Mantle / Vault"),
    RecoverySpace UMETA(DisplayName = "Recovery Space"),
    CameraProbe UMETA(DisplayName = "Camera Probe"),
    CameraSightline UMETA(DisplayName = "Camera Sightline"),
    Trajectory UMETA(DisplayName = "Trajectory")
};

enum class EMetricQueryEvidenceKind : uint8
{
    Trace,
    Sweep,
    Sample,
    Contact,
    Threshold
};

enum class EMetricQueryEvidenceStatus : uint8
{
    Info,
    Pass,
    Warning,
    Fail
};

struct FMetricQueryEvidence
{
    EMetricInspectionTarget Target = EMetricInspectionTarget::Clearance;
    EMetricQueryEvidenceKind Kind = EMetricQueryEvidenceKind::Trace;
    EMetricQueryEvidenceStatus Status = EMetricQueryEvidenceStatus::Info;
    FVector StartOffset = FVector::ZeroVector;
    FVector EndOffset = FVector::ZeroVector;
    FVector ImpactOffset = FVector::ZeroVector;
    FVector ImpactNormal = FVector::UpVector;
    bool bHasImpact = false;
    FString Label;
};

struct FMetricLiveState
{
    bool bHasHit = false;
    bool bShowHull = true;
    bool bShowForward = true;
    FVector Location = FVector::ZeroVector;
    FVector SurfaceNormal = FVector::UpVector;
    FVector Forward = FVector::ForwardVector;
    FVector AnalysisUp = FVector::UpVector;
    EMetricAnalysisFrameMode AnalysisFrameMode = EMetricAnalysisFrameMode::World;
    EMetricFrameSamplingPolicy FrameSampling = EMetricFrameSamplingPolicy::FixedAtOrigin;
    float AnalysisGravityMagnitude = 0.0f;
    TWeakObjectPtr<AActor> AnalysisContextActor;
    FVector AnalysisForward = FVector::ForwardVector;
    FVector AnalysisRight = FVector::RightVector;
    FVector ViewerLocation = FVector::ZeroVector;
    FMetricHullSettings Hull;
    FMetricVisualizationTheme VisualTheme;
    FMetricLayerVisibility ProfileVisibility;
    FMetricClearanceSettings ClearanceSettings;
    FMetricClearanceResult Clearance;
    FMetricOverheadClearanceSettings OverheadClearanceSettings;
    FMetricOverheadClearanceResult OverheadClearance;
    FMetricSensorSettings Sensors;
    FMetricSensorFieldSettings SensorFieldSettings;
    FMetricSensorFieldResult SensorField;
    FMetricRangeSettings Ranges;
    FMetricLocomotionSettings Locomotion;
    FMetricTrajectorySettings TrajectorySettings;
    FMetricTrajectoryResult Trajectory;
    FMetricCameraProbeSettings CameraProbeSettings;
    FMetricCameraProbeResult CameraProbe;
    FMetricCameraSightlineSettings CameraSightlineSettings;
    FMetricCameraSightlineResult CameraSightline;
    FMetricGroundValidationSettings GroundValidationSettings;
    FMetricGroundValidationResult GroundValidation;
    FMetricFitAnalysisSettings FitAnalysisSettings;
    FMetricFitAnalysisResult FitAnalysis;
    FMetricThresholdAnalysisSettings ThresholdAnalysisSettings;
    FMetricThresholdAnalysisResult ThresholdAnalysis;
    FMetricTurnClearanceSettings TurnClearanceSettings;
    FMetricTurnClearanceResult TurnClearance;
    FMetricLedgeAnalysisSettings LedgeAnalysisSettings;
    FMetricLedgeAnalysisResult LedgeAnalysis;
    FMetricStoppingDistanceSettings StoppingDistanceSettings;
    FMetricStoppingDistanceResult StoppingDistance;
    FMetricCoverExposureSettings CoverExposureSettings;
    FMetricCoverExposureResult CoverExposure;
    FMetricJumpRunwaySettings JumpRunwaySettings;
    FMetricJumpRunwayResult JumpRunway;
    FMetricInteractionReachSettings InteractionReachSettings;
    FMetricInteractionReachResult InteractionReach;
    FMetricShoulderClearanceSettings ShoulderClearanceSettings;
    FMetricShoulderClearanceResult ShoulderClearance;
    FMetricCombatDensitySettings CombatDensitySettings;
    FMetricCombatDensityResult CombatDensity;
    FMetricAlternateStanceSettings AlternateStanceSettings;
    FMetricAlternateStanceResult AlternateStance;
    FMetricMantleVaultSettings MantleVaultSettings;
    FMetricMantleVaultResult MantleVault;
    FMetricRecoverySpaceSettings RecoverySpaceSettings;
    FMetricRecoverySpaceResult RecoverySpace;
    FMetricCrossfireSettings CrossfireSettings;
    FMetricCrossfireResult Crossfire;
    FText Label;
    EMetricCycleEntryType EntryType = EMetricCycleEntryType::BuiltInMetric;
    FSoftObjectPath PreviewMesh;
    FTransform PreviewRelativeTransform = FTransform::Identity;
    TArray<FMetricPinnedBenchmark> PinnedBenchmarks;
    TArray<FMetricQueryEvidence> QueryEvidence;
    int32 SelectedPinnedBenchmarkIndex = INDEX_NONE;

    void ApplyVisualVisibilityMask(const FMetricLayerVisibility& Visibility)
    {
        bShowHull = bShowHull && Visibility.bHullVisual;
        ClearanceSettings.bShowClearanceProbe = ClearanceSettings.bShowClearanceProbe && Visibility.bClearanceVisual;
        OverheadClearanceSettings.bShowOverheadClearance = OverheadClearanceSettings.bShowOverheadClearance && Visibility.bOverheadVisual;
        Sensors.bShowSensorHeights = Sensors.bShowSensorHeights && Visibility.bSensorsVisual;
        SensorFieldSettings.bShowSensorField = SensorFieldSettings.bShowSensorField && Visibility.bSensorFieldVisual;
        Ranges.bShowEngagementRanges = Ranges.bShowEngagementRanges && Visibility.bRangesVisual;
        Locomotion.bShowLocomotionBands = Locomotion.bShowLocomotionBands && Visibility.bLocomotionVisual;
        TrajectorySettings.bShowTrajectory = TrajectorySettings.bShowTrajectory && Visibility.bTrajectoryVisual;
        CameraProbeSettings.bShowCameraProbe = CameraProbeSettings.bShowCameraProbe && Visibility.bCameraVisual;
        CameraSightlineSettings.bShowCameraSightline = CameraSightlineSettings.bShowCameraSightline && Visibility.bSightlineVisual;
        GroundValidationSettings.bShowGroundValidation = GroundValidationSettings.bShowGroundValidation && Visibility.bGroundVisual;
        FitAnalysisSettings.bShowFitAnalysis = FitAnalysisSettings.bShowFitAnalysis && Visibility.bFitVisual;
        ThresholdAnalysisSettings.bShowThresholdAnalysis = ThresholdAnalysisSettings.bShowThresholdAnalysis && Visibility.bThresholdVisual;
        TurnClearanceSettings.bShowTurnClearance = TurnClearanceSettings.bShowTurnClearance && Visibility.bTurnVisual;
        LedgeAnalysisSettings.bShowLedgeAnalysis = LedgeAnalysisSettings.bShowLedgeAnalysis && Visibility.bLedgeVisual;
        StoppingDistanceSettings.bShowStoppingDistance = StoppingDistanceSettings.bShowStoppingDistance && Visibility.bStoppingVisual;
        CoverExposureSettings.bShowCoverExposure = CoverExposureSettings.bShowCoverExposure && Visibility.bCoverVisual;
        JumpRunwaySettings.bShowJumpRunway = JumpRunwaySettings.bShowJumpRunway && Visibility.bJumpRunwayVisual;
        InteractionReachSettings.bShowInteractionReach = InteractionReachSettings.bShowInteractionReach && Visibility.bInteractionVisual;
        ShoulderClearanceSettings.bShowShoulderClearance = ShoulderClearanceSettings.bShowShoulderClearance && Visibility.bShoulderVisual;
        CombatDensitySettings.bShowCombatDensity = CombatDensitySettings.bShowCombatDensity && Visibility.bCombatDensityVisual;
        AlternateStanceSettings.bShowAlternateStance = AlternateStanceSettings.bShowAlternateStance && Visibility.bAlternateStanceVisual;
        MantleVaultSettings.bShowMantleVault = MantleVaultSettings.bShowMantleVault && Visibility.bMantleVisual;
        RecoverySpaceSettings.bShowRecoverySpace = RecoverySpaceSettings.bShowRecoverySpace && Visibility.bRecoveryVisual;
        CrossfireSettings.bShowCrossfire = CrossfireSettings.bShowCrossfire && Visibility.bCrossfireVisual;
    }
};
