#include "AssetDefinition_MetricProfile.h"

#include "MetricProfile.h"
#include "MetricProfileEditorToolkit.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_MetricProfile"

FText UAssetDefinition_MetricProfile::GetAssetDisplayName() const
{
    return LOCTEXT("MetricProfileDisplayName", "Metric Profile");
}

FText UAssetDefinition_MetricProfile::GetAssetDisplayName(const FAssetData& AssetData) const
{
    (void)AssetData;
    return GetAssetDisplayName();
}

FLinearColor UAssetDefinition_MetricProfile::GetAssetColor() const
{
    return FLinearColor::FromSRGBColor(FColor(0xFF, 0x94, 0x0A, 0xFF));
}

TSoftClassPtr<UObject> UAssetDefinition_MetricProfile::GetAssetClass() const
{
    return UMetricProfile::StaticClass();
}

EAssetCommandResult UAssetDefinition_MetricProfile::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
    for (UMetricProfile* Preset : OpenArgs.LoadObjects<UMetricProfile>())
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
