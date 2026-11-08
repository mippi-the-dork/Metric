#include "MetricSubsystem.h"

#include "MetricEditorSettings.h"
#include "MetricEnvironmentQuery.h"
#include "MetricPerformance.h"
#include "MetricProfile.h"
#include "MetricProfileStyle.h"
#include "MetricProjectSettings.h"
#include "MetricRenderHostActor.h"
#include "CollisionShape.h"
#include "Containers/Ticker.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/World.h"
#include "Settings/LevelEditorViewportSettings.h"

#define LOCTEXT_NAMESPACE "MetricSubsystem"

namespace MetricFrame
{
    static FVector Up(const FMetricLiveState& State)
    {
        return State.AnalysisUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    }

    static FVector Forward(const FMetricLiveState& State)
    {
        const FVector FrameUp = Up(State);
        FVector Result = FVector::VectorPlaneProject(State.AnalysisForward, FrameUp).GetSafeNormal();
        if (Result.IsNearlyZero()) Result = FVector::VectorPlaneProject(State.Forward, FrameUp).GetSafeNormal();
        if (Result.IsNearlyZero()) Result = FVector::VectorPlaneProject(FVector::ForwardVector, FrameUp).GetSafeNormal();
        return Result;
    }

    static FVector Right(const FMetricLiveState& State)
    {
        return FVector::CrossProduct(Up(State), Forward(State)).GetSafeNormal();
    }

    static FVector LocalOffset(const FMetricLiveState& State, const FVector& Offset)
    {
        return Forward(State) * Offset.X + Right(State) * Offset.Y + Up(State) * Offset.Z;
    }
}


namespace MetricQueryAudit
{
    static void BuildHullShape(
        const FMetricLiveState& State,
        FCollisionShape& OutShape,
        FQuat& OutRotation,
        float& OutCenterHeight,
        bool& bOutApproximateShape)
    {
        const float Height = FMath::Max(1.0f, State.Hull.Height);
        const float Width = FMath::Max(1.0f, State.Hull.Width);
        const float Length = FMath::Max(1.0f, State.Hull.Length);
        const FVector Forward = MetricFrame::Forward(State);
        const FVector Up = MetricFrame::Up(State);

        OutRotation = FQuat::Identity;
        OutCenterHeight = Height * 0.5f;
        bOutApproximateShape = false;

        switch (State.Hull.HullType)
        {
        case EMetricHullType::Capsule:
        {
            const float Radius = FMath::Max(1.0f, Width * 0.5f);
            const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
            OutShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
            OutRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
            OutCenterHeight = HalfHeight;
            break;
        }
        case EMetricHullType::OrientedBox:
        {
            const FVector HalfExtent(Length * 0.5f, Width * 0.5f, Height * 0.5f);
            OutShape = FCollisionShape::MakeBox(HalfExtent);
            OutRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
            OutCenterHeight = HalfExtent.Z;
            break;
        }
        case EMetricHullType::Sphere:
        {
            const float Radius = FMath::Max3(Width, Length, Height) * 0.5f;
            OutShape = FCollisionShape::MakeSphere(FMath::Max(1.0f, Radius));
            OutCenterHeight = Radius;
            break;
        }
        case EMetricHullType::Cylinder:
        default:
        {
            const float Radius = FMath::Max(1.0f, Width * 0.5f);
            const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
            OutShape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
            OutRotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
            OutCenterHeight = HalfHeight;
            bOutApproximateShape = true;
            break;
        }
        }
    }

    static float ComputeHullSupportHeightAlongUp(
        const FMetricLiveState& State,
        const FVector& SurfaceNormal,
        const FVector& PlanarHullOffset)
    {
        const FVector Up = MetricFrame::Up(State);
        const FVector Forward = MetricFrame::Forward(State);
        const FVector Right = MetricFrame::Right(State);
        const FVector N = SurfaceNormal.GetSafeNormal(SMALL_NUMBER, Up);
        const float UpDot = FMath::Max(0.05f, FVector::DotProduct(N, Up));
        const float Height = FMath::Max(1.0f, State.Hull.Height);
        const float Width = FMath::Max(1.0f, State.Hull.Width);
        const float Length = FMath::Max(1.0f, State.Hull.Length);

        float SupportAlongNormal = Height * 0.5f;
        switch (State.Hull.HullType)
        {
        case EMetricHullType::Capsule:
        case EMetricHullType::Cylinder:
        {
            const float Radius = FMath::Max(1.0f, Width * 0.5f);
            const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
            const float HalfSegment = FMath::Max(0.0f, HalfHeight - Radius);
            SupportAlongNormal = HalfSegment * FMath::Abs(FVector::DotProduct(N, Up)) + Radius;
            break;
        }
        case EMetricHullType::OrientedBox:
            SupportAlongNormal =
                FMath::Abs(FVector::DotProduct(N, Forward)) * (Length * 0.5f) +
                FMath::Abs(FVector::DotProduct(N, Right)) * (Width * 0.5f) +
                FMath::Abs(FVector::DotProduct(N, Up)) * (Height * 0.5f);
            break;
        case EMetricHullType::Sphere:
        {
            const float Radius = FMath::Max3(Width, Length, Height) * 0.5f;
            SupportAlongNormal = Radius;
            break;
        }
        default:
            break;
        }

        // The planar local hull offset can move the hull uphill/downhill on the
        // supporting plane. Compensate for that without consuming the authored
        // vertical local offset.
        return FMath::Max(0.0f, (SupportAlongNormal - FVector::DotProduct(N, PlanarHullOffset)) / UpDot);
    }

    static float ComputeSurfaceDiscontinuity(
        const FVector& PreviousPoint,
        const FVector& PreviousNormal,
        const FVector& CurrentPoint,
        const FVector& CurrentNormal,
        const FVector& Up)
    {
        const FVector SafeUp = Up.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
        const FVector Delta = CurrentPoint - PreviousPoint;
        const float ActualUpDelta = FVector::DotProduct(Delta, SafeUp);
        const FVector PlanarDelta = Delta - SafeUp * ActualUpDelta;

        auto ExpectedDeltaFromPlane = [&](const FVector& Normal, bool& bValid)
        {
            const FVector N = Normal.GetSafeNormal();
            const float UpDot = FVector::DotProduct(N, SafeUp);
            bValid = FMath::Abs(UpDot) > 0.05f;
            return bValid ? -FVector::DotProduct(N, PlanarDelta) / UpDot : 0.0f;
        };

        bool bPreviousValid = false;
        bool bCurrentValid = false;
        const float PreviousExpected = ExpectedDeltaFromPlane(PreviousNormal, bPreviousValid);
        const float CurrentExpected = ExpectedDeltaFromPlane(CurrentNormal, bCurrentValid);

        float ExpectedUpDelta = 0.0f;
        if (bPreviousValid && bCurrentValid)
        {
            ExpectedUpDelta = (PreviousExpected + CurrentExpected) * 0.5f;
        }
        else if (bPreviousValid)
        {
            ExpectedUpDelta = PreviousExpected;
        }
        else if (bCurrentValid)
        {
            ExpectedUpDelta = CurrentExpected;
        }

        return ActualUpDelta - ExpectedUpDelta;
    }
}

namespace MetricBenchmark
{
    static void ApplyVisualVisibility(FMetricLiveState& State, const FMetricLayerVisibility& Visibility)
    {
        State.bShowHull = State.bShowHull && Visibility.bHullVisual;
        State.bShowForward = State.bShowForward && Visibility.bHullVisual;
        State.ClearanceSettings.bShowClearanceProbe = State.ClearanceSettings.bShowClearanceProbe && Visibility.bClearanceVisual;
        State.OverheadClearanceSettings.bShowOverheadClearance = State.OverheadClearanceSettings.bShowOverheadClearance && Visibility.bOverheadVisual;
        State.GroundValidationSettings.bShowGroundValidation = State.GroundValidationSettings.bShowGroundValidation && Visibility.bGroundVisual;
        State.FitAnalysisSettings.bShowFitAnalysis = State.FitAnalysisSettings.bShowFitAnalysis && Visibility.bFitVisual;
        State.ThresholdAnalysisSettings.bShowThresholdAnalysis = State.ThresholdAnalysisSettings.bShowThresholdAnalysis && Visibility.bThresholdVisual;
        State.TurnClearanceSettings.bShowTurnClearance = State.TurnClearanceSettings.bShowTurnClearance && Visibility.bTurnVisual;
        State.LedgeAnalysisSettings.bShowLedgeAnalysis = State.LedgeAnalysisSettings.bShowLedgeAnalysis && Visibility.bLedgeVisual;
        State.StoppingDistanceSettings.bShowStoppingDistance = State.StoppingDistanceSettings.bShowStoppingDistance && Visibility.bStoppingVisual;
        State.Sensors.bShowSensorHeights = State.Sensors.bShowSensorHeights && Visibility.bSensorsVisual;
        State.SensorFieldSettings.bShowSensorField = State.SensorFieldSettings.bShowSensorField && Visibility.bSensorFieldVisual;
        State.Ranges.bShowEngagementRanges = State.Ranges.bShowEngagementRanges && Visibility.bRangesVisual;
        State.Locomotion.bShowLocomotionBands = State.Locomotion.bShowLocomotionBands && Visibility.bLocomotionVisual;
        State.TrajectorySettings.bShowTrajectory = State.TrajectorySettings.bShowTrajectory && Visibility.bTrajectoryVisual;
        State.CameraProbeSettings.bShowCameraProbe = State.CameraProbeSettings.bShowCameraProbe && Visibility.bCameraVisual;
        State.CameraSightlineSettings.bShowCameraSightline = State.CameraSightlineSettings.bShowCameraSightline && Visibility.bSightlineVisual;
        State.CoverExposureSettings.bShowCoverExposure = State.CoverExposureSettings.bShowCoverExposure && Visibility.bCoverVisual;
        State.JumpRunwaySettings.bShowJumpRunway = State.JumpRunwaySettings.bShowJumpRunway && Visibility.bJumpRunwayVisual;
        State.InteractionReachSettings.bShowInteractionReach = State.InteractionReachSettings.bShowInteractionReach && Visibility.bInteractionVisual;
        State.ShoulderClearanceSettings.bShowShoulderClearance = State.ShoulderClearanceSettings.bShowShoulderClearance && Visibility.bShoulderVisual;
        State.CombatDensitySettings.bShowCombatDensity = State.CombatDensitySettings.bShowCombatDensity && Visibility.bCombatDensityVisual;
        State.AlternateStanceSettings.bShowAlternateStance = State.AlternateStanceSettings.bShowAlternateStance && Visibility.bAlternateStanceVisual;
        State.MantleVaultSettings.bShowMantleVault = State.MantleVaultSettings.bShowMantleVault && Visibility.bMantleVisual;
        State.RecoverySpaceSettings.bShowRecoverySpace = State.RecoverySpaceSettings.bShowRecoverySpace && Visibility.bRecoveryVisual;
        State.CrossfireSettings.bShowCrossfire = State.CrossfireSettings.bShowCrossfire && Visibility.bCrossfireVisual;

        // Result objects were already evaluated before presentation visibility is applied.
        // Hide their render-facing visibility explicitly so Visual and Results remain
        // genuinely independent.
        if (!Visibility.bClearanceVisual) State.Clearance.bVisible = false;
        if (!Visibility.bOverheadVisual) State.OverheadClearance.bVisible = false;
        if (!Visibility.bGroundVisual) State.GroundValidation.bVisible = false;
        if (!Visibility.bFitVisual) State.FitAnalysis.bVisible = false;
        if (!Visibility.bThresholdVisual) State.ThresholdAnalysis.bVisible = false;
        if (!Visibility.bTurnVisual) State.TurnClearance.bVisible = false;
        if (!Visibility.bLedgeVisual) State.LedgeAnalysis.bVisible = false;
        if (!Visibility.bStoppingVisual) State.StoppingDistance.bVisible = false;
        if (!Visibility.bSensorFieldVisual) State.SensorField.bVisible = false;
        if (!Visibility.bTrajectoryVisual) State.Trajectory.bVisible = false;
        if (!Visibility.bCameraVisual) State.CameraProbe.bVisible = false;
        if (!Visibility.bSightlineVisual) State.CameraSightline.bVisible = false;
        if (!Visibility.bCoverVisual) State.CoverExposure.bVisible = false;
        if (!Visibility.bJumpRunwayVisual) State.JumpRunway.bVisible = false;
        if (!Visibility.bInteractionVisual) State.InteractionReach.bVisible = false;
        if (!Visibility.bShoulderVisual) State.ShoulderClearance.bVisible = false;
        if (!Visibility.bCombatDensityVisual) State.CombatDensity.bVisible = false;
        if (!Visibility.bAlternateStanceVisual) State.AlternateStance.bVisible = false;
        if (!Visibility.bMantleVisual) State.MantleVault.bVisible = false;
        if (!Visibility.bRecoveryVisual) State.RecoverySpace.bVisible = false;
        if (!Visibility.bCrossfireVisual) State.Crossfire.bVisible = false;
    }
    static FString BuildCapturedResults(const FMetricLiveState& State)
    {
        TArray<FString> Lines;

        if (State.bShowHull)
        {
            Lines.Add(FString::Printf(TEXT("HULL        %.0f W x %.0f H x %.0f L"), State.Hull.Width, State.Hull.Height, State.Hull.Length));
        }

        if (State.Clearance.bVisible)
        {
            if (State.Clearance.bBlocked)
            {
                Lines.Add(FString::Printf(TEXT("CLEARANCE   FAIL   %.0fu / %.0fu"), State.Clearance.ClearDistance, State.Clearance.ProbeDistance));
            }
            else if (State.Clearance.bNearThreshold)
            {
                Lines.Add(FString::Printf(TEXT("CLEARANCE   TIGHT  %.0fu | obstacle %.0fu"), State.Clearance.ProbeDistance, State.Clearance.ObstacleDistance));
            }
            else
            {
                Lines.Add(FString::Printf(TEXT("CLEARANCE   PASS   %.0fu+"), State.Clearance.ProbeDistance));
            }
        }

        if (State.GroundValidation.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("GROUND      %s   %d/%d | slope %.0f deg | up %.0fu down %.0fu"),
                State.GroundValidation.bPass ? TEXT("PASS") : TEXT("FAIL"),
                State.GroundValidation.TotalSamples - State.GroundValidation.FailedSamples,
                State.GroundValidation.TotalSamples,
                State.GroundValidation.MaxObservedSlope,
                State.GroundValidation.MaxObservedStepUp,
                State.GroundValidation.MaxObservedStepDown));
        }

        if (State.FitAnalysis.bVisible)
        {
            const TCHAR* FitStatus = State.FitAnalysis.bPass ? (State.FitAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL");
            Lines.Add(FString::Printf(TEXT("FIT         %s   W %.0fu H %.0fu | deficit W %.0fu H %.0fu"),
                FitStatus,
                State.FitAnalysis.MinimumAvailableWidth,
                State.FitAnalysis.MinimumAvailableHeight,
                State.FitAnalysis.MaxWidthDeficit,
                State.FitAnalysis.MaxHeightDeficit));
        }

        if (State.ThresholdAnalysis.bVisible)
        {
            const TCHAR* ThresholdStatus = State.ThresholdAnalysis.bPass ? (State.ThresholdAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL");
            Lines.Add(FString::Printf(TEXT("THRESHOLD   %s   %d/%d | W %.0fu H %.0fu | step +%.0f/-%.0fu"),
                ThresholdStatus,
                State.ThresholdAnalysis.TotalSamples - State.ThresholdAnalysis.FailedSamples,
                State.ThresholdAnalysis.TotalSamples,
                State.ThresholdAnalysis.MinimumAvailableWidth,
                State.ThresholdAnalysis.MinimumAvailableHeight,
                State.ThresholdAnalysis.MaxObservedStepUp,
                State.ThresholdAnalysis.MaxObservedStepDown));
        }

        if (State.TurnClearance.bVisible)
        {
            auto PathStatus = [](const FMetricTurnPathResult& Path) -> FString
            {
                if (!Path.bTested) return TEXT("OFF");
                if (Path.bBlocked) return FString::Printf(TEXT("BLOCKED %.0f deg / %.0fu"), Path.ClearAngle, Path.ClearDistance);
                return FString::Printf(TEXT("PASS %.0f deg / %.0fu"), Path.ClearAngle, Path.ClearDistance);
            };
            Lines.Add(FString::Printf(TEXT("TURN        L %s | R %s"), *PathStatus(State.TurnClearance.Left), *PathStatus(State.TurnClearance.Right)));
        }

        if (State.LedgeAnalysis.bVisible)
        {
            if (!State.LedgeAnalysis.bHasEdge)
            {
                Lines.Add(FString::Printf(TEXT("LEDGE       NONE   %.0fu"), State.LedgeAnalysis.EdgeDistance));
            }
            else if (!State.LedgeAnalysis.bHasLanding)
            {
                Lines.Add(FString::Printf(TEXT("LEDGE       DROP   %.0fu | >%.0fu"), State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth));
            }
            else
            {
                const TCHAR* LedgeStatus = State.LedgeAnalysis.bSafeDrop ? (State.LedgeAnalysis.bWarning ? TEXT("WARN") : TEXT("SAFE")) : TEXT("FAIL");
                Lines.Add(FString::Printf(TEXT("LEDGE       %s   edge %.0fu | drop %.0fu"), LedgeStatus, State.LedgeAnalysis.EdgeDistance, State.LedgeAnalysis.DropDepth));
            }
        }

        if (State.StoppingDistance.bVisible)
        {
            const TCHAR* StopStatus = State.StoppingDistance.bBlocked ? TEXT("FAIL") : (State.StoppingDistance.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS"));
            Lines.Add(FString::Printf(TEXT("STOPPING    %s   need %.0fu | avail %.0fu | react %.0fu brake %.0fu"),
                StopStatus,
                State.StoppingDistance.RequiredDistance,
                State.StoppingDistance.AvailableDistance,
                State.StoppingDistance.ReactionDistance,
                State.StoppingDistance.BrakingDistance));
        }

        if (State.CoverExposure.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("COVER       %.0f%% exposed | low %s torso %s head %s"), State.CoverExposure.ExposurePercent,
                State.CoverExposure.bLowCovered ? TEXT("covered") : TEXT("open"), State.CoverExposure.bTorsoCovered ? TEXT("covered") : TEXT("open"), State.CoverExposure.bHeadCovered ? TEXT("covered") : TEXT("open")));
        }
        if (State.JumpRunway.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("RUNWAY      %s | takeoff %.0fu landing %.0fu"),
                (State.JumpRunway.bTakeoffPass && State.JumpRunway.bLandingPass) ? TEXT("PASS") : TEXT("FAIL"), State.JumpRunway.TakeoffAvailable, State.JumpRunway.LandingAvailable));
        }
        if (State.InteractionReach.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("REACH       %s | %.0fu"), State.InteractionReach.bHasTarget ? TEXT("TARGET") : TEXT("OPEN"), State.InteractionReach.TargetDistance));
        }
        if (State.ShoulderClearance.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("SHOULDER    L %s %.0fu | R %s %.0fu"), State.ShoulderClearance.bLeftBlocked ? TEXT("BLOCKED") : TEXT("CLEAR"), State.ShoulderClearance.LeftDistance,
                State.ShoulderClearance.bRightBlocked ? TEXT("BLOCKED") : TEXT("CLEAR"), State.ShoulderClearance.RightDistance));
        }
        if (State.CombatDensity.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("DENSITY     ~%d units | %.0fu personal"), State.CombatDensity.ApproxCapacity, State.CombatDensity.PersonalDiameter));
        }
        if (State.AlternateStance.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("STANCE      stand %s | alt %s | %.0fu high"), State.AlternateStance.bStandingFits ? TEXT("PASS") : TEXT("FAIL"),
                State.AlternateStance.bAlternateFits ? TEXT("PASS") : TEXT("FAIL"), State.AlternateStance.AvailableHeight));
        }
        if (State.MantleVault.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("MANTLE      %s | H %.0fu D %.0fu"),
                (State.MantleVault.bHasObstacle && State.MantleVault.bHeightValid && State.MantleVault.bDepthValid) ? TEXT("PASS") : (State.MantleVault.bHasObstacle ? TEXT("FAIL") : TEXT("NONE")),
                State.MantleVault.ObstacleHeight, State.MantleVault.ObstacleDepth));
        }
        if (State.RecoverySpace.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("RECOVERY    F %.0f B %.0f L %.0f R %.0fu"), State.RecoverySpace.ForwardClear, State.RecoverySpace.BackwardClear, State.RecoverySpace.LeftClear, State.RecoverySpace.RightClear));
        }
        if (State.Crossfire.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("CROSSFIRE   %.0f%% exposed | %d/%d dirs"), State.Crossfire.ExposurePercent, State.Crossfire.ExposedDirections, State.Crossfire.TotalDirections));
        }

        if (State.CameraProbe.bVisible)
        {
            const TCHAR* CameraStatus = State.CameraProbe.bBelowMinimum ? TEXT("TOO CLOSE") : (State.CameraProbe.bCompressed ? TEXT("COMPRESSED") : TEXT("CLEAR"));
            Lines.Add(FString::Printf(TEXT("CAMERA      %s   %.0fu / %.0fu | %.0f%%"),
                CameraStatus,
                State.CameraProbe.ResolvedDistance,
                State.CameraProbe.TargetDistance,
                State.CameraProbe.CompressionPercent));
        }

        if (State.CameraSightline.bVisible)
        {
            const TCHAR* ViewStatus = State.CameraSightline.bOccluded ? TEXT("OCCLUDED") : (State.CameraSightline.bFramingFail ? TEXT("FRAMING FAIL") : TEXT("PASS"));
            Lines.Add(FString::Printf(TEXT("VIEW        %s   %.1f deg"), ViewStatus, State.CameraSightline.SubjectAngle));
        }

        if (State.Trajectory.bVisible)
        {
            const TCHAR* ArcStatus = State.Trajectory.bBlocked ? TEXT("BLOCKED") : (State.Trajectory.bLandingValid ? TEXT("LANDING OK") : TEXT("NO LANDING"));
            Lines.Add(FString::Printf(TEXT("TRAJECTORY  %s   %.0fu | %.2fs | apex %.0fu | headroom %.0fu"),
                ArcStatus,
                State.Trajectory.HorizontalDistance,
                State.Trajectory.FlightTime,
                State.Trajectory.ApexHeight,
                State.Trajectory.ApexHeadroom));
        }

        if (State.OverheadClearance.bVisible)
        {
            if (State.OverheadClearance.bBlocked)
            {
                Lines.Add(FString::Printf(TEXT("OVERHEAD    FAIL   %.0fu / %.0fu (-%.0fu)"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight, State.OverheadClearance.HeightDeficit));
            }
            else if (State.OverheadClearance.bNearThreshold)
            {
                Lines.Add(FString::Printf(TEXT("OVERHEAD    TIGHT  %.0fu / %.0fu"), State.OverheadClearance.AvailableHeight, State.OverheadClearance.RequiredHeight));
            }
            else
            {
                Lines.Add(FString::Printf(TEXT("OVERHEAD    PASS   %.0fu+"), State.OverheadClearance.AvailableHeight));
            }
        }
        if (State.Sensors.bShowSensorHeights)
        {
            Lines.Add(FString::Printf(TEXT("SENSORS     primary %.0fu   low %.0fu"), State.Sensors.PrimarySensorZOffset, State.Sensors.LowProfileSensorZOffset));
        }

        if (State.SensorField.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("SENSOR      %.0f%% CLEAR   %d/%d blocked | nearest %.0fu"),
                State.SensorField.ClearPercent,
                State.SensorField.BlockedRays,
                State.SensorField.TotalRays,
                State.SensorField.NearestBlockDistance));
        }

        if (State.Locomotion.bShowLocomotionBands)
        {
            Lines.Add(FString::Printf(TEXT("MOVE        %.0fu/s   %.1f / %.1f / %.1fs"),
                State.Locomotion.Speed,
                State.Locomotion.NearTime,
                State.Locomotion.MidTime,
                State.Locomotion.FarTime));
        }

        if (State.Ranges.bShowEngagementRanges)
        {
            Lines.Add(FString::Printf(TEXT("RANGE       %.0f / %.0f / %.0fu"),
                State.Ranges.ShortRange,
                State.Ranges.MediumRange,
                State.Ranges.LongRange));
        }

        return Lines.IsEmpty() ? TEXT("No analysis layers were active when this benchmark was captured.") : FString::Join(Lines, TEXT("\n"));
    }
}

void UMetricSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RefreshCycleEntries();

    // The subsystem outlives individual editor worlds. Release world-owned
    // visualization before LoadMap and again when any world begins teardown.
    // The weak RenderHost pointer is the primary ownership safeguard; these
    // delegates make cleanup deterministic and immediate.
    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(
        this,
        &UMetricSubsystem::HandlePreLoadMap);
    WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(
        this,
        &UMetricSubsystem::HandleWorldBeginTearDown);

    // Creating/saving starter assets during subsystem initialization can run
    // before UE has finished registering all Data Validation validators. Defer
    // the one-time bootstrap until the editor has been alive for a moment.
    TWeakObjectPtr<UMetricSubsystem> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(
        TEXT("MetricStarterProfileBootstrap"),
        1.0f,
        [WeakThis](float)
        {
            if (UMetricSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->EnsureStarterProfileAssets();
                StrongThis->RefreshCycleEntries();
            }
            return false;
        });
}

void UMetricSubsystem::EnsureStarterProfileAssets()
{
    if (IsRunningCommandlet())
    {
        return;
    }

    UMetricProjectSettings* ProjectSettings = GetMutableDefault<UMetricProjectSettings>();
    if (!ProjectSettings
        || !ProjectSettings->bCreateStarterProfileAssetsIfMissing
        || ProjectSettings->bStarterProfilesInitialized)
    {
        return;
    }

    FMetricHullSettings HumanHull;
    HumanHull.HullType = EMetricHullType::Capsule;
    HumanHull.Height = 180.0f;
    HumanHull.Width = 88.0f;
    HumanHull.Length = 88.0f;

    FMetricHullSettings HeavyHull;
    HeavyHull.HullType = EMetricHullType::OrientedBox;
    HeavyHull.Height = 300.0f;
    HeavyHull.Width = 180.0f;
    HeavyHull.Length = 140.0f;

    UMetricProfile* HumanProfile = CreateStarterProfileAsset(
        TEXT("/Game/Metric/Profiles/DA_Metric_Human"),
        LOCTEXT("StarterHumanDisplayName", "Human Reference"),
        HumanHull);

    UMetricProfile* HeavyProfile = CreateStarterProfileAsset(
        TEXT("/Game/Metric/Profiles/DA_Metric_Heavy"),
        LOCTEXT("StarterHeavyDisplayName", "Heavy Reference"),
        HeavyHull);

    bool bSettingsChanged = false;
    if (HumanProfile)
    {
        const TSoftObjectPtr<UMetricProfile> HumanSoft(HumanProfile);
        if (!ProjectSettings->RegisteredProfiles.Contains(HumanSoft))
        {
            ProjectSettings->RegisteredProfiles.Add(HumanSoft);
            bSettingsChanged = true;
        }
    }

    if (HeavyProfile)
    {
        const TSoftObjectPtr<UMetricProfile> HeavySoft(HeavyProfile);
        if (!ProjectSettings->RegisteredProfiles.Contains(HeavySoft))
        {
            ProjectSettings->RegisteredProfiles.Add(HeavySoft);
            bSettingsChanged = true;
        }
    }

    if (HumanProfile && HeavyProfile)
    {
        ProjectSettings->bStarterProfilesInitialized = true;
        bSettingsChanged = true;
    }

    if (bSettingsChanged)
    {
        ProjectSettings->SaveConfig();
    }
}

