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
        StyleSet->Set(TEXT("ClassIcon.MetricViewPreset"), new FSlateVectorImageBrush(ProfileIconPath, FVector2D(16.0f, 16.0f)));
        StyleSet->Set(TEXT("ClassThumbnail.MetricViewPreset"), new FSlateVectorImageBrush(ProfileIconPath, FVector2D(64.0f, 64.0f)));
    }

    // Use a rasterized copy of the exact Metric SVG for viewport chrome.
    // UE 5.8 reliably renders this through FSlateImageBrush in overlay buttons,
    // while the Content Browser continues to use the vector asset.
    const FString HUDIconPath = StyleSet->RootToContentDir(TEXT("MetricHUD_64"), TEXT(".png"));
    if (FPaths::FileExists(HUDIconPath))
    {
        StyleSet->Set(TEXT("Metric.HUDIcon"), new FSlateImageBrush(
            HUDIconPath,
            FVector2D(32.0f, 32.0f),
            FLinearColor::White,
            ESlateBrushTileType::NoTile,
            ESlateBrushImageType::FullColor));
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
