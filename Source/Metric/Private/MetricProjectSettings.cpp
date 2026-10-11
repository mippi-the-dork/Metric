#include "MetricProjectSettings.h"

#define LOCTEXT_NAMESPACE "MetricProjectSettings"

UMetricProjectSettings::UMetricProjectSettings()
{
}

FText UMetricProjectSettings::GetSectionText() const
{
    return LOCTEXT("SectionText", "Metric");
}

#undef LOCTEXT_NAMESPACE
