#include "AssetDefinition_MetricPreset.h"

#include "MetricPreset.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_MetricPreset"

FText UAssetDefinition_MetricPreset::GetAssetDisplayName() const
{
    return LOCTEXT("MetricProfileDisplayName", "Metric Profile");
}

FText UAssetDefinition_MetricPreset::GetAssetDisplayName(const FAssetData& AssetData) const
{
    (void)AssetData;
    return GetAssetDisplayName();
}

FLinearColor UAssetDefinition_MetricPreset::GetAssetColor() const
{
    return FLinearColor::FromSRGBColor(FColor(0xFF, 0x94, 0x0A, 0xFF));
}

TSoftClassPtr<UObject> UAssetDefinition_MetricPreset::GetAssetClass() const
{
    return UMetricPreset::StaticClass();
}

#undef LOCTEXT_NAMESPACE