UMetricProfile* UMetricSubsystem::CreateStarterProfileAsset(
    const FString& PackagePath,
    const FText& DisplayName,
    const FMetricHullSettings& HullSettings)
{
    const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
    const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);

    if (UMetricProfile* ExistingProfile = LoadObject<UMetricProfile>(nullptr, *ObjectPath))
    {
        return ExistingProfile;
    }

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return nullptr;
    }

    UMetricProfile* NewProfile = NewObject<UMetricProfile>(
        Package,
        *AssetName,
        RF_Public | RF_Standalone | RF_Transactional);
    if (!NewProfile)
    {
        return nullptr;
    }

    NewProfile->DisplayName = DisplayName;
    NewProfile->Hull = HullSettings;

    // Starter assets are authored from explicit Metric defaults. Existing
    // project assets are never overwritten by this path.
    const bool bHeavyStarter = AssetName.Contains(TEXT("Heavy"));
    NewProfile->Clearance.bShowClearanceProbe = true;
    NewProfile->Clearance.ProbeDistance = bHeavyStarter ? 400.0f : 300.0f;
    NewProfile->Clearance.WarningMargin = bHeavyStarter ? 50.0f : 25.0f;
    NewProfile->Clearance.TraceChannel = ECC_Visibility;

    NewProfile->OverheadClearance.bShowOverheadClearance = true;
    NewProfile->OverheadClearance.MaxProbeHeight = bHeavyStarter ? 450.0f : 320.0f;
    NewProfile->OverheadClearance.WarningMargin = bHeavyStarter ? 40.0f : 30.0f;
    NewProfile->OverheadClearance.TraceChannel = ECC_Visibility;

    NewProfile->Sensors.bShowSensorHeights = true;
    NewProfile->Sensors.PrimarySensorZOffset = bHeavyStarter ? 260.0f : 160.0f;
    NewProfile->Sensors.LowProfileSensorZOffset = bHeavyStarter ? 160.0f : 100.0f;
    NewProfile->Sensors.MarkerRadius = bHeavyStarter ? 32.0f : 24.0f;

    NewProfile->SensorField.bShowSensorField = false;
    NewProfile->SensorField.Range = bHeavyStarter ? 2200.0f : 1800.0f;
    NewProfile->SensorField.HorizontalHalfAngle = bHeavyStarter ? 40.0f : 45.0f;
    NewProfile->SensorField.VerticalHalfAngle = bHeavyStarter ? 25.0f : 30.0f;
    NewProfile->SensorField.HorizontalRayCount = bHeavyStarter ? 11 : 9;
    NewProfile->SensorField.TraceChannel = ECC_Visibility;

    NewProfile->EngagementRanges.bShowEngagementRanges = false;
    NewProfile->EngagementRanges.ShortRange = bHeavyStarter ? 700.0f : 500.0f;
    NewProfile->EngagementRanges.MediumRange = bHeavyStarter ? 2000.0f : 1500.0f;
    NewProfile->EngagementRanges.LongRange = bHeavyStarter ? 4000.0f : 3000.0f;

    NewProfile->LocomotionBands.bShowLocomotionBands = false;
    NewProfile->LocomotionBands.Speed = bHeavyStarter ? 350.0f : 600.0f;
    NewProfile->LocomotionBands.NearTime = 1.0f;
    NewProfile->LocomotionBands.MidTime = 3.0f;
    NewProfile->LocomotionBands.FarTime = 5.0f;

    NewProfile->Trajectory.bShowTrajectory = false;
    NewProfile->Trajectory.HorizontalSpeed = bHeavyStarter ? 400.0f : 600.0f;
    NewProfile->Trajectory.LaunchVerticalSpeed = bHeavyStarter ? 450.0f : 550.0f;
    NewProfile->Trajectory.GravityScale = 1.0f;
    NewProfile->Trajectory.MaxSimulationTime = bHeavyStarter ? 2.25f : 2.0f;
    NewProfile->Trajectory.SampleInterval = 0.05f;
    NewProfile->Trajectory.MaxLandingSlope = bHeavyStarter ? 40.0f : 45.0f;
    NewProfile->Trajectory.bShowTrajectoryGuideRails = true;
    NewProfile->Trajectory.GuideRailSampleStride = bHeavyStarter ? 4 : 3;
    NewProfile->Trajectory.ApexProbeDistance = bHeavyStarter ? 250.0f : 200.0f;
    NewProfile->Trajectory.ApexWarningMargin = bHeavyStarter ? 75.0f : 50.0f;
    NewProfile->Trajectory.TraceChannel = ECC_Visibility;

    NewProfile->CameraProbe.bShowCameraProbe = true;
    NewProfile->CameraProbe.PivotZOffset = bHeavyStarter ? 260.0f : 160.0f;
    NewProfile->CameraProbe.TargetDistance = bHeavyStarter ? 500.0f : 350.0f;
    NewProfile->CameraProbe.VerticalOffset = bHeavyStarter ? 100.0f : 60.0f;
    NewProfile->CameraProbe.ShoulderOffset = 0.0f;
    NewProfile->CameraProbe.ProbeRadius = bHeavyStarter ? 20.0f : 12.0f;
    NewProfile->CameraProbe.MinDistance = bHeavyStarter ? 100.0f : 60.0f;
    NewProfile->CameraProbe.TraceChannel = ECC_Visibility;

    NewProfile->CameraSightline.bShowCameraSightline = false;
    NewProfile->CameraSightline.SubjectZOffset = bHeavyStarter ? 240.0f : 140.0f;
    NewProfile->CameraSightline.FramingHalfAngle = bHeavyStarter ? 22.0f : 25.0f;
    NewProfile->CameraSightline.TraceRadius = bHeavyStarter ? 6.0f : 4.0f;
    NewProfile->CameraSightline.TraceChannel = ECC_Visibility;

    NewProfile->GroundValidation.bShowGroundValidation = true;
    NewProfile->GroundValidation.ForwardDistance = bHeavyStarter ? 400.0f : 300.0f;
    NewProfile->GroundValidation.SampleSpacing = bHeavyStarter ? 75.0f : 50.0f;
    NewProfile->GroundValidation.LateralCoverage = bHeavyStarter ? 180.0f : 88.0f;
    NewProfile->GroundValidation.MaxWalkableSlope = bHeavyStarter ? 35.0f : 45.0f;
    NewProfile->GroundValidation.MaxStepUp = bHeavyStarter ? 30.0f : 45.0f;
    NewProfile->GroundValidation.MaxStepDown = bHeavyStarter ? 40.0f : 60.0f;
    NewProfile->GroundValidation.TraceHeight = bHeavyStarter ? 160.0f : 120.0f;
    NewProfile->GroundValidation.TraceDepth = bHeavyStarter ? 250.0f : 200.0f;
    NewProfile->GroundValidation.TraceChannel = ECC_Visibility;

    NewProfile->FitAnalysis.bShowFitAnalysis = true;
    NewProfile->FitAnalysis.ForwardDistance = bHeavyStarter ? 400.0f : 300.0f;
    NewProfile->FitAnalysis.SampleSpacing = bHeavyStarter ? 75.0f : 50.0f;
    NewProfile->FitAnalysis.MaxSideProbeDistance = bHeavyStarter ? 400.0f : 250.0f;
    NewProfile->FitAnalysis.MaxCeilingProbeHeight = bHeavyStarter ? 450.0f : 300.0f;
    NewProfile->FitAnalysis.WarningMargin = bHeavyStarter ? 20.0f : 15.0f;
    NewProfile->FitAnalysis.FloorTraceHeight = bHeavyStarter ? 160.0f : 120.0f;
    NewProfile->FitAnalysis.FloorTraceDepth = bHeavyStarter ? 250.0f : 200.0f;
    NewProfile->FitAnalysis.TraceChannel = ECC_Visibility;

    NewProfile->ThresholdAnalysis.bShowThresholdAnalysis = true;
    NewProfile->ThresholdAnalysis.ProbeDepth = bHeavyStarter ? 180.0f : 120.0f;
    NewProfile->ThresholdAnalysis.MaxSideProbeDistance = bHeavyStarter ? 400.0f : 250.0f;
    NewProfile->ThresholdAnalysis.MaxCeilingProbeHeight = bHeavyStarter ? 450.0f : 300.0f;
    NewProfile->ThresholdAnalysis.WarningMargin = bHeavyStarter ? 20.0f : 15.0f;
    NewProfile->ThresholdAnalysis.MaxStepUp = bHeavyStarter ? 30.0f : 45.0f;
    NewProfile->ThresholdAnalysis.MaxStepDown = bHeavyStarter ? 40.0f : 60.0f;
    NewProfile->ThresholdAnalysis.FloorTraceHeight = bHeavyStarter ? 160.0f : 120.0f;
    NewProfile->ThresholdAnalysis.FloorTraceDepth = bHeavyStarter ? 250.0f : 200.0f;
    NewProfile->ThresholdAnalysis.TraceChannel = ECC_Visibility;

    NewProfile->TurnClearance.bShowTurnClearance = false;
    NewProfile->TurnClearance.TurnRadius = bHeavyStarter ? 400.0f : 250.0f;
    NewProfile->TurnClearance.TurnAngle = 90.0f;
    NewProfile->TurnClearance.SampleAngle = bHeavyStarter ? 10.0f : 7.5f;
    NewProfile->TurnClearance.Direction = EMetricTurnDirection::Both;
    NewProfile->TurnClearance.TraceChannel = ECC_Visibility;

    NewProfile->LedgeAnalysis.bShowLedgeAnalysis = false;
    NewProfile->LedgeAnalysis.ForwardDistance = bHeavyStarter ? 200.0f : 150.0f;
    NewProfile->LedgeAnalysis.SampleSpacing = bHeavyStarter ? 20.0f : 15.0f;
    NewProfile->LedgeAnalysis.MaxSafeDrop = bHeavyStarter ? 80.0f : 120.0f;
    NewProfile->LedgeAnalysis.MaxProbeDepth = bHeavyStarter ? 650.0f : 500.0f;
    NewProfile->LedgeAnalysis.TraceHeight = bHeavyStarter ? 150.0f : 100.0f;
    NewProfile->LedgeAnalysis.TraceChannel = ECC_Visibility;

    NewProfile->StoppingDistance.bShowStoppingDistance = false;
    NewProfile->StoppingDistance.Speed = bHeavyStarter ? 350.0f : 600.0f;
    NewProfile->StoppingDistance.Deceleration = bHeavyStarter ? 900.0f : 1800.0f;
    NewProfile->StoppingDistance.ReactionTime = bHeavyStarter ? 0.20f : 0.15f;
    NewProfile->StoppingDistance.WarningMargin = bHeavyStarter ? 100.0f : 75.0f;
    NewProfile->StoppingDistance.TraceChannel = ECC_Visibility;

    NewProfile->CoverExposure.bShowCoverExposure = false;
    NewProfile->CoverExposure.ThreatDistance = bHeavyStarter ? 1500.0f : 1200.0f;
    NewProfile->CoverExposure.ThreatHeight = bHeavyStarter ? 240.0f : 160.0f;
    NewProfile->CoverExposure.LowSampleHeight = bHeavyStarter ? 90.0f : 60.0f;
    NewProfile->CoverExposure.TorsoSampleHeight = bHeavyStarter ? 190.0f : 120.0f;
    NewProfile->CoverExposure.HeadSampleHeight = bHeavyStarter ? 275.0f : 170.0f;
    NewProfile->CoverExposure.TraceChannel = ECC_Visibility;

    NewProfile->JumpRunway.bShowJumpRunway = false;
    NewProfile->JumpRunway.TakeoffRunway = bHeavyStarter ? 220.0f : 150.0f;
    NewProfile->JumpRunway.LandingRunway = bHeavyStarter ? 260.0f : 180.0f;
    NewProfile->JumpRunway.GroundProbeDepth = bHeavyStarter ? 220.0f : 150.0f;
    NewProfile->JumpRunway.TraceChannel = ECC_Visibility;

    NewProfile->InteractionReach.bShowInteractionReach = false;
    NewProfile->InteractionReach.ReachDistance = bHeavyStarter ? 220.0f : 180.0f;
    NewProfile->InteractionReach.ReachHeight = bHeavyStarter ? 220.0f : 130.0f;
    NewProfile->InteractionReach.ProbeRadius = bHeavyStarter ? 28.0f : 18.0f;
    NewProfile->InteractionReach.TraceChannel = ECC_Visibility;

    NewProfile->ShoulderClearance.bShowShoulderClearance = false;
    NewProfile->ShoulderClearance.PivotHeight = bHeavyStarter ? 250.0f : 150.0f;
    NewProfile->ShoulderClearance.ShoulderOffset = bHeavyStarter ? 85.0f : 55.0f;
    NewProfile->ShoulderClearance.CameraDistance = bHeavyStarter ? 450.0f : 300.0f;
    NewProfile->ShoulderClearance.ProbeRadius = bHeavyStarter ? 20.0f : 12.0f;
    NewProfile->ShoulderClearance.TraceChannel = ECC_Visibility;

    NewProfile->CombatDensity.bShowCombatDensity = false;
    NewProfile->CombatDensity.AreaRadius = bHeavyStarter ? 900.0f : 600.0f;
    NewProfile->CombatDensity.PersonalSpacePadding = bHeavyStarter ? 90.0f : 50.0f;

    NewProfile->AlternateStance.bShowAlternateStance = false;
    NewProfile->AlternateStance.AlternateHeight = bHeavyStarter ? 190.0f : 110.0f;
    NewProfile->AlternateStance.WarningMargin = bHeavyStarter ? 25.0f : 15.0f;
    NewProfile->AlternateStance.TraceChannel = ECC_Visibility;

    NewProfile->MantleVault.bShowMantleVault = false;
    NewProfile->MantleVault.ForwardReach = bHeavyStarter ? 160.0f : 120.0f;
    NewProfile->MantleVault.MinObstacleHeight = bHeavyStarter ? 60.0f : 45.0f;
    NewProfile->MantleVault.MaxObstacleHeight = bHeavyStarter ? 220.0f : 180.0f;
    NewProfile->MantleVault.MaxObstacleDepth = bHeavyStarter ? 130.0f : 100.0f;
    NewProfile->MantleVault.TraceChannel = ECC_Visibility;

    NewProfile->RecoverySpace.bShowRecoverySpace = false;
    NewProfile->RecoverySpace.ForwardDistance = bHeavyStarter ? 300.0f : 250.0f;
    NewProfile->RecoverySpace.BackwardDistance = bHeavyStarter ? 220.0f : 180.0f;
    NewProfile->RecoverySpace.SideDistance = bHeavyStarter ? 300.0f : 220.0f;
    NewProfile->RecoverySpace.TraceChannel = ECC_Visibility;

    NewProfile->Crossfire.bShowCrossfire = false;
    NewProfile->Crossfire.ThreatDistance = bHeavyStarter ? 1600.0f : 1200.0f;
    NewProfile->Crossfire.DirectionCount = 8;
    NewProfile->Crossfire.SampleHeight = bHeavyStarter ? 220.0f : 130.0f;
    NewProfile->Crossfire.TraceChannel = ECC_Visibility;

    // Starter profiles intentionally use distinct visual identities. Custom profiles
    // start with the Human/cool defaults from FMetricVisualizationTheme and can
    // override every theme color in the Metric Profile Editor.
    if (bHeavyStarter)
    {
        MetricProfileStyle::ApplyWarmTheme(NewProfile->VisualTheme);
    }

    NewProfile->Representation.bShowStaticMesh = false;
    NewProfile->Representation.StaticMesh.Reset();
    NewProfile->Representation.RelativeTransform = FTransform::Identity;

    FAssetRegistryModule::AssetCreated(NewProfile);
    Package->MarkPackageDirty();

    const FString Filename = FPackageName::LongPackageNameToFilename(
        PackagePath,
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;

    if (!UPackage::SavePackage(Package, NewProfile, *Filename, SaveArgs))
    {
        return nullptr;
    }

    return NewProfile;
}

void UMetricSubsystem::Deinitialize()
{
    if (PreLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
        PreLoadMapHandle.Reset();
    }

    if (WorldBeginTearDownHandle.IsValid())
    {
        FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);
        WorldBeginTearDownHandle.Reset();
    }

    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }

    SetMetricActive(false);
    ClearRenderHost();
    CycleEntries.Reset();
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    bHasSelectedPinnedEvaluatedState = false;
    SelectedPinnedEvaluatedState = FMetricLiveState();
    ActivePinnedBenchmarkMap.Reset();
    Super::Deinitialize();
}

void UMetricSubsystem::HandlePreLoadMap(const FString& /*MapName*/)
{
    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }

    // LoadMap will synchronously tear down the current editor world. Drop every
    // world-owned reference before that process begins. Keep Metric's enabled
    // state so the Lens can resume lazily in the newly loaded editor world.
    ClearRenderHost();
    LiveState.bHasHit = false;
    LiveState.Clearance = FMetricClearanceResult();
    LiveState.OverheadClearance = FMetricOverheadClearanceResult();
    LiveState.Trajectory = FMetricTrajectoryResult();
    LiveState.CameraProbe = FMetricCameraProbeResult();
    LiveState.CameraSightline = FMetricCameraSightlineResult();
    LiveState.GroundValidation = FMetricGroundValidationResult();
    LiveState.FitAnalysis = FMetricFitAnalysisResult();
    LiveState.TurnClearance = FMetricTurnClearanceResult();
    LiveState.LedgeAnalysis = FMetricLedgeAnalysisResult();
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    bHasSelectedPinnedEvaluatedState = false;
    SelectedPinnedEvaluatedState = FMetricLiveState();
    ActivePinnedBenchmarkMap.Reset();
}

void UMetricSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
    // PreLoadMap normally saves first and clears ActivePinnedBenchmarkMap. If a
    // world is torn down through another editor path, persist it here only while
    // it is still the active pin owner. This avoids an empty post-PreLoad save
    // erasing the layout we just stored.
    if (World && !ActivePinnedBenchmarkMap.IsEmpty()
        && GetPersistableMapPackageName(World) == ActivePinnedBenchmarkMap)
    {
        SavePinnedBenchmarksForWorld(World);
    }

    AMetricRenderHostActor* Host = RenderHost.Get();
    if (!World || !Host || Host->GetWorld() != World)
    {
        return;
    }

    // Do not call DestroyActor from the world's teardown callback. The world is
    // already dismantling its Actors; simply release Metric's weak handle and
    // live visualization state so nothing attempts to use the dying world.
    RenderHost.Reset();
    LiveState.bHasHit = false;
    LiveState.Clearance = FMetricClearanceResult();
    LiveState.OverheadClearance = FMetricOverheadClearanceResult();
    LiveState.Trajectory = FMetricTrajectoryResult();
    LiveState.CameraProbe = FMetricCameraProbeResult();
    LiveState.CameraSightline = FMetricCameraSightlineResult();
    LiveState.GroundValidation = FMetricGroundValidationResult();
    LiveState.FitAnalysis = FMetricFitAnalysisResult();
    LiveState.TurnClearance = FMetricTurnClearanceResult();
    LiveState.LedgeAnalysis = FMetricLedgeAnalysisResult();
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    ActivePinnedBenchmarkMap.Reset();
}

bool UMetricSubsystem::ArePinnedBenchmarksPersistent() const
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    return Settings && Settings->bPersistPinnedBenchmarksPerMap;
}

FString UMetricSubsystem::GetPersistableMapPackageName(UWorld* World) const
{
    if (!World || World->WorldType != EWorldType::Editor)
    {
        return FString();
    }

    UPackage* Package = World->GetPackage();
    if (!Package)
    {
        return FString();
    }

    const FString PackageName = Package->GetName();
    if (PackageName.IsEmpty() || PackageName.StartsWith(TEXT("/Temp/")) || PackageName.StartsWith(TEXT("/Engine/Transient")))
    {
        return FString();
    }

    return PackageName;
}

void UMetricSubsystem::SavePinnedBenchmarksForWorld(UWorld* World)
{
    if (!ArePinnedBenchmarksPersistent())
    {
        return;
    }

    const FString MapPackageName = GetPersistableMapPackageName(World);
    if (MapPackageName.IsEmpty())
    {
        return;
    }

    UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>();
    if (!Settings)
    {
        return;
    }

    int32 ExistingIndex = INDEX_NONE;
    for (int32 Index = 0; Index < Settings->PersistentBenchmarkMaps.Num(); ++Index)
    {
        if (Settings->PersistentBenchmarkMaps[Index].MapPackageName == MapPackageName)
        {
            ExistingIndex = Index;
            break;
        }
    }

    if (PinnedBenchmarks.IsEmpty())
    {
        if (ExistingIndex != INDEX_NONE)
        {
            Settings->PersistentBenchmarkMaps.RemoveAt(ExistingIndex);
            Settings->SaveConfig();
        }
        return;
    }

    FMetricPersistentBenchmarkMap* StoredMap = ExistingIndex != INDEX_NONE
        ? &Settings->PersistentBenchmarkMaps[ExistingIndex]
        : &Settings->PersistentBenchmarkMaps.AddDefaulted_GetRef();

    StoredMap->MapPackageName = MapPackageName;
    StoredMap->Benchmarks.Reset();
    StoredMap->Benchmarks.Reserve(PinnedBenchmarks.Num());

    for (const FMetricPinnedBenchmark& Pin : PinnedBenchmarks)
    {
        FMetricPersistentBenchmark& StoredPin = StoredMap->Benchmarks.AddDefaulted_GetRef();
        StoredPin.WorldLocation = Pin.WorldLocation;
        StoredPin.Forward = Pin.Forward;
        StoredPin.Hull = Pin.Hull;
        StoredPin.VisualTheme = Pin.VisualTheme;
        StoredPin.Label = Pin.Label.ToString();
        StoredPin.SourceProfileLabel = Pin.SourceProfileLabel.ToString();
        StoredPin.CapturedResults = Pin.CapturedResults;
        StoredPin.bHasVisualSnapshot = Pin.bHasVisualSnapshot;
        StoredPin.Visibility = Pin.Visibility;
        StoredPin.SurfaceNormal = Pin.SurfaceNormal;
        StoredPin.AnalysisUp = Pin.AnalysisUp;
        StoredPin.ClearanceSettings = Pin.ClearanceSettings;
        StoredPin.OverheadClearanceSettings = Pin.OverheadClearanceSettings;
        StoredPin.Sensors = Pin.Sensors;
        StoredPin.SensorFieldSettings = Pin.SensorFieldSettings;
        StoredPin.Ranges = Pin.Ranges;
        StoredPin.Locomotion = Pin.Locomotion;
        StoredPin.TrajectorySettings = Pin.TrajectorySettings;
        StoredPin.CameraProbeSettings = Pin.CameraProbeSettings;
        StoredPin.CameraSightlineSettings = Pin.CameraSightlineSettings;
        StoredPin.GroundValidationSettings = Pin.GroundValidationSettings;
        StoredPin.FitAnalysisSettings = Pin.FitAnalysisSettings;
        StoredPin.ThresholdAnalysisSettings = Pin.ThresholdAnalysisSettings;
        StoredPin.TurnClearanceSettings = Pin.TurnClearanceSettings;
        StoredPin.LedgeAnalysisSettings = Pin.LedgeAnalysisSettings;
        StoredPin.StoppingDistanceSettings = Pin.StoppingDistanceSettings;
        StoredPin.CoverExposureSettings = Pin.CoverExposureSettings;
        StoredPin.JumpRunwaySettings = Pin.JumpRunwaySettings;
        StoredPin.InteractionReachSettings = Pin.InteractionReachSettings;
        StoredPin.ShoulderClearanceSettings = Pin.ShoulderClearanceSettings;
        StoredPin.CombatDensitySettings = Pin.CombatDensitySettings;
        StoredPin.AlternateStanceSettings = Pin.AlternateStanceSettings;
        StoredPin.MantleVaultSettings = Pin.MantleVaultSettings;
        StoredPin.RecoverySpaceSettings = Pin.RecoverySpaceSettings;
        StoredPin.CrossfireSettings = Pin.CrossfireSettings;
    }

    Settings->SaveConfig();
}

void UMetricSubsystem::LoadPinnedBenchmarksForWorld(UWorld* World)
{
    PinnedBenchmarks.Reset();

    const FString MapPackageName = GetPersistableMapPackageName(World);
    ActivePinnedBenchmarkMap = MapPackageName;
    if (MapPackageName.IsEmpty() || !ArePinnedBenchmarksPersistent())
    {
        LiveState.PinnedBenchmarks.Reset();
        return;
    }

    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    if (!Settings)
    {
        return;
    }

    const int32 MaxPins = FMath::Clamp(Settings->MaxPinnedBenchmarks, 1, 32);
    for (const FMetricPersistentBenchmarkMap& StoredMap : Settings->PersistentBenchmarkMaps)
    {
        if (StoredMap.MapPackageName != MapPackageName)
        {
            continue;
        }

        const int32 StartIndex = FMath::Max(0, StoredMap.Benchmarks.Num() - MaxPins);
        for (int32 Index = StartIndex; Index < StoredMap.Benchmarks.Num(); ++Index)
        {
            const FMetricPersistentBenchmark& StoredPin = StoredMap.Benchmarks[Index];
            FMetricPinnedBenchmark& Pin = PinnedBenchmarks.AddDefaulted_GetRef();
            Pin.WorldLocation = StoredPin.WorldLocation;
            Pin.Forward = StoredPin.Forward.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
            Pin.Hull = StoredPin.Hull;
            Pin.VisualTheme = StoredPin.VisualTheme;
            Pin.Label = FText::FromString(StoredPin.Label);
            Pin.SourceProfileLabel = StoredPin.SourceProfileLabel.IsEmpty() ? Pin.Label : FText::FromString(StoredPin.SourceProfileLabel);
            Pin.CapturedResults = StoredPin.CapturedResults;
            Pin.bHasVisualSnapshot = StoredPin.bHasVisualSnapshot;
            Pin.Visibility = StoredPin.Visibility;
            Pin.SurfaceNormal = StoredPin.SurfaceNormal;
            Pin.AnalysisUp = StoredPin.AnalysisUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
            Pin.ClearanceSettings = StoredPin.ClearanceSettings;
            Pin.OverheadClearanceSettings = StoredPin.OverheadClearanceSettings;
            Pin.Sensors = StoredPin.Sensors;
            Pin.SensorFieldSettings = StoredPin.SensorFieldSettings;
            Pin.Ranges = StoredPin.Ranges;
            Pin.Locomotion = StoredPin.Locomotion;
            Pin.TrajectorySettings = StoredPin.TrajectorySettings;
            Pin.CameraProbeSettings = StoredPin.CameraProbeSettings;
            Pin.CameraSightlineSettings = StoredPin.CameraSightlineSettings;
            Pin.GroundValidationSettings = StoredPin.GroundValidationSettings;
            Pin.FitAnalysisSettings = StoredPin.FitAnalysisSettings;
            Pin.ThresholdAnalysisSettings = StoredPin.ThresholdAnalysisSettings;
            Pin.TurnClearanceSettings = StoredPin.TurnClearanceSettings;
            Pin.LedgeAnalysisSettings = StoredPin.LedgeAnalysisSettings;
            Pin.StoppingDistanceSettings = StoredPin.StoppingDistanceSettings;
            Pin.CoverExposureSettings = StoredPin.CoverExposureSettings;
            Pin.JumpRunwaySettings = StoredPin.JumpRunwaySettings;
            Pin.InteractionReachSettings = StoredPin.InteractionReachSettings;
            Pin.ShoulderClearanceSettings = StoredPin.ShoulderClearanceSettings;
            Pin.CombatDensitySettings = StoredPin.CombatDensitySettings;
            Pin.AlternateStanceSettings = StoredPin.AlternateStanceSettings;
            Pin.MantleVaultSettings = StoredPin.MantleVaultSettings;
            Pin.RecoverySpaceSettings = StoredPin.RecoverySpaceSettings;
            Pin.CrossfireSettings = StoredPin.CrossfireSettings;
        }
        break;
    }

    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    bHasSelectedPinnedEvaluatedState = false;
    SelectedPinnedEvaluatedState = FMetricLiveState();
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    LiveState.SelectedPinnedBenchmarkIndex = INDEX_NONE;
}

