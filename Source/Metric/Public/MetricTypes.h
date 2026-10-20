#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "MetricTypes.generated.h"

class UMetricPreset;
class UStaticMesh;

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
    MetricPreset,
    NativePreviewMesh
};

USTRUCT(BlueprintType)
struct FMetricHullSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Hull")
    EMetricHullType HullType = EMetricHullType::Capsule;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (ClampMin = "1.0", Units = "cm"))
    float Height = 180.0f;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (ClampMin = "1.0", Units = "cm"))
    float Width = 88.0f;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (ClampMin = "1.0", Units = "cm"))
    float Length = 88.0f;

    UPROPERTY(EditAnywhere, Category = "Hull", meta = (Units = "cm"))
    FVector LocalOffset = FVector::ZeroVector;
};


USTRUCT(BlueprintType)
struct FMetricClearanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Clearance")
    bool bShowClearanceProbe = true;

    UPROPERTY(EditAnywhere, Category = "Clearance", meta = (EditCondition = "bShowClearanceProbe", ClampMin = "10.0", Units = "cm"))
    float ProbeDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Clearance", meta = (EditCondition = "bShowClearanceProbe", ClampMin = "0.0", Units = "cm"))
    float WarningMargin = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Clearance", meta = (EditCondition = "bShowClearanceProbe"))
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

    UPROPERTY(EditAnywhere, Category = "Sensors")
    bool bShowSensorHeights = true;

    UPROPERTY(EditAnywhere, Category = "Sensors", meta = (EditCondition = "bShowSensorHeights", ClampMin = "0.0", Units = "cm"))
    float PrimarySensorZOffset = 160.0f;

    UPROPERTY(EditAnywhere, Category = "Sensors", meta = (EditCondition = "bShowSensorHeights", ClampMin = "0.0", Units = "cm"))
    float LowProfileSensorZOffset = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Sensors", meta = (EditCondition = "bShowSensorHeights", ClampMin = "5.0", Units = "cm"))
    float MarkerRadius = 24.0f;
};

USTRUCT(BlueprintType)
struct FMetricRangeSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges")
    bool bShowEngagementRanges = false;

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges", meta = (EditCondition = "bShowEngagementRanges", ClampMin = "0.0", Units = "cm"))
    float ShortRange = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges", meta = (EditCondition = "bShowEngagementRanges", ClampMin = "0.0", Units = "cm"))
    float MediumRange = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Engagement Ranges", meta = (EditCondition = "bShowEngagementRanges", ClampMin = "0.0", Units = "cm"))
    float LongRange = 3000.0f;
};

USTRUCT(BlueprintType)
struct FMetricLocomotionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands")
    bool bShowLocomotionBands = false;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (EditCondition = "bShowLocomotionBands", ClampMin = "1.0", Units = "cm/s"))
    float Speed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (EditCondition = "bShowLocomotionBands", ClampMin = "0.1", Units = "s"))
    float NearTime = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (EditCondition = "bShowLocomotionBands", ClampMin = "0.1", Units = "s"))
    float MidTime = 3.0f;

    UPROPERTY(EditAnywhere, Category = "Locomotion Bands", meta = (EditCondition = "bShowLocomotionBands", ClampMin = "0.1", Units = "s"))
    float FarTime = 5.0f;
};


USTRUCT(BlueprintType)
struct FMetricTrajectorySettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Trajectory")
    bool bShowTrajectory = false;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "1.0", Units = "cm/s"))
    float HorizontalSpeed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "1.0", Units = "cm/s"))
    float LaunchVerticalSpeed = 550.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "0.05", ClampMax = "5.0"))
    float GravityScale = 1.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "0.25", ClampMax = "10.0", Units = "s"))
    float MaxSimulationTime = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "0.01", ClampMax = "0.2", Units = "s"))
    float SampleInterval = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "0.0", ClampMax = "89.0", Units = "deg"))
    float MaxLandingSlope = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory"))
    TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
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
    TArray<FVector> SampleOffsets;
};


