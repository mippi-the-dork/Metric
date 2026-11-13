#include "AssetDefinition_MetricGravityProviderBlueprint.h"

#include "MetricGravityProviderBlueprint.h"
#include "MetricStyle.h"

#define LOCTEXT_NAMESPACE "AssetDefinition_MetricGravityProviderBlueprint"

namespace
{
    const TArray<FAssetCategoryPath>& GetMetricTestingCategories()
    {
        static const TArray<FAssetCategoryPath> Categories =
        {
            FAssetCategoryPath(INVTEXT("Testing")) / INVTEXT("Metric")
        };
        return Categories;
    }
}

FText UAssetDefinition_MetricGravityProviderBlueprint::GetAssetDisplayName() const
{
    return LOCTEXT("DisplayName", "Metric Gravity Provider");
}

FText UAssetDefinition_MetricGravityProviderBlueprint::GetAssetDisplayName(const FAssetData& AssetData) const
{
    (void)AssetData;
    return GetAssetDisplayName();
}

FLinearColor UAssetDefinition_MetricGravityProviderBlueprint::GetAssetColor() const
{
    return FLinearColor::FromSRGBColor(FColor(0x3F, 0x7E, 0xFF, 0xFF));
}

TSoftClassPtr<UObject> UAssetDefinition_MetricGravityProviderBlueprint::GetAssetClass() const
{
    return UMetricGravityProviderBlueprint::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_MetricGravityProviderBlueprint::GetAssetCategories() const
{
    return GetMetricTestingCategories();
}

const FSlateBrush* UAssetDefinition_MetricGravityProviderBlueprint::GetIconBrush(const FAssetData& InAssetData, const FName InClassName) const
{
    (void)InAssetData;
    (void)InClassName;
    return FMetricStyle::GetBrush(TEXT("ClassIcon.MetricGravityProviderBlueprint"));
}

const FSlateBrush* UAssetDefinition_MetricGravityProviderBlueprint::GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const
{
    (void)InAssetData;
    (void)InClassName;
    return FMetricStyle::GetBrush(TEXT("ClassThumbnail.MetricGravityProviderBlueprint"));
}

#undef LOCTEXT_NAMESPACE
