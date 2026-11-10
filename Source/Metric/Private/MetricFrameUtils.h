#pragma once

#include "CoreMinimal.h"
#include "MetricEditorSettings.h"
#include "MetricTypes.h"

struct FMetricAnalysisBasis
{
    FVector Up = FVector::UpVector;
    FVector Forward = FVector::ForwardVector;
    FVector Right = FVector::RightVector;
};

class AActor;
class UWorld;

namespace MetricFrameUtils
{
    /** Resolve the configured Metric gravity provider at an exact world location. */
    bool TryResolveProviderGravity(
        UWorld* World,
        const FVector& WorldLocation,
        AActor* ContextActor,
        FVector& OutUpDirection,
        float& OutGravityMagnitude);

    /** Convenience overload for callers that only need Up. */
    bool TryResolveProviderUp(UWorld* World, const FVector& WorldLocation, FVector& OutUpDirection);

    /** Surface-following helper retained for Surface Normal frame sampling. */
    FVector ResolveSurfaceUp(bool bFollowSurface, const FVector& FixedUp, const FVector& SampleSurfaceNormal);

    /**
     * Re-project a previous forward vector onto a new local Up plane. This is
     * Metric's stable frame-transport rule for changing gravity/surface fields.
     */
    FVector TransportForward(const FVector& PreviousForward, const FVector& NewUp);

    FMetricAnalysisBasis BuildAnalysisBasis(
        EMetricAnalysisFrameMode Mode,
        const FVector& SurfaceNormal,
        const FVector& CustomUp,
        const FVector& PreferredForward);
}