USTRUCT(BlueprintType)
struct FMetricCameraProbeSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Camera Probe")
    bool bShowCameraProbe = false;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (EditCondition = "bShowCameraProbe", ClampMin = "0.0", Units = "cm"))
    float PivotZOffset = 160.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (EditCondition = "bShowCameraProbe", ClampMin = "10.0", Units = "cm"))
    float TargetDistance = 350.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (EditCondition = "bShowCameraProbe", Units = "cm"))
    float VerticalOffset = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (EditCondition = "bShowCameraProbe", Units = "cm"))
    float ShoulderOffset = 0.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (EditCondition = "bShowCameraProbe", ClampMin = "1.0", Units = "cm"))
    float ProbeRadius = 12.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (EditCondition = "bShowCameraProbe", ClampMin = "0.0", Units = "cm"))
    float MinDistance = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Probe", meta = (EditCondition = "bShowCameraProbe"))
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

    UPROPERTY(EditAnywhere, Category = "Camera Sightline")
    bool bShowCameraSightline = false;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (EditCondition = "bShowCameraSightline", ClampMin = "0.0", Units = "cm"))
    float SubjectZOffset = 140.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (EditCondition = "bShowCameraSightline", ClampMin = "1.0", ClampMax = "80.0", Units = "deg"))
    float FramingHalfAngle = 25.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (EditCondition = "bShowCameraSightline", ClampMin = "0.0", Units = "cm"))
    float TraceRadius = 4.0f;

    UPROPERTY(EditAnywhere, Category = "Camera Sightline", meta = (EditCondition = "bShowCameraSightline"))
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
struct FMetricGroundValidationSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Ground Validation")
    bool bShowGroundValidation = false;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "50.0", Units = "cm"))
    float ForwardDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "10.0", Units = "cm"))
    float SampleSpacing = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "0.0", Units = "cm"))
    float LateralCoverage = 88.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "0.0", ClampMax = "89.0", Units = "deg"))
    float MaxWalkableSlope = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "0.0", Units = "cm"))
    float MaxStepUp = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "0.0", Units = "cm"))
    float MaxStepDown = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "10.0", Units = "cm"))
    float TraceHeight = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation", ClampMin = "10.0", Units = "cm"))
    float TraceDepth = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Ground Validation", meta = (EditCondition = "bShowGroundValidation"))
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

    UPROPERTY(EditAnywhere, Category = "Fit Analysis")
    bool bShowFitAnalysis = false;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis", ClampMin = "0.0", Units = "cm"))
    float ForwardDistance = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis", ClampMin = "10.0", Units = "cm"))
    float SampleSpacing = 50.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis", ClampMin = "10.0", Units = "cm"))
    float MaxSideProbeDistance = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis", ClampMin = "10.0", Units = "cm"))
    float MaxCeilingProbeHeight = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis", ClampMin = "0.0", Units = "cm"))
    float WarningMargin = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis", ClampMin = "10.0", Units = "cm"))
    float FloorTraceHeight = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis", ClampMin = "10.0", Units = "cm"))
    float FloorTraceDepth = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Fit Analysis", meta = (EditCondition = "bShowFitAnalysis"))
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
    bool bNearThreshold = false;
    float AvailableWidth = 0.0f;
    float AvailableHeight = 0.0f;
    float WidthDeficit = 0.0f;
    float HeightDeficit = 0.0f;
};

struct FMetricFitAnalysisResult
{
    bool bVisible = false;
    bool bPass = true;
    bool bNearThreshold = false;
    int32 TotalSamples = 0;
    int32 FailedSamples = 0;
    int32 InvalidSamples = 0;
    float RequiredWidth = 0.0f;
    float RequiredHeight = 0.0f;
    float MinimumAvailableWidth = 0.0f;
    float MinimumAvailableHeight = 0.0f;
    float MaxWidthDeficit = 0.0f;
    float MaxHeightDeficit = 0.0f;
    TArray<FMetricFitSample> Samples;
};

USTRUCT(BlueprintType)
struct FMetricRepresentationSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Representation")
    bool bShowStaticMesh = false;

    UPROPERTY(EditAnywhere, Category = "Representation", meta = (EditCondition = "bShowStaticMesh"))
    TSoftObjectPtr<UStaticMesh> StaticMesh;

    UPROPERTY(EditAnywhere, Category = "Representation", meta = (EditCondition = "bShowStaticMesh"))
    FTransform RelativeTransform = FTransform::Identity;
};

struct FMetricCycleEntry
{
    EMetricCycleEntryType Type = EMetricCycleEntryType::BuiltInMetric;
    FText DisplayName;
    TSoftObjectPtr<UMetricPreset> Preset;
    FSoftObjectPath NativePreviewMesh;

    bool IsMetricEntry() const
    {
        return Type == EMetricCycleEntryType::BuiltInMetric || Type == EMetricCycleEntryType::MetricPreset;
    }
};


struct FMetricPinnedBenchmark
{
    FVector WorldLocation = FVector::ZeroVector;
    FVector Forward = FVector::ForwardVector;
    FMetricHullSettings Hull;
    FText Label;
};

struct FMetricLiveState
{
    bool bHasHit = false;
    bool bShowHull = true;
    bool bShowForward = true;
    FVector Location = FVector::ZeroVector;
    FVector SurfaceNormal = FVector::UpVector;
    FVector Forward = FVector::ForwardVector;
    FVector ViewerLocation = FVector::ZeroVector;
    FMetricHullSettings Hull;
    FMetricClearanceSettings ClearanceSettings;
    FMetricClearanceResult Clearance;
    FMetricSensorSettings Sensors;
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
    FText Label;
    EMetricCycleEntryType EntryType = EMetricCycleEntryType::BuiltInMetric;
    FSoftObjectPath PreviewMesh;
    FTransform PreviewRelativeTransform = FTransform::Identity;
    TArray<FMetricPinnedBenchmark> PinnedBenchmarks;
};
