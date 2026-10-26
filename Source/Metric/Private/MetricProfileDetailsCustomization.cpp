#include "MetricProfileDetailsCustomization.h"

#include "MetricPreset.h"
#include "MetricProfileEditorToolkit.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IDetailPropertyRow.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "MetricProfileDetailsCustomization"

TSharedRef<IDetailCustomization> FMetricProfileDetailsCustomization::MakeInstance(TSharedRef<FMetricProfileEditorState> InState)
{
    return MakeShared<FMetricProfileDetailsCustomization>(InState);
}

FMetricProfileDetailsCustomization::FMetricProfileDetailsCustomization(TSharedRef<FMetricProfileEditorState> InState)
    : State(InState)
{
}

void FMetricProfileDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    for (TFieldIterator<FProperty> It(UMetricPreset::StaticClass(), EFieldIteratorFlags::ExcludeSuper); It; ++It)
    {
        DetailBuilder.HideProperty(It->GetFName());
    }

    IDetailCategoryBuilder& IdentityCategory = DetailBuilder.EditCategory(
        TEXT("Profile"),
        LOCTEXT("ProfileCategory", "Profile"),
        ECategoryPriority::Important);

    TSharedRef<IPropertyHandle> DisplayNameHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UMetricPreset, DisplayName));
    if (DisplayNameHandle->IsValidHandle())
    {
        IdentityCategory.AddProperty(DisplayNameHandle);
    }

    if (State->SelectedProperty.IsNone())
    {
        return;
    }

    TSharedRef<IPropertyHandle> SelectedHandle = DetailBuilder.GetProperty(State->SelectedProperty);
    if (!SelectedHandle->IsValidHandle())
    {
        return;
    }

    IDetailCategoryBuilder& SettingsCategory = DetailBuilder.EditCategory(
        TEXT("Selected Metric"),
        LOCTEXT("SelectedMetricCategory", "Selected Metric"),
        ECategoryPriority::Important);

    SettingsCategory.AddProperty(SelectedHandle).ShouldAutoExpand(true);
}

#undef LOCTEXT_NAMESPACE
