#pragma once

#include "Modules/ModuleManager.h"

class FMetricInputProcessor;
class FMetricViewportController;
class AActor;
class UObject;
class UWorld;
struct FPropertyChangedEvent;

class FMetricModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenus();
    void HandleLevelActorChanged(AActor* Actor);
    void HandleObjectPropertyChanged(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);
    void RequestMetricActorUpdates(UWorld* World, const AActor* SourceActor = nullptr);
    TSharedPtr<FMetricInputProcessor> InputProcessor;
    TUniquePtr<FMetricViewportController> ViewportController;
};
