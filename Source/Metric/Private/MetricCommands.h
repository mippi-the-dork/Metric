#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "InputCoreTypes.h"
#include "Styling/AppStyle.h"

class FMetricCommands final : public TCommands<FMetricCommands>
{
public:
    FMetricCommands()
        : TCommands<FMetricCommands>(
            TEXT("Metric"),
            NSLOCTEXT("Contexts", "Metric", "Metric"),
            NAME_None,
            FAppStyle::GetAppStyleSetName())
    {
    }

    virtual void RegisterCommands() override;

    TSharedPtr<FUICommandInfo> ToggleMetric;
    TSharedPtr<FUICommandInfo> CycleMetric;
    TSharedPtr<FUICommandInfo> PinMetricBenchmark;
    TSharedPtr<FUICommandInfo> NextMetricBenchmark;
    TSharedPtr<FUICommandInfo> ClearMetricBenchmarks;
};
