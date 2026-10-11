#include "MetricViewportComponent.h"

#include "MetricEditorSettings.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"

namespace MetricDraw
{
    static void DrawCircle3D(
        FPrimitiveDrawInterface* PDI,
        const FVector& Center,
        const FVector& AxisA,
        const FVector& AxisB,
        float Radius,
        const FLinearColor& Color,
        float Thickness,
        int32 Segments = 32)
    {
        FVector Prev = Center + AxisA * Radius;
        for (int32 Index = 1; Index <= Segments; ++Index)
        {
            const float Angle = (2.0f * PI * static_cast<float>(Index)) / static_cast<float>(Segments);
            const FVector Next = Center + (AxisA * FMath::Cos(Angle) + AxisB * FMath::Sin(Angle)) * Radius;
            PDI->DrawLine(Prev, Next, Color, SDPG_World, Thickness, 0.0f, true);
            Prev = Next;
        }
    }

    static void DrawSegmentedCircle3D(
        FPrimitiveDrawInterface* PDI,
        const FVector& Center,
        const FVector& AxisA,
        const FVector& AxisB,
        float Radius,
        const FLinearColor& Color,
        float Thickness,
        int32 Segments,
        int32 DrawEvery)
    {
        DrawEvery = FMath::Max(1, DrawEvery);
        for (int32 Index = 0; Index < Segments; ++Index)
        {
            if ((Index % DrawEvery) != 0)
            {
                continue;
            }

            const float AngleA = (2.0f * PI * static_cast<float>(Index)) / static_cast<float>(Segments);
            const float AngleB = (2.0f * PI * static_cast<float>(Index + 1)) / static_cast<float>(Segments);
            const FVector Start = Center + (AxisA * FMath::Cos(AngleA) + AxisB * FMath::Sin(AngleA)) * Radius;
            const FVector End = Center + (AxisA * FMath::Cos(AngleB) + AxisB * FMath::Sin(AngleB)) * Radius;
            PDI->DrawLine(Start, End, Color, SDPG_World, Thickness, 0.0f, true);
        }
    }

    static void DrawArc(
        FPrimitiveDrawInterface* PDI,
        const FVector& Center,
        const FVector& AxisA,
        const FVector& AxisB,
        float Radius,
        float StartAngle,
        float EndAngle,
        const FLinearColor& Color,
        float Thickness,
        int32 Segments = 16)
    {
        FVector Prev = Center + (AxisA * FMath::Cos(StartAngle) + AxisB * FMath::Sin(StartAngle)) * Radius;
        for (int32 Index = 1; Index <= Segments; ++Index)
        {
            const float Alpha = static_cast<float>(Index) / static_cast<float>(Segments);
            const float Angle = FMath::Lerp(StartAngle, EndAngle, Alpha);
            const FVector Next = Center + (AxisA * FMath::Cos(Angle) + AxisB * FMath::Sin(Angle)) * Radius;
            PDI->DrawLine(Prev, Next, Color, SDPG_World, Thickness, 0.0f, true);
            Prev = Next;
        }
    }

    static void DrawCapsule(
        FPrimitiveDrawInterface* PDI,
        const FVector& Base,
        const FVector& Forward,
        float Height,
        float Width,
        const FLinearColor& Color,
        float Thickness)
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        const float CylinderHalf = FMath::Max(0.0f, HalfHeight - Radius);

        FVector Fwd = Forward.GetSafeNormal();
        if (Fwd.IsNearlyZero())
        {
            Fwd = FVector::ForwardVector;
        }
        const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();
        const FVector Up = FVector::UpVector;
        const FVector Center = Base + Up * HalfHeight;
        const FVector TopRing = Center + Up * CylinderHalf;
        const FVector BottomRing = Center - Up * CylinderHalf;

        DrawCircle3D(PDI, TopRing, Fwd, Right, Radius, Color, Thickness);
        DrawCircle3D(PDI, BottomRing, Fwd, Right, Radius, Color, Thickness);

        for (const FVector& Radial : {Fwd, -Fwd, Right, -Right})
        {
            PDI->DrawLine(BottomRing + Radial * Radius, TopRing + Radial * Radius, Color, SDPG_World, Thickness, 0.0f, true);
        }

        DrawArc(PDI, TopRing, Fwd, Up, Radius, 0.0f, PI, Color, Thickness);
        DrawArc(PDI, TopRing, -Fwd, Up, Radius, 0.0f, PI, Color, Thickness);
        DrawArc(PDI, TopRing, Right, Up, Radius, 0.0f, PI, Color, Thickness);
        DrawArc(PDI, TopRing, -Right, Up, Radius, 0.0f, PI, Color, Thickness);

