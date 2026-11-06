#include "MetricActorDetailsCustomization.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"

TSharedRef<IDetailCustomization> FMetricActorDetailsCustomization::MakeInstance()
{
    return MakeShared<FMetricActorDetailsCustomization>();
}

void FMetricActorDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    // Metric owns one top-level category. Analysis, Display, and Results remain
    // nested groups declared with Metric|... category paths on AMetricActor.
    IDetailCategoryBuilder& MetricCategory = DetailBuilder.EditCategory(TEXT("Metric"));
    MetricCategory.InitiallyCollapsed(false);
    MetricCategory.RestoreExpansionState(true);

    // Position Metric after Surface when Surface is present, otherwise directly
    // after Transform. Metric's sort callback can run after other customizations,
    // so anchoring to Transform unconditionally can accidentally pull Metric back
    // in front of categories that another plugin already inserted there.
    DetailBuilder.SortCategories([](const TMap<FName, IDetailCategoryBuilder*>& Categories)
    {
        IDetailCategoryBuilder* const* MetricEntry =
            Categories.Find(FName(TEXT("Metric")));

        if (MetricEntry == nullptr || *MetricEntry == nullptr)
        {
            return;
        }

        IDetailCategoryBuilder* Anchor = nullptr;

        auto UseCategoryAsAnchor =
            [&Categories, &Anchor](FName CategoryName)
            {
                IDetailCategoryBuilder* const* Entry =
                    Categories.Find(CategoryName);

                if (Entry != nullptr && *Entry != nullptr)
                {
                    Anchor = *Entry;
                    return true;
                }

                return false;
            };

        // Surface's Actor layout ends with Surface_Components. Prefer that exact
        // anchor so Metric naturally follows both Surface sections.
        if (!UseCategoryAsAnchor(FName(TEXT("Surface_Components"))))
        {
            // Older or partial Surface layouts may expose only Surface_Favorites.
            if (!UseCategoryAsAnchor(FName(TEXT("Surface_Favorites"))))
            {
                if (!UseCategoryAsAnchor(FName(TEXT("TransformCommon"))))
                {
                    UseCategoryAsAnchor(FName(TEXT("Transform")));
                }
            }
        }

        if (Anchor == nullptr)
        {
            return;
        }

        IDetailCategoryBuilder* Metric = *MetricEntry;
        const int32 AnchorOrder = Anchor->GetSortOrder();

        if (AnchorOrder >= MAX_int32 - 1)
        {
            return;
        }

        for (const TPair<FName, IDetailCategoryBuilder*>& Entry : Categories)
        {
            IDetailCategoryBuilder* Other = Entry.Value;

            if (Other == nullptr || Other == Metric || Other == Anchor)
            {
                continue;
            }

            const int32 ExistingOrder = Other->GetSortOrder();

            if (ExistingOrder > AnchorOrder && ExistingOrder < MAX_int32)
            {
                Other->SetSortOrder(ExistingOrder + 1);
            }
        }

        Metric->SetSortOrder(AnchorOrder + 1);
    });
}
