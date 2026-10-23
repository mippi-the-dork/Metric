#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
struct FSlateBrush;

class FMetricStyle
{
public:
    static void Initialize();
    static void Shutdown();
    static const FSlateBrush* GetBrush(FName BrushName);

private:
    static TSharedPtr<FSlateStyleSet> StyleSet;
};
