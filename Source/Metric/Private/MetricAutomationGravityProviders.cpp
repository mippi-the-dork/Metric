#include "MetricAutomationGravityProviders.h"

bool UMetricAutomationRotatingGravityProvider::SampleGravityForMetric_Implementation(
    UObject* WorldContextObject,
    FVector WorldLocation,
    AActor* ContextActor,
    FVector& OutUpDirection,
    float& OutGravityMagnitude) const
{
    (void)WorldContextObject;
    (void)ContextActor;
    const float Angle = FMath::DegreesToRadians((WorldLocation.X / 100.0f) * DegreesPer100Units);
    OutUpDirection = FVector(FMath::Sin(Angle), 0.0f, FMath::Cos(Angle)).GetSafeNormal();
    OutGravityMagnitude = FMath::Max(0.0f, GravityMagnitude);
    return !OutUpDirection.IsNearlyZero();
}