void UMetricSubsystem::EnsurePinnedBenchmarkMap(UWorld* World)
{
    const FString MapPackageName = GetPersistableMapPackageName(World);
    if (MapPackageName == ActivePinnedBenchmarkMap)
    {
        return;
    }

    LoadPinnedBenchmarksForWorld(World);
}

void UMetricSubsystem::ToggleMetric()
{
    SetMetricActive(!bMetricActive);
}

void UMetricSubsystem::SetMetricActive(bool bActive)
{
    if (bMetricActive == bActive)
    {
        return;
    }

    bMetricActive = bActive;

    if (bMetricActive)
    {
        RefreshCycleEntries();
    }
    else
    {
        LiveState.bHasHit = false;
        if (AMetricRenderHostActor* Host = RenderHost.Get())
        {
            Host->SetMetricVisible(false);
        }
    }

    NotifyStateChanged();
}

void UMetricSubsystem::CycleNext()
{
    RefreshCycleEntries();
    if (CycleEntries.IsEmpty())
    {
        ActiveIndex = 0;
        return;
    }

    ActiveIndex = (ActiveIndex + 1) % CycleEntries.Num();
    bMetricActive = true;
    ApplyEntryToState(LiveState);
    NotifyStateChanged();
}

void UMetricSubsystem::PinCurrentBenchmark()
{
    if (!bMetricActive || !LiveState.bHasHit || LiveState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    const int32 MaxPins = FMath::Clamp(Settings ? Settings->MaxPinnedBenchmarks : 8, 1, 32);
    if (PinnedBenchmarks.Num() >= MaxPins)
    {
        PinnedBenchmarks.RemoveAt(0, PinnedBenchmarks.Num() - MaxPins + 1, EAllowShrinking::No);
    }

    FMetricPinnedBenchmark Pin;
    Pin.WorldLocation = LiveState.Location;
    Pin.SurfaceNormal = LiveState.SurfaceNormal;
    Pin.AnalysisUp = LiveState.AnalysisUp;
    Pin.Forward = LiveState.Forward;
    Pin.Hull = LiveState.Hull;
    Pin.VisualTheme = LiveState.VisualTheme;
    Pin.Label = LiveState.Label;
    Pin.SourceProfileLabel = LiveState.Label;
    Pin.CapturedResults = MetricBenchmark::BuildCapturedResults(LiveState);
    Pin.bHasVisualSnapshot = true;
    if (Settings) Pin.Visibility = Settings->LiveVisibility;
    Pin.ClearanceSettings = LiveState.ClearanceSettings;
    Pin.OverheadClearanceSettings = LiveState.OverheadClearanceSettings;
    Pin.Sensors = LiveState.Sensors;
    Pin.SensorFieldSettings = LiveState.SensorFieldSettings;
    Pin.Ranges = LiveState.Ranges;
    Pin.Locomotion = LiveState.Locomotion;
    Pin.TrajectorySettings = LiveState.TrajectorySettings;
    Pin.CameraProbeSettings = LiveState.CameraProbeSettings;
    Pin.CameraSightlineSettings = LiveState.CameraSightlineSettings;
    Pin.GroundValidationSettings = LiveState.GroundValidationSettings;
    Pin.FitAnalysisSettings = LiveState.FitAnalysisSettings;
    Pin.ThresholdAnalysisSettings = LiveState.ThresholdAnalysisSettings;
    Pin.TurnClearanceSettings = LiveState.TurnClearanceSettings;
    Pin.LedgeAnalysisSettings = LiveState.LedgeAnalysisSettings;
    Pin.StoppingDistanceSettings = LiveState.StoppingDistanceSettings;
    Pin.CoverExposureSettings = LiveState.CoverExposureSettings;
    Pin.JumpRunwaySettings = LiveState.JumpRunwaySettings;
    Pin.InteractionReachSettings = LiveState.InteractionReachSettings;
    Pin.ShoulderClearanceSettings = LiveState.ShoulderClearanceSettings;
    Pin.CombatDensitySettings = LiveState.CombatDensitySettings;
    Pin.AlternateStanceSettings = LiveState.AlternateStanceSettings;
    Pin.MantleVaultSettings = LiveState.MantleVaultSettings;
    Pin.RecoverySpaceSettings = LiveState.RecoverySpaceSettings;
    Pin.CrossfireSettings = LiveState.CrossfireSettings;
    PinnedBenchmarks.Add(MoveTemp(Pin));
    SelectedPinnedBenchmarkIndex = PinnedBenchmarks.Num() - 1;

    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }

    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    LiveState.SelectedPinnedBenchmarkIndex = SelectedPinnedBenchmarkIndex;
    if (AMetricRenderHostActor* Host = RenderHost.Get())
    {
        Host->ApplyMetricState(LiveState);
    }
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::ClearPinnedBenchmarks()
{
    // Do not early-out when the runtime array is empty. A saved map may still
    // have persistent pins that the user is explicitly asking to clear.
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    bHasSelectedPinnedEvaluatedState = false;
    SelectedPinnedEvaluatedState = FMetricLiveState();
    LiveState.PinnedBenchmarks.Reset();
    LiveState.SelectedPinnedBenchmarkIndex = INDEX_NONE;
    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }
    if (AMetricRenderHostActor* Host = RenderHost.Get())
    {
        Host->ApplyMetricState(LiveState);
        Host->ApplyPinnedMetricState(nullptr);
    }
    NotifyStateChanged();
}

const FMetricPinnedBenchmark* UMetricSubsystem::GetPinnedBenchmark(int32 Index) const
{
    return PinnedBenchmarks.IsValidIndex(Index) ? &PinnedBenchmarks[Index] : nullptr;
}

