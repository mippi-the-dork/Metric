#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "MetricGravityProviderBlueprint.generated.h"

/**
 * Specialized Blueprint asset for Metric gravity providers.
 *
 * The generated class is always expected to derive from UMetricGravityProvider.
 * This keeps authoring in the normal Blueprint editor while allowing Metric to
 * provide a one-click Content Browser creation workflow with no parent picker.
 */
UCLASS(BlueprintType)
class METRIC_API UMetricGravityProviderBlueprint : public UBlueprint
{
    GENERATED_BODY()
};
