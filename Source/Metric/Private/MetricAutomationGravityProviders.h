#pragma once

#include "CoreMinimal.h"
#include "MetricGravityProvider.h"
#include "MetricAutomationGravityProviders.generated.h"

/** Automation-only provider whose Up rotates around world Y as X increases. */
UCLASS()
class UMetricAutomationRotatingGravityProvider : public UMetricGravityProvider
{
    GENERATED_BODY()

public:
    UPROPERTY()
    float DegreesPer100Units = 20.0f;

    UPROPERTY()
    float GravityMagnitude = 980.0f;

    virtual bool SampleGravityForMetric_Implementation(
        UObject* WorldContextObject,
        FVector WorldLocation,
        AActor* ContextActor,
        FVector& OutUpDirection,
        float& OutGravityMagnitude) const override;
};
