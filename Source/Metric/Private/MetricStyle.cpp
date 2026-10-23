#include "MetricStyle.h"

#include "Brushes/SlateImageBrush.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FMetricStyle::StyleSet;

void FMetricStyle::Initialize()
{
    if (StyleSet.IsValid())
    {
        return;
    }

    const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Metric"));
    if (!Plugin.IsValid())
    {
        return;
    }

    StyleSet = MakeShared<FSlateStyleSet>(TEXT("MetricStyle"));
    StyleSet->SetContentRoot(FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources")));

    const FString ProfileIconPath = StyleSet->RootToContentDir(TEXT("MetricPresetAsset_64"), TEXT(".svg"));
    if (FPaths::FileExists(ProfileIconPath))
    {
        StyleSet->Set(TEXT("ClassIcon.MetricPreset"), new FSlateVectorImageBrush(ProfileIconPath, FVector2D(16.0f, 16.0f)));
        StyleSet->Set(TEXT("ClassThumbnail.MetricPreset"), new FSlateVectorImageBrush(ProfileIconPath, FVector2D(64.0f, 64.0f)));
    }

    // Reuse the exact Metric Profile SVG for viewport chrome as well. Slate's
    // vector brush stays crisp at both the full HUD and minimized button sizes.
    if (FPaths::FileExists(ProfileIconPath))
    {
        StyleSet->Set(TEXT("Metric.HUDIcon"), new FSlateVectorImageBrush(ProfileIconPath, FVector2D(28.0f, 28.0f)));
    }

    FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
}

void FMetricStyle::Shutdown()
{
    if (!StyleSet.IsValid())
    {
        return;
    }

    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
    StyleSet.Reset();
}

const FSlateBrush* FMetricStyle::GetBrush(FName BrushName)
{
    return StyleSet.IsValid() ? StyleSet->GetBrush(BrushName) : nullptr;
}