void UMetricSubsystem::SelectPinnedBenchmark(int32 Index)
{
    SelectedPinnedBenchmarkIndex = PinnedBenchmarks.IsValidIndex(Index) ? Index : INDEX_NONE;
    LiveState.SelectedPinnedBenchmarkIndex = SelectedPinnedBenchmarkIndex;
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::CyclePinnedBenchmarkSelection()
{
    if (PinnedBenchmarks.IsEmpty())
    {
        SelectPinnedBenchmark(INDEX_NONE);
        return;
    }

    // None is part of the cycle so designers can quickly return to a live-only view.
    const int32 NextIndex = SelectedPinnedBenchmarkIndex == INDEX_NONE
        ? 0
        : (SelectedPinnedBenchmarkIndex + 1 < PinnedBenchmarks.Num() ? SelectedPinnedBenchmarkIndex + 1 : INDEX_NONE);
    SelectPinnedBenchmark(NextIndex);
}

void UMetricSubsystem::DeletePinnedBenchmark(int32 Index)
{
    if (!PinnedBenchmarks.IsValidIndex(Index)) return;
    PinnedBenchmarks.RemoveAt(Index);
    if (SelectedPinnedBenchmarkIndex == Index) SelectedPinnedBenchmarkIndex = INDEX_NONE;
    else if (SelectedPinnedBenchmarkIndex > Index) --SelectedPinnedBenchmarkIndex;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    LiveState.SelectedPinnedBenchmarkIndex = SelectedPinnedBenchmarkIndex;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::RenamePinnedBenchmark(int32 Index, const FText& NewLabel)
{
    if (!PinnedBenchmarks.IsValidIndex(Index)) return;
    const FString Trimmed = NewLabel.ToString().TrimStartAndEnd();
    PinnedBenchmarks[Index].Label = Trimmed.IsEmpty() ? FText::FromString(FString::Printf(TEXT("Benchmark %d"), Index + 1)) : FText::FromString(Trimmed);
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    NotifyStateChanged();
}

void UMetricSubsystem::ReplacePinnedBenchmarkWithLive(int32 Index)
{
    if (!PinnedBenchmarks.IsValidIndex(Index) || !bMetricActive || !LiveState.bHasHit || LiveState.EntryType == EMetricCycleEntryType::NativePreviewMesh) return;
    FText ExistingLabel = PinnedBenchmarks[Index].Label;
    PinnedBenchmarks[Index].WorldLocation = LiveState.Location;
    PinnedBenchmarks[Index].SurfaceNormal = LiveState.SurfaceNormal;
    PinnedBenchmarks[Index].AnalysisUp = LiveState.AnalysisUp;
    PinnedBenchmarks[Index].Forward = LiveState.Forward;
    PinnedBenchmarks[Index].Hull = LiveState.Hull;
    PinnedBenchmarks[Index].VisualTheme = LiveState.VisualTheme;
    PinnedBenchmarks[Index].Label = ExistingLabel;
    PinnedBenchmarks[Index].SourceProfileLabel = LiveState.Label;
    PinnedBenchmarks[Index].CapturedResults = MetricBenchmark::BuildCapturedResults(LiveState);
    PinnedBenchmarks[Index].bHasVisualSnapshot = true;
    PinnedBenchmarks[Index].ClearanceSettings = LiveState.ClearanceSettings;
    PinnedBenchmarks[Index].OverheadClearanceSettings = LiveState.OverheadClearanceSettings;
    PinnedBenchmarks[Index].Sensors = LiveState.Sensors;
    PinnedBenchmarks[Index].SensorFieldSettings = LiveState.SensorFieldSettings;
    PinnedBenchmarks[Index].Ranges = LiveState.Ranges;
    PinnedBenchmarks[Index].Locomotion = LiveState.Locomotion;
    PinnedBenchmarks[Index].TrajectorySettings = LiveState.TrajectorySettings;
    PinnedBenchmarks[Index].CameraProbeSettings = LiveState.CameraProbeSettings;
    PinnedBenchmarks[Index].CameraSightlineSettings = LiveState.CameraSightlineSettings;
    PinnedBenchmarks[Index].GroundValidationSettings = LiveState.GroundValidationSettings;
    PinnedBenchmarks[Index].FitAnalysisSettings = LiveState.FitAnalysisSettings;
    PinnedBenchmarks[Index].ThresholdAnalysisSettings = LiveState.ThresholdAnalysisSettings;
    PinnedBenchmarks[Index].TurnClearanceSettings = LiveState.TurnClearanceSettings;
    PinnedBenchmarks[Index].LedgeAnalysisSettings = LiveState.LedgeAnalysisSettings;
    PinnedBenchmarks[Index].StoppingDistanceSettings = LiveState.StoppingDistanceSettings;
    PinnedBenchmarks[Index].CoverExposureSettings = LiveState.CoverExposureSettings;
    PinnedBenchmarks[Index].JumpRunwaySettings = LiveState.JumpRunwaySettings;
    PinnedBenchmarks[Index].InteractionReachSettings = LiveState.InteractionReachSettings;
    PinnedBenchmarks[Index].ShoulderClearanceSettings = LiveState.ShoulderClearanceSettings;
    PinnedBenchmarks[Index].CombatDensitySettings = LiveState.CombatDensitySettings;
    PinnedBenchmarks[Index].AlternateStanceSettings = LiveState.AlternateStanceSettings;
    PinnedBenchmarks[Index].MantleVaultSettings = LiveState.MantleVaultSettings;
    PinnedBenchmarks[Index].RecoverySpaceSettings = LiveState.RecoverySpaceSettings;
    PinnedBenchmarks[Index].CrossfireSettings = LiveState.CrossfireSettings;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::SetPinnedBenchmarkVisibility(int32 Index, const FMetricLayerVisibility& Visibility)
{
    if (!PinnedBenchmarks.IsValidIndex(Index)) return;
    PinnedBenchmarks[Index].Visibility = Visibility;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::FocusPinnedBenchmark(int32 Index)
{
    if (!PinnedBenchmarks.IsValidIndex(Index) || !GEditor) return;
    SelectPinnedBenchmark(Index);
    FViewport* ActiveViewport = GEditor->GetActiveViewport();
    if (!ActiveViewport) return;
    FViewportClient* ActiveClient = ActiveViewport->GetClient();
    for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
    {
        if (!Client || Client != ActiveClient || !Client->IsPerspective()) continue;
        const FMetricPinnedBenchmark& Pin = PinnedBenchmarks[Index];
        FVector Forward = Pin.Forward.GetSafeNormal();
        if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
        const FVector Target = Pin.WorldLocation + FVector::UpVector * FMath::Max(50.0f, Pin.Hull.Height * 0.5f);
        const FVector Camera = Target - Forward * FMath::Max(450.0f, Pin.Hull.Height * 2.0f) + FVector::UpVector * FMath::Max(120.0f, Pin.Hull.Height * 0.4f);
        Client->SetViewLocation(Camera);
        Client->SetViewRotation((Target - Camera).Rotation());
        Client->Invalidate();
        break;
    }
}

void UMetricSubsystem::RefreshCycleEntries()
{
    FSoftObjectPath PreviousIdentity;
    EMetricCycleEntryType PreviousType = EMetricCycleEntryType::BuiltInMetric;
    if (CycleEntries.IsValidIndex(ActiveIndex))
    {
        PreviousType = CycleEntries[ActiveIndex].Type;
        PreviousIdentity = PreviousType == EMetricCycleEntryType::MetricProfile
            ? CycleEntries[ActiveIndex].Profile.ToSoftObjectPath()
            : CycleEntries[ActiveIndex].NativePreviewMesh;
    }

    CycleEntries.Reset();

    const UMetricProjectSettings* ProjectSettings = GetDefault<UMetricProjectSettings>();
    if (!ProjectSettings || (ProjectSettings->bIncludeBuiltInHumanReference && ProjectSettings->RegisteredProfiles.IsEmpty()))
    {
        FMetricCycleEntry& BuiltIn = CycleEntries.AddDefaulted_GetRef();
        BuiltIn.Type = EMetricCycleEntryType::BuiltInMetric;
        BuiltIn.DisplayName = LOCTEXT("BuiltInHuman", "Human Reference");
    }

    if (ProjectSettings)
    {
        for (const TSoftObjectPtr<UMetricProfile>& ProfilePtr : ProjectSettings->RegisteredProfiles)
        {
            if (ProfilePtr.IsNull())
            {
                continue;
            }

            FMetricCycleEntry& Entry = CycleEntries.AddDefaulted_GetRef();
            Entry.Type = EMetricCycleEntryType::MetricProfile;
            Entry.Profile = ProfilePtr;
            if (const UMetricProfile* LoadedPreset = ProfilePtr.LoadSynchronous())
            {
                Entry.DisplayName = LoadedPreset->DisplayName.IsEmpty()
                    ? FText::FromString(LoadedPreset->GetName())
                    : LoadedPreset->DisplayName;
            }
            else
            {
                Entry.DisplayName = FText::FromString(ProfilePtr.ToSoftObjectPath().GetAssetName());
            }
        }
    }

    const UMetricEditorSettings* EditorSettings = GetDefault<UMetricEditorSettings>();
    if (!EditorSettings || EditorSettings->bIncludeUnrealPreviewMeshes)
    {
        const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();
        if (ViewportSettings)
        {
            for (const FSoftObjectPath& MeshPath : ViewportSettings->PreviewMeshes)
            {
                if (!MeshPath.IsValid())
                {
                    continue;
                }

                FMetricCycleEntry& Entry = CycleEntries.AddDefaulted_GetRef();
                Entry.Type = EMetricCycleEntryType::NativePreviewMesh;
                Entry.NativePreviewMesh = MeshPath;
                Entry.DisplayName = FText::FromString(MeshPath.GetAssetName());
            }
        }
    }

    ActiveIndex = FMath::Clamp(ActiveIndex, 0, FMath::Max(0, CycleEntries.Num() - 1));

    if (PreviousIdentity.IsValid())
    {
        for (int32 Index = 0; Index < CycleEntries.Num(); ++Index)
        {
            const FMetricCycleEntry& Candidate = CycleEntries[Index];
            const FSoftObjectPath CandidateIdentity = Candidate.Type == EMetricCycleEntryType::MetricProfile
                ? Candidate.Profile.ToSoftObjectPath()
                : Candidate.NativePreviewMesh;
            if (Candidate.Type == PreviousType && CandidateIdentity == PreviousIdentity)
            {
                ActiveIndex = Index;
                break;
            }
        }
    }

    ApplyEntryToState(LiveState);
}

const FMetricCycleEntry* UMetricSubsystem::GetActiveEntry() const
{
    return CycleEntries.IsValidIndex(ActiveIndex) ? &CycleEntries[ActiveIndex] : nullptr;
}

void UMetricSubsystem::ApplyEntryToState(FMetricLiveState& InOutState) const
{
    InOutState.Hull = FMetricHullSettings();
    InOutState.VisualTheme = FMetricVisualizationTheme();
    InOutState.ClearanceSettings = FMetricClearanceSettings();
    InOutState.Clearance = FMetricClearanceResult();
    InOutState.OverheadClearanceSettings = FMetricOverheadClearanceSettings();
    InOutState.OverheadClearance = FMetricOverheadClearanceResult();
    InOutState.Sensors = FMetricSensorSettings();
    InOutState.SensorFieldSettings = FMetricSensorFieldSettings();
    InOutState.SensorField = FMetricSensorFieldResult();
    InOutState.Ranges = FMetricRangeSettings();
    InOutState.Locomotion = FMetricLocomotionSettings();
    InOutState.TrajectorySettings = FMetricTrajectorySettings();
    InOutState.Trajectory = FMetricTrajectoryResult();
    InOutState.CameraProbeSettings = FMetricCameraProbeSettings();
    InOutState.CameraProbe = FMetricCameraProbeResult();
    InOutState.CameraSightlineSettings = FMetricCameraSightlineSettings();
    InOutState.CameraSightline = FMetricCameraSightlineResult();
    InOutState.GroundValidationSettings = FMetricGroundValidationSettings();
    InOutState.GroundValidation = FMetricGroundValidationResult();
    InOutState.FitAnalysisSettings = FMetricFitAnalysisSettings();
    InOutState.FitAnalysis = FMetricFitAnalysisResult();
    InOutState.ThresholdAnalysisSettings = FMetricThresholdAnalysisSettings();
    InOutState.TurnClearanceSettings = FMetricTurnClearanceSettings();
    InOutState.TurnClearance = FMetricTurnClearanceResult();
    InOutState.LedgeAnalysisSettings = FMetricLedgeAnalysisSettings();
    InOutState.StoppingDistanceSettings = FMetricStoppingDistanceSettings();
    InOutState.LedgeAnalysis = FMetricLedgeAnalysisResult();
    InOutState.StoppingDistance = FMetricStoppingDistanceResult();
    InOutState.CoverExposureSettings = FMetricCoverExposureSettings();
    InOutState.CoverExposure = FMetricCoverExposureResult();
    InOutState.JumpRunwaySettings = FMetricJumpRunwaySettings();
    InOutState.JumpRunway = FMetricJumpRunwayResult();
    InOutState.InteractionReachSettings = FMetricInteractionReachSettings();
    InOutState.InteractionReach = FMetricInteractionReachResult();
    InOutState.ShoulderClearanceSettings = FMetricShoulderClearanceSettings();
    InOutState.ShoulderClearance = FMetricShoulderClearanceResult();
    InOutState.CombatDensitySettings = FMetricCombatDensitySettings();
    InOutState.CombatDensity = FMetricCombatDensityResult();
    InOutState.AlternateStanceSettings = FMetricAlternateStanceSettings();
    InOutState.AlternateStance = FMetricAlternateStanceResult();
    InOutState.MantleVaultSettings = FMetricMantleVaultSettings();
    InOutState.MantleVault = FMetricMantleVaultResult();
    InOutState.RecoverySpaceSettings = FMetricRecoverySpaceSettings();
    InOutState.RecoverySpace = FMetricRecoverySpaceResult();
    InOutState.CrossfireSettings = FMetricCrossfireSettings();
    InOutState.Crossfire = FMetricCrossfireResult();
    InOutState.ThresholdAnalysis = FMetricThresholdAnalysisResult();
    InOutState.bShowHull = true;
    InOutState.bShowForward = true;
    InOutState.PreviewMesh = FSoftObjectPath();
    InOutState.PreviewRelativeTransform = FTransform::Identity;
    InOutState.EntryType = EMetricCycleEntryType::BuiltInMetric;
    InOutState.Label = LOCTEXT("MetricNoEntry", "METRIC");

    const FMetricCycleEntry* Entry = GetActiveEntry();
    if (!Entry)
    {
        return;
    }

    InOutState.EntryType = Entry->Type;

    if (Entry->Type == EMetricCycleEntryType::NativePreviewMesh)
    {
        InOutState.Label = FText::Format(LOCTEXT("NativePreviewLabel", "PREVIEW MESH - {0}"), Entry->DisplayName);
        InOutState.bShowHull = false;
        InOutState.bShowForward = false;
        InOutState.ClearanceSettings.bShowClearanceProbe = false;
        InOutState.OverheadClearanceSettings.bShowOverheadClearance = false;
        InOutState.Sensors.bShowSensorHeights = false;
        InOutState.SensorFieldSettings.bShowSensorField = false;
        InOutState.Ranges.bShowEngagementRanges = false;
        InOutState.Locomotion.bShowLocomotionBands = false;
        InOutState.TrajectorySettings.bShowTrajectory = false;
        InOutState.CameraProbeSettings.bShowCameraProbe = false;
        InOutState.CameraSightlineSettings.bShowCameraSightline = false;
        InOutState.GroundValidationSettings.bShowGroundValidation = false;
        InOutState.FitAnalysisSettings.bShowFitAnalysis = false;
        InOutState.ThresholdAnalysisSettings.bShowThresholdAnalysis = false;
        InOutState.TurnClearanceSettings.bShowTurnClearance = false;
        InOutState.LedgeAnalysisSettings.bShowLedgeAnalysis = false;
        InOutState.StoppingDistanceSettings.bShowStoppingDistance = false;
        InOutState.CoverExposureSettings.bShowCoverExposure = false;
        InOutState.JumpRunwaySettings.bShowJumpRunway = false;
        InOutState.InteractionReachSettings.bShowInteractionReach = false;
        InOutState.ShoulderClearanceSettings.bShowShoulderClearance = false;
        InOutState.CombatDensitySettings.bShowCombatDensity = false;
        InOutState.AlternateStanceSettings.bShowAlternateStance = false;
        InOutState.MantleVaultSettings.bShowMantleVault = false;
        InOutState.RecoverySpaceSettings.bShowRecoverySpace = false;
        InOutState.CrossfireSettings.bShowCrossfire = false;
        InOutState.PreviewMesh = Entry->NativePreviewMesh;
        return;
    }

    if (Entry->Type == EMetricCycleEntryType::MetricProfile)
    {
        if (const UMetricProfile* Preset = Entry->Profile.LoadSynchronous())
        {
            InOutState.Hull = Preset->Hull;
            InOutState.VisualTheme = Preset->VisualTheme;
            InOutState.ClearanceSettings = Preset->Clearance;
            InOutState.OverheadClearanceSettings = Preset->OverheadClearance;
            InOutState.Sensors = Preset->Sensors;
            InOutState.SensorFieldSettings = Preset->SensorField;
            InOutState.Ranges = Preset->EngagementRanges;
            InOutState.Locomotion = Preset->LocomotionBands;
            InOutState.TrajectorySettings = Preset->Trajectory;
            InOutState.CameraProbeSettings = Preset->CameraProbe;
            InOutState.CameraSightlineSettings = Preset->CameraSightline;
            InOutState.GroundValidationSettings = Preset->GroundValidation;
            InOutState.FitAnalysisSettings = Preset->FitAnalysis;
            InOutState.ThresholdAnalysisSettings = Preset->ThresholdAnalysis;
            InOutState.TurnClearanceSettings = Preset->TurnClearance;
            InOutState.LedgeAnalysisSettings = Preset->LedgeAnalysis;
            InOutState.StoppingDistanceSettings = Preset->StoppingDistance;
            InOutState.CoverExposureSettings = Preset->CoverExposure;
            InOutState.JumpRunwaySettings = Preset->JumpRunway;
            InOutState.InteractionReachSettings = Preset->InteractionReach;
            InOutState.ShoulderClearanceSettings = Preset->ShoulderClearance;
            InOutState.CombatDensitySettings = Preset->CombatDensity;
            InOutState.AlternateStanceSettings = Preset->AlternateStance;
            InOutState.MantleVaultSettings = Preset->MantleVault;
            InOutState.RecoverySpaceSettings = Preset->RecoverySpace;
            InOutState.CrossfireSettings = Preset->Crossfire;
            InOutState.Label = FText::Format(LOCTEXT("PresetLabel", "METRIC - {0}"), Entry->DisplayName);
            if (Preset->Representation.bShowStaticMesh)
            {
                InOutState.PreviewMesh = Preset->Representation.StaticMesh.ToSoftObjectPath();
                InOutState.PreviewRelativeTransform = Preset->Representation.RelativeTransform;
            }
            return;
        }
    }

    InOutState.Label = FText::Format(LOCTEXT("BuiltInLabel", "METRIC - {0}"), Entry->DisplayName);
}

void UMetricSubsystem::EvaluateProfileAtLocation(
    UWorld* World,
    const UMetricProfile* Preset,
    const FVector& Location,
    const FVector& Forward,
    const FVector& SurfaceNormal,
    const FVector& AnalysisUp,
    FMetricLiveState& OutState) const
{
    OutState = FMetricLiveState();
    if (!World || !Preset)
    {
        return;
    }

    OutState.bHasHit = true;
    OutState.Location = Location;
    OutState.Forward = Forward.GetSafeNormal();
    if (OutState.Forward.IsNearlyZero()) OutState.Forward = FVector::ForwardVector;
    OutState.SurfaceNormal = SurfaceNormal.GetSafeNormal();
    if (OutState.SurfaceNormal.IsNearlyZero()) OutState.SurfaceNormal = FVector::UpVector;
    OutState.AnalysisUp = AnalysisUp.GetSafeNormal();
    if (OutState.AnalysisUp.IsNearlyZero()) OutState.AnalysisUp = FVector::UpVector;
    OutState.AnalysisForward = FVector::VectorPlaneProject(OutState.Forward, OutState.AnalysisUp).GetSafeNormal();
    if (OutState.AnalysisForward.IsNearlyZero())
    {
        const FVector Fallback = FMath::Abs(FVector::DotProduct(OutState.AnalysisUp, FVector::ForwardVector)) < 0.98f
            ? FVector::ForwardVector : FVector::RightVector;
        OutState.AnalysisForward = FVector::VectorPlaneProject(Fallback, OutState.AnalysisUp).GetSafeNormal();
    }
    OutState.AnalysisRight = FVector::CrossProduct(OutState.AnalysisUp, OutState.AnalysisForward).GetSafeNormal();
    OutState.ViewerLocation = Location - OutState.AnalysisForward * 500.0f + OutState.AnalysisUp * 200.0f;
    OutState.EntryType = EMetricCycleEntryType::MetricProfile;
    OutState.Label = FText::Format(LOCTEXT("MetricActorPresetLabel", "METRIC ACTOR - {0}"),
        Preset->DisplayName.IsEmpty() ? FText::FromString(Preset->GetName()) : Preset->DisplayName);

    OutState.Hull = Preset->Hull;
    OutState.VisualTheme = Preset->VisualTheme;
    OutState.ClearanceSettings = Preset->Clearance;
    OutState.OverheadClearanceSettings = Preset->OverheadClearance;
    OutState.Sensors = Preset->Sensors;
    OutState.SensorFieldSettings = Preset->SensorField;
    OutState.Ranges = Preset->EngagementRanges;
    OutState.Locomotion = Preset->LocomotionBands;
    OutState.TrajectorySettings = Preset->Trajectory;
    OutState.CameraProbeSettings = Preset->CameraProbe;
    OutState.CameraSightlineSettings = Preset->CameraSightline;
    OutState.GroundValidationSettings = Preset->GroundValidation;
    OutState.FitAnalysisSettings = Preset->FitAnalysis;
    OutState.ThresholdAnalysisSettings = Preset->ThresholdAnalysis;
    OutState.TurnClearanceSettings = Preset->TurnClearance;
    OutState.LedgeAnalysisSettings = Preset->LedgeAnalysis;
    OutState.StoppingDistanceSettings = Preset->StoppingDistance;
    OutState.CoverExposureSettings = Preset->CoverExposure;
    OutState.JumpRunwaySettings = Preset->JumpRunway;
    OutState.InteractionReachSettings = Preset->InteractionReach;
    OutState.ShoulderClearanceSettings = Preset->ShoulderClearance;
    OutState.CombatDensitySettings = Preset->CombatDensity;
    OutState.AlternateStanceSettings = Preset->AlternateStance;
    OutState.MantleVaultSettings = Preset->MantleVault;
    OutState.RecoverySpaceSettings = Preset->RecoverySpace;
    OutState.CrossfireSettings = Preset->Crossfire;
    if (Preset->Representation.bShowStaticMesh)
    {
        OutState.PreviewMesh = Preset->Representation.StaticMesh.ToSoftObjectPath();
        OutState.PreviewRelativeTransform = Preset->Representation.RelativeTransform;
    }

    MetricPerformance::BeginGeneration();
#define METRIC_PERF_JOB(Name, Expr) { MetricPerformance::FScopedAnalysis Scope(TEXT(Name)); Expr; }
    METRIC_PERF_JOB("Clearance", EvaluateClearance(World, OutState));
    METRIC_PERF_JOB("Overhead", EvaluateOverheadClearance(World, OutState));
    METRIC_PERF_JOB("SensorField", EvaluateSensorField(World, OutState));
    METRIC_PERF_JOB("Trajectory", EvaluateTrajectory(World, OutState));
    METRIC_PERF_JOB("CameraProbe", EvaluateCameraProbe(World, OutState));
    METRIC_PERF_JOB("CameraSightline", EvaluateCameraSightline(World, OutState));
    METRIC_PERF_JOB("Ground", EvaluateGroundValidation(World, OutState));
    METRIC_PERF_JOB("Fit", EvaluateFitAnalysis(World, OutState));
    METRIC_PERF_JOB("Threshold", EvaluateThresholdAnalysis(World, OutState));
    METRIC_PERF_JOB("TurnClearance", EvaluateTurnClearance(World, OutState));
    METRIC_PERF_JOB("Ledge", EvaluateLedgeAnalysis(World, OutState));
    METRIC_PERF_JOB("StoppingDistance", EvaluateStoppingDistance(World, OutState));
    METRIC_PERF_JOB("CoverExposure", EvaluateCoverExposure(World, OutState));
    METRIC_PERF_JOB("JumpRunway", EvaluateJumpRunway(World, OutState));
    METRIC_PERF_JOB("InteractionReach", EvaluateInteractionReach(World, OutState));
    METRIC_PERF_JOB("ShoulderClearance", EvaluateShoulderClearance(World, OutState));
    METRIC_PERF_JOB("CombatDensity", EvaluateCombatDensity(World, OutState));
    METRIC_PERF_JOB("AlternateStance", EvaluateAlternateStance(World, OutState));
    METRIC_PERF_JOB("MantleVault", EvaluateMantleVault(World, OutState));
    METRIC_PERF_JOB("RecoverySpace", EvaluateRecoverySpace(World, OutState));
    METRIC_PERF_JOB("Crossfire", EvaluateCrossfire(World, OutState));
    METRIC_PERF_JOB("QueryEvidence", BuildQueryEvidence(OutState));
#undef METRIC_PERF_JOB
    MetricPerformance::EndGeneration();
}

void UMetricSubsystem::PublishLiveState(UWorld* World)
{
    MetricPerformance::FScopedPipelineStage PublishScope(TEXT("Publish.Total"));
    {
        MetricPerformance::FScopedPipelineStage Scope(TEXT("Publish.EnsureRenderHost"));
        EnsureRenderHost(World);
    }

    if (AMetricRenderHostActor* Host = RenderHost.Get())
    {
        FMetricLiveState RenderState = LiveState;
        if (const UMetricEditorSettings* EditorSettings = GetDefault<UMetricEditorSettings>())
        {
            FMetricLayerVisibility EffectiveVisibility = EditorSettings->LiveVisibility;
            if (!EditorSettings->bShowLiveVisualizer)
            {
                EffectiveVisibility.bHullVisual = false;
                EffectiveVisibility.bClearanceVisual = false;
                EffectiveVisibility.bOverheadVisual = false;
                EffectiveVisibility.bGroundVisual = false;
                EffectiveVisibility.bFitVisual = false;
                EffectiveVisibility.bThresholdVisual = false;
                EffectiveVisibility.bSensorsVisual = false;
                EffectiveVisibility.bSensorFieldVisual = false;
                EffectiveVisibility.bRangesVisual = false;
                EffectiveVisibility.bLocomotionVisual = false;
                EffectiveVisibility.bTrajectoryVisual = false;
                EffectiveVisibility.bCameraVisual = false;
                EffectiveVisibility.bSightlineVisual = false;
                EffectiveVisibility.bLedgeVisual = false;
                EffectiveVisibility.bStoppingVisual = false;
            }
            MetricBenchmark::ApplyVisualVisibility(RenderState, EffectiveVisibility);
            if (!EditorSettings->bShowLiveVisualizer)
            {
                RenderState.bHasHit = false;
            }
        }
        {
            MetricPerformance::FScopedPipelineStage Scope(TEXT("Publish.HostApplyMetricState"));
            Host->ApplyMetricState(RenderState);
        }
        {
            MetricPerformance::FScopedPipelineStage Scope(TEXT("Publish.HostVisibility"));
            Host->SetMetricVisible(RenderState.bHasHit || !PinnedBenchmarks.IsEmpty());
        }
    }
    MetricPerformance::RecordPipelinePublish();
}

void UMetricSubsystem::CancelPendingLiveAnalysis()
{
    bLiveAnalysisPending = false;
    PendingLiveJobIndex = 0;
    PendingLiveWorld.Reset();
    PendingLiveState = FMetricLiveState();
}

void UMetricSubsystem::BeginLiveAnalysis(UWorld* World, const FMetricLiveState& NewState)
{
    EnsurePinnedBenchmarkMap(World);
    if (!bMetricActive || !World)
    {
        CancelPendingLiveAnalysis();
        return;
    }

    PendingLiveState = NewState;
    ApplyEntryToState(PendingLiveState);
    PendingLiveState.PinnedBenchmarks = PinnedBenchmarks;
    PendingLiveWorld = World;
    PendingLiveJobIndex = 0;
    bLiveAnalysisPending = true;
    MetricPerformance::BeginGeneration();

    // No placement hit means there is no spatial query work to spread over
    // future frames. Commit the profile/settings immediately so HUD rows remain
    // structurally stable and report N/A instead of retaining stale results.
    if (!PendingLiveState.bHasHit)
    {
        { MetricPerformance::FScopedAnalysis Scope(TEXT("QueryEvidence")); BuildQueryEvidence(PendingLiveState); }
        LiveState = PendingLiveState;
        MetricPerformance::EndGeneration();
        CancelPendingLiveAnalysis();
        PublishLiveState(World);
        RefreshSelectedPinnedVisualization(World);
    }
}

bool UMetricSubsystem::ProcessLiveAnalysisBudget(UWorld* World, double TimeBudgetMs)
{
    if (!bLiveAnalysisPending)
    {
        return true;
    }

    if (!World || PendingLiveWorld.Get() != World || !bMetricActive)
    {
        CancelPendingLiveAnalysis();
        return true;
    }

    const double StartSeconds = FPlatformTime::Seconds();
    bool bRanJob = false;

    while (bLiveAnalysisPending)
    {
        // The budget is deliberately soft. Never split an individual analysis
        // halfway through because that would expose internally inconsistent
        // results. Check elapsed time only between complete analysis jobs.
        if (bRanJob && TimeBudgetMs > 0.0)
        {
            const double ElapsedMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
            if (ElapsedMs >= TimeBudgetMs)
            {
                break;
            }
        }

        switch (PendingLiveJobIndex)
        {
        case 0: { MetricPerformance::FScopedAnalysis Scope(TEXT("Clearance")); EvaluateClearance(World, PendingLiveState); } break;
        case 1: { MetricPerformance::FScopedAnalysis Scope(TEXT("Overhead")); EvaluateOverheadClearance(World, PendingLiveState); } break;
        case 2: { MetricPerformance::FScopedAnalysis Scope(TEXT("Ground")); EvaluateGroundValidation(World, PendingLiveState); } break;
        case 3: { MetricPerformance::FScopedAnalysis Scope(TEXT("Fit")); EvaluateFitAnalysis(World, PendingLiveState); } break;
        case 4: { MetricPerformance::FScopedAnalysis Scope(TEXT("Threshold")); EvaluateThresholdAnalysis(World, PendingLiveState); } break;
        case 5: { MetricPerformance::FScopedAnalysis Scope(TEXT("TurnClearance")); EvaluateTurnClearance(World, PendingLiveState); } break;
        case 6: { MetricPerformance::FScopedAnalysis Scope(TEXT("Ledge")); EvaluateLedgeAnalysis(World, PendingLiveState); } break;
        case 7: { MetricPerformance::FScopedAnalysis Scope(TEXT("StoppingDistance")); EvaluateStoppingDistance(World, PendingLiveState); } break;
        case 8: { MetricPerformance::FScopedAnalysis Scope(TEXT("SensorField")); EvaluateSensorField(World, PendingLiveState); } break;
        case 9: { MetricPerformance::FScopedAnalysis Scope(TEXT("Trajectory")); EvaluateTrajectory(World, PendingLiveState); } break;
        case 10: { MetricPerformance::FScopedAnalysis Scope(TEXT("CameraProbe")); EvaluateCameraProbe(World, PendingLiveState); } break;
        case 11: { MetricPerformance::FScopedAnalysis Scope(TEXT("CameraSightline")); EvaluateCameraSightline(World, PendingLiveState); } break;
        case 12: { MetricPerformance::FScopedAnalysis Scope(TEXT("CoverExposure")); EvaluateCoverExposure(World, PendingLiveState); } break;
        case 13: { MetricPerformance::FScopedAnalysis Scope(TEXT("JumpRunway")); EvaluateJumpRunway(World, PendingLiveState); } break;
        case 14: { MetricPerformance::FScopedAnalysis Scope(TEXT("InteractionReach")); EvaluateInteractionReach(World, PendingLiveState); } break;
        case 15: { MetricPerformance::FScopedAnalysis Scope(TEXT("ShoulderClearance")); EvaluateShoulderClearance(World, PendingLiveState); } break;
        case 16: { MetricPerformance::FScopedAnalysis Scope(TEXT("CombatDensity")); EvaluateCombatDensity(World, PendingLiveState); } break;
        case 17: { MetricPerformance::FScopedAnalysis Scope(TEXT("AlternateStance")); EvaluateAlternateStance(World, PendingLiveState); } break;
        case 18: { MetricPerformance::FScopedAnalysis Scope(TEXT("MantleVault")); EvaluateMantleVault(World, PendingLiveState); } break;
        case 19: { MetricPerformance::FScopedAnalysis Scope(TEXT("RecoverySpace")); EvaluateRecoverySpace(World, PendingLiveState); } break;
        case 20: { MetricPerformance::FScopedAnalysis Scope(TEXT("Crossfire")); EvaluateCrossfire(World, PendingLiveState); } break;
        case 21: { MetricPerformance::FScopedAnalysis Scope(TEXT("QueryEvidence")); BuildQueryEvidence(PendingLiveState); } break;
        default:
            LiveState = PendingLiveState;
            MetricPerformance::EndGeneration();
            CancelPendingLiveAnalysis();
            PublishLiveState(World);
            RefreshSelectedPinnedVisualization(World);
            return true;
        }

        ++PendingLiveJobIndex;
        bRanJob = true;

        if (PendingLiveJobIndex > 21)
        {
            // Publish once for the completed generation. SetMetricState() dirties
            // the primitive render state, so publishing after every analysis job
            // caused dozens of scene-proxy rebuilds for a single cursor sample.
            LiveState = PendingLiveState;
            MetricPerformance::EndGeneration();
            CancelPendingLiveAnalysis();
            PublishLiveState(World);
            RefreshSelectedPinnedVisualization(World);
            return true;
        }
    }

    const double BudgetElapsedMs = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
    MetricPerformance::RecordBudgetFrame(BudgetElapsedMs, TimeBudgetMs);

    // Publish at most once per time-budget slice. This keeps progressive/N/A
    // feedback for genuinely multi-frame generations without rebuilding the
    // render proxy after every individual analysis job.
    if (bRanJob)
    {
        LiveState = PendingLiveState;
        PublishLiveState(World);
    }

    return !bLiveAnalysisPending;
}

void UMetricSubsystem::UpdateLiveState(UWorld* World, const FMetricLiveState& NewState)
{
    BeginLiveAnalysis(World, NewState);
    ProcessLiveAnalysisBudget(World, 0.0);
}


void UMetricSubsystem::UpdateWithLiveAnalysisDisabled(UWorld* World, const FVector& ViewerLocation)
{
    EnsurePinnedBenchmarkMap(World);
    if (!bMetricActive || !World)
    {
        return;
    }

    // Live Metric Off is a real analysis pause, not merely a render toggle.
    // Do not perform cursor placement, live sweeps, traces, or sampling here.
    // Keep only lightweight state needed by the HUD and pinned benchmark view.
    LiveState.bHasHit = false;
    LiveState.ViewerLocation = ViewerLocation;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;

    EnsureRenderHost(World);
    if (AMetricRenderHostActor* Host = RenderHost.Get())
    {
        FMetricLiveState RenderState = LiveState;
        RenderState.bHasHit = false;
        Host->ApplyMetricState(RenderState);
        Host->SetMetricVisible(!PinnedBenchmarks.IsEmpty());
    }

    // A selected benchmark remains a live stationary analysis probe even when
    // the cursor-driven Live Metric analyzer is paused.
    RefreshSelectedPinnedVisualization(World);
}

bool UMetricSubsystem::BuildSelectedPinnedVisualState(UWorld* World, FMetricLiveState& OutState) const
{
    if (!World || !PinnedBenchmarks.IsValidIndex(SelectedPinnedBenchmarkIndex))
    {
        return false;
    }

    const FMetricPinnedBenchmark& Pin = PinnedBenchmarks[SelectedPinnedBenchmarkIndex];
    if (!Pin.bHasVisualSnapshot)
    {
        return false;
    }

    OutState = FMetricLiveState();
    OutState.bHasHit = true;
    OutState.bShowHull = false;
    OutState.bShowForward = false;
    OutState.Location = Pin.WorldLocation;
    OutState.SurfaceNormal = Pin.SurfaceNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    OutState.AnalysisUp = Pin.AnalysisUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    OutState.AnalysisForward = FVector::VectorPlaneProject(Pin.Forward, OutState.AnalysisUp).GetSafeNormal();
    if (OutState.AnalysisForward.IsNearlyZero()) OutState.AnalysisForward = FVector::ForwardVector;
    OutState.AnalysisRight = FVector::CrossProduct(OutState.AnalysisUp, OutState.AnalysisForward).GetSafeNormal();
    OutState.Forward = OutState.AnalysisForward;
    OutState.ViewerLocation = LiveState.ViewerLocation;
    OutState.Hull = Pin.Hull;
    OutState.VisualTheme = Pin.VisualTheme;
    OutState.ClearanceSettings = Pin.ClearanceSettings;
    OutState.OverheadClearanceSettings = Pin.OverheadClearanceSettings;
    OutState.Sensors = Pin.Sensors;
    OutState.SensorFieldSettings = Pin.SensorFieldSettings;
    OutState.Ranges = Pin.Ranges;
    OutState.Locomotion = Pin.Locomotion;
    OutState.TrajectorySettings = Pin.TrajectorySettings;
    OutState.CameraProbeSettings = Pin.CameraProbeSettings;
    OutState.CameraSightlineSettings = Pin.CameraSightlineSettings;
    OutState.GroundValidationSettings = Pin.GroundValidationSettings;
    OutState.FitAnalysisSettings = Pin.FitAnalysisSettings;
    OutState.ThresholdAnalysisSettings = Pin.ThresholdAnalysisSettings;
    OutState.TurnClearanceSettings = Pin.TurnClearanceSettings;
    OutState.LedgeAnalysisSettings = Pin.LedgeAnalysisSettings;
    OutState.StoppingDistanceSettings = Pin.StoppingDistanceSettings;
    OutState.CoverExposureSettings = Pin.CoverExposureSettings;
    OutState.JumpRunwaySettings = Pin.JumpRunwaySettings;
    OutState.InteractionReachSettings = Pin.InteractionReachSettings;
    OutState.ShoulderClearanceSettings = Pin.ShoulderClearanceSettings;
    OutState.CombatDensitySettings = Pin.CombatDensitySettings;
    OutState.AlternateStanceSettings = Pin.AlternateStanceSettings;
    OutState.MantleVaultSettings = Pin.MantleVaultSettings;
    OutState.RecoverySpaceSettings = Pin.RecoverySpaceSettings;
    OutState.CrossfireSettings = Pin.CrossfireSettings;
    OutState.Label = Pin.Label;

    EvaluateClearance(World, OutState);
    EvaluateOverheadClearance(World, OutState);
    EvaluateSensorField(World, OutState);
    EvaluateTrajectory(World, OutState);
    EvaluateCameraProbe(World, OutState);
    EvaluateCameraSightline(World, OutState);
    EvaluateGroundValidation(World, OutState);
    EvaluateFitAnalysis(World, OutState);
    EvaluateThresholdAnalysis(World, OutState);
    EvaluateTurnClearance(World, OutState);
    EvaluateLedgeAnalysis(World, OutState);
    EvaluateStoppingDistance(World, OutState);
    EvaluateCoverExposure(World, OutState);
    EvaluateJumpRunway(World, OutState);
    EvaluateInteractionReach(World, OutState);
    EvaluateShoulderClearance(World, OutState);
    EvaluateCombatDensity(World, OutState);
    EvaluateAlternateStance(World, OutState);
    EvaluateMantleVault(World, OutState);
    EvaluateRecoverySpace(World, OutState);
    EvaluateCrossfire(World, OutState);
    BuildQueryEvidence(OutState);
    return true;
}

void UMetricSubsystem::RefreshSelectedPinnedVisualization(UWorld* World)
{
    FMetricLiveState PinnedState;
    const bool bHasPinnedState = BuildSelectedPinnedVisualState(World, PinnedState);
    if (bHasPinnedState)
    {
        SelectedPinnedEvaluatedState = PinnedState;
        bHasSelectedPinnedEvaluatedState = true;
    }
    else
    {
        SelectedPinnedEvaluatedState = FMetricLiveState();
        bHasSelectedPinnedEvaluatedState = false;
    }

    AMetricRenderHostActor* Host = RenderHost.Get();
    if (!Host)
    {
        return;
    }

    if (bHasPinnedState)
    {
        FMetricLiveState RenderPinnedState = PinnedState;
        if (PinnedBenchmarks.IsValidIndex(SelectedPinnedBenchmarkIndex))
        {
            MetricBenchmark::ApplyVisualVisibility(RenderPinnedState, PinnedBenchmarks[SelectedPinnedBenchmarkIndex].Visibility);
        }
        Host->ApplyPinnedMetricState(&RenderPinnedState);
    }
    else
    {
        Host->ApplyPinnedMetricState(nullptr);
    }
}

void UMetricSubsystem::EvaluateClearance(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.Clearance = FMetricClearanceResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.ClearanceSettings.bShowClearanceProbe
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    const float ProbeDistance = FMath::Max(10.0f, InOutState.ClearanceSettings.ProbeDistance);

    FCollisionShape Shape;
    FQuat Rotation = FQuat::Identity;
    float CenterHeight = 0.0f;
    bool bApproximateShape = false;
    MetricQueryAudit::BuildHullShape(InOutState, Shape, Rotation, CenterHeight, bApproximateShape);

    const float WarningMargin = FMath::Max(0.0f, InOutState.ClearanceSettings.WarningMargin);
    const FVector StartOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + InOutState.AnalysisUp * (CenterHeight + 1.0f);
    const FVector EndOffset = StartOffset + Forward * ProbeDistance;
    const FVector ExtendedEndOffset = StartOffset + Forward * (ProbeDistance + WarningMargin);
    const FVector Start = InOutState.Location + StartOffset;
    const FVector End = InOutState.Location + ExtendedEndOffset;

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricClearanceSweep), true);
    QueryParams.bTraceComplex = true;

    FHitResult Hit;
    const bool bBlocked = MetricEnvironmentQuery::SweepSingleByChannel(World, 
        Hit,
        Start,
        End,
        Rotation,
        InOutState.ClearanceSettings.TraceChannel.GetValue(),
        Shape,
        QueryParams);

    FMetricClearanceResult& Result = InOutState.Clearance;
    Result.bVisible = true;
    Result.bHasObstacle = bBlocked;
    Result.bApproximateShape = bApproximateShape;
    Result.ProbeDistance = ProbeDistance;
    Result.WarningMargin = WarningMargin;
    Result.SweepStartOffset = StartOffset;
    Result.SweepEndOffset = EndOffset;

    if (bBlocked)
    {
        Result.ObstacleDistance = FMath::Max(0.0f, Hit.Distance);
        Result.bBlocked = Result.ObstacleDistance <= ProbeDistance + KINDA_SMALL_NUMBER;
        Result.bNearThreshold = !Result.bBlocked && Result.ObstacleDistance <= ProbeDistance + WarningMargin + KINDA_SMALL_NUMBER;
        Result.ClearDistance = FMath::Min(Result.ObstacleDistance, ProbeDistance);
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
        Result.ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
    }
    else
    {
        Result.bBlocked = false;
        Result.bNearThreshold = false;
        Result.ObstacleDistance = ProbeDistance + WarningMargin;
        Result.ClearDistance = ProbeDistance;
        Result.ImpactPointOffset = EndOffset;
        Result.ImpactNormal = FVector::ZeroVector;
    }
}


void UMetricSubsystem::EvaluateOverheadClearance(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.OverheadClearance = FMetricOverheadClearanceResult();
    if (!World || !InOutState.bHasHit || !InOutState.OverheadClearanceSettings.bShowOverheadClearance
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FMetricOverheadClearanceResult& Result = InOutState.OverheadClearance;
    const float RequiredHeight = FMath::Max(1.0f, InOutState.Hull.Height + FVector::DotProduct(MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset), InOutState.AnalysisUp));
    const float MaxProbe = FMath::Max(RequiredHeight, InOutState.OverheadClearanceSettings.MaxProbeHeight);
    const float Warning = FMath::Max(0.0f, InOutState.OverheadClearanceSettings.WarningMargin);

    const FVector Start = InOutState.Location + InOutState.AnalysisUp * 1.0f;
    const FVector End = InOutState.Location + InOutState.AnalysisUp * MaxProbe;
    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricOverheadClearanceTrace), true);
    Params.bTraceComplex = true;

    FHitResult Hit;
    const bool bHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World, Hit, Start, End, InOutState.OverheadClearanceSettings.TraceChannel.GetValue(), Params);

    Result.bVisible = true;
    Result.RequiredHeight = RequiredHeight;
    Result.AvailableHeight = bHit ? FMath::Max(0.0f, Hit.Distance + 1.0f) : MaxProbe;
    Result.bBlocked = Result.AvailableHeight + KINDA_SMALL_NUMBER < RequiredHeight;
    Result.bNearThreshold = !Result.bBlocked && bHit && Result.AvailableHeight < RequiredHeight + Warning;
    Result.HeightDeficit = Result.bBlocked ? RequiredHeight - Result.AvailableHeight : 0.0f;
    if (bHit)
    {
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
    }
}

