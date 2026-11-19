#pragma once

#include "CoreMinimal.h"
#include "MetricTypes.h"

namespace MetricDiagnosticUtils
{
    METRIC_API FText GetInspectionTargetDisplayName(EMetricInspectionTarget Target);

    /** Resolve Auto to the most relevant visible target, preserving explicit targets. */
    METRIC_API EMetricInspectionTarget ResolveInspectionTarget(
        const FMetricLiveState& State,
        EMetricInspectionTarget RequestedTarget,
        FString* OutReason = nullptr,
        EMetricQueryEvidenceStatus* OutStatus = nullptr);

    METRIC_API bool IsInspectionTargetVisible(const FMetricLiveState& State, EMetricInspectionTarget Target);

    /** Return one representative warning/failure record for normal Primary Issue rendering. */
    METRIC_API const FMetricQueryEvidence* ResolvePrimaryProblemEvidence(const FMetricLiveState& State);
}
