#include "AssetDefinition_MetricViewPreset.h"

#include "MetricViewPreset.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_MetricViewPreset"

namespace
{
    const TArray<FAssetCategoryPath>& GetMetricTestingCategories()
    {
        static const TArray<FAssetCategoryPath> Categories =
        {
            FAssetCategoryPath(INVTEXT("Testing")) / INVTEXT("Metric")
        };
        return Categories;
    }
}

FText UAssetDefinition_MetricViewPreset::GetAssetDisplayName() const
{
    return LOCTEXT("MetricViewPresetDisplayName", "Metric View Preset");
}

FText UAssetDefinition_MetricViewPreset::GetAssetDisplayName(const FAssetData& AssetData) const
{
    (void)AssetData;
    return GetAssetDisplayName();
}

FLinearColor UAssetDefinition_MetricViewPreset::GetAssetColor() const
{
    return FLinearColor::FromSRGBColor(FColor(0xFF, 0x5A, 0x24, 0xFF));
}

TSoftClassPtr<UObject> UAssetDefinition_MetricViewPreset::GetAssetClass() const
{
    return UMetricViewPreset::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_MetricViewPreset::GetAssetCategories() const
{
    return GetMetricTestingCategories();
}

#undef LOCTEXT_NAMESPACE
