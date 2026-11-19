#include "MetricGravityProvider.h"

bool UMetricGravityProvider::SampleGravityForMetric_Implementation(
    UObject* WorldContextObject,
    FVector WorldLocation,
    AActor* ContextActor,
    FVector& OutUpDirection,
    float& OutGravityMagnitude) const
{
    OutUpDirection = FVector::UpVector;
    OutGravityMagnitude = 0.0f;
    return false;
}


bool UMetricConstantGravityProvider::SampleGravityForMetric_Implementation(
    UObject* WorldContextObject,
    FVector WorldLocation,
    AActor* ContextActor,
    FVector& OutUpDirection,
    float& OutGravityMagnitude) const
{
    (void)WorldContextObject;
    (void)WorldLocation;
    (void)ContextActor;
    if (UpDirection.IsNearlyZero())
    {
        OutUpDirection = FVector::UpVector;
        OutGravityMagnitude = 0.0f;
        return false;
    }

    OutUpDirection = UpDirection.GetSafeNormal();
    OutGravityMagnitude = FMath::Max(0.0f, GravityMagnitude);
    return true;
}