        DrawArc(PDI, BottomRing, Fwd, -Up, Radius, 0.0f, PI, Color, Thickness);
        DrawArc(PDI, BottomRing, -Fwd, -Up, Radius, 0.0f, PI, Color, Thickness);
        DrawArc(PDI, BottomRing, Right, -Up, Radius, 0.0f, PI, Color, Thickness);
        DrawArc(PDI, BottomRing, -Right, -Up, Radius, 0.0f, PI, Color, Thickness);
    }

    static void DrawBox(
        FPrimitiveDrawInterface* PDI,
        const FVector& Base,
        const FVector& Forward,
        const FVector& Extents,
        const FLinearColor& Color,
        float Thickness)
    {
        FVector Fwd = Forward.GetSafeNormal();
        if (Fwd.IsNearlyZero())
        {
            Fwd = FVector::ForwardVector;
        }
        const FVector Right = FVector::CrossProduct(FVector::UpVector, Fwd).GetSafeNormal();
        const FVector Up = FVector::UpVector;
        const FVector Center = Base + Up * Extents.Z;

        FVector P[8];
        int32 N = 0;
        for (int32 Z = -1; Z <= 1; Z += 2)
        {
            for (int32 Y = -1; Y <= 1; Y += 2)
            {
                for (int32 X = -1; X <= 1; X += 2)
                {
                    P[N++] = Center + Fwd * (Extents.X * X) + Right * (Extents.Y * Y) + Up * (Extents.Z * Z);
                }
            }
        }

        const int32 Edges[][2] = {
            {0,1},{0,2},{0,4},{1,3},{1,5},{2,3},{2,6},{3,7},{4,5},{4,6},{5,7},{6,7}
        };
        for (const auto& Edge : Edges)
        {
            PDI->DrawLine(P[Edge[0]], P[Edge[1]], Color, SDPG_World, Thickness, 0.0f, true);
        }
    }

    static void DrawSphere(
        FPrimitiveDrawInterface* PDI,
        const FVector& Center,
        float Radius,
        const FLinearColor& Color,
        float Thickness)
    {
        DrawCircle3D(PDI, Center, FVector::ForwardVector, FVector::RightVector, Radius, Color, Thickness);
        DrawCircle3D(PDI, Center, FVector::ForwardVector, FVector::UpVector, Radius, Color, Thickness);
        DrawCircle3D(PDI, Center, FVector::RightVector, FVector::UpVector, Radius, Color, Thickness);
    }
}

