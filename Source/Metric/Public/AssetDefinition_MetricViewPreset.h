#pragma once

#include "AssetDefinition_DataAsset.h"
#include "AssetDefinition_MetricViewPreset.generated.h"

UCLASS()
class METRIC_API UAssetDefinition_MetricViewPreset final : public UAssetDefinition_DataAsset
{
    GENERATED_BODY()
public:
    virtual FText GetAssetDisplayName() const override;
    virtual FText GetAssetDisplayName(const FAssetData& AssetData) const override;
    virtual FLinearColor GetAssetColor() const override;
    virtual TSoftClassPtr<UObject> GetAssetClass() const override;
};
