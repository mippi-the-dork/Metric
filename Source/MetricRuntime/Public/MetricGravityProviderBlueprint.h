#pragma once

#include "CoreMinimal.h"
#include "Engine/Blueprint.h"
#include "MetricGravityProviderBlueprint.generated.h"

/**
 * Specialized Blueprint asset for Metric gravity providers.
 *
 * The generated class is always expected to derive from UMetricGravityProvider.
 * This lives in MetricRuntime so provider Blueprints remain normal runtime-capable
 * Blueprint assets even though Metric's authoring and analysis tools are editor-only.
 * Metric still provides a one-click Content Browser creation workflow with no parent picker.
 */
UCLASS(BlueprintType)
class METRICRUNTIME_API UMetricGravityProviderBlueprint : public UBlueprint
{
    GENERATED_BODY()
};