class FMetricViewportSceneProxy final : public FPrimitiveSceneProxy
{
public:
    FMetricViewportSceneProxy(const UMetricViewportComponent* Component, const FMetricLiveState& InState)
        : FPrimitiveSceneProxy(Component)
        , State(InState)
    {
        const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
        HullColor = Settings ? Settings->HullColor : FLinearColor(0.0f, 0.75f, 1.0f, 1.0f);
        ForwardColor = Settings ? Settings->ForwardColor : FLinearColor::White;
        ClearancePassColor = Settings ? Settings->ClearancePassColor : FLinearColor(0.1f, 1.0f, 0.2f, 1.0f);
        ClearanceWarningColor = Settings ? Settings->ClearanceWarningColor : FLinearColor(1.0f, 0.65f, 0.05f, 1.0f);
        ClearanceFailColor = Settings ? Settings->ClearanceFailColor : FLinearColor(1.0f, 0.08f, 0.05f, 1.0f);
        SensorColor = Settings ? Settings->SensorColor : FLinearColor(0.15f, 0.55f, 1.0f, 1.0f);
        EngagementRangeColor = Settings ? Settings->EngagementRangeColor : FLinearColor(0.1f, 0.45f, 1.0f, 1.0f);
        LocomotionBandColor = Settings ? Settings->LocomotionBandColor : FLinearColor(0.72f, 0.32f, 1.0f, 1.0f);
        TrajectoryColor = Settings ? Settings->TrajectoryColor : FLinearColor(1.0f, 0.42f, 0.08f, 1.0f);
        TrajectoryLandingColor = Settings ? Settings->TrajectoryLandingColor : FLinearColor(0.2f, 1.0f, 0.35f, 1.0f);
        TrajectoryBlockedColor = Settings ? Settings->TrajectoryBlockedColor : FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);
        CameraProbeColor = Settings ? Settings->CameraProbeColor : FLinearColor(1.0f, 0.15f, 0.85f, 1.0f);
        CameraCompressedColor = Settings ? Settings->CameraCompressedColor : FLinearColor(1.0f, 0.08f, 0.22f, 1.0f);
        CameraSightlineColor = Settings ? Settings->CameraSightlineColor : FLinearColor(0.95f, 0.45f, 1.0f, 1.0f);
        GroundPassColor = Settings ? Settings->GroundPassColor : FLinearColor(0.12f, 0.95f, 0.25f, 1.0f);
        GroundStepFailColor = Settings ? Settings->GroundStepFailColor : FLinearColor(1.0f, 0.62f, 0.08f, 1.0f);
        GroundSlopeFailColor = Settings ? Settings->GroundSlopeFailColor : FLinearColor(1.0f, 0.10f, 0.08f, 1.0f);
        GroundGridColor = Settings ? Settings->GroundGridColor : FLinearColor(0.25f, 0.85f, 0.85f, 0.55f);
        SecondaryLineThickness = Settings ? Settings->SecondaryLineThickness : 1.0f;
        Thickness = Settings ? Settings->HullLineThickness : 1.5f;
        AnchorLift = Settings ? Settings->AnchorLift : 10.0f;
        AnchorRadius = Settings ? Settings->AnchorRadius : 16.0f;
        AnchorLineThickness = Settings ? Settings->AnchorLineThickness : 2.0f;
    }

    virtual SIZE_T GetTypeHash() const override
    {
        static size_t UniquePointer;
        return reinterpret_cast<size_t>(&UniquePointer);
    }

    virtual uint32 GetMemoryFootprint() const override
    {
        return sizeof(*this) + GetAllocatedSize();
    }

    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
    {
        FPrimitiveViewRelevance Result;
        Result.bDrawRelevance = IsShown(View) && State.bHasHit;
        Result.bDynamicRelevance = true;
        Result.bShadowRelevance = false;
        Result.bEditorPrimitiveRelevance = true;
        return Result;
    }

    virtual void GetDynamicMeshElements(
        const TArray<const FSceneView*>& Views,
        const FSceneViewFamily& ViewFamily,
        uint32 VisibilityMap,
        FMeshElementCollector& Collector) const override
    {
        if (!State.bHasHit)
        {
            return;
        }

        for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
        {
            if ((VisibilityMap & (1u << ViewIndex)) == 0)
            {
                continue;
            }

            FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);
            if (!PDI)
            {
                continue;
            }

            // Metric state is stored in render-host local space. Convert the
            // anchor and orientation to world space before submitting PDI lines.
            const FMatrix& ProxyLocalToWorld = GetLocalToWorld();
            const FVector Base = ProxyLocalToWorld.TransformPosition(State.Location + State.Hull.LocalOffset);
            FVector WorldForward = ProxyLocalToWorld.TransformVector(State.Forward).GetSafeNormal();
            if (WorldForward.IsNearlyZero())
            {
                WorldForward = FVector::ForwardVector;
            }

            const float Height = FMath::Max(1.0f, State.Hull.Height);
            const float Width = FMath::Max(1.0f, State.Hull.Width);
            const float Length = FMath::Max(1.0f, State.Hull.Length);

            if (State.bShowHull)
            {
                switch (State.Hull.HullType)
                {
                case EMetricHullType::Capsule:
                    MetricDraw::DrawCapsule(PDI, Base, WorldForward, Height, Width, HullColor, Thickness);
                    break;
                case EMetricHullType::OrientedBox:
                    MetricDraw::DrawBox(PDI, Base, WorldForward, FVector(Length * 0.5f, Width * 0.5f, Height * 0.5f), HullColor, Thickness);
                    break;
                case EMetricHullType::Sphere:
                    MetricDraw::DrawSphere(PDI, Base + FVector::UpVector * (Height * 0.5f), FMath::Max(Width, Height) * 0.5f, HullColor, Thickness);
                    break;
                case EMetricHullType::Cylinder:
                default:
                {
                    const FVector CylinderRight = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                    MetricDraw::DrawCircle3D(PDI, Base, WorldForward, CylinderRight, Width * 0.5f, HullColor, Thickness);
                    MetricDraw::DrawCircle3D(PDI, Base + FVector::UpVector * Height, WorldForward, CylinderRight, Width * 0.5f, HullColor, Thickness);
                    break;
                }
                }
            }

            if (State.Clearance.bVisible)
            {
                const FLinearColor ClearanceColor = State.Clearance.bBlocked
                    ? ClearanceFailColor
                    : (State.Clearance.bNearThreshold ? ClearanceWarningColor : ClearancePassColor);

                const FVector SweepStart = ProxyLocalToWorld.TransformPosition(State.Clearance.SweepStartOffset);
                const FVector RequestedEnd = ProxyLocalToWorld.TransformPosition(State.Clearance.SweepEndOffset);
                const FVector ResolvedCenter = SweepStart + WorldForward * State.Clearance.ClearDistance;

                // Main measurement line. The requested segment stays visible even on failure.
                PDI->DrawLine(SweepStart, ResolvedCenter, ClearanceColor, SDPG_World, 3.0f, 0.0f, true);
                if ((RequestedEnd - ResolvedCenter).SizeSquared() > 1.0f)
                {
                    FLinearColor RemainingColor = ClearanceFailColor;
                    RemainingColor.A = 0.35f;
                    PDI->DrawLine(ResolvedCenter, RequestedEnd, RemainingColor, SDPG_World, 1.5f, 0.0f, true);
                }

                // Quiet side rails make the requested passage width readable without
                // turning the probe into a solid debug tunnel. They deliberately sit
                // below the primary center line in the visual hierarchy.
                const FVector SweepRight = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                const float HalfVisualWidth = FMath::Max(4.0f, Width * 0.5f);
                FLinearColor RailColor = ClearanceColor;
                RailColor.A = 0.35f;
                const FVector RailStartA = SweepStart + SweepRight * HalfVisualWidth;
                const FVector RailStartB = SweepStart - SweepRight * HalfVisualWidth;
                const FVector RailEndA = RequestedEnd + SweepRight * HalfVisualWidth;
                const FVector RailEndB = RequestedEnd - SweepRight * HalfVisualWidth;
                PDI->DrawLine(RailStartA, RailEndA, RailColor, SDPG_World, 1.0f, 0.0f, true);
                PDI->DrawLine(RailStartB, RailEndB, RailColor, SDPG_World, 1.0f, 0.0f, true);
                PDI->DrawLine(RailEndA, RailEndB, RailColor, SDPG_World, 1.0f, 0.0f, true);

                // Draw a ghost hull at the requested destination when clear, or at the
                // resolved stopping point when blocked. This reads as a swept corridor
                // without flooding the viewport with repeated debug shapes.
                const float VisualDistance = State.Clearance.bBlocked
                    ? State.Clearance.ClearDistance
                    : State.Clearance.ProbeDistance;
                const FVector GhostBaseOffset = State.Hull.LocalOffset
                    + State.Forward.GetSafeNormal() * VisualDistance
                    + FVector::UpVector * 1.0f;
                const FVector GhostBase = ProxyLocalToWorld.TransformPosition(GhostBaseOffset);
                FLinearColor GhostColor = ClearanceColor;
                GhostColor.A = 0.45f;

                switch (State.Hull.HullType)
                {
                case EMetricHullType::Capsule:
                    MetricDraw::DrawCapsule(PDI, GhostBase, WorldForward, Height, Width, GhostColor, 1.0f);
                    break;
                case EMetricHullType::OrientedBox:
                    MetricDraw::DrawBox(PDI, GhostBase, WorldForward, FVector(Length * 0.5f, Width * 0.5f, Height * 0.5f), GhostColor, 1.0f);
                    break;
                case EMetricHullType::Sphere:
                    MetricDraw::DrawSphere(PDI, GhostBase + FVector::UpVector * (Height * 0.5f), FMath::Max3(Width, Length, Height) * 0.5f, GhostColor, 1.0f);
                    break;
                case EMetricHullType::Cylinder:
                default:
                {
                    const FVector CylinderRight = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                    MetricDraw::DrawCircle3D(PDI, GhostBase, WorldForward, CylinderRight, Width * 0.5f, GhostColor, 1.0f);
                    MetricDraw::DrawCircle3D(PDI, GhostBase + FVector::UpVector * Height, WorldForward, CylinderRight, Width * 0.5f, GhostColor, 1.0f);
                    break;
                }
                }

                if (State.Clearance.bHasObstacle)
                {
                    const FVector Impact = ProxyLocalToWorld.TransformPosition(State.Clearance.ImpactPointOffset);
                    const float MarkerSize = 14.0f;
                    const FVector Right = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                    PDI->DrawLine(Impact - Right * MarkerSize - FVector::UpVector * MarkerSize, Impact + Right * MarkerSize + FVector::UpVector * MarkerSize, ClearanceColor, SDPG_Foreground, 3.0f, 0.0f, true);
                    PDI->DrawLine(Impact + Right * MarkerSize - FVector::UpVector * MarkerSize, Impact - Right * MarkerSize + FVector::UpVector * MarkerSize, ClearanceColor, SDPG_Foreground, 3.0f, 0.0f, true);
                }
            }

            if (State.Trajectory.bVisible && State.Trajectory.SampleOffsets.Num() >= 2)
            {
                const FLinearColor ArcColor = State.Trajectory.bBlocked
                    ? TrajectoryBlockedColor
                    : (State.Trajectory.bLandingValid ? TrajectoryLandingColor : TrajectoryColor);

                FVector Prev = ProxyLocalToWorld.TransformPosition(State.Trajectory.SampleOffsets[0]);
                for (int32 Index = 1; Index < State.Trajectory.SampleOffsets.Num(); ++Index)
                {
                    const FVector Next = ProxyLocalToWorld.TransformPosition(State.Trajectory.SampleOffsets[Index]);
                    PDI->DrawLine(Prev, Next, ArcColor, SDPG_World, 2.5f, 0.0f, true);
                    Prev = Next;
                }

                if (State.Trajectory.bHasLanding || State.Trajectory.bBlocked)
                {
                    const FVector Impact = ProxyLocalToWorld.TransformPosition(State.Trajectory.ImpactPointOffset);
                    const FVector ImpactNormal = State.Trajectory.ImpactNormal.IsNearlyZero()
                        ? FVector::UpVector
                        : State.Trajectory.ImpactNormal.GetSafeNormal();
                    FVector ImpactRight = FVector::CrossProduct(ImpactNormal, WorldForward).GetSafeNormal();
                    if (ImpactRight.IsNearlyZero())
                    {
                        ImpactRight = FVector::RightVector;
                    }
                    const FVector ImpactForward = FVector::CrossProduct(ImpactRight, ImpactNormal).GetSafeNormal();
                    const float MarkerRadius = State.Trajectory.bLandingValid ? 28.0f : 18.0f;

                    MetricDraw::DrawCircle3D(PDI, Impact + ImpactNormal * 2.0f, ImpactForward, ImpactRight, MarkerRadius, ArcColor, 2.5f, 28);
                    PDI->DrawLine(Impact, Impact + ImpactNormal * 45.0f, ArcColor, SDPG_World, 2.0f, 0.0f, true);

                    if (State.Trajectory.bBlocked)
                    {
                        PDI->DrawLine(Impact - ImpactRight * 14.0f - ImpactNormal * 14.0f, Impact + ImpactRight * 14.0f + ImpactNormal * 14.0f, ArcColor, SDPG_Foreground, 3.0f, 0.0f, true);
                        PDI->DrawLine(Impact + ImpactRight * 14.0f - ImpactNormal * 14.0f, Impact - ImpactRight * 14.0f + ImpactNormal * 14.0f, ArcColor, SDPG_Foreground, 3.0f, 0.0f, true);
                    }
                }
            }

            if (State.CameraProbe.bVisible)
            {
                const FVector Pivot = ProxyLocalToWorld.TransformPosition(State.CameraProbe.PivotOffset);
                const FVector DesiredCamera = ProxyLocalToWorld.TransformPosition(State.CameraProbe.DesiredCameraOffset);
                const FVector ResolvedCamera = ProxyLocalToWorld.TransformPosition(State.CameraProbe.ResolvedCameraOffset);
                const FLinearColor ResolvedColor = State.CameraProbe.bCompressed ? CameraCompressedColor : CameraProbeColor;

                // The desired spring arm remains visible as a quiet magenta guide.
                FLinearColor DesiredColor = CameraProbeColor;
                DesiredColor.A = State.CameraProbe.bCompressed ? 0.35f : 0.75f;
                PDI->DrawLine(Pivot, DesiredCamera, DesiredColor, SDPG_World, 1.5f, 0.0f, true);

                // The resolved arm is the primary read. When geometry compresses the
                // camera, this segment stops at the swept sphere center.
                PDI->DrawLine(Pivot, ResolvedCamera, ResolvedColor, SDPG_World, 3.0f, 0.0f, true);

                const float ProbeRadius = FMath::Max(1.0f, State.CameraProbeSettings.ProbeRadius);
                MetricDraw::DrawSphere(PDI, ResolvedCamera, ProbeRadius, ResolvedColor, 2.0f);

                // Requested endpoint stays visible as a crosshair so designers can
                // compare intended camera position against the compressed position.
                const float EndMarker = FMath::Max(10.0f, ProbeRadius * 0.8f);
                PDI->DrawLine(DesiredCamera - FVector::RightVector * EndMarker, DesiredCamera + FVector::RightVector * EndMarker, CameraProbeColor, SDPG_World, 1.0f, 0.0f, true);
                PDI->DrawLine(DesiredCamera - FVector::UpVector * EndMarker, DesiredCamera + FVector::UpVector * EndMarker, CameraProbeColor, SDPG_World, 1.0f, 0.0f, true);

                const float PivotMarker = 12.0f;
                PDI->DrawLine(Pivot - FVector::RightVector * PivotMarker, Pivot + FVector::RightVector * PivotMarker, CameraProbeColor, SDPG_World, 2.0f, 0.0f, true);
                PDI->DrawLine(Pivot - FVector::UpVector * PivotMarker, Pivot + FVector::UpVector * PivotMarker, CameraProbeColor, SDPG_World, 2.0f, 0.0f, true);

                if (State.CameraProbe.bCompressed)
                {
                    const FVector Impact = ProxyLocalToWorld.TransformPosition(State.CameraProbe.ImpactPointOffset);
                    FVector ImpactNormal = ProxyLocalToWorld.TransformVector(State.CameraProbe.ImpactNormal).GetSafeNormal();
                    if (ImpactNormal.IsNearlyZero())
                    {
                        ImpactNormal = FVector::UpVector;
                    }
                    PDI->DrawLine(Impact, Impact + ImpactNormal * 35.0f, ResolvedColor, SDPG_World, 2.0f, 0.0f, true);
                }
            }


            if (State.CameraSightline.bVisible)
            {
                const FVector Camera = ProxyLocalToWorld.TransformPosition(State.CameraSightline.CameraOffset);
                const FVector Aim = ProxyLocalToWorld.TransformPosition(State.CameraSightline.AimOffset);
                const FVector Subject = ProxyLocalToWorld.TransformPosition(State.CameraSightline.SubjectOffset);
                const bool bFail = State.CameraSightline.bOccluded || State.CameraSightline.bFramingFail;
                const FLinearColor ViewColor = bFail ? ClearanceFailColor : CameraSightlineColor;

                PDI->DrawLine(Camera, Subject, ViewColor, SDPG_World, bFail ? 3.0f : 2.0f, 0.0f, true);
                PDI->DrawLine(Camera, Aim, CameraSightlineColor, SDPG_World, 1.0f, 0.0f, true);

                const FVector AimDir = (Aim - Camera).GetSafeNormal();
                FVector ConeRight = FVector::CrossProduct(FVector::UpVector, AimDir).GetSafeNormal();
                if (ConeRight.IsNearlyZero()) ConeRight = FVector::RightVector;
                const float ConeLength = FMath::Min(300.0f, FMath::Max(80.0f, (Subject - Camera).Size()));
                const float HalfAngleRad = FMath::DegreesToRadians(FMath::Clamp(State.CameraSightlineSettings.FramingHalfAngle, 1.0f, 80.0f));
                const float HalfWidth = FMath::Tan(HalfAngleRad) * ConeLength;
                const FVector ConeCenter = Camera + AimDir * ConeLength;
                PDI->DrawLine(Camera, ConeCenter + ConeRight * HalfWidth, CameraSightlineColor, SDPG_World, 1.0f, 0.0f, true);
                PDI->DrawLine(Camera, ConeCenter - ConeRight * HalfWidth, CameraSightlineColor, SDPG_World, 1.0f, 0.0f, true);

                MetricDraw::DrawSphere(PDI, Subject, FMath::Max(6.0f, State.CameraSightlineSettings.TraceRadius), ViewColor, 1.5f);
                if (State.CameraSightline.bOccluded)
                {
                    const FVector Impact = ProxyLocalToWorld.TransformPosition(State.CameraSightline.ImpactPointOffset);
                    const float Cross = 12.0f;
                    PDI->DrawLine(Impact - FVector::RightVector * Cross - FVector::UpVector * Cross, Impact + FVector::RightVector * Cross + FVector::UpVector * Cross, ClearanceFailColor, SDPG_Foreground, 3.0f, 0.0f, true);
                    PDI->DrawLine(Impact + FVector::RightVector * Cross - FVector::UpVector * Cross, Impact - FVector::RightVector * Cross + FVector::UpVector * Cross, ClearanceFailColor, SDPG_Foreground, 3.0f, 0.0f, true);
                }
            }

            if (State.GroundValidation.bVisible && !State.GroundValidation.Samples.IsEmpty())
            {
                const int32 SampleCount = State.GroundValidation.Samples.Num();
                const float MarkerRadius = 7.0f;
                FVector PreviousWorld = FVector::ZeroVector;
                bool bHavePreviousWorld = false;

                for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
                {
                    const FMetricGroundSample& Sample = State.GroundValidation.Samples[SampleIndex];
                    const FVector Point = ProxyLocalToWorld.TransformPosition(Sample.Offset + FVector::UpVector * 3.0f);
                    FVector Normal = ProxyLocalToWorld.TransformVector(Sample.Normal).GetSafeNormal();
                    if (Normal.IsNearlyZero())
                    {
                        Normal = FVector::UpVector;
                    }

                    FLinearColor SampleColor = GroundPassColor;
                    if (!Sample.bHit || Sample.bSlopeFail)
                    {
                        SampleColor = GroundSlopeFailColor;
                    }
                    else if (Sample.bStepUpFail || Sample.bStepDownFail)
                    {
                        SampleColor = GroundStepFailColor;
                    }

                    FVector SampleRight = FVector::CrossProduct(Normal, WorldForward).GetSafeNormal();
                    if (SampleRight.IsNearlyZero())
                    {
                        SampleRight = FVector::RightVector;
                    }
                    const FVector SampleForward = FVector::CrossProduct(SampleRight, Normal).GetSafeNormal();
                    MetricDraw::DrawCircle3D(PDI, Point, SampleForward, SampleRight, MarkerRadius, SampleColor, 1.5f, 16);
                    PDI->DrawLine(Point, Point + Normal * 18.0f, SampleColor, SDPG_World, 1.25f, 0.0f, true);

                    // Samples are stored row-major with three lanes when lateral coverage is enabled.
                    // Connect samples within each row lightly so the field reads as a traversability strip.
                    const int32 LaneCount = State.GroundValidationSettings.LateralCoverage > KINDA_SMALL_NUMBER ? 3 : 1;
                    if (LaneCount > 1 && (SampleIndex % LaneCount) != 0)
                    {
                        const FMetricGroundSample& PrevSample = State.GroundValidation.Samples[SampleIndex - 1];
                        const FVector PrevPoint = ProxyLocalToWorld.TransformPosition(PrevSample.Offset + FVector::UpVector * 3.0f);
                        PDI->DrawLine(PrevPoint, Point, GroundGridColor, SDPG_World, 0.75f, 0.0f, true);
                    }

                    // Connect the center lane forward to make the sampled route direction obvious.
                    if (LaneCount == 1 || (SampleIndex % LaneCount) == (LaneCount / 2))
                    {
                        if (bHavePreviousWorld)
                        {
                            PDI->DrawLine(PreviousWorld, Point, GroundGridColor, SDPG_World, 1.0f, 0.0f, true);
                        }
                        PreviousWorld = Point;
                        bHavePreviousWorld = true;
                    }
                }
            }


            if (State.FitAnalysis.bVisible && !State.FitAnalysis.Samples.IsEmpty())
            {
                for (const FMetricFitSample& Sample : State.FitAnalysis.Samples)
                {
                    const FLinearColor FitColor = (Sample.bWidthFail || Sample.bHeightFail || !Sample.bHasFloor)
                        ? ClearanceFailColor
                        : (Sample.bNearThreshold ? ClearanceWarningColor : ClearancePassColor);

                    if (!Sample.bHasFloor)
                    {
                        const FVector P = ProxyLocalToWorld.TransformPosition(Sample.BaseOffset);
                        const float Cross = 10.0f;
                        PDI->DrawLine(P - FVector::RightVector * Cross, P + FVector::RightVector * Cross, FitColor, SDPG_World, 1.5f, 0.0f, true);
                        PDI->DrawLine(P - FVector::ForwardVector * Cross, P + FVector::ForwardVector * Cross, FitColor, SDPG_World, 1.5f, 0.0f, true);
                        continue;
                    }

                    const FVector BasePoint = ProxyLocalToWorld.TransformPosition(Sample.BaseOffset);
                    const FVector LeftPoint = ProxyLocalToWorld.TransformPosition(Sample.LeftOffset);
                    const FVector RightPoint = ProxyLocalToWorld.TransformPosition(Sample.RightOffset);
                    const FVector CeilingPoint = ProxyLocalToWorld.TransformPosition(Sample.CeilingOffset);

                    FLinearColor QuietColor = FitColor;
                    QuietColor.A = 0.55f;
                    PDI->DrawLine(LeftPoint, RightPoint, QuietColor, SDPG_World, 1.25f, 0.0f, true);
                    PDI->DrawLine(BasePoint, CeilingPoint, QuietColor, SDPG_World, 1.25f, 0.0f, true);

                    const float Tick = 10.0f;
                    PDI->DrawLine(LeftPoint - FVector::UpVector * Tick, LeftPoint + FVector::UpVector * Tick, FitColor, SDPG_World, 1.5f, 0.0f, true);
                    PDI->DrawLine(RightPoint - FVector::UpVector * Tick, RightPoint + FVector::UpVector * Tick, FitColor, SDPG_World, 1.5f, 0.0f, true);
                    PDI->DrawLine(CeilingPoint - FVector::RightVector * Tick, CeilingPoint + FVector::RightVector * Tick, FitColor, SDPG_World, 1.5f, 0.0f, true);
                }
            }

            if (State.Ranges.bShowEngagementRanges)
            {
                const FVector RangeCenter = Base + FVector::UpVector * 2.0f;
                const FVector RangeAxisA = FVector::ForwardVector;
                const FVector RangeAxisB = FVector::RightVector;

                if (State.Ranges.ShortRange > KINDA_SMALL_NUMBER)
                {
                    MetricDraw::DrawSegmentedCircle3D(PDI, RangeCenter, RangeAxisA, RangeAxisB, State.Ranges.ShortRange, EngagementRangeColor, SecondaryLineThickness + 0.5f, 64, 1);
                }
                if (State.Ranges.MediumRange > KINDA_SMALL_NUMBER)
                {
                    MetricDraw::DrawSegmentedCircle3D(PDI, RangeCenter, RangeAxisA, RangeAxisB, State.Ranges.MediumRange, EngagementRangeColor, SecondaryLineThickness, 72, 2);
                }
                if (State.Ranges.LongRange > KINDA_SMALL_NUMBER)
                {
                    MetricDraw::DrawSegmentedCircle3D(PDI, RangeCenter, RangeAxisA, RangeAxisB, State.Ranges.LongRange, EngagementRangeColor, FMath::Max(0.5f, SecondaryLineThickness - 0.25f), 96, 3);
                }
            }

            if (State.Locomotion.bShowLocomotionBands)
            {
                const float Speed = FMath::Max(1.0f, State.Locomotion.Speed);
                const float NearRadius = Speed * FMath::Max(0.1f, State.Locomotion.NearTime);
                const float MidRadius = Speed * FMath::Max(0.1f, State.Locomotion.MidTime);
                const float FarRadius = Speed * FMath::Max(0.1f, State.Locomotion.FarTime);
                const FVector BandCenter = Base + FVector::UpVector * 4.0f;
                const FVector BandAxisA = FVector::ForwardVector;
                const FVector BandAxisB = FVector::RightVector;

                // Locomotion bands deliberately use a different line language than
                // engagement ranges. The nearest band is dashed, the middle is
                // more open, and the far band is sparse so nested rings remain readable.
                if (NearRadius > KINDA_SMALL_NUMBER)
                {
                    MetricDraw::DrawSegmentedCircle3D(PDI, BandCenter, BandAxisA, BandAxisB, NearRadius, LocomotionBandColor, SecondaryLineThickness + 0.5f, 64, 2);
                }
                if (MidRadius > KINDA_SMALL_NUMBER)
                {
                    MetricDraw::DrawSegmentedCircle3D(PDI, BandCenter, BandAxisA, BandAxisB, MidRadius, LocomotionBandColor, SecondaryLineThickness, 80, 3);
                }
                if (FarRadius > KINDA_SMALL_NUMBER)
                {
                    MetricDraw::DrawSegmentedCircle3D(PDI, BandCenter, BandAxisA, BandAxisB, FarRadius, LocomotionBandColor, FMath::Max(0.5f, SecondaryLineThickness - 0.25f), 96, 4);
                }
            }

            if (State.Sensors.bShowSensorHeights)
            {
                const FVector SensorRight = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                const float MarkerRadius = FMath::Max(5.0f, State.Sensors.MarkerRadius);

                auto DrawSensorMarker = [&](float ZOffset, bool bLowProfile)
                {
                    if (ZOffset <= KINDA_SMALL_NUMBER)
                    {
                        return;
                    }

                    const FVector Center = Base + FVector::UpVector * ZOffset;
                    if (bLowProfile)
                    {
                        MetricDraw::DrawSegmentedCircle3D(PDI, Center, WorldForward, SensorRight, MarkerRadius * 0.85f, SensorColor, SecondaryLineThickness, 24, 2);
                    }
                    else
                    {
                        MetricDraw::DrawCircle3D(PDI, Center, WorldForward, SensorRight, MarkerRadius, SensorColor, SecondaryLineThickness + 0.5f, 24);
                    }

                    PDI->DrawLine(Center - SensorRight * MarkerRadius, Center + SensorRight * MarkerRadius, SensorColor, SDPG_World, SecondaryLineThickness, 0.0f, true);
                    PDI->DrawLine(Center, Center + WorldForward * (MarkerRadius * 1.5f), SensorColor, SDPG_World, SecondaryLineThickness, 0.0f, true);
                };

                DrawSensorMarker(State.Sensors.PrimarySensorZOffset, false);
                DrawSensorMarker(State.Sensors.LowProfileSensorZOffset, true);
            }

            // The anchor marker is deliberately lifted off the hit surface and
            // oriented to that surface. Keeping it flush to the ground made it
            // disappear under floor pixels unless the camera looked downhill.
            FVector SurfaceNormal = ProxyLocalToWorld.TransformVector(State.SurfaceNormal).GetSafeNormal();
            if (SurfaceNormal.IsNearlyZero())
            {
                SurfaceNormal = FVector::UpVector;
            }

            FVector MarkerForward = WorldForward - SurfaceNormal * FVector::DotProduct(WorldForward, SurfaceNormal);
            MarkerForward = MarkerForward.GetSafeNormal();
            if (MarkerForward.IsNearlyZero())
            {
                MarkerForward = FVector::CrossProduct(FVector::RightVector, SurfaceNormal).GetSafeNormal();
                if (MarkerForward.IsNearlyZero())
                {
                    MarkerForward = FVector::ForwardVector;
                }
            }
            const FVector MarkerRight = FVector::CrossProduct(SurfaceNormal, MarkerForward).GetSafeNormal();
            const FVector MarkerCenter = Base + SurfaceNormal * AnchorLift;

            MetricDraw::DrawCircle3D(
                PDI,
                MarkerCenter,
                MarkerForward,
                MarkerRight,
                AnchorRadius,
                ForwardColor,
                AnchorLineThickness,
                24);

            if (State.bShowForward)
            {
                const FVector ArrowStart = MarkerCenter;
                const FVector ArrowEnd = ArrowStart + MarkerForward * FMath::Max(100.0f, Length * 0.75f);
                PDI->DrawLine(ArrowStart, ArrowEnd, ForwardColor, SDPG_World, AnchorLineThickness, 0.0f, true);

                const FVector Back = MarkerForward * -18.0f;
                PDI->DrawLine(ArrowEnd, ArrowEnd + Back + MarkerRight * 10.0f, ForwardColor, SDPG_World, AnchorLineThickness, 0.0f, true);
                PDI->DrawLine(ArrowEnd, ArrowEnd + Back - MarkerRight * 10.0f, ForwardColor, SDPG_World, AnchorLineThickness, 0.0f, true);
            }
        }
    }

