#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;

class FMetricStyle
{
public:
    static void Initialize();
    static void Shutdown();

private:
    static TSharedPtr<FSlateStyleSet> StyleSet;
};
