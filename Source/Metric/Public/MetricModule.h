#pragma once

#include "Modules/ModuleManager.h"

class FMetricInputProcessor;
class FMetricViewportController;

class FMetricModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    TSharedPtr<FMetricInputProcessor> InputProcessor;
    TUniquePtr<FMetricViewportController> ViewportController;
};