private:
    FMetricLiveState State;
    FLinearColor HullColor;
    FLinearColor ForwardColor;
    FLinearColor ClearancePassColor;
    FLinearColor ClearanceWarningColor;
    FLinearColor ClearanceFailColor;
    FLinearColor SensorColor;
    FLinearColor EngagementRangeColor;
    FLinearColor LocomotionBandColor;
    FLinearColor TrajectoryColor;
    FLinearColor TrajectoryLandingColor;
    FLinearColor TrajectoryBlockedColor;
    FLinearColor CameraProbeColor;
    FLinearColor CameraCompressedColor;
    FLinearColor CameraSightlineColor;
    FLinearColor GroundPassColor;
    FLinearColor GroundStepFailColor;
    FLinearColor GroundSlopeFailColor;
    FLinearColor GroundGridColor;
    float SecondaryLineThickness = 1.0f;
    float Thickness = 1.5f;
    float AnchorLift = 10.0f;
    float AnchorRadius = 16.0f;
    float AnchorLineThickness = 2.0f;
};

UMetricViewportComponent::UMetricViewportComponent()
{
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCastShadow(false);
    SetHiddenInGame(true);
    SetIsVisualizationComponent(true);
    Mobility = EComponentMobility::Movable;
}

void UMetricViewportComponent::SetMetricState(const FMetricLiveState& NewState)
{
    State = NewState;
    MarkRenderStateDirty();
}

