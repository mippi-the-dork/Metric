#include "MetricProfileFactory.h"

#include "MetricProfile.h"

#define LOCTEXT_NAMESPACE "MetricProfileFactory"

UMetricProfileFactory::UMetricProfileFactory()
{
    SupportedClass = UMetricProfile::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UMetricProfileFactory::FactoryCreateNew(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UMetricProfile>(InParent, Class, Name, Flags | RF_Transactional);
}

FText UMetricProfileFactory::GetDisplayName() const
{
    return LOCTEXT("DisplayName", "Metric Profile");
}

FText UMetricProfileFactory::GetToolTip() const
{
    return LOCTEXT("ToolTip", "Create a Metric Profile that defines an entity's analysis settings, enablement, HUD defaults, Visualizer defaults, and visualization colors.");
}

FString UMetricProfileFactory::GetDefaultNewAssetName() const
{
    return TEXT("DA_Metric_Profile");
}

#undef LOCTEXT_NAMESPACE
