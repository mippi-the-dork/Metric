#pragma once

#include "IDetailCustomization.h"

class FMetricActorDetailsCustomization final : public IDetailCustomization
{
public:
    static TSharedRef<IDetailCustomization> MakeInstance();
    virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
