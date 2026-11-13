#pragma once

#include "Script/AssetDefinition_Blueprint.h"
#include "AssetDefinition_MetricGravityProviderBlueprint.generated.h"

UCLASS()
class METRIC_API UAssetDefinition_MetricGravityProviderBlueprint final : public UAssetDefinition_Blueprint
{
    GENERATED_BODY()

public:
    virtual FText GetAssetDisplayName() const override;
    virtual FText GetAssetDisplayName(const FAssetData& AssetData) const override;
    virtual FLinearColor GetAssetColor() const override;
    virtual TSoftClassPtr<UObject> GetAssetClass() const override;
    virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
    virtual const FSlateBrush* GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const override;
    virtual const FSlateBrush* GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const override;
};
