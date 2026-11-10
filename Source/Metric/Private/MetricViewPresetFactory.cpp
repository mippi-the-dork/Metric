#include "MetricViewPresetFactory.h"

#include "MetricViewPreset.h"

#define LOCTEXT_NAMESPACE "MetricViewPresetFactory"

UMetricViewPresetFactory::UMetricViewPresetFactory()
{
    SupportedClass = UMetricViewPreset::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UMetricViewPresetFactory::FactoryCreateNew(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UMetricViewPreset>(InParent, Class, Name, Flags | RF_Transactional);
}

FText UMetricViewPresetFactory::GetDisplayName() const
{
    return LOCTEXT("DisplayName", "Metric View Preset");
}

FText UMetricViewPresetFactory::GetToolTip() const
{
    return LOCTEXT("ToolTip", "Create a presentation-only Metric View Preset that temporarily filters which world visualizations and HUD results are shown.");
}

FString UMetricViewPresetFactory::GetDefaultNewAssetName() const
{
    return TEXT("DA_Metric_ViewPreset");
}

#undef LOCTEXT_NAMESPACE
