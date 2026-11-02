#pragma once

#include "CoreMinimal.h"
#include "MetricEditorSettings.h"

struct FMetricAnalysisBasis
{
    FVector Up = FVector::UpVector;
    FVector Forward = FVector::ForwardVector;
    FVector Right = FVector::RightVector;
};

namespace MetricFrameUtils
{
    FMetricAnalysisBasis BuildAnalysisBasis(
        EMetricAnalysisFrameMode Mode,
        const FVector& SurfaceNormal,
        const FVector& CustomUp,
        const FVector& PreferredForward);
}
