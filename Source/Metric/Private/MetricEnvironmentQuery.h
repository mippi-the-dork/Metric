#pragma once

#include "CoreMinimal.h"
#include "CollisionQueryParams.h"

class UWorld;

namespace MetricEnvironmentQuery
{
    // Central collision-query policy for Metric. All Metric world queries pass
    // through this function so environment-source behavior, exclusions,
    // inspection and failure-contact capture can evolve in one place.
    void ApplyExclusions(UWorld* World, FCollisionQueryParams& Params);

    FCollisionQueryParams MakeParams(UWorld* World, FName TraceTag, bool bTraceComplex = true);
    FCollisionQueryParams MakeParams(UWorld* World, FName TraceTag, const TStatId& StatId, bool bTraceComplex = true);

    int32 GetSelectedActorCount();
    int32 GetSelectedComponentCount();
    int32 GetInstanceExclusionCount();

    int32 ExcludeSelectedActors();
    int32 ExcludeSelectedComponents();
    void ClearInstanceExclusions();
}
