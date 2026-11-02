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

    bool LineTraceSingleByChannel(UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel Channel, const FCollisionQueryParams& Params);
    bool SweepSingleByChannel(UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rotation, ECollisionChannel Channel, const FCollisionShape& Shape, const FCollisionQueryParams& Params);

    int32 GetSelectedActorCount();
    int32 GetSelectedComponentCount();
    int32 GetInstanceExclusionCount();

    int32 ExcludeSelectedActors();
    int32 ExcludeSelectedComponents();
    void ClearInstanceExclusions();

#if WITH_DEV_AUTOMATION_TESTS
    void AddActorExclusionForAutomation(UWorld* World, class AActor* Actor);
    void AddComponentExclusionForAutomation(UWorld* World, class UPrimitiveComponent* Component);
    int32 GetInstanceExclusionCountForAutomation(UWorld* World);
    void ClearInstanceExclusionsForAutomation(UWorld* World);
#endif
}