void UMetricSubsystem::EvaluateSensorField(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.SensorField = FMetricSensorFieldResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.SensorFieldSettings.bShowSensorField
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    const FVector Up = InOutState.AnalysisUp;
    const float Range = FMath::Max(10.0f, InOutState.SensorFieldSettings.Range);
    const float HorizontalHalfAngle = FMath::Clamp(InOutState.SensorFieldSettings.HorizontalHalfAngle, 1.0f, 89.0f);
    const float VerticalHalfAngle = FMath::Clamp(InOutState.SensorFieldSettings.VerticalHalfAngle, 1.0f, 89.0f);
    int32 HorizontalRayCount = FMath::Clamp(InOutState.SensorFieldSettings.HorizontalRayCount, 3, 31);
    if ((HorizontalRayCount % 2) == 0)
    {
        ++HorizontalRayCount;
    }

    // Three vertical bands make VerticalHalfAngle part of the actual query rather
    // than a visualization-only cone: lower boundary, center, upper boundary.
    constexpr int32 VerticalRayCount = 3;
    const FVector OriginOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + Up * FMath::Max(0.0f, InOutState.Sensors.PrimarySensorZOffset);
    const FVector Origin = InOutState.Location + OriginOffset;

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricSensorFieldTrace), true);
    QueryParams.bTraceComplex = true;

    FMetricSensorFieldResult& Result = InOutState.SensorField;
    Result.bVisible = true;
    Result.TotalRays = HorizontalRayCount * VerticalRayCount;
    Result.OriginOffset = OriginOffset;
    Result.NearestBlockDistance = Range;
    Result.Rays.Reserve(Result.TotalRays);

    for (int32 HorizontalIndex = 0; HorizontalIndex < HorizontalRayCount; ++HorizontalIndex)
    {
        const float HorizontalAlpha = HorizontalRayCount > 1
            ? static_cast<float>(HorizontalIndex) / static_cast<float>(HorizontalRayCount - 1)
            : 0.5f;
        const float YawDegrees = FMath::Lerp(-HorizontalHalfAngle, HorizontalHalfAngle, HorizontalAlpha);
        const FVector HorizontalDirection = FQuat(Up, FMath::DegreesToRadians(YawDegrees)).RotateVector(Forward).GetSafeNormal();
        FVector PitchAxis = FVector::CrossProduct(Up, HorizontalDirection).GetSafeNormal();
        if (PitchAxis.IsNearlyZero())
        {
            PitchAxis = InOutState.AnalysisRight;
        }

        for (int32 VerticalIndex = 0; VerticalIndex < VerticalRayCount; ++VerticalIndex)
        {
            const float VerticalAlpha = static_cast<float>(VerticalIndex) / static_cast<float>(VerticalRayCount - 1);
            const float PitchDegrees = FMath::Lerp(-VerticalHalfAngle, VerticalHalfAngle, VerticalAlpha);
            const FVector Direction = FQuat(PitchAxis, FMath::DegreesToRadians(PitchDegrees)).RotateVector(HorizontalDirection).GetSafeNormal();
            const FVector End = Origin + Direction * Range;

            FHitResult Hit;
            const bool bBlocked = MetricEnvironmentQuery::LineTraceSingleByChannel(
                World,
                Hit,
                Origin,
                End,
                InOutState.SensorFieldSettings.TraceChannel.GetValue(),
                QueryParams);

            FMetricSensorFieldRay Ray;
            Ray.bBlocked = bBlocked;
            Ray.Distance = bBlocked ? FMath::Max(0.0f, Hit.Distance) : Range;
            Ray.EndOffset = (bBlocked ? Hit.ImpactPoint : End) - InOutState.Location;
            Result.Rays.Add(Ray);

            if (bBlocked)
            {
                ++Result.BlockedRays;
                Result.NearestBlockDistance = FMath::Min(Result.NearestBlockDistance, Ray.Distance);
            }
        }
    }

    Result.ClearPercent = Result.TotalRays > 0
        ? (static_cast<float>(Result.TotalRays - Result.BlockedRays) / static_cast<float>(Result.TotalRays)) * 100.0f
        : 100.0f;
}

void UMetricSubsystem::EvaluateTrajectory(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.Trajectory = FMetricTrajectoryResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.TrajectorySettings.bShowTrajectory
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FVector Up = InOutState.AnalysisUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    FVector Forward = FVector::VectorPlaneProject(InOutState.AnalysisForward, Up).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::VectorPlaneProject(InOutState.Forward, Up).GetSafeNormal();
    }
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::VectorPlaneProject(FVector::ForwardVector, Up).GetSafeNormal();
    }

    const float Height = FMath::Max(1.0f, InOutState.Hull.Height);
    const float Width = FMath::Max(1.0f, InOutState.Hull.Width);
    const float Length = FMath::Max(1.0f, InOutState.Hull.Length);

    FCollisionShape Shape;
    FQuat Rotation = FQuat::Identity;
    float CenterHeight = Height * 0.5f;
    bool bApproximateShape = false;

    switch (InOutState.Hull.HullType)
    {
    case EMetricHullType::Capsule:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
        CenterHeight = HalfHeight;
        break;
    }
    case EMetricHullType::OrientedBox:
    {
        const FVector HalfExtent(Length * 0.5f, Width * 0.5f, Height * 0.5f);
        Shape = FCollisionShape::MakeBox(HalfExtent);
        Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
        CenterHeight = HalfExtent.Z;
        break;
    }
    case EMetricHullType::Sphere:
    {
        const float Radius = FMath::Max3(Width, Length, Height) * 0.5f;
        Shape = FCollisionShape::MakeSphere(FMath::Max(1.0f, Radius));
        CenterHeight = Radius;
        break;
    }
    case EMetricHullType::Cylinder:
    default:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
        CenterHeight = HalfHeight;
        bApproximateShape = true;
        break;
    }
    }

    const FMetricTrajectorySettings& Settings = InOutState.TrajectorySettings;
    const float MaximumHorizontalSpeed = FMath::Max(1.0f, Settings.HorizontalSpeed);
    const float LaunchVerticalSpeed = FMath::Max(1.0f, Settings.LaunchVerticalSpeed);
    const float GravityScale = FMath::Max(0.05f, Settings.GravityScale);
    const float GravityMagnitude = FMath::Max(1.0f, FMath::Abs(World->GetGravityZ())) * GravityScale;
    const FVector Gravity = -Up * GravityMagnitude;
    const float MaxTime = FMath::Clamp(Settings.MaxSimulationTime, 0.25f, 10.0f);
    const float Step = FMath::Clamp(Settings.SampleInterval, 0.01f, 0.2f);
    const float MaxDrop = FMath::Max(0.0f, Settings.MaximumDropDistance);
    const float MinLandingDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Settings.MaxLandingSlope, 0.0f, 89.0f)));
    const FVector StartOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + Up * (CenterHeight + 1.0f);
    const FVector StartWorld = InOutState.Location + StartOffset;

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricTrajectorySweep), true);
    QueryParams.bTraceComplex = true;

    auto SimulateArc = [&](float HorizontalFraction) -> FMetricTrajectoryArcResult
    {
        FMetricTrajectoryArcResult Arc;
        Arc.HorizontalSpeedFraction = FMath::Clamp(HorizontalFraction, 0.0f, 1.0f);
        Arc.SampleOffsets.Add(StartOffset);

        const float HorizontalSpeed = MaximumHorizontalSpeed * Arc.HorizontalSpeedFraction;
        FVector PreviousWorld = StartWorld;
        float PreviousTime = 0.0f;

        for (float Time = Step; Time <= MaxTime + KINDA_SMALL_NUMBER; Time += Step)
        {
            const float T = FMath::Min(Time, MaxTime);
            const FVector CurrentOffset = StartOffset
                + Forward * (HorizontalSpeed * T)
                + Up * (LaunchVerticalSpeed * T)
                + 0.5f * Gravity * (T * T);
            const FVector CurrentWorld = InOutState.Location + CurrentOffset;

            const float RelativeHeight = FVector::DotProduct(CurrentOffset - StartOffset, Up);
            if (MaxDrop > KINDA_SMALL_NUMBER && RelativeHeight < -MaxDrop)
            {
                Arc.SampleOffsets.Add(CurrentOffset);
                Arc.FlightTime = T;
                Arc.HorizontalDistance = HorizontalSpeed * T;
                Arc.LandingDrop = -RelativeHeight;
                Arc.bExceededMaxDrop = true;
                return Arc;
            }

            FHitResult Hit;
            const bool bHit = MetricEnvironmentQuery::SweepSingleByChannel(
                World, Hit, PreviousWorld, CurrentWorld, Rotation,
                Settings.TraceChannel.GetValue(), Shape, QueryParams);

            if (bHit)
            {
                const float SegmentTime = T - PreviousTime;
                const float HitTime = PreviousTime + SegmentTime * FMath::Clamp(Hit.Time, 0.0f, 1.0f);
                const float VerticalVelocityAtHit = LaunchVerticalSpeed - GravityMagnitude * HitTime;
                const bool bDescending = VerticalVelocityAtHit <= 0.0f;
                const FVector HitNormal = Hit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, Up);
                const bool bWalkableLanding = FVector::DotProduct(HitNormal, Up) >= MinLandingDot;

                Arc.FlightTime = HitTime;
                Arc.HorizontalDistance = HorizontalSpeed * HitTime;
                Arc.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
                Arc.ImpactNormal = HitNormal;
                Arc.LandingDrop = FMath::Max(0.0f, -FVector::DotProduct(Arc.ImpactPointOffset - StartOffset, Up));
                Arc.bHasLanding = bDescending && bWalkableLanding;
                Arc.bLandingValid = Arc.bHasLanding && (MaxDrop <= KINDA_SMALL_NUMBER || Arc.LandingDrop <= MaxDrop + KINDA_SMALL_NUMBER);
                Arc.bExceededMaxDrop = Arc.bHasLanding && !Arc.bLandingValid;
                Arc.bBlocked = !Arc.bHasLanding;
                Arc.SampleOffsets.Add(Hit.Location - InOutState.Location);
                return Arc;
            }

            Arc.SampleOffsets.Add(CurrentOffset);
            PreviousWorld = CurrentWorld;
            PreviousTime = T;
            if (T >= MaxTime - KINDA_SMALL_NUMBER)
            {
                break;
            }
        }

        Arc.FlightTime = MaxTime;
        Arc.HorizontalDistance = HorizontalSpeed * MaxTime;
        return Arc;
    };

    FMetricTrajectoryResult& Result = InOutState.Trajectory;
    Result.bVisible = true;
    Result.bApproximateShape = bApproximateShape;

    const bool bEnvelope = Settings.DisplayMode != EMetricTrajectoryDisplayMode::SingleArc;
    const int32 ArcCount = bEnvelope ? FMath::Clamp(Settings.JumpEnvelopeArcCount, 2, 12) : 1;
    const float MinFraction = bEnvelope ? FMath::Clamp(Settings.MinimumHorizontalSpeedFraction, 0.0f, 1.0f) : 1.0f;
    Result.EnvelopeArcs.Reserve(ArcCount);

    for (int32 ArcIndex = 0; ArcIndex < ArcCount; ++ArcIndex)
    {
        const float Alpha = ArcCount > 1 ? static_cast<float>(ArcIndex) / static_cast<float>(ArcCount - 1) : 1.0f;
        const float Fraction = FMath::Lerp(MinFraction, 1.0f, Alpha);
        FMetricTrajectoryArcResult Arc = SimulateArc(Fraction);
        if (Arc.bLandingValid)
        {
            ++Result.ValidEnvelopeLandings;
        }
        Result.EnvelopeArcs.Add(MoveTemp(Arc));
    }
    Result.TestedEnvelopeArcs = Result.EnvelopeArcs.Num();

    int32 PrimaryArcIndex = Result.EnvelopeArcs.Num() - 1;
    if (Settings.DisplayMode == EMetricTrajectoryDisplayMode::AnimatedEnvelope && Result.EnvelopeArcs.Num() > 1)
    {
        const double Phase = FPlatformTime::Seconds() * FMath::Max(0.05f, Settings.EnvelopeAnimationRate);
        const double PingPong = 1.0 - FMath::Abs(FMath::Fmod(Phase, 2.0) - 1.0);
        PrimaryArcIndex = FMath::Clamp(FMath::RoundToInt(PingPong * (Result.EnvelopeArcs.Num() - 1)), 0, Result.EnvelopeArcs.Num() - 1);
    }
    Result.ActiveEnvelopeArcIndex = PrimaryArcIndex;

    if (Result.EnvelopeArcs.IsValidIndex(PrimaryArcIndex))
    {
        const FMetricTrajectoryArcResult& Primary = Result.EnvelopeArcs[PrimaryArcIndex];
        Result.bBlocked = Primary.bBlocked;
        Result.bHasLanding = Primary.bHasLanding;
        Result.bLandingValid = Primary.bLandingValid;
        Result.FlightTime = Primary.FlightTime;
        Result.HorizontalDistance = Primary.HorizontalDistance;
        Result.ImpactPointOffset = Primary.ImpactPointOffset;
        Result.ImpactNormal = Primary.ImpactNormal;
        Result.SampleOffsets = Primary.SampleOffsets;
    }

    float HighestAlongUp = -FLT_MAX;
    for (int32 SampleIndex = 0; SampleIndex < Result.SampleOffsets.Num(); ++SampleIndex)
    {
        const float AlongUp = FVector::DotProduct(Result.SampleOffsets[SampleIndex] - StartOffset, Up);
        if (AlongUp > HighestAlongUp)
        {
            HighestAlongUp = AlongUp;
            Result.ApexSampleIndex = SampleIndex;
            Result.ApexOffset = Result.SampleOffsets[SampleIndex];
        }
    }
    Result.ApexHeight = FMath::Max(0.0f, HighestAlongUp);

    const float ApexProbeDistance = FMath::Max(0.0f, Settings.ApexProbeDistance);
    if (Result.ApexSampleIndex != INDEX_NONE && ApexProbeDistance > KINDA_SMALL_NUMBER)
    {
        const FVector ApexCenterWorld = InOutState.Location + Result.ApexOffset;
        const FVector ProbeStart = ApexCenterWorld + Up * CenterHeight;
        const FVector ProbeEnd = ProbeStart + Up * ApexProbeDistance;
        FHitResult ApexHit;
        Result.bApexHasObstacle = MetricEnvironmentQuery::LineTraceSingleByChannel(
            World, ApexHit, ProbeStart, ProbeEnd, Settings.TraceChannel.GetValue(), QueryParams);
        Result.ApexHeadroom = Result.bApexHasObstacle ? ApexHit.Distance : ApexProbeDistance;
        Result.bApexTight = Result.bApexHasObstacle
            && Result.ApexHeadroom <= FMath::Max(0.0f, Settings.ApexWarningMargin);
        if (Result.bApexHasObstacle)
        {
            Result.ApexClearanceImpactOffset = ApexHit.ImpactPoint - InOutState.Location;
        }
    }
}


void UMetricSubsystem::EvaluateCameraProbe(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.CameraProbe = FMetricCameraProbeResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.CameraProbeSettings.bShowCameraProbe
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = InOutState.AnalysisRight;
    }

    const FMetricCameraProbeSettings& Settings = InOutState.CameraProbeSettings;
    const float PivotZ = FMath::Max(0.0f, Settings.PivotZOffset);
    const float TargetDistance = FMath::Max(10.0f, Settings.TargetDistance);
    const float ProbeRadius = FMath::Max(1.0f, Settings.ProbeRadius);
    const float MinDistance = FMath::Max(0.0f, Settings.MinDistance);

    const FVector PivotOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + InOutState.AnalysisUp * PivotZ;
    const FVector DesiredDelta = -Forward * TargetDistance
        + Right * Settings.ShoulderOffset
        + InOutState.AnalysisUp * Settings.VerticalOffset;
    const FVector DesiredCameraOffset = PivotOffset + DesiredDelta;
    const float DesiredArmLength = FMath::Max(1.0f, DesiredDelta.Size());

    const FVector PivotWorld = InOutState.Location + PivotOffset;
    const FVector DesiredWorld = InOutState.Location + DesiredCameraOffset;

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricCameraProbeSweep), true);
    QueryParams.bTraceComplex = true;

    FHitResult Hit;
    const bool bHit = MetricEnvironmentQuery::SweepSingleByChannel(World, 
        Hit,
        PivotWorld,
        DesiredWorld,
        FQuat::Identity,
        Settings.TraceChannel.GetValue(),
        FCollisionShape::MakeSphere(ProbeRadius),
        QueryParams);

    FMetricCameraProbeResult& Result = InOutState.CameraProbe;
    Result.bVisible = true;
    Result.TargetDistance = DesiredArmLength;
    Result.PivotOffset = PivotOffset;
    Result.DesiredCameraOffset = DesiredCameraOffset;

    if (bHit)
    {
        Result.bCompressed = true;
        Result.ResolvedCameraOffset = Hit.Location - InOutState.Location;
        Result.ResolvedDistance = FMath::Max(0.0f, (Hit.Location - PivotWorld).Size());
        Result.CompressionPercent = 1.0f - FMath::Clamp(Result.ResolvedDistance / DesiredArmLength, 0.0f, 1.0f);
        Result.bBelowMinimum = Result.ResolvedDistance < MinDistance;
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
        Result.ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
    }
    else
    {
        Result.bCompressed = false;
        Result.bBelowMinimum = false;
        Result.ResolvedCameraOffset = DesiredCameraOffset;
        Result.ResolvedDistance = DesiredArmLength;
        Result.CompressionPercent = 0.0f;
        Result.ImpactPointOffset = DesiredCameraOffset;
        Result.ImpactNormal = FVector::ZeroVector;
    }
}


void UMetricSubsystem::EvaluateCameraSightline(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.CameraSightline = FMetricCameraSightlineResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.CameraSightlineSettings.bShowCameraSightline
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricCameraSightlineSettings& Settings = InOutState.CameraSightlineSettings;

    FVector CameraOffset;
    FVector AimOffset;
    if (InOutState.CameraProbe.bVisible)
    {
        CameraOffset = InOutState.CameraProbe.ResolvedCameraOffset;
        AimOffset = InOutState.CameraProbe.PivotOffset;
    }
    else
    {
        FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, InOutState.AnalysisUp).GetSafeNormal();
        if (Forward.IsNearlyZero())
        {
            Forward = InOutState.AnalysisForward;
        }
        CameraOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) - Forward * 350.0f + InOutState.AnalysisUp * 220.0f;
        AimOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + InOutState.AnalysisUp * (InOutState.Hull.Height * 0.5f);
    }

    const FVector SubjectOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + InOutState.AnalysisUp * FMath::Max(0.0f, Settings.SubjectZOffset);
    const FVector CameraWorld = InOutState.Location + CameraOffset;
    const FVector AimWorld = InOutState.Location + AimOffset;
    const FVector SubjectWorld = InOutState.Location + SubjectOffset;

    const FVector AimDirection = (AimWorld - CameraWorld).GetSafeNormal();
    const FVector SubjectDirection = (SubjectWorld - CameraWorld).GetSafeNormal();
    if (AimDirection.IsNearlyZero() || SubjectDirection.IsNearlyZero())
    {
        return;
    }

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricCameraSightlineTrace), true);
    QueryParams.bTraceComplex = true;

    FHitResult Hit;
    const float TraceRadius = FMath::Max(0.0f, Settings.TraceRadius);
    bool bHit = false;
    if (TraceRadius > KINDA_SMALL_NUMBER)
    {
        bHit = MetricEnvironmentQuery::SweepSingleByChannel(World, 
            Hit, CameraWorld, SubjectWorld, FQuat::Identity, Settings.TraceChannel.GetValue(),
            FCollisionShape::MakeSphere(TraceRadius), QueryParams);
    }
    else
    {
        bHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World, 
            Hit, CameraWorld, SubjectWorld, Settings.TraceChannel.GetValue(), QueryParams);
    }

    FMetricCameraSightlineResult& Result = InOutState.CameraSightline;
    Result.bVisible = true;
    Result.CameraOffset = CameraOffset;
    Result.AimOffset = AimOffset;
    Result.SubjectOffset = SubjectOffset;
    Result.SubjectAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(AimDirection, SubjectDirection), -1.0f, 1.0f)));
    Result.bFramingFail = Result.SubjectAngle > FMath::Clamp(Settings.FramingHalfAngle, 1.0f, 80.0f);
    Result.bOccluded = bHit && Hit.Time < 0.995f;
    if (Result.bOccluded)
    {
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
    }
}

void UMetricSubsystem::EvaluateGroundValidation(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.GroundValidation = FMetricGroundValidationResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.GroundValidationSettings.bShowGroundValidation
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricGroundValidationSettings& Settings = InOutState.GroundValidationSettings;
    FMetricGroundValidationResult& Result = InOutState.GroundValidation;
    Result.bVisible = true;
    Result.bPass = true;

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = InOutState.AnalysisRight;
    }

    const float ForwardDistance = FMath::Max(50.0f, Settings.ForwardDistance);
    const float SampleSpacing = FMath::Max(10.0f, Settings.SampleSpacing);
    const float HalfCoverage = FMath::Max(0.0f, Settings.LateralCoverage * 0.5f);
    const float TraceHeight = FMath::Max(10.0f, Settings.TraceHeight);
    const float TraceDepth = FMath::Max(10.0f, Settings.TraceDepth);
    const float MaxSlope = FMath::Clamp(Settings.MaxWalkableSlope, 0.0f, 89.0f);
    const float MaxStepUp = FMath::Max(0.0f, Settings.MaxStepUp);
    const float MaxStepDown = FMath::Max(0.0f, Settings.MaxStepDown);

    const int32 RowCount = FMath::Max(1, FMath::CeilToInt(ForwardDistance / SampleSpacing));
    const int32 LaneCount = HalfCoverage > KINDA_SMALL_NUMBER ? 3 : 1;
    TArray<FVector> PreviousPoints;
    TArray<FVector> PreviousNormals;
    TArray<bool> bPreviousValid;
    PreviousPoints.SetNumZeroed(LaneCount);
    PreviousNormals.Init(InOutState.AnalysisUp, LaneCount);
    bPreviousValid.Init(false, LaneCount);

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricGroundValidationTrace), true);
    QueryParams.bTraceComplex = true;

    for (int32 Row = 0; Row <= RowCount; ++Row)
    {
        const float ForwardOffset = FMath::Min(ForwardDistance, Row * SampleSpacing);
        for (int32 Lane = 0; Lane < LaneCount; ++Lane)
        {
            const float LateralAlpha = LaneCount == 1 ? 0.0f : (static_cast<float>(Lane) - 1.0f);
            const float LateralOffset = LateralAlpha * HalfCoverage;
            const FVector PlanarOffset = Forward * ForwardOffset + Right * LateralOffset;
            const FVector Target = InOutState.Location + PlanarOffset;
            const FVector TraceStart = Target + InOutState.AnalysisUp * TraceHeight;
            const FVector TraceEnd = Target - InOutState.AnalysisUp * TraceDepth;

            FHitResult Hit;
            const bool bHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
                Hit,
                TraceStart,
                TraceEnd,
                Settings.TraceChannel.GetValue(),
                QueryParams);

            FMetricGroundSample& Sample = Result.Samples.AddDefaulted_GetRef();
            Sample.bHit = bHit;
            ++Result.TotalSamples;

            if (!bHit)
            {
                Sample.Offset = PlanarOffset - InOutState.AnalysisUp * TraceDepth;
                Sample.Normal = InOutState.AnalysisUp;
                Sample.bStepDownFail = true;
                Sample.StepDelta = -TraceDepth;
                ++Result.FailedSamples;
                Result.bPass = false;
                Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, TraceDepth);
                bPreviousValid[Lane] = false;
                continue;
            }

            Sample.Offset = Hit.ImpactPoint - InOutState.Location;
            Sample.Normal = Hit.ImpactNormal.GetSafeNormal();
            Sample.SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Sample.Normal, InOutState.AnalysisUp), -1.0f, 1.0f)));
            Result.MaxObservedSlope = FMath::Max(Result.MaxObservedSlope, Sample.SlopeDegrees);
            Sample.bSlopeFail = Sample.SlopeDegrees > MaxSlope + KINDA_SMALL_NUMBER;

            // Step limits describe a discontinuity, not ordinary elevation change.
            // Remove the grade predicted by the adjacent surface planes so a
            // continuous ramp is judged by MaxWalkableSlope instead of MaxStepUp.
            if (bPreviousValid[Lane])
            {
                Sample.StepDelta = MetricQueryAudit::ComputeSurfaceDiscontinuity(
                    PreviousPoints[Lane],
                    PreviousNormals[Lane],
                    Hit.ImpactPoint,
                    Sample.Normal,
                    InOutState.AnalysisUp);
                Sample.bStepUpFail = Sample.StepDelta > MaxStepUp + KINDA_SMALL_NUMBER;
                Sample.bStepDownFail = Sample.StepDelta < -(MaxStepDown + KINDA_SMALL_NUMBER);
                Result.MaxObservedStepUp = FMath::Max(Result.MaxObservedStepUp, FMath::Max(0.0f, Sample.StepDelta));
                Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, FMath::Max(0.0f, -Sample.StepDelta));
            }

            PreviousPoints[Lane] = Hit.ImpactPoint;
            PreviousNormals[Lane] = Sample.Normal;
            bPreviousValid[Lane] = true;

            if (Sample.bSlopeFail || Sample.bStepUpFail || Sample.bStepDownFail)
            {
                ++Result.FailedSamples;
                Result.bPass = false;
            }
        }
    }
}


void UMetricSubsystem::EvaluateLedgeAnalysis(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.LedgeAnalysis = FMetricLedgeAnalysisResult();

    if (!World || !InOutState.bHasHit || !InOutState.LedgeAnalysisSettings.bShowLedgeAnalysis
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricLedgeAnalysisSettings& Settings = InOutState.LedgeAnalysisSettings;
    FMetricLedgeAnalysisResult& Result = InOutState.LedgeAnalysis;
    Result.bVisible = true;

    const FVector Up = InOutState.AnalysisUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, Up).GetSafeNormal();
    if (Forward.IsNearlyZero()) Forward = InOutState.AnalysisForward;

    const float ForwardDistance = FMath::Max(10.0f, Settings.ForwardDistance);
    const float SampleSpacing = FMath::Max(5.0f, Settings.SampleSpacing);
    const float TraceHeight = FMath::Max(10.0f, Settings.TraceHeight);
    const float MaxProbeDepth = FMath::Max(10.0f, Settings.MaxProbeDepth);
    const float MaxSafeDrop = FMath::Max(0.0f, Settings.MaxSafeDrop);
    const float MaxTraversableStepDown = FMath::Max(0.0f, InOutState.GroundValidationSettings.MaxStepDown);
    const int32 SampleCount = FMath::Max(1, FMath::CeilToInt(ForwardDistance / SampleSpacing));

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricLedgeAnalysisTrace), true);
    QueryParams.bTraceComplex = true;

    FVector PreviousPoint = InOutState.Location;
    FVector PreviousNormal = InOutState.SurfaceNormal.GetSafeNormal(SMALL_NUMBER, Up);
    FHitResult InitialGround;
    if (MetricEnvironmentQuery::LineTraceSingleByChannel(World, InitialGround,
        InOutState.Location + Up * TraceHeight, InOutState.Location - Up * MaxProbeDepth,
        Settings.TraceChannel.GetValue(), QueryParams))
    {
        PreviousPoint = InitialGround.ImpactPoint;
        PreviousNormal = InitialGround.ImpactNormal.GetSafeNormal(SMALL_NUMBER, Up);
    }

    for (int32 SampleIndex = 1; SampleIndex <= SampleCount; ++SampleIndex)
    {
        const float Distance = FMath::Min(ForwardDistance, SampleIndex * SampleSpacing);
        const FVector Station = InOutState.Location + Forward * Distance;
        FHitResult GroundHit;
        const bool bGround = MetricEnvironmentQuery::LineTraceSingleByChannel(World, GroundHit,
            Station + Up * TraceHeight, Station - Up * MaxProbeDepth,
            Settings.TraceChannel.GetValue(), QueryParams);

        bool bEdge = !bGround;
        float DiscreteDrop = 0.0f;
        if (bGround)
        {
            const FVector CurrentNormal = GroundHit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, Up);
            const float Discontinuity = MetricQueryAudit::ComputeSurfaceDiscontinuity(
                PreviousPoint, PreviousNormal, GroundHit.ImpactPoint, CurrentNormal, Up);
            DiscreteDrop = FMath::Max(0.0f, -Discontinuity);
            bEdge = DiscreteDrop > MaxTraversableStepDown + KINDA_SMALL_NUMBER;
            if (!bEdge)
            {
                PreviousPoint = GroundHit.ImpactPoint;
                PreviousNormal = CurrentNormal;
                Result.EdgeDistance = Distance;
                continue;
            }
        }

        Result.bHasEdge = true;
        Result.EdgeDistance = FMath::Max(0.0f, Distance - SampleSpacing);
        Result.EdgeOffset = PreviousPoint - InOutState.Location;
        Result.bHasLanding = bGround;
        if (bGround)
        {
            Result.LandingOffset = GroundHit.ImpactPoint - InOutState.Location;
            Result.DropDepth = FMath::Max(DiscreteDrop,
                FMath::Max(0.0f, FVector::DotProduct(PreviousPoint - GroundHit.ImpactPoint, Up)));
            Result.bSafeDrop = Result.DropDepth <= MaxSafeDrop + KINDA_SMALL_NUMBER;
            Result.bWarning = Result.bSafeDrop && MaxSafeDrop > KINDA_SMALL_NUMBER
                && Result.DropDepth >= MaxSafeDrop * 0.75f;
        }
        else
        {
            Result.DropDepth = MaxProbeDepth;
            Result.bSafeDrop = false;
            Result.bWarning = false;
            Result.LandingOffset = Forward * Distance - Up * MaxProbeDepth;
        }
        return;
    }

    Result.EdgeDistance = ForwardDistance;
    Result.bSafeDrop = true;
}

