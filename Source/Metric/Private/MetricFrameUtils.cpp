#include "MetricFrameUtils.h"

namespace MetricFrameUtils
{
    FMetricAnalysisBasis BuildAnalysisBasis(
        EMetricAnalysisFrameMode Mode,
        const FVector& SurfaceNormal,
        const FVector& CustomUp,
        const FVector& PreferredForward)
    {
        FMetricAnalysisBasis Basis;

        switch (Mode)
        {
        case EMetricAnalysisFrameMode::SurfaceNormal:
            Basis.Up = SurfaceNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
            break;
        case EMetricAnalysisFrameMode::CustomUp:
            Basis.Up = CustomUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
            break;
        case EMetricAnalysisFrameMode::World:
        default:
            Basis.Up = FVector::UpVector;
            break;
        }

        Basis.Forward = FVector::VectorPlaneProject(PreferredForward, Basis.Up).GetSafeNormal();
        if (Basis.Forward.IsNearlyZero())
        {
            Basis.Forward = FVector::VectorPlaneProject(FVector::ForwardVector, Basis.Up).GetSafeNormal();
        }
        if (Basis.Forward.IsNearlyZero())
        {
            Basis.Forward = FVector::VectorPlaneProject(FVector::RightVector, Basis.Up).GetSafeNormal();
        }

        Basis.Right = FVector::CrossProduct(Basis.Up, Basis.Forward).GetSafeNormal();
        if (Basis.Right.IsNearlyZero())
        {
            Basis.Right = FVector::RightVector;
        }
        Basis.Forward = FVector::CrossProduct(Basis.Right, Basis.Up).GetSafeNormal();
        return Basis;
    }
}
