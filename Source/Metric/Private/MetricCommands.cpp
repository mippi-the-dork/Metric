#include "MetricCommands.h"

#define LOCTEXT_NAMESPACE "MetricCommands"

void FMetricCommands::RegisterCommands()
{
    UI_COMMAND(
        ToggleMetric,
        "Toggle Metric",
        "Show or hide the Metric viewport lens.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::Backslash));

    UI_COMMAND(
        CycleMetric,
        "Cycle Metric Entry",
        "Cycle to the next Metric preset or included Unreal preview mesh.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::Backslash, EModifierKey::Shift));
}

#undef LOCTEXT_NAMESPACE
