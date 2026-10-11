#include "MetricEditorSettings.h"

#define LOCTEXT_NAMESPACE "MetricEditorSettings"

FText UMetricEditorSettings::GetSectionText() const
{
    return LOCTEXT("SectionText", "Metric - User");
}

#undef LOCTEXT_NAMESPACE
