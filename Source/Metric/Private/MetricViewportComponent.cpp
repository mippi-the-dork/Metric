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
        SensorFieldColor = Settings ? Settings->SensorFieldColor : FLinearColor(0.12f, 0.72f, 1.0f, 1.0f);
        SensorFieldBlockedColor = Settings ? Settings->SensorFieldBlockedColor : FLinearColor(1.0f, 0.18f, 0.08f, 1.0f);
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
        PinnedBenchmarkColor = Settings ? Settings->PinnedBenchmarkColor : FLinearColor(0.95f, 0.95f, 0.35f, 1.0f);
        SecondaryLineThickness = Settings ? Settings->SecondaryLineThickness : 1.0f;
        Thickness = Settings ? Settings->HullLineThickness : 1.5f;
        bEmphasizeProblemLines = Settings ? Settings->bEmphasizeProblemLines : true;
        WarningLineThicknessMultiplier = Settings ? Settings->WarningLineThicknessMultiplier : 1.75f;
        FailureLineThicknessMultiplier = Settings ? Settings->FailureLineThicknessMultiplier : 2.5f;
        AnchorLift = Settings ? Settings->AnchorLift : 10.0f;
        AnchorRadius = Settings ? Settings->AnchorRadius : 16.0f;
        AnchorLineThickness = Settings ? Settings->AnchorLineThickness : 2.0f;
    }

    float StatusThickness(float BaseThickness, bool bFail, bool bWarning = false) const
    {
        if (!bEmphasizeProblemLines) return BaseThickness;
        if (bFail) return FMath::Max(BaseThickness, BaseThickness * FailureLineThicknessMultiplier);
        if (bWarning) return FMath::Max(BaseThickness, BaseThickness * WarningLineThicknessMultiplier);
        return BaseThickness;
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
        Result.bDrawRelevance = IsShown(View) && (State.bHasHit || !State.PinnedBenchmarks.IsEmpty());
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
        if (!State.bHasHit && State.PinnedBenchmarks.IsEmpty())
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

            if (State.bHasHit)
            {
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

                // Status drives visual weight: clean paths stay quiet, warnings/failures demand attention.
                const float ClearanceThickness = StatusThickness(FMath::Max(1.25f, SecondaryLineThickness + 0.5f), State.Clearance.bBlocked, State.Clearance.bNearThreshold);
                PDI->DrawLine(SweepStart, ResolvedCenter, ClearanceColor, SDPG_World, ClearanceThickness, 0.0f, true);
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
                    const float ImpactThickness = StatusThickness(2.0f, true);
                    PDI->DrawLine(Impact - Right * MarkerSize - FVector::UpVector * MarkerSize, Impact + Right * MarkerSize + FVector::UpVector * MarkerSize, ClearanceColor, SDPG_Foreground, ImpactThickness, 0.0f, true);
                    PDI->DrawLine(Impact + Right * MarkerSize - FVector::UpVector * MarkerSize, Impact - Right * MarkerSize + FVector::UpVector * MarkerSize, ClearanceColor, SDPG_Foreground, ImpactThickness, 0.0f, true);
                }
            }

            if (State.Trajectory.bVisible && State.Trajectory.SampleOffsets.Num() >= 2)
            {
                const FLinearColor ArcColor = State.Trajectory.bBlocked
                    ? TrajectoryBlockedColor
                    : (State.Trajectory.bLandingValid ? TrajectoryLandingColor : TrajectoryColor);

                const bool bTrajectoryWarning = !State.Trajectory.bBlocked && !State.Trajectory.bLandingValid;
                const float TrajectoryThickness = StatusThickness(FMath::Max(1.25f, SecondaryLineThickness + 0.5f), State.Trajectory.bBlocked, bTrajectoryWarning);
                FVector Prev = ProxyLocalToWorld.TransformPosition(State.Trajectory.SampleOffsets[0]);
                for (int32 Index = 1; Index < State.Trajectory.SampleOffsets.Num(); ++Index)
                {
                    const FVector Next = ProxyLocalToWorld.TransformPosition(State.Trajectory.SampleOffsets[Index]);
                    PDI->DrawLine(Prev, Next, ArcColor, SDPG_World, TrajectoryThickness, 0.0f, true);
                    Prev = Next;
                }

                if (State.TrajectorySettings.bShowTrajectoryEnvelope)
                {
                    FLinearColor EnvelopeColor = ArcColor;
                    EnvelopeColor.A = 0.45f;
                    const FVector EnvelopeRight = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                    const float HalfWidth = FMath::Max(1.0f, State.Hull.Width * 0.5f);
                    const float HalfHeight = FMath::Max(1.0f, State.Hull.Height * 0.5f);
                    const int32 Stride = FMath::Clamp(State.TrajectorySettings.EnvelopeSampleStride, 1, 12);

                    for (int32 Index = 1; Index < State.Trajectory.SampleOffsets.Num(); ++Index)
                    {
                        const FVector A = ProxyLocalToWorld.TransformPosition(State.Trajectory.SampleOffsets[Index - 1]);
                        const FVector B = ProxyLocalToWorld.TransformPosition(State.Trajectory.SampleOffsets[Index]);
                        PDI->DrawLine(A - EnvelopeRight * HalfWidth, B - EnvelopeRight * HalfWidth, EnvelopeColor, SDPG_World, 1.0f, 0.0f, true);
                        PDI->DrawLine(A + EnvelopeRight * HalfWidth, B + EnvelopeRight * HalfWidth, EnvelopeColor, SDPG_World, 1.0f, 0.0f, true);
                        PDI->DrawLine(A - FVector::UpVector * HalfHeight, B - FVector::UpVector * HalfHeight, EnvelopeColor, SDPG_World, 1.0f, 0.0f, true);
                        PDI->DrawLine(A + FVector::UpVector * HalfHeight, B + FVector::UpVector * HalfHeight, EnvelopeColor, SDPG_World, 1.0f, 0.0f, true);

                        if ((Index % Stride) == 0 || Index == State.Trajectory.SampleOffsets.Num() - 1)
                        {
                            PDI->DrawLine(B - EnvelopeRight * HalfWidth, B + EnvelopeRight * HalfWidth, EnvelopeColor, SDPG_World, 1.0f, 0.0f, true);
                            PDI->DrawLine(B - FVector::UpVector * HalfHeight, B + FVector::UpVector * HalfHeight, EnvelopeColor, SDPG_World, 1.0f, 0.0f, true);
                        }
                    }
                }

                if (State.Trajectory.ApexSampleIndex != INDEX_NONE)
                {
                    const FVector ApexCenter = ProxyLocalToWorld.TransformPosition(State.Trajectory.ApexOffset);
                    const float HalfHeight = FMath::Max(1.0f, State.Hull.Height * 0.5f);
                    const FVector ApexTop = ApexCenter + FVector::UpVector * HalfHeight;
                    const float ProbeDistance = FMath::Max(0.0f, State.TrajectorySettings.ApexProbeDistance);
                    FVector ProbeEnd = ApexTop + FVector::UpVector * ProbeDistance;
                    if (State.Trajectory.bApexHasObstacle)
                    {
                        ProbeEnd = FVector(ProxyLocalToWorld.TransformPosition(State.Trajectory.ApexClearanceImpactOffset));
                    }
                    const FLinearColor ApexColor = State.Trajectory.bApexTight
                        ? TrajectoryBlockedColor
                        : (State.Trajectory.bApexHasObstacle ? TrajectoryColor : TrajectoryLandingColor);

                    PDI->DrawLine(ApexTop, ProbeEnd, ApexColor, SDPG_World, 2.0f, 0.0f, true);
                    const FVector ApexRight = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                    MetricDraw::DrawCircle3D(PDI, ApexCenter, WorldForward, ApexRight, 16.0f, ApexColor, 2.0f, 20);
                    PDI->DrawLine(ApexCenter - ApexRight * 22.0f, ApexCenter + ApexRight * 22.0f, ApexColor, SDPG_World, 2.0f, 0.0f, true);
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

                // The resolved arm is the primary read. Compression gets heavier line weight.
                const float CameraThickness = StatusThickness(FMath::Max(1.25f, SecondaryLineThickness + 0.5f), State.CameraProbe.bBelowMinimum, State.CameraProbe.bCompressed && !State.CameraProbe.bBelowMinimum);
                PDI->DrawLine(Pivot, ResolvedCamera, ResolvedColor, SDPG_World, CameraThickness, 0.0f, true);

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

                PDI->DrawLine(Camera, Subject, ViewColor, SDPG_World, StatusThickness(FMath::Max(1.0f, SecondaryLineThickness), bFail), 0.0f, true);
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
                    const float BlockedThickness = StatusThickness(1.5f, true);
                    PDI->DrawLine(Impact - FVector::RightVector * Cross - FVector::UpVector * Cross, Impact + FVector::RightVector * Cross + FVector::UpVector * Cross, ClearanceFailColor, SDPG_Foreground, BlockedThickness, 0.0f, true);
                    PDI->DrawLine(Impact + FVector::RightVector * Cross - FVector::UpVector * Cross, Impact - FVector::RightVector * Cross + FVector::UpVector * Cross, ClearanceFailColor, SDPG_Foreground, BlockedThickness, 0.0f, true);
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

                    const bool bGroundFail = !Sample.bHit || Sample.bSlopeFail;
                    const bool bGroundWarning = !bGroundFail && (Sample.bStepUpFail || Sample.bStepDownFail);
                    FLinearColor SampleColor = GroundPassColor;
                    if (bGroundFail)
                    {
                        SampleColor = GroundSlopeFailColor;
                    }
                    else if (bGroundWarning)
                    {
                        SampleColor = GroundStepFailColor;
                    }
                    const float GroundThickness = StatusThickness(FMath::Max(0.9f, SecondaryLineThickness), bGroundFail, bGroundWarning);

                    FVector SampleRight = FVector::CrossProduct(Normal, WorldForward).GetSafeNormal();
                    if (SampleRight.IsNearlyZero())
                    {
                        SampleRight = FVector::RightVector;
                    }
                    const FVector SampleForward = FVector::CrossProduct(SampleRight, Normal).GetSafeNormal();
                    MetricDraw::DrawCircle3D(PDI, Point, SampleForward, SampleRight, MarkerRadius, SampleColor, GroundThickness, 16);
                    PDI->DrawLine(Point, Point + Normal * 18.0f, SampleColor, SDPG_World, GroundThickness, 0.0f, true);

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
                    const bool bFitFail = Sample.bWidthFail || Sample.bHeightFail || !Sample.bHasFloor;
                    const FLinearColor FitColor = bFitFail
                        ? ClearanceFailColor
                        : (Sample.bNearThreshold ? ClearanceWarningColor : ClearancePassColor);
                    const float FitThickness = StatusThickness(FMath::Max(0.9f, SecondaryLineThickness), bFitFail, Sample.bNearThreshold);

                    if (!Sample.bHasFloor)
                    {
                        const FVector P = ProxyLocalToWorld.TransformPosition(Sample.BaseOffset);
                        const float Cross = 10.0f;
                        PDI->DrawLine(P - FVector::RightVector * Cross, P + FVector::RightVector * Cross, FitColor, SDPG_World, FitThickness, 0.0f, true);
                        PDI->DrawLine(P - FVector::ForwardVector * Cross, P + FVector::ForwardVector * Cross, FitColor, SDPG_World, FitThickness, 0.0f, true);
                        continue;
                    }

                    const FVector BasePoint = ProxyLocalToWorld.TransformPosition(Sample.BaseOffset);
                    const FVector LeftPoint = ProxyLocalToWorld.TransformPosition(Sample.LeftOffset);
                    const FVector RightPoint = ProxyLocalToWorld.TransformPosition(Sample.RightOffset);
                    const FVector CeilingPoint = ProxyLocalToWorld.TransformPosition(Sample.CeilingOffset);

                    FLinearColor QuietColor = FitColor;
                    QuietColor.A = 0.55f;
                    PDI->DrawLine(LeftPoint, RightPoint, QuietColor, SDPG_World, FitThickness, 0.0f, true);
                    PDI->DrawLine(BasePoint, CeilingPoint, QuietColor, SDPG_World, FitThickness, 0.0f, true);

                    const float Tick = 10.0f;
                    PDI->DrawLine(LeftPoint - FVector::UpVector * Tick, LeftPoint + FVector::UpVector * Tick, FitColor, SDPG_World, 1.5f, 0.0f, true);
                    PDI->DrawLine(RightPoint - FVector::UpVector * Tick, RightPoint + FVector::UpVector * Tick, FitColor, SDPG_World, 1.5f, 0.0f, true);
                    PDI->DrawLine(CeilingPoint - FVector::RightVector * Tick, CeilingPoint + FVector::RightVector * Tick, FitColor, SDPG_World, 1.5f, 0.0f, true);
                }
            }

            if (State.ThresholdAnalysis.bVisible && !State.ThresholdAnalysis.Samples.IsEmpty())
            {
                for (const FMetricThresholdSample& Sample : State.ThresholdAnalysis.Samples)
                {
                    const bool bFail = !Sample.bHasFloor || Sample.bWidthFail || Sample.bHeightFail || Sample.bStepUpFail || Sample.bStepDownFail;
                    const FLinearColor ThresholdColor = bFail
                        ? ClearanceFailColor
                        : (Sample.bNearThreshold ? ClearanceWarningColor : ClearancePassColor);
                    const float ThresholdThickness = StatusThickness(FMath::Max(1.0f, SecondaryLineThickness), bFail, Sample.bNearThreshold);

                    const FVector BasePoint = ProxyLocalToWorld.TransformPosition(Sample.BaseOffset);
                    if (!Sample.bHasFloor)
                    {
                        const float Cross = 12.0f;
                        PDI->DrawLine(BasePoint - FVector::RightVector * Cross, BasePoint + FVector::RightVector * Cross, ThresholdColor, SDPG_World, ThresholdThickness, 0.0f, true);
                        PDI->DrawLine(BasePoint - FVector::ForwardVector * Cross, BasePoint + FVector::ForwardVector * Cross, ThresholdColor, SDPG_World, ThresholdThickness, 0.0f, true);
                        continue;
                    }

                    const FVector LeftPoint = ProxyLocalToWorld.TransformPosition(Sample.LeftOffset);
                    const FVector RightPoint = ProxyLocalToWorld.TransformPosition(Sample.RightOffset);
                    const FVector CeilingPoint = ProxyLocalToWorld.TransformPosition(Sample.CeilingOffset);
                    FLinearColor QuietColor = ThresholdColor;
                    QuietColor.A = 0.65f;

                    PDI->DrawLine(LeftPoint, RightPoint, QuietColor, SDPG_World, ThresholdThickness, 0.0f, true);
                    PDI->DrawLine(BasePoint, CeilingPoint, QuietColor, SDPG_World, ThresholdThickness, 0.0f, true);

                    const float Tick = 12.0f;
                    PDI->DrawLine(LeftPoint - FVector::UpVector * Tick, LeftPoint + FVector::UpVector * Tick, ThresholdColor, SDPG_World, 2.0f, 0.0f, true);
                    PDI->DrawLine(RightPoint - FVector::UpVector * Tick, RightPoint + FVector::UpVector * Tick, ThresholdColor, SDPG_World, 2.0f, 0.0f, true);
                    PDI->DrawLine(CeilingPoint - FVector::RightVector * Tick, CeilingPoint + FVector::RightVector * Tick, ThresholdColor, SDPG_World, 2.0f, 0.0f, true);

                    if (Sample.bStepUpFail || Sample.bStepDownFail)
                    {
                        const FVector StepTop = BasePoint + FVector::UpVector * FMath::Clamp(FMath::Abs(Sample.StepDelta), 12.0f, 80.0f);
                        PDI->DrawLine(BasePoint, StepTop, ClearanceWarningColor, SDPG_World, 2.5f, 0.0f, true);
                    }
                }
            }

            if (State.TurnClearance.bVisible)
            {
                auto DrawTurnPath = [&](const FMetricTurnPathResult& Path)
                {
                    if (!Path.bTested || Path.SampleOffsets.Num() < 2) return;
                    const FLinearColor PathColor = Path.bBlocked ? ClearanceFailColor : ClearancePassColor;
                    const float PathThickness = StatusThickness(FMath::Max(1.25f, SecondaryLineThickness + 0.5f), Path.bBlocked);
                    FVector Prev = ProxyLocalToWorld.TransformPosition(Path.SampleOffsets[0]);
                    for (int32 Index = 1; Index < Path.SampleOffsets.Num(); ++Index)
                    {
                        const FVector Next = ProxyLocalToWorld.TransformPosition(Path.SampleOffsets[Index]);
                        PDI->DrawLine(Prev, Next, PathColor, SDPG_World, PathThickness, 0.0f, true);
                        Prev = Next;
                    }

                    const FVector EndCenter = ProxyLocalToWorld.TransformPosition(Path.SampleOffsets.Last());
                    const FVector PathForward = Path.SampleOffsets.Num() >= 2
                        ? (Path.SampleOffsets.Last() - Path.SampleOffsets[Path.SampleOffsets.Num() - 2]).GetSafeNormal()
                        : State.Forward.GetSafeNormal();
                    FLinearColor GhostColor = PathColor;
                    GhostColor.A = 0.45f;
                    const FVector GhostBase = EndCenter - FVector::UpVector * (Height * 0.5f);
                    switch (State.Hull.HullType)
                    {
                    case EMetricHullType::Capsule:
                        MetricDraw::DrawCapsule(PDI, GhostBase, PathForward, Height, Width, GhostColor, 1.0f);
                        break;
                    case EMetricHullType::OrientedBox:
                        MetricDraw::DrawBox(PDI, GhostBase, PathForward, FVector(Length * 0.5f, Width * 0.5f, Height * 0.5f), GhostColor, 1.0f);
                        break;
                    case EMetricHullType::Sphere:
                        MetricDraw::DrawSphere(PDI, EndCenter, FMath::Max3(Width, Length, Height) * 0.5f, GhostColor, 1.0f);
                        break;
                    case EMetricHullType::Cylinder:
                    default:
                    {
                        const FVector TurnRight = FVector::CrossProduct(FVector::UpVector, PathForward).GetSafeNormal();
                        MetricDraw::DrawCircle3D(PDI, GhostBase, PathForward, TurnRight, Width * 0.5f, GhostColor, 1.0f);
                        MetricDraw::DrawCircle3D(PDI, GhostBase + FVector::UpVector * Height, PathForward, TurnRight, Width * 0.5f, GhostColor, 1.0f);
                        break;
                    }
                    }

                    if (Path.bBlocked)
                    {
                        const FVector Impact = ProxyLocalToWorld.TransformPosition(Path.ImpactPointOffset);
                        FVector ImpactNormal = ProxyLocalToWorld.TransformVector(Path.ImpactNormal).GetSafeNormal();
                        if (ImpactNormal.IsNearlyZero()) ImpactNormal = FVector::UpVector;
                        PDI->DrawLine(Impact, Impact + ImpactNormal * 32.0f, ClearanceFailColor, SDPG_World, PathThickness, 0.0f, true);
                        const FVector CrossRight = FVector::CrossProduct(ImpactNormal, FVector::UpVector).GetSafeNormal();
                        const FVector CrossAxis = CrossRight.IsNearlyZero() ? FVector::RightVector : CrossRight;
                        PDI->DrawLine(Impact - CrossAxis * 10.0f, Impact + CrossAxis * 10.0f, ClearanceFailColor, SDPG_World, PathThickness, 0.0f, true);
                    }
                };

                DrawTurnPath(State.TurnClearance.Left);
                DrawTurnPath(State.TurnClearance.Right);
            }

            if (State.SensorField.bVisible && !State.SensorField.Rays.IsEmpty())
            {
                const FVector SensorOrigin = ProxyLocalToWorld.TransformPosition(State.SensorField.OriginOffset);
                const FVector SensorRight = FVector::CrossProduct(FVector::UpVector, WorldForward).GetSafeNormal();
                const float Range = FMath::Max(10.0f, State.SensorFieldSettings.Range);
                const float HalfAngleRad = FMath::DegreesToRadians(FMath::Clamp(State.SensorFieldSettings.HorizontalHalfAngle, 1.0f, 89.0f));

                FLinearColor QuietFieldColor = SensorFieldColor;
                QuietFieldColor.A = 0.55f;
                MetricDraw::DrawArc(
                    PDI,
                    SensorOrigin,
                    WorldForward,
                    SensorRight,
                    Range,
                    -HalfAngleRad,
                    HalfAngleRad,
                    QuietFieldColor,
                    FMath::Max(0.5f, SecondaryLineThickness),
                    32);

                for (int32 RayIndex = 0; RayIndex < State.SensorField.Rays.Num(); ++RayIndex)
                {
                    const FMetricSensorFieldRay& Ray = State.SensorField.Rays[RayIndex];
                    const FVector RayEnd = ProxyLocalToWorld.TransformPosition(Ray.EndOffset);
                    const FLinearColor RayColor = Ray.bBlocked ? SensorFieldBlockedColor : SensorFieldColor;
                    const bool bBoundary = RayIndex == 0 || RayIndex == State.SensorField.Rays.Num() - 1;
                    PDI->DrawLine(
                        SensorOrigin,
                        RayEnd,
                        RayColor,
                        SDPG_World,
                        Ray.bBlocked
                            ? StatusThickness(FMath::Max(0.75f, SecondaryLineThickness), true)
                            : (bBoundary ? SecondaryLineThickness + 0.75f : FMath::Max(0.5f, SecondaryLineThickness - 0.15f)),
                        0.0f,
                        true);

                    if (Ray.bBlocked)
                    {
                        const float Tick = 7.0f;
                        PDI->DrawLine(RayEnd - SensorRight * Tick, RayEnd + SensorRight * Tick, SensorFieldBlockedColor, SDPG_World, 1.75f, 0.0f, true);
                    }
                }

                const float VerticalHalfAngleRad = FMath::DegreesToRadians(FMath::Clamp(State.SensorFieldSettings.VerticalHalfAngle, 1.0f, 89.0f));
                const FVector TopDir = (WorldForward * FMath::Cos(VerticalHalfAngleRad) + FVector::UpVector * FMath::Sin(VerticalHalfAngleRad)).GetSafeNormal();
                const FVector BottomDir = (WorldForward * FMath::Cos(VerticalHalfAngleRad) - FVector::UpVector * FMath::Sin(VerticalHalfAngleRad)).GetSafeNormal();
                PDI->DrawLine(SensorOrigin, SensorOrigin + TopDir * Range, QuietFieldColor, SDPG_World, SecondaryLineThickness, 0.0f, true);
                PDI->DrawLine(SensorOrigin, SensorOrigin + BottomDir * Range, QuietFieldColor, SDPG_World, SecondaryLineThickness, 0.0f, true);
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

            // Pinned benchmarks are stored as world-space snapshots but rendered by
            // the same transient Metric proxy. Convert them into this host actor's
            // local space so no additional world-owned Actors are required.
            for (int32 PinIndex = 0; PinIndex < State.PinnedBenchmarks.Num(); ++PinIndex)
            {
                const FMetricPinnedBenchmark& Pin = State.PinnedBenchmarks[PinIndex];
                const FVector PinBase = ProxyLocalToWorld.TransformPosition(Pin.WorldLocation + Pin.Hull.LocalOffset + FVector::UpVector * 1.0f);
                FVector PinForward = ProxyLocalToWorld.TransformVector(Pin.Forward);
                PinForward.Z = 0.0f;
                if (PinForward.Normalize() == false)
                {
                    PinForward = FVector::ForwardVector;
                }

                const bool bSelectedPin = PinIndex == State.SelectedPinnedBenchmarkIndex;
                FLinearColor PinColor = PinnedBenchmarkColor;
                PinColor.A = bSelectedPin ? 1.0f : 0.65f;
                const float PinThickness = bSelectedPin ? 3.0f : 1.5f;
                const float PinHeight = FMath::Max(1.0f, Pin.Hull.Height);
                const float PinWidth = FMath::Max(1.0f, Pin.Hull.Width);
                const float PinLength = FMath::Max(1.0f, Pin.Hull.Length);

                switch (Pin.Hull.HullType)
                {
                case EMetricHullType::Capsule:
                    MetricDraw::DrawCapsule(PDI, PinBase, PinForward, PinHeight, PinWidth, PinColor, PinThickness);
                    break;
                case EMetricHullType::OrientedBox:
                    MetricDraw::DrawBox(PDI, PinBase, PinForward, FVector(PinLength * 0.5f, PinWidth * 0.5f, PinHeight * 0.5f), PinColor, PinThickness);
                    break;
                case EMetricHullType::Sphere:
                    MetricDraw::DrawSphere(PDI, PinBase + FVector::UpVector * (PinHeight * 0.5f), FMath::Max3(PinWidth, PinLength, PinHeight) * 0.5f, PinColor, PinThickness);
                    break;
                case EMetricHullType::Cylinder:
                default:
                {
                    const FVector PinRight = FVector::CrossProduct(FVector::UpVector, PinForward).GetSafeNormal();
                    MetricDraw::DrawCircle3D(PDI, PinBase, PinForward, PinRight, PinWidth * 0.5f, PinColor, PinThickness);
                    MetricDraw::DrawCircle3D(PDI, PinBase + FVector::UpVector * PinHeight, PinForward, PinRight, PinWidth * 0.5f, PinColor, PinThickness);
                    break;
                }
                }

                const FVector PinRight = FVector::CrossProduct(FVector::UpVector, PinForward).GetSafeNormal();
                const FVector PinAnchor = ProxyLocalToWorld.TransformPosition(Pin.WorldLocation + FVector::UpVector * 4.0f);
                MetricDraw::DrawCircle3D(PDI, PinAnchor, PinForward, PinRight, 18.0f, PinColor, bSelectedPin ? 3.5f : 2.0f, 24);
                PDI->DrawLine(PinAnchor, PinAnchor + PinForward * 45.0f, PinColor, SDPG_World, bSelectedPin ? 3.5f : 2.0f, 0.0f, true);
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
    FLinearColor SensorFieldColor;
    FLinearColor SensorFieldBlockedColor;
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
    FLinearColor PinnedBenchmarkColor;
    float SecondaryLineThickness = 1.0f;
    float Thickness = 1.5f;
    bool bEmphasizeProblemLines = true;
    float WarningLineThicknessMultiplier = 1.75f;
    float FailureLineThicknessMultiplier = 2.5f;
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
    return (State.bHasHit || !State.PinnedBenchmarks.IsEmpty()) ? new FMetricViewportSceneProxy(this, State) : nullptr;
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
    const float SensorFieldReach = State.SensorField.bVisible
        ? State.SensorFieldSettings.Range + State.Sensors.PrimarySensorZOffset
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
    const float ThresholdReach = State.ThresholdAnalysis.bVisible
        ? FMath::Max3(
            State.ThresholdAnalysisSettings.ProbeDepth + MaxDimension,
            State.ThresholdAnalysisSettings.MaxSideProbeDistance + MaxDimension,
            State.ThresholdAnalysisSettings.MaxCeilingProbeHeight + MaxDimension)
        : 0.0f;
    const float TurnReach = State.TurnClearance.bVisible
        ? State.TurnClearanceSettings.TurnRadius * 2.0f + MaxDimension
        : 0.0f;
    float PinnedReach = 0.0f;
    for (const FMetricPinnedBenchmark& Pin : State.PinnedBenchmarks)
    {
        PinnedReach = FMath::Max(PinnedReach, Pin.WorldLocation.Size() + FMath::Max3(Pin.Hull.Height, Pin.Hull.Width, Pin.Hull.Length));
    }

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
        FMath::Max(FMath::Max3(RangeReach, SensorReach, LocomotionReach), SensorFieldReach),
        FMath::Max(FMath::Max3(TrajectoryReach, CameraReach, GroundReach), FMath::Max(FMath::Max3(FitReach, SightlineReach, ThresholdReach), FMath::Max(TurnReach, PinnedReach))));
    const FVector Extent(FMath::Max(FMath::Max3(100.0f, MaxDimension, ProbeReach), SecondaryReach));
    return FBoxSphereBounds(FBox(-Extent, Extent)).TransformBy(LocalToWorld);
}
