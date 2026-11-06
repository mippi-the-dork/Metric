#pragma once

#include "CoreMinimal.h"

class SDockTab;
class SWidget;
class FSpawnTabArgs;
class AMetricActor;

namespace MetricActorOverview
{
    extern const FName TabName;
    void Register();
    void Unregister();
    void Open();
    void OpenAndFocusActor(AMetricActor* Actor);
}