FPrimitiveSceneProxy* UMetricViewportComponent::CreateSceneProxy()
{
    return State.bHasHit ? new FMetricViewportSceneProxy(this, State) : nullptr;
}

FBoxSphereBounds UMetricViewportComponent::CalcBounds(const FTransform& LocalToWorld) const
{
    const float MaxDimension = FMath::Max3(State.Hull.Height, State.Hull.Width, State.Hull.Length);
    const float ProbeReach = State.Clearance.bVisible
        ? State.Clearance.ProbeDistance + State.Clearance.WarningMargin + MaxDimension
        : 0.0f;
    const float RangeReach = State.Ranges.bShowEngagementRanges
        ? FMath::Max3(State.Ranges.ShortRange, State.Ranges.MediumRange, State.Ranges.LongRange)
        : 0.0f;
    const float SensorReach = State.Sensors.bShowSensorHeights
        ? FMath::Max(State.Sensors.PrimarySensorZOffset, State.Sensors.LowProfileSensorZOffset) + State.Sensors.MarkerRadius
        : 0.0f;
    const float LocomotionReach = State.Locomotion.bShowLocomotionBands
        ? FMath::Max3(
            State.Locomotion.Speed * State.Locomotion.NearTime,
            State.Locomotion.Speed * State.Locomotion.MidTime,
            State.Locomotion.Speed * State.Locomotion.FarTime)
        : 0.0f;
    const float CameraReach = State.CameraProbe.bVisible
        ? FMath::Max3(
            State.CameraProbe.PivotOffset.Size(),
            State.CameraProbe.DesiredCameraOffset.Size(),
            State.CameraProbe.ResolvedCameraOffset.Size()) + FMath::Max(1.0f, State.CameraProbeSettings.ProbeRadius)
        : 0.0f;
    const float SightlineReach = State.CameraSightline.bVisible
        ? FMath::Max3(State.CameraSightline.CameraOffset.Size(), State.CameraSightline.AimOffset.Size(), State.CameraSightline.SubjectOffset.Size()) + 100.0f
        : 0.0f;
    const float GroundReach = State.GroundValidation.bVisible
        ? State.GroundValidationSettings.ForwardDistance + State.GroundValidationSettings.TraceDepth + MaxDimension
        : 0.0f;
    const float FitReach = State.FitAnalysis.bVisible
        ? FMath::Max3(
            State.FitAnalysisSettings.ForwardDistance + MaxDimension,
            State.FitAnalysisSettings.MaxSideProbeDistance + MaxDimension,
            State.FitAnalysisSettings.MaxCeilingProbeHeight + MaxDimension)
        : 0.0f;
    float TrajectoryReach = 0.0f;
    if (State.Trajectory.bVisible && !State.Trajectory.SampleOffsets.IsEmpty())
    {
        for (const FVector& SampleOffset : State.Trajectory.SampleOffsets)
        {
            TrajectoryReach = FMath::Max(TrajectoryReach, SampleOffset.Size());
        }
        TrajectoryReach += MaxDimension;
    }
    const float SecondaryReach = FMath::Max(
        FMath::Max3(RangeReach, SensorReach, LocomotionReach),
        FMath::Max(FMath::Max3(TrajectoryReach, CameraReach, GroundReach), FMath::Max(FitReach, SightlineReach)));
    const FVector Extent(FMath::Max(FMath::Max3(100.0f, MaxDimension, ProbeReach), SecondaryReach));
    return FBoxSphereBounds(FBox(-Extent, Extent)).TransformBy(LocalToWorld);
}
