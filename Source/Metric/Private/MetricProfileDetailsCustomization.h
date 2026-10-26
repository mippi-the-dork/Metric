#pragma once

#include "IDetailCustomization.h"

struct FMetricProfileEditorState;

class FMetricProfileDetailsCustomization final : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance(TSharedRef<FMetricProfileEditorState> InState);
    explicit FMetricProfileDetailsCustomization(TSharedRef<FMetricProfileEditorState> InState);

    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
    TSharedRef<FMetricProfileEditorState> State;
};
