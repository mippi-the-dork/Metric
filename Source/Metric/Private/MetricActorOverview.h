#pragma once

#include "CoreMinimal.h"

class SDockTab;
class SWidget;
class FSpawnTabArgs;

namespace MetricActorOverview
{
    extern const FName TabName;
    void Register();
    void Unregister();
    void Open();
}
