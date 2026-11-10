#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MetricGravityProvider.generated.h"

class AActor;

/**
 * Blueprint/C++ adapter used by Metric when Analysis Frame is Gravity / Provider.
 *
 * Metric supplies the world-space location it is evaluating. Integrations return
 * the gravity-relative Up direction and magnitude at that location. The optional
 * Context Actor lets gravity systems apply receiver-specific rules when needed.
 *
 * Metric does not know which gravity system is behind this provider. A project can
 * implement this class in Blueprint, or a gravity plugin can provide a native child
 * class in a separate optional integration module.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class METRIC_API UMetricGravityProvider : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Sample gravity for a Metric analysis point.
     *
     * @param WorldContextObject The world Metric is evaluating. Usually only needed by integrations that query a world subsystem.
     * @param WorldLocation      Exact world-space position Metric wants gravity for.
     * @param ContextActor       Optional receiver/context actor. Live viewport analysis may provide none.
     * @param OutUpDirection     World-space gravity-relative Up direction. Must be non-zero when returning true.
     * @param OutGravityMagnitude Gravity magnitude in cm/s^2. Zero is allowed for systems that provide orientation without acceleration.
     * @return true when this provider supplied a valid Up direction for the requested location.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Metric|Gravity",
        meta = (DisplayName = "Sample Gravity for Metric", AdvancedDisplay = "WorldContextObject,ContextActor"))
    bool SampleGravityForMetric(
        UObject* WorldContextObject,
        FVector WorldLocation,
        AActor* ContextActor,
        FVector& OutUpDirection,
        float& OutGravityMagnitude) const;

    virtual bool SampleGravityForMetric_Implementation(
        UObject* WorldContextObject,
        FVector WorldLocation,
        AActor* ContextActor,
        FVector& OutUpDirection,
        float& OutGravityMagnitude) const;
};


/** Simple built-in provider for projects that only need one constant gravity frame. */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "Metric Constant Gravity Provider"))
class METRIC_API UMetricConstantGravityProvider : public UMetricGravityProvider
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Metric|Gravity", meta = (ToolTip = "World-space gravity-relative Up direction returned at every sampled location."))
    FVector UpDirection = FVector::UpVector;

    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Metric|Gravity", meta = (ClampMin = "0.0", Units = "cm/s^2", ToolTip = "Gravity magnitude returned at every sampled location."))
    float GravityMagnitude = 980.0f;

    virtual bool SampleGravityForMetric_Implementation(
        UObject* WorldContextObject,
        FVector WorldLocation,
        AActor* ContextActor,
        FVector& OutUpDirection,
        float& OutGravityMagnitude) const override;
};
