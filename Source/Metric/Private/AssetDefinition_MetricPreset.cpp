#include "AssetDefinition_MetricPreset.h"

#include "MetricPreset.h"
#include "MetricProfileEditorToolkit.h"

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

EAssetCommandResult UAssetDefinition_MetricPreset::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    for (UMetricPreset* Preset : OpenArgs.LoadObjects<UMetricPreset>())
    {
        if (!Preset)
        {
            continue;
        }

        TSharedRef<FMetricProfileEditorToolkit> Editor = MakeShared<FMetricProfileEditorToolkit>();
        Editor->Initialize(Preset, EToolkitMode::Standalone, nullptr);
    }

    return EAssetCommandResult::Handled;
}

#undef LOCTEXT_NAMESPACE
