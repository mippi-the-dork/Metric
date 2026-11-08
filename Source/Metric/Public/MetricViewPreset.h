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
    virtual void Serialize(FArchive& Ar) override;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metric View", meta=(ToolTip="Optional designer-facing name shown in Metric Display Options. If empty, Metric uses the asset name."))
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Metric View", meta=(ToolTip="Presentation-only Visual and Results visibility mask. Applying a View Preset never changes Metric Profile measurements or analysis enablement."))
    FMetricLayerVisibility Visibility;
};
