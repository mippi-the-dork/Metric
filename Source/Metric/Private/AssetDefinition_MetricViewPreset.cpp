#include "AssetDefinition_MetricViewPreset.h"
#include "MetricViewPreset.h"
#define LOCTEXT_NAMESPACE "AssetDefinition_MetricViewPreset"
FText UAssetDefinition_MetricViewPreset::GetAssetDisplayName() const { return LOCTEXT("MetricViewPresetDisplayName", "Metric View Preset"); }
FText UAssetDefinition_MetricViewPreset::GetAssetDisplayName(const FAssetData& AssetData) const { (void)AssetData; return GetAssetDisplayName(); }
FLinearColor UAssetDefinition_MetricViewPreset::GetAssetColor() const { return FLinearColor::FromSRGBColor(FColor(0xFF,0x94,0x0A,0xFF)); }
TSoftClassPtr<UObject> UAssetDefinition_MetricViewPreset::GetAssetClass() const { return UMetricViewPreset::StaticClass(); }
#undef LOCTEXT_NAMESPACE
