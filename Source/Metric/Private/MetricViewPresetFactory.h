#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MetricViewPresetFactory.generated.h"

UCLASS()
class METRIC_API UMetricViewPresetFactory final : public UFactory
{
    GENERATED_BODY()

public:
    UMetricViewPresetFactory();

    virtual UObject* FactoryCreateNew(
        UClass* Class,
        UObject* InParent,
        FName Name,
        EObjectFlags Flags,
        UObject* Context,
        FFeedbackContext* Warn) override;

    virtual FText GetDisplayName() const override;
    virtual FText GetToolTip() const override;
    virtual FString GetDefaultNewAssetName() const override;
};
