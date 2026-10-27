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
        ToggleLiveAnalysis,
        "Toggle Live Metric Analysis",
        "Enable or pause the cursor-driven Live Metric analyzer while leaving Metric and pinned benchmarks active.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::Backslash, EModifierKey::FromBools(true, true, false, false)));

    UI_COMMAND(
        CycleMetric,
        "Cycle Metric Entry",
        "Cycle to the next Metric preset or included Unreal preview mesh.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::Backslash, EModifierKey::Shift));

    UI_COMMAND(
        PinMetricBenchmark,
        "Pin Metric Benchmark",
        "Pin the current Metric hull at the live lens position for this editor world.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::Backslash, EModifierKey::Control));

    UI_COMMAND(
        NextMetricBenchmark,
        "Next Metric Benchmark",
        "Select the next pinned Metric benchmark. Wraps through None after the last benchmark.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::Backslash, EModifierKey::Alt));

    UI_COMMAND(
        ClearMetricBenchmarks,
        "Clear Metric Benchmarks",
        "Clear all pinned Metric benchmarks in the current editor world.",
        EUserInterfaceActionType::Button,
        FInputChord(EKeys::Backslash, EModifierKey::FromBools(true, false, true, false)));
}

#undef LOCTEXT_NAMESPACE