void UMetricSubsystem::EvaluateFitAnalysis(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.FitAnalysis = FMetricFitAnalysisResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.FitAnalysisSettings.bShowFitAnalysis
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricFitAnalysisSettings& Settings = InOutState.FitAnalysisSettings;
    FMetricFitAnalysisResult& Result = InOutState.FitAnalysis;
    Result.bVisible = true;
    Result.bPass = true;

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = InOutState.AnalysisRight;
    }

    const float RequiredWidth = FMath::Max(1.0f, InOutState.Hull.Width);
    const float RequiredHeight = FMath::Max(1.0f, InOutState.Hull.Height);
    const float RequiredHalfWidth = RequiredWidth * 0.5f;
    const float WarningMargin = FMath::Max(0.0f, Settings.WarningMargin);
    const float ForwardDistance = FMath::Max(0.0f, Settings.ForwardDistance);
    const float SampleSpacing = FMath::Max(10.0f, Settings.SampleSpacing);
    const float SideProbeDistance = FMath::Max(FMath::Max(10.0f, Settings.MaxSideProbeDistance), RequiredHalfWidth + WarningMargin);
    const float CeilingProbeHeight = FMath::Max(FMath::Max(10.0f, Settings.MaxCeilingProbeHeight), RequiredHeight + WarningMargin);
    const float FloorTraceHeight = FMath::Max(10.0f, Settings.FloorTraceHeight);
    const float FloorTraceDepth = FMath::Max(10.0f, Settings.FloorTraceDepth);
    const int32 SampleCount = ForwardDistance <= KINDA_SMALL_NUMBER ? 0 : FMath::Max(1, FMath::CeilToInt(ForwardDistance / SampleSpacing));
    const FVector HullLocalOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset);
    const float HullVerticalOffset = FVector::DotProduct(HullLocalOffset, InOutState.AnalysisUp);
    const FVector HullPlanarOffset = HullLocalOffset - InOutState.AnalysisUp * HullVerticalOffset;

    FCollisionShape HullShape;
    FQuat HullRotation = FQuat::Identity;
    float HullCenterHeight = 0.0f;
    bool bApproximateHull = false;
    MetricQueryAudit::BuildHullShape(InOutState, HullShape, HullRotation, HullCenterHeight, bApproximateHull);

    Result.RequiredWidth = RequiredWidth;
    Result.RequiredHeight = RequiredHeight;
    Result.MinimumAvailableWidth = TNumericLimits<float>::Max();
    Result.MinimumAvailableHeight = TNumericLimits<float>::Max();

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricFitAnalysisTrace), true);
    QueryParams.bTraceComplex = true;

    for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
    {
        const float ForwardOffset = ForwardDistance <= KINDA_SMALL_NUMBER
            ? 0.0f
            : FMath::Min(ForwardDistance, SampleIndex * SampleSpacing);
        const FVector PlanarOffset = Forward * ForwardOffset;
        const FVector Station = InOutState.Location + PlanarOffset;

        FMetricFitSample& Sample = Result.Samples.AddDefaulted_GetRef();
        ++Result.TotalSamples;

        FHitResult FloorHit;
        const bool bHasFloor = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
            FloorHit,
            Station + InOutState.AnalysisUp * FloorTraceHeight,
            Station - InOutState.AnalysisUp * FloorTraceDepth,
            Settings.TraceChannel.GetValue(),
            QueryParams);

        Sample.bHasFloor = bHasFloor;
        if (!bHasFloor)
        {
            Sample.BaseOffset = PlanarOffset - InOutState.AnalysisUp * FloorTraceDepth;
            ++Result.InvalidSamples;
            ++Result.FailedSamples;
            Result.bPass = false;
            continue;
        }

        const FVector FloorPoint = FloorHit.ImpactPoint;
        const FVector HullBase = FloorPoint + HullLocalOffset;
        Sample.BaseOffset = HullBase - InOutState.Location;

        // First answer the authoritative question: does the configured hull occupy
        // free space at this station? A short sweep catches geometry that side and
        // ceiling rays can miss, including centered or partial-height obstructions.
        const float SupportHeight = MetricQueryAudit::ComputeHullSupportHeightAlongUp(
            InOutState, FloorHit.ImpactNormal, HullPlanarOffset);
        const FVector HullCenter = FloorPoint + HullPlanarOffset
            + InOutState.AnalysisUp * (HullVerticalOffset + SupportHeight + 1.0f);
        FHitResult HullHit;
        Sample.bHullBlocked = MetricEnvironmentQuery::SweepSingleByChannel(World,
            HullHit,
            HullCenter - Forward,
            HullCenter + Forward,
            HullRotation,
            Settings.TraceChannel.GetValue(),
            HullShape,
            QueryParams);
        if (Sample.bHullBlocked)
        {
            Sample.HullImpactOffset = HullHit.ImpactPoint - InOutState.Location;
            ++Result.HullBlockedSamples;
        }

        // Width probes remain useful descriptive evidence. Sample several heights
        // and retain the tightest cross-section instead of trusting one mid-height ray.
        float BestAvailableWidth = TNumericLimits<float>::Max();
        float BestWidthDeficit = 0.0f;
        bool bBestLeftHit = false;
        bool bBestRightHit = false;
        FVector BestLeftOffset = FVector::ZeroVector;
        FVector BestRightOffset = FVector::ZeroVector;
        bool bWidthNear = false;
        constexpr float WidthFractions[] = { 0.10f, 0.50f, 0.90f };

        for (const float Fraction : WidthFractions)
        {
            const float WidthMeasureHeight = FMath::Clamp(RequiredHeight * Fraction, 1.0f, FMath::Max(1.0f, RequiredHeight - 1.0f));
            const FVector WidthCenter = HullBase + InOutState.AnalysisUp * WidthMeasureHeight;

            FHitResult LeftHit;
            FHitResult RightHit;
            const bool bLeftHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
                LeftHit, WidthCenter, WidthCenter - Right * SideProbeDistance, Settings.TraceChannel.GetValue(), QueryParams);
            const bool bRightHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
                RightHit, WidthCenter, WidthCenter + Right * SideProbeDistance, Settings.TraceChannel.GetValue(), QueryParams);

            const float LeftDistance = bLeftHit ? FMath::Max(0.0f, LeftHit.Distance) : SideProbeDistance;
            const float RightDistance = bRightHit ? FMath::Max(0.0f, RightHit.Distance) : SideProbeDistance;
            const float AvailableWidth = LeftDistance + RightDistance;
            const float WidthDeficit = FMath::Max(0.0f, RequiredHalfWidth - LeftDistance)
                + FMath::Max(0.0f, RequiredHalfWidth - RightDistance);
            bWidthNear = bWidthNear || (WidthDeficit <= KINDA_SMALL_NUMBER
                && (LeftDistance < RequiredHalfWidth + WarningMargin || RightDistance < RequiredHalfWidth + WarningMargin));

            if (AvailableWidth < BestAvailableWidth)
            {
                BestAvailableWidth = AvailableWidth;
                BestWidthDeficit = WidthDeficit;
                bBestLeftHit = bLeftHit;
                bBestRightHit = bRightHit;
                BestLeftOffset = (bLeftHit ? LeftHit.Location : WidthCenter - Right * SideProbeDistance) - InOutState.Location;
                BestRightOffset = (bRightHit ? RightHit.Location : WidthCenter + Right * SideProbeDistance) - InOutState.Location;
            }
        }

        Sample.bLeftBounded = bBestLeftHit;
        Sample.bRightBounded = bBestRightHit;
        Sample.LeftOffset = BestLeftOffset;
        Sample.RightOffset = BestRightOffset;
        Sample.AvailableWidth = BestAvailableWidth == TNumericLimits<float>::Max() ? 0.0f : BestAvailableWidth;
        Sample.WidthDeficit = BestWidthDeficit;
        Sample.bWidthFail = Sample.WidthDeficit > KINDA_SMALL_NUMBER;

        FHitResult CeilingHit;
        const FVector CeilingStart = HullBase + InOutState.AnalysisUp * 1.0f;
        const bool bCeilingHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
            CeilingHit,
            CeilingStart,
            HullBase + InOutState.AnalysisUp * CeilingProbeHeight,
            Settings.TraceChannel.GetValue(),
            QueryParams);

        const float AvailableHeight = bCeilingHit
            ? FMath::Max(0.0f, FVector::DotProduct(CeilingHit.Location - HullBase, InOutState.AnalysisUp))
            : CeilingProbeHeight;
        Sample.bCeilingBounded = bCeilingHit;
        Sample.CeilingOffset = (bCeilingHit ? CeilingHit.Location : HullBase + InOutState.AnalysisUp * CeilingProbeHeight) - InOutState.Location;
        Sample.AvailableHeight = AvailableHeight;
        Sample.HeightDeficit = bCeilingHit ? FMath::Max(0.0f, RequiredHeight - AvailableHeight) : 0.0f;
        Sample.bHeightFail = Sample.HeightDeficit > KINDA_SMALL_NUMBER;

        const bool bHeightNear = !Sample.bHeightFail
            && bCeilingHit
            && AvailableHeight < RequiredHeight + WarningMargin;
        Sample.bNearThreshold = !Sample.bHullBlocked && (bWidthNear || bHeightNear);

        Result.MinimumAvailableWidth = FMath::Min(Result.MinimumAvailableWidth, Sample.AvailableWidth);
        Result.MinimumAvailableHeight = FMath::Min(Result.MinimumAvailableHeight, Sample.AvailableHeight);
        Result.MaxWidthDeficit = FMath::Max(Result.MaxWidthDeficit, Sample.WidthDeficit);
        Result.MaxHeightDeficit = FMath::Max(Result.MaxHeightDeficit, Sample.HeightDeficit);
        Result.bNearThreshold = Result.bNearThreshold || Sample.bNearThreshold;

        if (Sample.bHullBlocked || Sample.bWidthFail || Sample.bHeightFail)
        {
            ++Result.FailedSamples;
            Result.bPass = false;
        }
    }

    if (Result.MinimumAvailableWidth == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableWidth = 0.0f;
    }
    if (Result.MinimumAvailableHeight == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableHeight = 0.0f;
    }
}


void UMetricSubsystem::EvaluateThresholdAnalysis(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.ThresholdAnalysis = FMetricThresholdAnalysisResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.ThresholdAnalysisSettings.bShowThresholdAnalysis
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricThresholdAnalysisSettings& Settings = InOutState.ThresholdAnalysisSettings;
    FMetricThresholdAnalysisResult& Result = InOutState.ThresholdAnalysis;
    Result.bVisible = true;
    Result.bPass = true;

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = InOutState.AnalysisRight;
    }

    const float RequiredWidth = FMath::Max(1.0f, InOutState.Hull.Width);
    const float RequiredHeight = FMath::Max(1.0f, InOutState.Hull.Height);
    const float RequiredHalfWidth = RequiredWidth * 0.5f;
    const float ProbeDepth = FMath::Max(10.0f, Settings.ProbeDepth);
    const float WarningMargin = FMath::Max(0.0f, Settings.WarningMargin);
    const float SideProbeDistance = FMath::Max(FMath::Max(10.0f, Settings.MaxSideProbeDistance), RequiredHalfWidth + WarningMargin);
    const float CeilingProbeHeight = FMath::Max(FMath::Max(10.0f, Settings.MaxCeilingProbeHeight), RequiredHeight + WarningMargin);
    const float MaxStepUp = FMath::Max(0.0f, Settings.MaxStepUp);
    const float MaxStepDown = FMath::Max(0.0f, Settings.MaxStepDown);
    const float FloorTraceHeight = FMath::Max(10.0f, Settings.FloorTraceHeight);
    const float FloorTraceDepth = FMath::Max(10.0f, Settings.FloorTraceDepth);
    const FVector HullLocalOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset);
    const float HullVerticalOffset = FVector::DotProduct(HullLocalOffset, InOutState.AnalysisUp);
    const FVector HullPlanarOffset = HullLocalOffset - InOutState.AnalysisUp * HullVerticalOffset;

    FCollisionShape HullShape;
    FQuat HullRotation = FQuat::Identity;
    float HullCenterHeight = 0.0f;
    bool bApproximateHull = false;
    MetricQueryAudit::BuildHullShape(InOutState, HullShape, HullRotation, HullCenterHeight, bApproximateHull);

    Result.RequiredWidth = RequiredWidth;
    Result.RequiredHeight = RequiredHeight;
    Result.MinimumAvailableWidth = TNumericLimits<float>::Max();
    Result.MinimumAvailableHeight = TNumericLimits<float>::Max();

    FCollisionQueryParams QueryParams = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricThresholdAnalysisTrace), true);
    QueryParams.bTraceComplex = true;

    bool bHavePreviousFloor = false;
    FVector PreviousFloorPoint = FVector::ZeroVector;
    FVector PreviousFloorNormal = InOutState.AnalysisUp;

    // Entry, midpoint, and exit. Threshold analysis is intentionally a compact
    // transition test rather than another long corridor sampler.
    for (int32 SampleIndex = 0; SampleIndex < 3; ++SampleIndex)
    {
        const float Alpha = static_cast<float>(SampleIndex) * 0.5f;
        const float ForwardOffset = ProbeDepth * Alpha;
        const FVector Station = InOutState.Location + Forward * ForwardOffset;

        FMetricThresholdSample& Sample = Result.Samples.AddDefaulted_GetRef();
        ++Result.TotalSamples;

        FHitResult FloorHit;
        const bool bHasFloor = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
            FloorHit,
            Station + InOutState.AnalysisUp * FloorTraceHeight,
            Station - InOutState.AnalysisUp * FloorTraceDepth,
            Settings.TraceChannel.GetValue(),
            QueryParams);

        Sample.bHasFloor = bHasFloor;
        if (!bHasFloor)
        {
            Sample.BaseOffset = Forward * ForwardOffset - InOutState.AnalysisUp * FloorTraceDepth;
            Sample.bStepDownFail = true;
            Sample.StepDelta = -FloorTraceDepth;
            ++Result.InvalidSamples;
            ++Result.FailedSamples;
            Result.bPass = false;
            Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, FloorTraceDepth);
            bHavePreviousFloor = false;
            continue;
        }

        const FVector FloorPoint = FloorHit.ImpactPoint;
        Sample.FloorNormal = FloorHit.ImpactNormal.GetSafeNormal();
        const FVector HullBase = FloorPoint + HullLocalOffset;
        Sample.BaseOffset = HullBase - InOutState.Location;

        if (bHavePreviousFloor)
        {
            Sample.StepDelta = MetricQueryAudit::ComputeSurfaceDiscontinuity(
                PreviousFloorPoint,
                PreviousFloorNormal,
                FloorPoint,
                Sample.FloorNormal,
                InOutState.AnalysisUp);
            Sample.bStepUpFail = Sample.StepDelta > MaxStepUp + KINDA_SMALL_NUMBER;
            Sample.bStepDownFail = Sample.StepDelta < -(MaxStepDown + KINDA_SMALL_NUMBER);
            Result.MaxObservedStepUp = FMath::Max(Result.MaxObservedStepUp, FMath::Max(0.0f, Sample.StepDelta));
            Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, FMath::Max(0.0f, -Sample.StepDelta));
        }
        PreviousFloorPoint = FloorPoint;
        PreviousFloorNormal = Sample.FloorNormal;
        bHavePreviousFloor = true;

        const float SupportHeight = MetricQueryAudit::ComputeHullSupportHeightAlongUp(
            InOutState, Sample.FloorNormal, HullPlanarOffset);
        const FVector HullCenter = FloorPoint + HullPlanarOffset
            + InOutState.AnalysisUp * (HullVerticalOffset + SupportHeight + 1.0f);
        FHitResult HullHit;
        Sample.bHullBlocked = MetricEnvironmentQuery::SweepSingleByChannel(World,
            HullHit,
            HullCenter - Forward,
            HullCenter + Forward,
            HullRotation,
            Settings.TraceChannel.GetValue(),
            HullShape,
            QueryParams);
        if (Sample.bHullBlocked)
        {
            Sample.HullImpactOffset = HullHit.ImpactPoint - InOutState.Location;
            ++Result.HullBlockedSamples;
        }

        float BestAvailableWidth = TNumericLimits<float>::Max();
        float BestWidthDeficit = 0.0f;
        FVector BestLeftOffset = FVector::ZeroVector;
        FVector BestRightOffset = FVector::ZeroVector;
        bool bWidthNear = false;
        constexpr float WidthFractions[] = { 0.10f, 0.50f, 0.90f };

        for (const float Fraction : WidthFractions)
        {
            const float WidthMeasureHeight = FMath::Clamp(RequiredHeight * Fraction, 1.0f, FMath::Max(1.0f, RequiredHeight - 1.0f));
            const FVector WidthCenter = HullBase + InOutState.AnalysisUp * WidthMeasureHeight;

            FHitResult LeftHit;
            FHitResult RightHit;
            const bool bLeftHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
                LeftHit, WidthCenter, WidthCenter - Right * SideProbeDistance, Settings.TraceChannel.GetValue(), QueryParams);
            const bool bRightHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
                RightHit, WidthCenter, WidthCenter + Right * SideProbeDistance, Settings.TraceChannel.GetValue(), QueryParams);

            const float LeftDistance = bLeftHit ? FMath::Max(0.0f, LeftHit.Distance) : SideProbeDistance;
            const float RightDistance = bRightHit ? FMath::Max(0.0f, RightHit.Distance) : SideProbeDistance;
            const float AvailableWidth = LeftDistance + RightDistance;
            const float WidthDeficit = FMath::Max(0.0f, RequiredHalfWidth - LeftDistance)
                + FMath::Max(0.0f, RequiredHalfWidth - RightDistance);
            bWidthNear = bWidthNear || (WidthDeficit <= KINDA_SMALL_NUMBER
                && (LeftDistance < RequiredHalfWidth + WarningMargin || RightDistance < RequiredHalfWidth + WarningMargin));

            if (AvailableWidth < BestAvailableWidth)
            {
                BestAvailableWidth = AvailableWidth;
                BestWidthDeficit = WidthDeficit;
                BestLeftOffset = (bLeftHit ? LeftHit.Location : WidthCenter - Right * SideProbeDistance) - InOutState.Location;
                BestRightOffset = (bRightHit ? RightHit.Location : WidthCenter + Right * SideProbeDistance) - InOutState.Location;
            }
        }

        Sample.LeftOffset = BestLeftOffset;
        Sample.RightOffset = BestRightOffset;
        Sample.AvailableWidth = BestAvailableWidth == TNumericLimits<float>::Max() ? 0.0f : BestAvailableWidth;
        Sample.WidthDeficit = BestWidthDeficit;
        Sample.bWidthFail = Sample.WidthDeficit > KINDA_SMALL_NUMBER;

        FHitResult CeilingHit;
        const bool bCeilingHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World,
            CeilingHit,
            HullBase + InOutState.AnalysisUp * 1.0f,
            HullBase + InOutState.AnalysisUp * CeilingProbeHeight,
            Settings.TraceChannel.GetValue(),
            QueryParams);
        Sample.CeilingOffset = (bCeilingHit ? CeilingHit.Location : HullBase + InOutState.AnalysisUp * CeilingProbeHeight) - InOutState.Location;
        Sample.AvailableHeight = bCeilingHit ? FMath::Max(0.0f, FVector::DotProduct(CeilingHit.Location - HullBase, InOutState.AnalysisUp)) : CeilingProbeHeight;
        Sample.HeightDeficit = bCeilingHit ? FMath::Max(0.0f, RequiredHeight - Sample.AvailableHeight) : 0.0f;
        Sample.bHeightFail = Sample.HeightDeficit > KINDA_SMALL_NUMBER;

        const bool bHeightNear = !Sample.bHeightFail
            && bCeilingHit
            && Sample.AvailableHeight < RequiredHeight + WarningMargin;
        Sample.bNearThreshold = !Sample.bHullBlocked && (bWidthNear || bHeightNear);

        Result.MinimumAvailableWidth = FMath::Min(Result.MinimumAvailableWidth, Sample.AvailableWidth);
        Result.MinimumAvailableHeight = FMath::Min(Result.MinimumAvailableHeight, Sample.AvailableHeight);
        Result.MaxWidthDeficit = FMath::Max(Result.MaxWidthDeficit, Sample.WidthDeficit);
        Result.MaxHeightDeficit = FMath::Max(Result.MaxHeightDeficit, Sample.HeightDeficit);
        Result.bNearThreshold = Result.bNearThreshold || Sample.bNearThreshold;

        if (Sample.bHullBlocked || Sample.bWidthFail || Sample.bHeightFail || Sample.bStepUpFail || Sample.bStepDownFail)
        {
            ++Result.FailedSamples;
            Result.bPass = false;
        }
    }

    if (Result.MinimumAvailableWidth == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableWidth = 0.0f;
    }
    if (Result.MinimumAvailableHeight == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableHeight = 0.0f;
    }
}


void UMetricSubsystem::EnsureRenderHost(UWorld* World)
{
    if (!World)
    {
        return;
    }

    if (AMetricRenderHostActor* ExistingHost = RenderHost.Get())
    {
        if (IsValid(ExistingHost) && ExistingHost->GetWorld() == World)
        {
            return;
        }
    }

    ClearRenderHost();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = MakeUniqueObjectName(World, AMetricRenderHostActor::StaticClass(), TEXT("MetricRenderHost"));
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bHideFromSceneOutliner = true;
    SpawnParams.bTemporaryEditorActor = true;

    RenderHost = World->SpawnActor<AMetricRenderHostActor>(AMetricRenderHostActor::StaticClass(), FTransform::Identity, SpawnParams);
}


FString UMetricSubsystem::RunPerformanceProfileCurrentLevel(int32 Iterations)
{
    Iterations = FMath::Clamp(Iterations, 1, 2000);
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World || !LiveState.bHasHit)
    {
        return TEXT("Metric performance profile requires an active Live Metric sample on valid geometry.");
    }

    MetricPerformance::BeginCapture(FString::Printf(TEXT("Current Level | %d iterations"), Iterations));
    for (int32 Iteration = 0; Iteration < Iterations; ++Iteration)
    {
        FMetricLiveState State = LiveState;
        State.QueryEvidence.Reset();
        MetricPerformance::BeginGeneration();
#define METRIC_PERF_CURRENT(Name, Expr) { MetricPerformance::FScopedAnalysis Scope(TEXT(Name)); Expr; }
        METRIC_PERF_CURRENT("Clearance", EvaluateClearance(World, State));
        METRIC_PERF_CURRENT("Overhead", EvaluateOverheadClearance(World, State));
        METRIC_PERF_CURRENT("Ground", EvaluateGroundValidation(World, State));
        METRIC_PERF_CURRENT("Fit", EvaluateFitAnalysis(World, State));
        METRIC_PERF_CURRENT("Threshold", EvaluateThresholdAnalysis(World, State));
        METRIC_PERF_CURRENT("TurnClearance", EvaluateTurnClearance(World, State));
        METRIC_PERF_CURRENT("Ledge", EvaluateLedgeAnalysis(World, State));
        METRIC_PERF_CURRENT("StoppingDistance", EvaluateStoppingDistance(World, State));
        METRIC_PERF_CURRENT("SensorField", EvaluateSensorField(World, State));
        METRIC_PERF_CURRENT("Trajectory", EvaluateTrajectory(World, State));
        METRIC_PERF_CURRENT("CameraProbe", EvaluateCameraProbe(World, State));
        METRIC_PERF_CURRENT("CameraSightline", EvaluateCameraSightline(World, State));
        METRIC_PERF_CURRENT("CoverExposure", EvaluateCoverExposure(World, State));
        METRIC_PERF_CURRENT("JumpRunway", EvaluateJumpRunway(World, State));
        METRIC_PERF_CURRENT("InteractionReach", EvaluateInteractionReach(World, State));
        METRIC_PERF_CURRENT("ShoulderClearance", EvaluateShoulderClearance(World, State));
        METRIC_PERF_CURRENT("CombatDensity", EvaluateCombatDensity(World, State));
        METRIC_PERF_CURRENT("AlternateStance", EvaluateAlternateStance(World, State));
        METRIC_PERF_CURRENT("MantleVault", EvaluateMantleVault(World, State));
        METRIC_PERF_CURRENT("RecoverySpace", EvaluateRecoverySpace(World, State));
        METRIC_PERF_CURRENT("Crossfire", EvaluateCrossfire(World, State));
        METRIC_PERF_CURRENT("QueryEvidence", BuildQueryEvidence(State));
#undef METRIC_PERF_CURRENT
        MetricPerformance::EndGeneration();
    }

    const MetricPerformance::FCaptureSummary Summary = MetricPerformance::EndCapture();
    return Summary.ToString();
}

void UMetricSubsystem::ClearRenderHost()
{
    // Reset the subsystem handle first. Even if actor destruction triggers GC or
    // world callbacks, Metric can no longer be part of a strong reference chain.
    AMetricRenderHostActor* Host = RenderHost.Get();
    RenderHost.Reset();

    if (!IsValid(Host))
    {
        return;
    }

    UWorld* World = Host->GetWorld();
    if (!World || World->IsBeingCleanedUp())
    {
        return;
    }

    Host->SetMetricVisible(false);
    World->DestroyActor(Host, false, true);
}

void UMetricSubsystem::NotifyStateChanged()
{
    if (GEditor)
    {
        GEditor->RedrawAllViewports(false);
    }
}


