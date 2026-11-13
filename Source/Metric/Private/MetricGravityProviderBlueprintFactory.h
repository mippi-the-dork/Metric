#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "MetricGravityProviderBlueprintFactory.generated.h"

UCLASS()
class UMetricGravityProviderBlueprintFactory final : public UFactory
{
    GENERATED_BODY()

public:
    UMetricGravityProviderBlueprintFactory();

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
    virtual FName GetNewAssetIconOverride() const override;
};
