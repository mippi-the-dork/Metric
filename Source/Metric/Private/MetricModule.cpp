#include "MetricModule.h"

#include "MetricCommands.h"
#include "MetricInputProcessor.h"
#include "MetricLog.h"
#include "MetricViewportController.h"
#include "Framework/Application/SlateApplication.h"

#define LOCTEXT_NAMESPACE "FMetricModule"

void FMetricModule::StartupModule()
{
    FMetricCommands::Register();
    ViewportController = MakeUnique<FMetricViewportController>();

    if (FSlateApplication::IsInitialized())
    {
        InputProcessor = MakeShared<FMetricInputProcessor>();
        // Index 0 gives Metric first opportunity to consume the two legacy
        // preview-mesh chords while the Level Editor viewport has focus.
        FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
        UE_LOG(LogMetric, Log, TEXT("Metric 0.4.1 input processor registered."));
    }
}

void FMetricModule::ShutdownModule()
{
    if (InputProcessor.IsValid() && FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
    }

    InputProcessor.Reset();
    ViewportController.Reset();
    FMetricCommands::Unregister();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMetricModule, Metric)
