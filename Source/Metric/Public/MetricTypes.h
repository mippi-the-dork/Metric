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
struct FMetricSensorFieldSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Sensor Field")
    bool bShowSensorField = false;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (EditCondition = "bShowSensorField", ClampMin = "10.0", Units = "cm"))
    float Range = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (EditCondition = "bShowSensorField", ClampMin = "1.0", ClampMax = "89.0", Units = "deg"))
    float HorizontalHalfAngle = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (EditCondition = "bShowSensorField", ClampMin = "1.0", ClampMax = "89.0", Units = "deg"))
    float VerticalHalfAngle = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (EditCondition = "bShowSensorField", ClampMin = "3", ClampMax = "31"))
    int32 HorizontalRayCount = 9;

    UPROPERTY(EditAnywhere, Category = "Sensor Field", meta = (EditCondition = "bShowSensorField"))
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
    bool bShowTrajectoryEnvelope = true;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory && bShowTrajectoryEnvelope", ClampMin = "1", ClampMax = "12"))
    int32 EnvelopeSampleStride = 3;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "0.0", Units = "cm"))
    float ApexProbeDistance = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Trajectory", meta = (EditCondition = "bShowTrajectory", ClampMin = "0.0", Units = "cm"))
    float ApexWarningMargin = 50.0f;

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
    int32 ApexSampleIndex = INDEX_NONE;
    FVector ApexOffset = FVector::ZeroVector;
    FVector ApexClearanceImpactOffset = FVector::ZeroVector;
    float ApexHeight = 0.0f;
    float ApexHeadroom = 0.0f;
    bool bApexHasObstacle = false;
    bool bApexTight = false;
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
struct FMetricOverheadClearanceSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance")
    bool bShowOverheadClearance = false;

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance", meta = (EditCondition = "bShowOverheadClearance", ClampMin = "50.0", Units = "cm"))
    float MaxProbeHeight = 350.0f;

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance", meta = (EditCondition = "bShowOverheadClearance", ClampMin = "0.0", Units = "cm"))
    float WarningMargin = 30.0f;

    UPROPERTY(EditAnywhere, Category = "Overhead Clearance", meta = (EditCondition = "bShowOverheadClearance"))
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
struct FMetricThresholdAnalysisSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis")
    bool bShowThresholdAnalysis = false;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "10.0", Units = "cm"))
    float ProbeDepth = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "10.0", Units = "cm"))
    float MaxSideProbeDistance = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "10.0", Units = "cm"))
    float MaxCeilingProbeHeight = 300.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "0.0", Units = "cm"))
    float WarningMargin = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "0.0", Units = "cm"))
    float MaxStepUp = 45.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "0.0", Units = "cm"))
    float MaxStepDown = 60.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "10.0", Units = "cm"))
    float FloorTraceHeight = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis", ClampMin = "10.0", Units = "cm"))
    float FloorTraceDepth = 200.0f;

    UPROPERTY(EditAnywhere, Category = "Threshold Analysis", meta = (EditCondition = "bShowThresholdAnalysis"))
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
    bool bStepUpFail = false;
    bool bStepDownFail = false;
    bool bNearThreshold = false;
    float AvailableWidth = 0.0f;
    float AvailableHeight = 0.0f;
    float WidthDeficit = 0.0f;
    float HeightDeficit = 0.0f;
    float StepDelta = 0.0f;
};