void UMetricSubsystem::EvaluateStoppingDistance(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.StoppingDistance = FMetricStoppingDistanceResult();
    if (!World || !InOutState.bHasHit || !InOutState.StoppingDistanceSettings.bShowStoppingDistance
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero()) Forward = InOutState.AnalysisForward;

    const float Speed = FMath::Max(1.0f, InOutState.StoppingDistanceSettings.Speed);
    const float Decel = FMath::Max(1.0f, InOutState.StoppingDistanceSettings.Deceleration);
    const float ReactionTime = FMath::Max(0.0f, InOutState.StoppingDistanceSettings.ReactionTime);
    const float WarningMargin = FMath::Max(0.0f, InOutState.StoppingDistanceSettings.WarningMargin);
    const float ReactionDistance = Speed * ReactionTime;
    const float BrakingDistance = (Speed * Speed) / (2.0f * Decel);
    const float RequiredDistance = ReactionDistance + BrakingDistance;

    const float Height = FMath::Max(1.0f, InOutState.Hull.Height);
    const float Width = FMath::Max(1.0f, InOutState.Hull.Width);
    const float Length = FMath::Max(1.0f, InOutState.Hull.Length);
    FCollisionShape Shape;
    FQuat Rotation = FQuat::Identity;
    float CenterHeight = Height * 0.5f;
    bool bApproximateShape = false;

    switch (InOutState.Hull.HullType)
    {
    case EMetricHullType::Capsule:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        Rotation = FRotationMatrix::MakeFromXZ(Forward, InOutState.AnalysisUp).ToQuat();
        CenterHeight = HalfHeight;
        break;
    }
    case EMetricHullType::OrientedBox:
        Shape = FCollisionShape::MakeBox(FVector(Length * 0.5f, Width * 0.5f, Height * 0.5f));
        Rotation = FRotationMatrix::MakeFromXZ(Forward, InOutState.AnalysisUp).ToQuat();
        break;
    case EMetricHullType::Sphere:
    {
        const float Radius = FMath::Max3(Width, Length, Height) * 0.5f;
        Shape = FCollisionShape::MakeSphere(FMath::Max(1.0f, Radius));
        CenterHeight = Radius;
        break;
    }
    case EMetricHullType::Cylinder:
    default:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        Rotation = FRotationMatrix::MakeFromXZ(Forward, InOutState.AnalysisUp).ToQuat();
        CenterHeight = HalfHeight;
        bApproximateShape = true;
        break;
    }
    }

    const FVector StartOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + InOutState.AnalysisUp * (CenterHeight + 1.0f);
    const FVector EndOffset = StartOffset + Forward * RequiredDistance;
    const FVector Start = InOutState.Location + StartOffset;
    const FVector End = Start + Forward * (RequiredDistance + WarningMargin);

    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricStoppingDistanceSweep), true);
    Params.bTraceComplex = true;
    FHitResult Hit;
    const bool bHit = MetricEnvironmentQuery::SweepSingleByChannel(World, Hit, Start, End, Rotation,
        InOutState.StoppingDistanceSettings.TraceChannel.GetValue(), Shape, Params);

    FMetricStoppingDistanceResult& Result = InOutState.StoppingDistance;
    Result.bVisible = true;
    Result.bApproximateShape = bApproximateShape;
    Result.ReactionDistance = ReactionDistance;
    Result.BrakingDistance = BrakingDistance;
    Result.RequiredDistance = RequiredDistance;
    Result.StartOffset = StartOffset;
    Result.EndOffset = EndOffset;
    Result.bHasObstacle = bHit;

    if (bHit)
    {
        Result.ObstacleDistance = FMath::Max(0.0f, Hit.Distance);
        Result.AvailableDistance = Result.ObstacleDistance;
        Result.bBlocked = Result.ObstacleDistance + KINDA_SMALL_NUMBER < RequiredDistance;
        Result.bNearThreshold = !Result.bBlocked && Result.ObstacleDistance < RequiredDistance + WarningMargin;
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
        Result.ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
    }
    else
    {
        Result.ObstacleDistance = RequiredDistance + WarningMargin;
        Result.AvailableDistance = RequiredDistance + WarningMargin;
    }
}


void UMetricSubsystem::EvaluateCoverExposure(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.CoverExposure = FMetricCoverExposureResult();
    const FMetricCoverExposureSettings& Settings = InOutState.CoverExposureSettings;
    if (!World || !InOutState.bHasHit || !Settings.bShowCoverExposure || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    const FVector Base = InOutState.Location + MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset);
    const FVector Threat = Base + Forward * FMath::Max(50.0f, Settings.ThreatDistance) + InOutState.AnalysisUp * Settings.ThreatHeight;
    const FVector Low = Base + InOutState.AnalysisUp * Settings.LowSampleHeight;
    const FVector Torso = Base + InOutState.AnalysisUp * Settings.TorsoSampleHeight;
    const FVector Head = Base + InOutState.AnalysisUp * Settings.HeadSampleHeight;

    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricCoverExposure), true);
    const auto IsCovered = [&](const FVector& Target)
    {
        FHitResult Hit;
        const bool bHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World, Hit, Threat, Target, Settings.TraceChannel.GetValue(), Params);
        return bHit && Hit.Distance + 2.0f < FVector::Distance(Threat, Target);
    };

    FMetricCoverExposureResult& Result = InOutState.CoverExposure;
    Result.bVisible = true;
    Result.ThreatOffset = Threat - InOutState.Location;
    Result.LowOffset = Low - InOutState.Location;
    Result.TorsoOffset = Torso - InOutState.Location;
    Result.HeadOffset = Head - InOutState.Location;
    Result.bLowCovered = IsCovered(Low);
    Result.bTorsoCovered = IsCovered(Torso);
    Result.bHeadCovered = IsCovered(Head);
    const int32 CoveredCount = (Result.bLowCovered ? 1 : 0) + (Result.bTorsoCovered ? 1 : 0) + (Result.bHeadCovered ? 1 : 0);
    Result.ExposurePercent = 100.0f * static_cast<float>(3 - CoveredCount) / 3.0f;
}

void UMetricSubsystem::EvaluateJumpRunway(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.JumpRunway = FMetricJumpRunwayResult();
    const FMetricJumpRunwaySettings& S = InOutState.JumpRunwaySettings;
    if (!World || !InOutState.bHasHit || !S.bShowJumpRunway
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FVector Up = InOutState.AnalysisUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, Up).GetSafeNormal();
    if (Forward.IsNearlyZero()) Forward = InOutState.AnalysisForward;

    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricJumpRunway), true);
    Params.bTraceComplex = true;

    const float ProbeDepth = FMath::Max(20.0f, S.GroundProbeDepth);
    const float SampleSpacing = FMath::Clamp(FMath::Max(25.0f, InOutState.Hull.Width * 0.5f), 25.0f, 75.0f);
    const float MaxWalkableSlope = FMath::Clamp(InOutState.GroundValidationSettings.MaxWalkableSlope, 0.0f, 89.0f);
    const float MaxStepUp = FMath::Max(0.0f, InOutState.GroundValidationSettings.MaxStepUp);
    const float MaxStepDown = FMath::Max(0.0f, InOutState.GroundValidationSettings.MaxStepDown);

    auto MeasureSupportedRunway = [&](const FVector& Start, const FVector& Direction, float RequestedDistance, float& OutAvailable)
    {
        RequestedDistance = FMath::Max(0.0f, RequestedDistance);
        OutAvailable = 0.0f;
        if (RequestedDistance <= KINDA_SMALL_NUMBER) return true;

        const int32 SegmentCount = FMath::Max(1, FMath::CeilToInt(RequestedDistance / SampleSpacing));
        bool bHavePrevious = false;
        FVector PreviousPoint = FVector::ZeroVector;
        FVector PreviousNormal = Up;

        for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
        {
            const float Distance = RequestedDistance * (static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount));
            const FVector Sample = Start + Direction * Distance;
            FHitResult GroundHit;
            const bool bHasGround = MetricEnvironmentQuery::LineTraceSingleByChannel(World, GroundHit,
                Sample + Up * 80.0f, Sample - Up * ProbeDepth,
                S.TraceChannel.GetValue(), Params);
            if (!bHasGround) return false;

            const FVector Normal = GroundHit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, Up);
            const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(
                FMath::Clamp(FVector::DotProduct(Normal, Up), -1.0f, 1.0f)));
            if (SlopeDegrees > MaxWalkableSlope + KINDA_SMALL_NUMBER) return false;

            if (bHavePrevious)
            {
                const float Discontinuity = MetricQueryAudit::ComputeSurfaceDiscontinuity(
                    PreviousPoint, PreviousNormal, GroundHit.ImpactPoint, Normal, Up);
                if (Discontinuity > MaxStepUp + KINDA_SMALL_NUMBER
                    || -Discontinuity > MaxStepDown + KINDA_SMALL_NUMBER)
                {
                    return false;
                }
            }

            PreviousPoint = GroundHit.ImpactPoint;
            PreviousNormal = Normal;
            bHavePrevious = true;
            OutAvailable = Distance;
        }
        return true;
    };

    const float TakeoffDistance = FMath::Max(0.0f, S.TakeoffRunway);
    const float LandingDistance = FMath::Max(0.0f, S.LandingRunway);
    const FVector TakeoffEnd = InOutState.Location - Forward * TakeoffDistance;

    FMetricTrajectoryResult LandingTrajectory = InOutState.Trajectory;
    if (!LandingTrajectory.bVisible || !LandingTrajectory.bLandingValid)
    {
        FMetricLiveState TrajectoryState = InOutState;
        TrajectoryState.TrajectorySettings.bShowTrajectory = true;
        TrajectoryState.TrajectorySettings.DisplayMode = EMetricTrajectoryDisplayMode::SingleArc;
        EvaluateTrajectory(World, TrajectoryState);
        LandingTrajectory = TrajectoryState.Trajectory;
    }

    FMetricJumpRunwayResult& R = InOutState.JumpRunway;
    R.bVisible = true;
    R.TakeoffEndOffset = TakeoffEnd - InOutState.Location;
    R.bTakeoffPass = MeasureSupportedRunway(InOutState.Location, -Forward, TakeoffDistance, R.TakeoffAvailable);

    if (!LandingTrajectory.bLandingValid)
    {
        R.bLandingPass = false;
        R.LandingStartOffset = FVector::ZeroVector;
        R.LandingEndOffset = FVector::ZeroVector;
        R.LandingAvailable = 0.0f;
        return;
    }

    const FVector LandingStart = InOutState.Location + LandingTrajectory.ImpactPointOffset;
    const FVector LandingEnd = LandingStart + Forward * LandingDistance;
    R.LandingStartOffset = LandingStart - InOutState.Location;
    R.LandingEndOffset = LandingEnd - InOutState.Location;
    R.bLandingPass = MeasureSupportedRunway(LandingStart, Forward, LandingDistance, R.LandingAvailable);
}

void UMetricSubsystem::EvaluateInteractionReach(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.InteractionReach = FMetricInteractionReachResult();
    const FMetricInteractionReachSettings& Settings = InOutState.InteractionReachSettings;
    if (!World || !InOutState.bHasHit || !Settings.bShowInteractionReach || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    const FVector Base = InOutState.Location + MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset);
    const FVector Start = Base + InOutState.AnalysisUp * Settings.ReachHeight;
    const FVector End = Start + Forward * FMath::Max(10.0f, Settings.ReachDistance);

    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricInteractionReach), true);
    FHitResult Hit;
    const bool bHit = MetricEnvironmentQuery::SweepSingleByChannel(
        World,
        Hit,
        Start,
        End,
        FQuat::Identity,
        Settings.TraceChannel.GetValue(),
        FCollisionShape::MakeSphere(FMath::Max(1.0f, Settings.ProbeRadius)),
        Params);

    FMetricInteractionReachResult& Result = InOutState.InteractionReach;
    Result.bVisible = true;
    Result.bHasTarget = bHit;
    Result.TargetDistance = bHit ? Hit.Distance : Settings.ReachDistance;
    Result.OriginOffset = Start - InOutState.Location;
    Result.EndOffset = End - InOutState.Location;
    if (bHit)
    {
        Result.HitOffset = Hit.ImpactPoint - InOutState.Location;
    }
}

void UMetricSubsystem::EvaluateShoulderClearance(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.ShoulderClearance = FMetricShoulderClearanceResult();
    const FMetricShoulderClearanceSettings& Settings = InOutState.ShoulderClearanceSettings;
    if (!World || !InOutState.bHasHit || !Settings.bShowShoulderClearance || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = InOutState.AnalysisRight;
    }

    const FVector Base = InOutState.Location + MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset);
    const FVector Pivot = Base + InOutState.AnalysisUp * Settings.PivotHeight;
    const float Distance = FMath::Max(10.0f, Settings.CameraDistance);
    const float Side = FMath::Max(0.0f, Settings.ShoulderOffset);
    const FVector LeftDesired = Pivot - Forward * Distance - Right * Side;
    const FVector RightDesired = Pivot - Forward * Distance + Right * Side;

    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricShoulderClearance), true);
    const FCollisionShape Shape = FCollisionShape::MakeSphere(FMath::Max(1.0f, Settings.ProbeRadius));
    FHitResult LeftHit;
    FHitResult RightHit;
    const bool bLeft = MetricEnvironmentQuery::SweepSingleByChannel(World, LeftHit, Pivot, LeftDesired, FQuat::Identity, Settings.TraceChannel.GetValue(), Shape, Params);
    const bool bRight = MetricEnvironmentQuery::SweepSingleByChannel(World, RightHit, Pivot, RightDesired, FQuat::Identity, Settings.TraceChannel.GetValue(), Shape, Params);

    FMetricShoulderClearanceResult& Result = InOutState.ShoulderClearance;
    Result.bVisible = true;
    Result.bLeftBlocked = bLeft;
    Result.bRightBlocked = bRight;
    Result.LeftDistance = bLeft ? LeftHit.Distance : FVector::Distance(Pivot, LeftDesired);
    Result.RightDistance = bRight ? RightHit.Distance : FVector::Distance(Pivot, RightDesired);
    Result.PivotOffset = Pivot - InOutState.Location;
    Result.LeftOffset = (bLeft ? LeftHit.Location : LeftDesired) - InOutState.Location;
    Result.RightOffset = (bRight ? RightHit.Location : RightDesired) - InOutState.Location;
}

void UMetricSubsystem::EvaluateCombatDensity(UWorld* World, FMetricLiveState& InOutState) const
{
    (void)World; InOutState.CombatDensity=FMetricCombatDensityResult(); const auto& S=InOutState.CombatDensitySettings;
    if(!InOutState.bHasHit||!S.bShowCombatDensity||InOutState.EntryType==EMetricCycleEntryType::NativePreviewMesh)return;
    auto& R=InOutState.CombatDensity; R.bVisible=true; R.AreaRadius=FMath::Max(50.0f,S.AreaRadius); R.PersonalDiameter=FMath::Max(10.0f,FMath::Max(InOutState.Hull.Width,InOutState.Hull.Length)+S.PersonalSpacePadding*2.0f);
    const float Area=PI*R.AreaRadius*R.AreaRadius; const float Cell=R.PersonalDiameter*R.PersonalDiameter; R.ApproxCapacity=FMath::Max(1,FMath::FloorToInt((Area/Cell)*0.78f));
}

void UMetricSubsystem::EvaluateAlternateStance(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.AlternateStance = FMetricAlternateStanceResult();
    const FMetricAlternateStanceSettings& S = InOutState.AlternateStanceSettings;
    if (!World || !InOutState.bHasHit || !S.bShowAlternateStance
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FVector Up = InOutState.AnalysisUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, Up).GetSafeNormal();
    if (Forward.IsNearlyZero()) Forward = InOutState.AnalysisForward;

    const float MaxProbe = FMath::Max(InOutState.Hull.Height, S.AlternateHeight)
        + FMath::Max(0.0f, S.WarningMargin) + 100.0f;
    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricAlternateStance), true);
    Params.bTraceComplex = true;

    FHitResult HeightHit;
    const bool bHeightHit = MetricEnvironmentQuery::LineTraceSingleByChannel(
        World, HeightHit, InOutState.Location + Up, InOutState.Location + Up * MaxProbe,
        S.TraceChannel.GetValue(), Params);
    const float Available = bHeightHit
        ? FMath::Max(0.0f, FVector::DotProduct(HeightHit.Location - InOutState.Location, Up))
        : MaxProbe;

    auto FitsHullAtHeight = [&](float RequestedHeight)
    {
        FMetricLiveState HullState = InOutState;
        HullState.Hull.Height = FMath::Max(1.0f, RequestedHeight);

        FCollisionShape Shape;
        FQuat Rotation = FQuat::Identity;
        float IgnoredCenterHeight = 0.0f;
        bool bApproximate = false;
        MetricQueryAudit::BuildHullShape(HullState, Shape, Rotation, IgnoredCenterHeight, bApproximate);

        const FVector HullLocalOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset);
        const float HullVerticalOffset = FVector::DotProduct(HullLocalOffset, Up);
        const FVector HullPlanarOffset = HullLocalOffset - Up * HullVerticalOffset;
        const float SupportHeight = MetricQueryAudit::ComputeHullSupportHeightAlongUp(
            HullState, InOutState.SurfaceNormal.GetSafeNormal(SMALL_NUMBER, Up), HullPlanarOffset);
        const FVector Center = InOutState.Location
            + HullPlanarOffset
            + Up * (HullVerticalOffset + SupportHeight + 1.0f);

        FHitResult OccupancyHit;
        return !MetricEnvironmentQuery::SweepSingleByChannel(
            World, OccupancyHit, Center, Center + Forward,
            Rotation, S.TraceChannel.GetValue(), Shape, Params);
    };

    FMetricAlternateStanceResult& R = InOutState.AlternateStance;
    R.bVisible = true;
    R.AvailableHeight = Available;
    R.bStandingFits = Available + KINDA_SMALL_NUMBER >= InOutState.Hull.Height
        && FitsHullAtHeight(InOutState.Hull.Height);
    R.bAlternateFits = Available + KINDA_SMALL_NUMBER >= S.AlternateHeight
        && FitsHullAtHeight(S.AlternateHeight);
}

void UMetricSubsystem::EvaluateMantleVault(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.MantleVault = FMetricMantleVaultResult();
    const FMetricMantleVaultSettings& S = InOutState.MantleVaultSettings;
    if (!World || !InOutState.bHasHit || !S.bShowMantleVault
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricMantleVault), true);
    Params.bTraceComplex = true;

    const float ForwardReach = FMath::Max(10.0f, S.ForwardReach);
    const FVector FrontStart = InOutState.Location + InOutState.AnalysisUp * FMath::Max(10.0f, S.MinObstacleHeight);
    const FVector FrontEnd = FrontStart + Forward * ForwardReach;

    FHitResult FrontHit;
    const bool bFront = MetricEnvironmentQuery::LineTraceSingleByChannel(World, 
        FrontHit,
        FrontStart,
        FrontEnd,
        S.TraceChannel.GetValue(),
        Params);

    FMetricMantleVaultResult& R = InOutState.MantleVault;
    R.bVisible = true;
    R.bHasObstacle = bFront;
    R.FrontOffset = (bFront ? FrontHit.ImpactPoint : FrontEnd) - InOutState.Location;
    if (!bFront)
    {
        return;
    }

    const FVector TopProbeXY = FrontHit.ImpactPoint + Forward * 5.0f;
    const FVector TopStart = TopProbeXY + InOutState.AnalysisUp * (FMath::Max(S.MaxObstacleHeight, S.MinObstacleHeight) + 50.0f);
    const FVector TopEnd = TopProbeXY - InOutState.AnalysisUp * 20.0f;

    FHitResult TopHit;
    const bool bTop = MetricEnvironmentQuery::LineTraceSingleByChannel(World, 
        TopHit,
        TopStart,
        TopEnd,
        S.TraceChannel.GetValue(),
        Params);
    if (!bTop)
    {
        return;
    }

    R.TopOffset = TopHit.ImpactPoint - InOutState.Location;
    R.ObstacleHeight = FVector::DotProduct(TopHit.ImpactPoint - InOutState.Location, InOutState.AnalysisUp);
    R.bHeightValid = R.ObstacleHeight >= S.MinObstacleHeight
        && R.ObstacleHeight <= S.MaxObstacleHeight;

    // Measure top-surface continuation with vertical probes. A horizontal trace
    // beginning on the top surface can self-hit and incorrectly report ~0 depth.
    const float MaxDepth = FMath::Max(10.0f, S.MaxObstacleDepth);
    const float DepthSampleSpacing = FMath::Clamp(MaxDepth / 10.0f, 5.0f, 20.0f);
    const float HeightTolerance = FMath::Clamp(InOutState.Hull.Height * 0.05f, 5.0f, 20.0f);
    const float ProbePastMax = MaxDepth + DepthSampleSpacing;
    const int32 DepthSegments = FMath::Max(1, FMath::CeilToInt(ProbePastMax / DepthSampleSpacing));

    float LastSupportedDepth = 0.0f;
    bool bStillSupportedPastMax = false;

    for (int32 SegmentIndex = 1; SegmentIndex <= DepthSegments; ++SegmentIndex)
    {
        const float Distance = FMath::Min(ProbePastMax, SegmentIndex * DepthSampleSpacing);
        const FVector ProbeCenter = TopHit.ImpactPoint + Forward * Distance;
        const FVector ProbeStart = ProbeCenter + InOutState.AnalysisUp * 25.0f;
        const FVector ProbeEnd = ProbeCenter - InOutState.AnalysisUp * 50.0f;

        FHitResult SurfaceHit;
        const bool bSurface = MetricEnvironmentQuery::LineTraceSingleByChannel(World, 
            SurfaceHit,
            ProbeStart,
            ProbeEnd,
            S.TraceChannel.GetValue(),
            Params);

        const bool bSameTop = bSurface
            && FMath::Abs(FVector::DotProduct(SurfaceHit.ImpactPoint - TopHit.ImpactPoint, InOutState.AnalysisUp)) <= HeightTolerance;

        if (!bSameTop)
        {
            break;
        }

        LastSupportedDepth = Distance;
        if (Distance > MaxDepth + KINDA_SMALL_NUMBER)
        {
            bStillSupportedPastMax = true;
            break;
        }
    }

    R.ObstacleDepth = LastSupportedDepth;
    R.bDepthValid = !bStillSupportedPastMax && LastSupportedDepth <= MaxDepth + KINDA_SMALL_NUMBER;
}

void UMetricSubsystem::EvaluateRecoverySpace(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.RecoverySpace = FMetricRecoverySpaceResult();
    const FMetricRecoverySpaceSettings& S = InOutState.RecoverySpaceSettings;
    if (!World || !InOutState.bHasHit || !S.bShowRecoverySpace
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }
    const FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();

    const float Height = FMath::Max(1.0f, InOutState.Hull.Height);
    const float Width = FMath::Max(1.0f, InOutState.Hull.Width);
    const float Length = FMath::Max(1.0f, InOutState.Hull.Length);

    FCollisionShape Shape;
    FQuat ShapeRotation = FQuat::Identity;
    float CenterHeight = Height * 0.5f;

    switch (InOutState.Hull.HullType)
    {
    case EMetricHullType::Capsule:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        ShapeRotation = FRotationMatrix::MakeFromXZ(Forward, InOutState.AnalysisUp).ToQuat();
        CenterHeight = HalfHeight;
        break;
    }
    case EMetricHullType::OrientedBox:
        Shape = FCollisionShape::MakeBox(FVector(Length * 0.5f, Width * 0.5f, Height * 0.5f));
        ShapeRotation = FRotationMatrix::MakeFromXZ(Forward, InOutState.AnalysisUp).ToQuat();
        CenterHeight = Height * 0.5f;
        break;
    case EMetricHullType::Sphere:
    {
        const float Radius = FMath::Max3(Height, Width, Length) * 0.5f;
        Shape = FCollisionShape::MakeSphere(FMath::Max(1.0f, Radius));
        CenterHeight = Radius;
        break;
    }
    case EMetricHullType::Cylinder:
    default:
    {
        // UE collision sweeps do not expose an exact arbitrary cylinder shape.
        // Approximate with a capsule, matching Metric's other passage analyses.
        const float Radius = FMath::Max(1.0f, FMath::Max(Width, Length) * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        ShapeRotation = FRotationMatrix::MakeFromXZ(Forward, InOutState.AnalysisUp).ToQuat();
        CenterHeight = HalfHeight;
        break;
    }
    }

    // Lift the recovery sweep a tiny amount above the placement surface. Starting
    // exactly tangent to a flat floor can produce initial-overlap / floating-point
    // hits at distance zero, which read as false blocked directions.
    constexpr float RecoveryFloorEpsilon = 1.0f;
    const FVector Start = InOutState.Location
        + MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset)
        + InOutState.AnalysisUp * (CenterHeight + RecoveryFloorEpsilon);

    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricRecoverySpace), true);
    Params.bTraceComplex = true;

    const float GroundProbeDepth = FMath::Max(50.0f, InOutState.GroundValidationSettings.TraceDepth);
    const float MaxWalkableSlope = FMath::Clamp(InOutState.GroundValidationSettings.MaxWalkableSlope, 0.0f, 89.0f);
    const float MaxStepUp = FMath::Max(0.0f, InOutState.GroundValidationSettings.MaxStepUp);
    const float MaxStepDown = FMath::Max(0.0f, InOutState.GroundValidationSettings.MaxStepDown);
    const float SupportSpacing = FMath::Clamp(FMath::Max(25.0f, Width * 0.5f), 25.0f, 75.0f);

    auto Sweep = [&](const FVector& Direction, float RequestedDistance, float& OutClearDistance, bool& bOutGroundSupported)
    {
        const float Distance = FMath::Max(0.0f, RequestedDistance);
        FHitResult Hit;
        const bool bBlocked = MetricEnvironmentQuery::SweepSingleByChannel(World,
            Hit, Start, Start + Direction * Distance, ShapeRotation,
            S.TraceChannel.GetValue(), Shape, Params);

        const float ObstructionClearDistance = bBlocked ? FMath::Max(0.0f, Hit.Distance) : Distance;
        OutClearDistance = ObstructionClearDistance;
        bOutGroundSupported = true;

        const int32 SegmentCount = FMath::Max(1, FMath::CeilToInt(ObstructionClearDistance / SupportSpacing));
        bool bHavePrevious = false;
        FVector PreviousPoint = FVector::ZeroVector;
        FVector PreviousNormal = InOutState.SurfaceNormal.GetSafeNormal(SMALL_NUMBER, InOutState.AnalysisUp);

        for (int32 SegmentIndex = 0; SegmentIndex <= SegmentCount; ++SegmentIndex)
        {
            const float SampleDistance = ObstructionClearDistance * (static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount));
            const FVector Sample = InOutState.Location + Direction * SampleDistance;
            FHitResult GroundHit;
            const bool bGround = MetricEnvironmentQuery::LineTraceSingleByChannel(
                World, GroundHit, Sample + InOutState.AnalysisUp * 80.0f,
                Sample - InOutState.AnalysisUp * GroundProbeDepth,
                S.TraceChannel.GetValue(), Params);

            if (!bGround)
            {
                bOutGroundSupported = false;
                OutClearDistance = FMath::Min(OutClearDistance, SampleDistance);
                return false;
            }

            const FVector Normal = GroundHit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, InOutState.AnalysisUp);
            const float SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(
                FMath::Clamp(FVector::DotProduct(Normal, InOutState.AnalysisUp), -1.0f, 1.0f)));
            if (SlopeDegrees > MaxWalkableSlope + KINDA_SMALL_NUMBER)
            {
                bOutGroundSupported = false;
                OutClearDistance = FMath::Min(OutClearDistance, SampleDistance);
                return false;
            }

            if (bHavePrevious)
            {
                const float Discontinuity = MetricQueryAudit::ComputeSurfaceDiscontinuity(
                    PreviousPoint, PreviousNormal, GroundHit.ImpactPoint, Normal, InOutState.AnalysisUp);
                if (Discontinuity > MaxStepUp + KINDA_SMALL_NUMBER
                    || -Discontinuity > MaxStepDown + KINDA_SMALL_NUMBER)
                {
                    bOutGroundSupported = false;
                    OutClearDistance = FMath::Min(OutClearDistance, SampleDistance);
                    return false;
                }
            }

            PreviousPoint = GroundHit.ImpactPoint;
            PreviousNormal = Normal;
            bHavePrevious = true;
        }

        return !bBlocked;
    };

    FMetricRecoverySpaceResult& R = InOutState.RecoverySpace;
    R.bVisible = true;
    R.bForwardClear = Sweep(Forward, S.ForwardDistance, R.ForwardClear, R.bForwardGroundSupported);
    R.bBackwardClear = Sweep(-Forward, S.BackwardDistance, R.BackwardClear, R.bBackwardGroundSupported);
    R.bLeftClear = Sweep(-Right, S.SideDistance, R.LeftClear, R.bLeftGroundSupported);
    R.bRightClear = Sweep(Right, S.SideDistance, R.RightClear, R.bRightGroundSupported);
}

