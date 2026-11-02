#include "MetricActorDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

TSharedRef<IDetailCustomization> FMetricActorDetailsCustomization::MakeInstance()
{
    return MakeShared<FMetricActorDetailsCustomization>();
}

void FMetricActorDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    DetailBuilder.EditCategory(TEXT("Metric")).SetSortOrder(3);
    DetailBuilder.EditCategory(TEXT("Metric - Analysis")).SetSortOrder(4);
    DetailBuilder.EditCategory(TEXT("Metric - Display")).SetSortOrder(5);
    DetailBuilder.EditCategory(TEXT("Metric - Results")).SetSortOrder(6);
}