struct FMetricThresholdAnalysisResult
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

    UPROPERTY(EditAnywhere, Category = "Turn Clearance")
    bool bShowTurnClearance = false;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (EditCondition = "bShowTurnClearance", ClampMin = "10.0", Units = "cm"))
    float TurnRadius = 250.0f;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (EditCondition = "bShowTurnClearance", ClampMin = "10.0", ClampMax = "180.0", Units = "deg"))
    float TurnAngle = 90.0f;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (EditCondition = "bShowTurnClearance", ClampMin = "2.0", ClampMax = "30.0", Units = "deg"))
    float SampleAngle = 7.5f;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (EditCondition = "bShowTurnClearance"))
    EMetricTurnDirection Direction = EMetricTurnDirection::Both;

    UPROPERTY(EditAnywhere, Category = "Turn Clearance", meta = (EditCondition = "bShowTurnClearance"))
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

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis")
    bool bShowLedgeAnalysis = false;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (EditCondition = "bShowLedgeAnalysis", ClampMin = "10.0", Units = "cm"))
    float ForwardDistance = 150.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (EditCondition = "bShowLedgeAnalysis", ClampMin = "5.0", Units = "cm"))
    float SampleSpacing = 15.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (EditCondition = "bShowLedgeAnalysis", ClampMin = "0.0", Units = "cm"))
    float MaxSafeDrop = 120.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (EditCondition = "bShowLedgeAnalysis", ClampMin = "10.0", Units = "cm"))
    float MaxProbeDepth = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (EditCondition = "bShowLedgeAnalysis", ClampMin = "10.0", Units = "cm"))
    float TraceHeight = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Ledge Analysis", meta = (EditCondition = "bShowLedgeAnalysis"))
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

    UPROPERTY(EditAnywhere, Category = "Stopping Distance")
    bool bShowStoppingDistance = false;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (EditCondition = "bShowStoppingDistance", ClampMin = "1.0", Units = "cm/s"))
    float Speed = 600.0f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (EditCondition = "bShowStoppingDistance", ClampMin = "1.0", Units = "cm/s^2"))
    float Deceleration = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (EditCondition = "bShowStoppingDistance", ClampMin = "0.0", Units = "s"))
    float ReactionTime = 0.15f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (EditCondition = "bShowStoppingDistance", ClampMin = "0.0", Units = "cm"))
    float WarningMargin = 75.0f;

    UPROPERTY(EditAnywhere, Category = "Stopping Distance", meta = (EditCondition = "bShowStoppingDistance"))
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
    UPROPERTY(EditAnywhere, Category="Cover / Exposure") bool bShowCoverExposure = false;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(EditCondition="bShowCoverExposure", ClampMin="50.0", Units="cm")) float ThreatDistance = 1200.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(EditCondition="bShowCoverExposure", ClampMin="0.0", Units="cm")) float ThreatHeight = 160.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(EditCondition="bShowCoverExposure", ClampMin="0.0", Units="cm")) float LowSampleHeight = 60.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(EditCondition="bShowCoverExposure", ClampMin="0.0", Units="cm")) float TorsoSampleHeight = 120.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(EditCondition="bShowCoverExposure", ClampMin="0.0", Units="cm")) float HeadSampleHeight = 170.0f;
    UPROPERTY(EditAnywhere, Category="Cover / Exposure", meta=(EditCondition="bShowCoverExposure")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
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
    UPROPERTY(EditAnywhere, Category="Jump Runway") bool bShowJumpRunway = false;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(EditCondition="bShowJumpRunway", ClampMin="0.0", Units="cm")) float TakeoffRunway = 150.0f;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(EditCondition="bShowJumpRunway", ClampMin="0.0", Units="cm")) float LandingRunway = 180.0f;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(EditCondition="bShowJumpRunway", ClampMin="10.0", Units="cm")) float GroundProbeDepth = 150.0f;
    UPROPERTY(EditAnywhere, Category="Jump Runway", meta=(EditCondition="bShowJumpRunway")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
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
    UPROPERTY(EditAnywhere, Category="Interaction Reach") bool bShowInteractionReach = false;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(EditCondition="bShowInteractionReach", ClampMin="10.0", Units="cm")) float ReachDistance = 180.0f;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(EditCondition="bShowInteractionReach", ClampMin="0.0", Units="cm")) float ReachHeight = 130.0f;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(EditCondition="bShowInteractionReach", ClampMin="1.0", Units="cm")) float ProbeRadius = 18.0f;
    UPROPERTY(EditAnywhere, Category="Interaction Reach", meta=(EditCondition="bShowInteractionReach")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricInteractionReachResult
{
    bool bVisible=false; bool bHasTarget=false; float TargetDistance=0.0f; FVector OriginOffset=FVector::ZeroVector; FVector EndOffset=FVector::ZeroVector; FVector HitOffset=FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FMetricShoulderClearanceSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance") bool bShowShoulderClearance = false;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(EditCondition="bShowShoulderClearance", ClampMin="0.0", Units="cm")) float PivotHeight = 150.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(EditCondition="bShowShoulderClearance", ClampMin="0.0", Units="cm")) float ShoulderOffset = 55.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(EditCondition="bShowShoulderClearance", ClampMin="10.0", Units="cm")) float CameraDistance = 300.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(EditCondition="bShowShoulderClearance", ClampMin="1.0", Units="cm")) float ProbeRadius = 12.0f;
    UPROPERTY(EditAnywhere, Category="Shoulder Camera Clearance", meta=(EditCondition="bShowShoulderClearance")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
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
    UPROPERTY(EditAnywhere, Category="Combat Spacing Density") bool bShowCombatDensity = false;
    UPROPERTY(EditAnywhere, Category="Combat Spacing Density", meta=(EditCondition="bShowCombatDensity", ClampMin="50.0", Units="cm")) float AreaRadius = 600.0f;
    UPROPERTY(EditAnywhere, Category="Combat Spacing Density", meta=(EditCondition="bShowCombatDensity", ClampMin="0.0", Units="cm")) float PersonalSpacePadding = 50.0f;
};
struct FMetricCombatDensityResult
{
    bool bVisible=false; float PersonalDiameter=0.0f; int32 ApproxCapacity=0; float AreaRadius=0.0f;
};

USTRUCT(BlueprintType)
struct FMetricAlternateStanceSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit") bool bShowAlternateStance = false;
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit", meta=(EditCondition="bShowAlternateStance", ClampMin="10.0", Units="cm")) float AlternateHeight = 110.0f;
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit", meta=(EditCondition="bShowAlternateStance", ClampMin="0.0", Units="cm")) float WarningMargin = 15.0f;
    UPROPERTY(EditAnywhere, Category="Alternate Stance Fit", meta=(EditCondition="bShowAlternateStance")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricAlternateStanceResult
{
    bool bVisible=false; bool bStandingFits=false; bool bAlternateFits=false; float AvailableHeight=0.0f;
};

USTRUCT(BlueprintType)
struct FMetricMantleVaultSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope") bool bShowMantleVault = false;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(EditCondition="bShowMantleVault", ClampMin="10.0", Units="cm")) float ForwardReach = 120.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(EditCondition="bShowMantleVault", ClampMin="10.0", Units="cm")) float MinObstacleHeight = 45.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(EditCondition="bShowMantleVault", ClampMin="10.0", Units="cm")) float MaxObstacleHeight = 180.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(EditCondition="bShowMantleVault", ClampMin="10.0", Units="cm")) float MaxObstacleDepth = 100.0f;
    UPROPERTY(EditAnywhere, Category="Mantle / Vault Envelope", meta=(EditCondition="bShowMantleVault")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
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
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space") bool bShowRecoverySpace = false;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(EditCondition="bShowRecoverySpace", ClampMin="10.0", Units="cm")) float ForwardDistance = 250.0f;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(EditCondition="bShowRecoverySpace", ClampMin="10.0", Units="cm")) float BackwardDistance = 180.0f;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(EditCondition="bShowRecoverySpace", ClampMin="10.0", Units="cm")) float SideDistance = 220.0f;
    UPROPERTY(EditAnywhere, Category="Recovery / Dodge Space", meta=(EditCondition="bShowRecoverySpace")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
};
struct FMetricRecoverySpaceResult
{
    bool bVisible=false; bool bForwardClear=false; bool bBackwardClear=false; bool bLeftClear=false; bool bRightClear=false;
    float ForwardClear=0.0f; float BackwardClear=0.0f; float LeftClear=0.0f; float RightClear=0.0f;
};

USTRUCT(BlueprintType)
struct FMetricCrossfireSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure") bool bShowCrossfire = false;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(EditCondition="bShowCrossfire", ClampMin="100.0", Units="cm")) float ThreatDistance = 1200.0f;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(EditCondition="bShowCrossfire", ClampMin="4", ClampMax="16")) int32 DirectionCount = 8;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(EditCondition="bShowCrossfire", ClampMin="0.0", Units="cm")) float SampleHeight = 130.0f;
    UPROPERTY(EditAnywhere, Category="Crossfire Exposure", meta=(EditCondition="bShowCrossfire")) TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;
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



USTRUCT(BlueprintType)
struct FMetricLayerVisibility
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bHullVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bHullResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bClearanceVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bClearanceResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bOverheadVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bOverheadResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bGroundVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bGroundResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bFitVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bFitResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bThresholdVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bThresholdResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bSensorsVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bSensorsResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bSensorFieldVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bSensorFieldResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bRangesVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bRangesResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bLocomotionVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bLocomotionResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bTrajectoryVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bTrajectoryResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bCameraVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bCameraResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bTurnVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bTurnResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bLedgeVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bLedgeResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bStoppingVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bStoppingResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bCoverVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bCoverResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bJumpRunwayVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bJumpRunwayResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bInteractionVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bInteractionResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bShoulderVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bShoulderResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bCombatDensityVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bCombatDensityResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bAlternateStanceVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bAlternateStanceResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bMantleVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bMantleResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bRecoveryVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bRecoveryResults = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bCrossfireVisual = true;
    UPROPERTY(EditAnywhere, Category = "Metric Visibility") bool bCrossfireResults = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bSightlineVisual = true;

    UPROPERTY(EditAnywhere, Category = "Metric Visibility")
    bool bSightlineResults = true;

};

struct FMetricPinnedBenchmark
{
    FVector WorldLocation = FVector::ZeroVector;
    FVector SurfaceNormal = FVector::UpVector;
    FVector Forward = FVector::ForwardVector;
    FMetricHullSettings Hull;
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
    int32 SelectedPinnedBenchmarkIndex = INDEX_NONE;
};