void UMetricSubsystem::EvaluateCrossfire(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.Crossfire = FMetricCrossfireResult();
    const FMetricCrossfireSettings& Settings = InOutState.CrossfireSettings;
    if (!World || !InOutState.bHasHit || !Settings.bShowCrossfire || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = FVector::VectorPlaneProject(InOutState.Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = InOutState.AnalysisForward;
    }

    FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = InOutState.AnalysisRight;
    }

    const FVector Base = InOutState.Location + MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset);
    const int32 Count = FMath::Clamp(Settings.DirectionCount, 4, 16);
    const FVector Target = Base + InOutState.AnalysisUp * Settings.SampleHeight;
    const float ThreatDistance = FMath::Max(100.0f, Settings.ThreatDistance);
    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricCrossfire), true);

    FMetricCrossfireResult& Result = InOutState.Crossfire;
    Result.bVisible = true;
    Result.TotalDirections = Count;
    Result.Rays.Reserve(Count);

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float Angle = 2.0f * PI * static_cast<float>(Index) / static_cast<float>(Count);
        const FVector Direction = (Forward * FMath::Cos(Angle) + Right * FMath::Sin(Angle)).GetSafeNormal();
        const FVector Threat = Target + Direction * ThreatDistance;
        FHitResult Hit;
        const bool bHit = MetricEnvironmentQuery::LineTraceSingleByChannel(World, Hit, Threat, Target, Settings.TraceChannel.GetValue(), Params);

        FMetricCrossfireRay& Ray = Result.Rays.AddDefaulted_GetRef();
        Ray.EndOffset = Threat - InOutState.Location;
        Ray.bExposed = !bHit || Hit.Distance + 2.0f >= FVector::Distance(Threat, Target);
        if (Ray.bExposed)
        {
            ++Result.ExposedDirections;
        }
    }

    Result.ExposurePercent = 100.0f * static_cast<float>(Result.ExposedDirections) / static_cast<float>(FMath::Max(1, Result.TotalDirections));
}

void UMetricSubsystem::EvaluateTurnClearance(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.TurnClearance = FMetricTurnClearanceResult();

    if (!World || !InOutState.bHasHit || !InOutState.TurnClearanceSettings.bShowTurnClearance
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward = FVector::VectorPlaneProject(Forward, InOutState.AnalysisUp).GetSafeNormal();
    if (Forward.IsNearlyZero()) Forward = InOutState.AnalysisForward;
    const FVector Right = FVector::CrossProduct(InOutState.AnalysisUp, Forward).GetSafeNormal();

    const float Height = FMath::Max(1.0f, InOutState.Hull.Height);
    const float Width = FMath::Max(1.0f, InOutState.Hull.Width);
    const float Length = FMath::Max(1.0f, InOutState.Hull.Length);
    const float Radius = FMath::Max(10.0f, InOutState.TurnClearanceSettings.TurnRadius);
    const float TurnAngle = FMath::Clamp(InOutState.TurnClearanceSettings.TurnAngle, 10.0f, 180.0f);
    const float StepAngle = FMath::Clamp(InOutState.TurnClearanceSettings.SampleAngle, 2.0f, 30.0f);

    FCollisionShape Shape;
    float CenterHeight = Height * 0.5f;
    bool bApproximateShape = false;
    switch (InOutState.Hull.HullType)
    {
    case EMetricHullType::Capsule:
    {
        const float HullRadius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(HullRadius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(HullRadius, HalfHeight);
        CenterHeight = HalfHeight;
        break;
    }
    case EMetricHullType::OrientedBox:
        Shape = FCollisionShape::MakeBox(FVector(Length * 0.5f, Width * 0.5f, Height * 0.5f));
        CenterHeight = Height * 0.5f;
        break;
    case EMetricHullType::Sphere:
    {
        const float HullRadius = FMath::Max3(Width, Length, Height) * 0.5f;
        Shape = FCollisionShape::MakeSphere(FMath::Max(1.0f, HullRadius));
        CenterHeight = HullRadius;
        break;
    }
    case EMetricHullType::Cylinder:
    default:
    {
        const float HullRadius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(HullRadius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(HullRadius, HalfHeight);
        CenterHeight = HalfHeight;
        bApproximateShape = true;
        break;
    }
    }

    const FVector StartOffset = MetricFrame::LocalOffset(InOutState, InOutState.Hull.LocalOffset) + InOutState.AnalysisUp * (CenterHeight + 1.0f);
    const FVector StartWorld = InOutState.Location + StartOffset;
    FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, SCENE_QUERY_STAT(MetricTurnClearanceSweep), true);
    Params.bTraceComplex = true;

    auto EvaluateSide = [&](float Sign, FMetricTurnPathResult& OutPath)
    {
        OutPath = FMetricTurnPathResult();
        OutPath.bTested = true;
        OutPath.SampleOffsets.Add(StartOffset);

        const FVector CenterWorld = StartWorld + Right * (Sign * Radius);
        const FVector RadialStart = -Right * Sign;
        FVector PreviousWorld = StartWorld;
        float PreviousAngle = 0.0f;
        float TravelDistance = 0.0f;

        const int32 Steps = FMath::Max(1, FMath::CeilToInt(TurnAngle / StepAngle));
        for (int32 Step = 1; Step <= Steps; ++Step)
        {
            const float Angle = FMath::Min(TurnAngle, Step * StepAngle);
            const FQuat ArcRotation(InOutState.AnalysisUp, FMath::DegreesToRadians(Sign * Angle));
            const FVector Radial = ArcRotation.RotateVector(RadialStart);
            const FVector CurrentWorld = CenterWorld + Radial * Radius;
            const FVector Tangent = ArcRotation.RotateVector(Forward).GetSafeNormal();
            FQuat ShapeRotation = FQuat::Identity;
            if (InOutState.Hull.HullType != EMetricHullType::Sphere)
            {
                ShapeRotation = FRotationMatrix::MakeFromXZ(Tangent, InOutState.AnalysisUp).ToQuat();
            }

            FHitResult Hit;
            if (MetricEnvironmentQuery::SweepSingleByChannel(World, Hit, PreviousWorld, CurrentWorld, ShapeRotation,
                InOutState.TurnClearanceSettings.TraceChannel.GetValue(), Shape, Params))
            {
                TravelDistance += Hit.Distance;
                OutPath.bBlocked = true;
                OutPath.ClearAngle = PreviousAngle + (Angle - PreviousAngle) * Hit.Time;
                OutPath.ClearDistance = TravelDistance;
                OutPath.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
                OutPath.ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
                const FVector HitCenter = FMath::Lerp(PreviousWorld, CurrentWorld, Hit.Time);
                OutPath.SampleOffsets.Add(HitCenter - InOutState.Location);
                return;
            }

            TravelDistance += FVector::Distance(PreviousWorld, CurrentWorld);
            OutPath.SampleOffsets.Add(CurrentWorld - InOutState.Location);
            PreviousWorld = CurrentWorld;
            PreviousAngle = Angle;
        }

        OutPath.bBlocked = false;
        OutPath.ClearAngle = TurnAngle;
        OutPath.ClearDistance = TravelDistance;
    };

    FMetricTurnClearanceResult& Result = InOutState.TurnClearance;
    Result.bVisible = true;
    Result.bApproximateShape = bApproximateShape;
    if (InOutState.TurnClearanceSettings.Direction == EMetricTurnDirection::Left
        || InOutState.TurnClearanceSettings.Direction == EMetricTurnDirection::Both)
    {
        EvaluateSide(-1.0f, Result.Left);
    }
    if (InOutState.TurnClearanceSettings.Direction == EMetricTurnDirection::Right
        || InOutState.TurnClearanceSettings.Direction == EMetricTurnDirection::Both)
    {
        EvaluateSide(1.0f, Result.Right);
    }
}



void UMetricSubsystem::BuildQueryEvidence(FMetricLiveState& InOutState) const
{
    InOutState.QueryEvidence.Reset();

    auto Add = [&](EMetricInspectionTarget Target, EMetricQueryEvidenceKind Kind, EMetricQueryEvidenceStatus Status,
        const FVector& Start, const FVector& End, const FString& Label,
        bool bHasImpact = false, const FVector& Impact = FVector::ZeroVector, const FVector& Normal = FVector::UpVector)
    {
        FMetricQueryEvidence& E = InOutState.QueryEvidence.AddDefaulted_GetRef();
        E.Target = Target;
        E.Kind = Kind;
        E.Status = Status;
        E.StartOffset = Start;
        E.EndOffset = End;
        E.Label = Label;
        E.bHasImpact = bHasImpact;
        E.ImpactOffset = Impact;
        E.ImpactNormal = Normal.GetSafeNormal();
    };

    if (InOutState.Clearance.bVisible)
    {
        const EMetricQueryEvidenceStatus Status = InOutState.Clearance.bBlocked ? EMetricQueryEvidenceStatus::Fail
            : (InOutState.Clearance.bNearThreshold ? EMetricQueryEvidenceStatus::Warning : EMetricQueryEvidenceStatus::Pass);
        Add(EMetricInspectionTarget::Clearance, EMetricQueryEvidenceKind::Sweep, Status,
            InOutState.Clearance.SweepStartOffset, InOutState.Clearance.SweepEndOffset,
            FString::Printf(TEXT("Clear %.0f / %.0f cm"), InOutState.Clearance.ClearDistance, InOutState.Clearance.ProbeDistance),
            InOutState.Clearance.bHasObstacle, InOutState.Clearance.ImpactPointOffset, InOutState.Clearance.ImpactNormal);
    }

    if (InOutState.GroundValidation.bVisible)
    {
        const float TraceHeight = FMath::Max(10.0f, InOutState.GroundValidationSettings.TraceHeight);
        const float TraceDepth = FMath::Max(10.0f, InOutState.GroundValidationSettings.TraceDepth);
        for (const FMetricGroundSample& Sample : InOutState.GroundValidation.Samples)
        {
            const bool bFail = !Sample.bHit || Sample.bSlopeFail || Sample.bStepUpFail || Sample.bStepDownFail;
            const FVector Surface = Sample.Offset;
            const FVector Start = Surface + InOutState.AnalysisUp * (Sample.bHit ? TraceHeight : (TraceHeight + TraceDepth));
            const FVector End = Surface - InOutState.AnalysisUp * (Sample.bHit ? TraceDepth : 0.0f);
            FString Reason = TEXT("Ground sample");
            if (!Sample.bHit) Reason = TEXT("Missing floor");
            else if (Sample.bSlopeFail) Reason = FString::Printf(TEXT("Slope %.1f deg"), Sample.SlopeDegrees);
            else if (Sample.bStepUpFail) Reason = FString::Printf(TEXT("Step up %.1f cm"), Sample.StepDelta);
            else if (Sample.bStepDownFail) Reason = FString::Printf(TEXT("Step down %.1f cm"), -Sample.StepDelta);
            Add(EMetricInspectionTarget::GroundValidation, EMetricQueryEvidenceKind::Trace,
                bFail ? EMetricQueryEvidenceStatus::Fail : EMetricQueryEvidenceStatus::Pass,
                Start, End, Reason, Sample.bHit, Sample.Offset, Sample.Normal);
        }
    }

    if (InOutState.FitAnalysis.bVisible)
    {
        for (const FMetricFitSample& Sample : InOutState.FitAnalysis.Samples)
        {
            const bool bFail = !Sample.bHasFloor || Sample.bHullBlocked || Sample.bWidthFail || Sample.bHeightFail;
            const bool bWarn = Sample.bNearThreshold && !bFail;
            const EMetricQueryEvidenceStatus Status = bFail ? EMetricQueryEvidenceStatus::Fail : (bWarn ? EMetricQueryEvidenceStatus::Warning : EMetricQueryEvidenceStatus::Pass);
            Add(EMetricInspectionTarget::FitAnalysis, EMetricQueryEvidenceKind::Sample, Status,
                Sample.LeftOffset, Sample.RightOffset,
                FString::Printf(TEXT("Width %.0f / %.0f cm"), Sample.AvailableWidth, InOutState.FitAnalysis.RequiredWidth));
            Add(EMetricInspectionTarget::FitAnalysis, EMetricQueryEvidenceKind::Threshold, Status,
                Sample.BaseOffset, Sample.CeilingOffset,
                FString::Printf(TEXT("Height %.0f / %.0f cm"), Sample.AvailableHeight, InOutState.FitAnalysis.RequiredHeight));
            if (Sample.bHullBlocked)
            {
                Add(EMetricInspectionTarget::FitAnalysis, EMetricQueryEvidenceKind::Sweep, EMetricQueryEvidenceStatus::Fail,
                    Sample.BaseOffset, Sample.HullImpactOffset, TEXT("Configured hull intersects geometry"),
                    true, Sample.HullImpactOffset, FVector::ZeroVector);
            }
        }
    }

    if (InOutState.ThresholdAnalysis.bVisible)
    {
        for (const FMetricThresholdSample& Sample : InOutState.ThresholdAnalysis.Samples)
        {
            const bool bFail = !Sample.bHasFloor || Sample.bHullBlocked || Sample.bWidthFail || Sample.bHeightFail || Sample.bStepUpFail || Sample.bStepDownFail;
            const bool bWarn = Sample.bNearThreshold && !bFail;
            const EMetricQueryEvidenceStatus Status = bFail ? EMetricQueryEvidenceStatus::Fail : (bWarn ? EMetricQueryEvidenceStatus::Warning : EMetricQueryEvidenceStatus::Pass);
            Add(EMetricInspectionTarget::ThresholdAnalysis, EMetricQueryEvidenceKind::Sample, Status,
                Sample.LeftOffset, Sample.RightOffset,
                FString::Printf(TEXT("Threshold width %.0f cm"), Sample.AvailableWidth));
            Add(EMetricInspectionTarget::ThresholdAnalysis, EMetricQueryEvidenceKind::Threshold, Status,
                Sample.BaseOffset, Sample.CeilingOffset,
                FString::Printf(TEXT("Threshold height %.0f cm"), Sample.AvailableHeight));
            if (Sample.bHullBlocked)
            {
                Add(EMetricInspectionTarget::ThresholdAnalysis, EMetricQueryEvidenceKind::Sweep, EMetricQueryEvidenceStatus::Fail,
                    Sample.BaseOffset, Sample.HullImpactOffset, TEXT("Configured hull intersects threshold geometry"),
                    true, Sample.HullImpactOffset, FVector::ZeroVector);
            }
        }
    }

    if (InOutState.LedgeAnalysis.bVisible)
    {
        const bool bFail = InOutState.LedgeAnalysis.bHasEdge && (!InOutState.LedgeAnalysis.bHasLanding || !InOutState.LedgeAnalysis.bSafeDrop);
        const bool bWarn = InOutState.LedgeAnalysis.bWarning && !bFail;
        const EMetricQueryEvidenceStatus Status = bFail ? EMetricQueryEvidenceStatus::Fail : (bWarn ? EMetricQueryEvidenceStatus::Warning : EMetricQueryEvidenceStatus::Pass);
        const FVector Start = FVector::ZeroVector;
        const FVector End = InOutState.AnalysisForward * InOutState.LedgeAnalysisSettings.ForwardDistance;
        Add(EMetricInspectionTarget::LedgeAnalysis, EMetricQueryEvidenceKind::Trace, Status, Start, End,
            InOutState.LedgeAnalysis.bHasEdge
                ? FString::Printf(TEXT("Edge %.0f cm, drop %.0f cm"), InOutState.LedgeAnalysis.EdgeDistance, InOutState.LedgeAnalysis.DropDepth)
                : TEXT("No ledge detected"),
            InOutState.LedgeAnalysis.bHasEdge, InOutState.LedgeAnalysis.EdgeOffset, InOutState.AnalysisUp);
        if (InOutState.LedgeAnalysis.bHasLanding)
        {
            Add(EMetricInspectionTarget::LedgeAnalysis, EMetricQueryEvidenceKind::Contact, Status,
                InOutState.LedgeAnalysis.EdgeOffset, InOutState.LedgeAnalysis.LandingOffset, TEXT("Landing probe"),
                true, InOutState.LedgeAnalysis.LandingOffset, InOutState.AnalysisUp);
        }
    }

    if (InOutState.StoppingDistance.bVisible)
    {
        const EMetricQueryEvidenceStatus Status = InOutState.StoppingDistance.bBlocked ? EMetricQueryEvidenceStatus::Fail
            : (InOutState.StoppingDistance.bNearThreshold ? EMetricQueryEvidenceStatus::Warning : EMetricQueryEvidenceStatus::Pass);
        Add(EMetricInspectionTarget::StoppingDistance, EMetricQueryEvidenceKind::Sweep, Status,
            InOutState.StoppingDistance.StartOffset, InOutState.StoppingDistance.EndOffset,
            FString::Printf(TEXT("Available %.0f / required %.0f cm"), InOutState.StoppingDistance.AvailableDistance, InOutState.StoppingDistance.RequiredDistance),
            InOutState.StoppingDistance.bHasObstacle, InOutState.StoppingDistance.ImpactPointOffset, InOutState.StoppingDistance.ImpactNormal);
    }

    if (InOutState.MantleVault.bVisible)
    {
        const bool bFail = InOutState.MantleVault.bHasObstacle && (!InOutState.MantleVault.bHeightValid || !InOutState.MantleVault.bDepthValid);
        const EMetricQueryEvidenceStatus Status = !InOutState.MantleVault.bHasObstacle ? EMetricQueryEvidenceStatus::Info
            : (bFail ? EMetricQueryEvidenceStatus::Fail : EMetricQueryEvidenceStatus::Pass);
        Add(EMetricInspectionTarget::MantleVault, EMetricQueryEvidenceKind::Trace, Status,
            InOutState.AnalysisUp * FMath::Max(10.0f, InOutState.MantleVaultSettings.MinObstacleHeight),
            InOutState.MantleVault.FrontOffset,
            InOutState.MantleVault.bHasObstacle ? FString::Printf(TEXT("Height %.0f cm"), InOutState.MantleVault.ObstacleHeight) : TEXT("No obstacle"),
            InOutState.MantleVault.bHasObstacle, InOutState.MantleVault.FrontOffset, -InOutState.AnalysisForward);
        if (InOutState.MantleVault.bHasObstacle)
        {
            Add(EMetricInspectionTarget::MantleVault, EMetricQueryEvidenceKind::Threshold, Status,
                InOutState.MantleVault.FrontOffset, InOutState.MantleVault.TopOffset,
                FString::Printf(TEXT("Depth %.0f / %.0f cm"), InOutState.MantleVault.ObstacleDepth, InOutState.MantleVaultSettings.MaxObstacleDepth),
                true, InOutState.MantleVault.TopOffset, InOutState.AnalysisUp);
        }
    }

    if (InOutState.RecoverySpace.bVisible)
    {
        struct FRecoveryEntry { FVector Dir; float Required; float Clear; bool bClear; bool bGroundSupported; const TCHAR* Label; };
        const FRecoveryEntry Entries[] = {
            { InOutState.AnalysisForward, InOutState.RecoverySpaceSettings.ForwardDistance, InOutState.RecoverySpace.ForwardClear, InOutState.RecoverySpace.bForwardClear, InOutState.RecoverySpace.bForwardGroundSupported, TEXT("Forward") },
            { -InOutState.AnalysisForward, InOutState.RecoverySpaceSettings.BackwardDistance, InOutState.RecoverySpace.BackwardClear, InOutState.RecoverySpace.bBackwardClear, InOutState.RecoverySpace.bBackwardGroundSupported, TEXT("Backward") },
            { -InOutState.AnalysisRight, InOutState.RecoverySpaceSettings.SideDistance, InOutState.RecoverySpace.LeftClear, InOutState.RecoverySpace.bLeftClear, InOutState.RecoverySpace.bLeftGroundSupported, TEXT("Left") },
            { InOutState.AnalysisRight, InOutState.RecoverySpaceSettings.SideDistance, InOutState.RecoverySpace.RightClear, InOutState.RecoverySpace.bRightClear, InOutState.RecoverySpace.bRightGroundSupported, TEXT("Right") }
        };
        for (const FRecoveryEntry& Entry : Entries)
        {
            const FString Label = Entry.bGroundSupported
                ? FString::Printf(TEXT("%s %.0f / %.0f cm"), Entry.Label, Entry.Clear, Entry.Required)
                : FString::Printf(TEXT("%s unsupported after %.0f / %.0f cm"), Entry.Label, Entry.Clear, Entry.Required);
            Add(EMetricInspectionTarget::RecoverySpace, EMetricQueryEvidenceKind::Sweep,
                Entry.bClear ? EMetricQueryEvidenceStatus::Pass : EMetricQueryEvidenceStatus::Fail,
                FVector::ZeroVector, Entry.Dir * Entry.Required,
                Label,
                !Entry.bClear && Entry.bGroundSupported, Entry.Dir * Entry.Clear, -Entry.Dir);
        }
    }

    if (InOutState.CameraProbe.bVisible)
    {
        const EMetricQueryEvidenceStatus Status = InOutState.CameraProbe.bBelowMinimum ? EMetricQueryEvidenceStatus::Fail
            : (InOutState.CameraProbe.bCompressed ? EMetricQueryEvidenceStatus::Warning : EMetricQueryEvidenceStatus::Pass);
        Add(EMetricInspectionTarget::CameraProbe, EMetricQueryEvidenceKind::Sweep, Status,
            InOutState.CameraProbe.PivotOffset, InOutState.CameraProbe.DesiredCameraOffset,
            FString::Printf(TEXT("Camera %.0f / %.0f cm"), InOutState.CameraProbe.ResolvedDistance, InOutState.CameraProbe.TargetDistance),
            InOutState.CameraProbe.bCompressed, InOutState.CameraProbe.ImpactPointOffset, InOutState.CameraProbe.ImpactNormal);
    }

    if (InOutState.CameraSightline.bVisible)
    {
        const bool bFail = InOutState.CameraSightline.bOccluded || InOutState.CameraSightline.bFramingFail;
        Add(EMetricInspectionTarget::CameraSightline, EMetricQueryEvidenceKind::Trace,
            bFail ? EMetricQueryEvidenceStatus::Fail : EMetricQueryEvidenceStatus::Pass,
            InOutState.CameraSightline.CameraOffset, InOutState.CameraSightline.SubjectOffset,
            InOutState.CameraSightline.bOccluded ? TEXT("Sightline occluded") : (InOutState.CameraSightline.bFramingFail ? TEXT("Framing outside tolerance") : TEXT("Sightline clear")),
            InOutState.CameraSightline.bOccluded, InOutState.CameraSightline.ImpactPointOffset, -InOutState.AnalysisForward);
    }

    if (InOutState.Trajectory.bVisible && InOutState.Trajectory.SampleOffsets.Num() > 1)
    {
        const EMetricQueryEvidenceStatus Status = InOutState.Trajectory.bBlocked ? EMetricQueryEvidenceStatus::Fail
            : ((InOutState.Trajectory.bHasLanding && !InOutState.Trajectory.bLandingValid) ? EMetricQueryEvidenceStatus::Warning : EMetricQueryEvidenceStatus::Pass);
        for (int32 Index = 1; Index < InOutState.Trajectory.SampleOffsets.Num(); ++Index)
        {
            Add(EMetricInspectionTarget::Trajectory, EMetricQueryEvidenceKind::Sweep, Status,
                InOutState.Trajectory.SampleOffsets[Index - 1], InOutState.Trajectory.SampleOffsets[Index],
                Index == 1 ? FString::Printf(TEXT("Flight %.2f s, range %.0f cm"), InOutState.Trajectory.FlightTime, InOutState.Trajectory.HorizontalDistance) : FString());
        }
        if (InOutState.Trajectory.bBlocked || InOutState.Trajectory.bHasLanding)
        {
            Add(EMetricInspectionTarget::Trajectory, EMetricQueryEvidenceKind::Contact, Status,
                InOutState.Trajectory.ImpactPointOffset, InOutState.Trajectory.ImpactPointOffset,
                InOutState.Trajectory.bBlocked ? TEXT("Trajectory blocked") : TEXT("Landing contact"),
                true, InOutState.Trajectory.ImpactPointOffset, InOutState.Trajectory.ImpactNormal);
        }
    }
}

namespace
{
    static FAutoConsoleCommand MetricProfileLivePipelineCommand(
        TEXT("Metric.ProfileLivePipeline"),
        TEXT("Profiles the next N live Metric publish/update cycles. Usage: Metric.ProfileLivePipeline 120"),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            int32 Publishes = 120;
            if (!Args.IsEmpty())
            {
                Publishes = FMath::Clamp(FCString::Atoi(*Args[0]), 1, 5000);
            }
            MetricPerformance::BeginPipelineCapture(Publishes);
            UE_LOG(LogTemp, Display, TEXT("Metric Live Pipeline capture started for %d publishes. Move Metric normally until the report prints."), Publishes);
        }));

    static FAutoConsoleCommand MetricProfilePerformanceCommand(
        TEXT("Metric.ProfilePerformance"),
        TEXT("Profile Metric analysis at the current Live Metric location. Optional argument: iterations (default 120)."),
        FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
        {
            int32 Iterations = 120;
            if (Args.Num() > 0)
            {
                Iterations = FMath::Max(1, FCString::Atoi(*Args[0]));
            }
            if (GEditor)
            {
                if (UMetricSubsystem* Subsystem = GEditor->GetEditorSubsystem<UMetricSubsystem>())
                {
                    UE_LOG(LogTemp, Display, TEXT("\n%s"), *Subsystem->RunPerformanceProfileCurrentLevel(Iterations));
                }
            }
        }));
}

#undef LOCTEXT_NAMESPACE
