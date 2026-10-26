#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MetricTypes.h"
#include "MetricViewPreset.generated.h"

UCLASS(BlueprintType)
class METRIC_API UMetricViewPreset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metric View")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metric View")
    FMetricLayerVisibility Visibility;
};
