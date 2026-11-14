#include "MetricGravityProviderBlueprintFactory.h"

#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "MetricGravityProvider.h"
#include "MetricGravityProviderBlueprint.h"

#define LOCTEXT_NAMESPACE "MetricGravityProviderBlueprintFactory"

UMetricGravityProviderBlueprintFactory::UMetricGravityProviderBlueprintFactory()
{
    SupportedClass = UMetricGravityProviderBlueprint::StaticClass();
    bCreateNew = true;
    bEditAfterNew = true;
}

UObject* UMetricGravityProviderBlueprintFactory::FactoryCreateNew(
    UClass* Class,
    UObject* InParent,
    FName Name,
    EObjectFlags Flags,
    UObject* Context,
    FFeedbackContext* Warn)
{
    (void)Class;
    (void)Flags;
    (void)Context;
    (void)Warn;

    UMetricGravityProviderBlueprint* Blueprint = Cast<UMetricGravityProviderBlueprint>(FKismetEditorUtilities::CreateBlueprint(
        UMetricGravityProvider::StaticClass(),
        InParent,
        Name,
        BPTYPE_Normal,
        UMetricGravityProviderBlueprint::StaticClass(),
        UBlueprintGeneratedClass::StaticClass(),
        TEXT("MetricGravityProviderBlueprintFactory")));

    if (!Blueprint)
    {
        return nullptr;
    }

    // Gravity providers are authoring assets, not data-only Blueprints. Force the
    // normal Blueprint editor. The inherited BlueprintNativeEvent is intentionally
    // left to Unreal's native Overrides workflow rather than manufacturing an override
    // graph here; doing so can produce a conflicting signature for native functions.
    Blueprint->bForceFullEditor = true;

    return Blueprint;
}

FText UMetricGravityProviderBlueprintFactory::GetDisplayName() const
{
    return LOCTEXT("DisplayName", "Metric Gravity Provider");
}

FText UMetricGravityProviderBlueprintFactory::GetToolTip() const
{
    return LOCTEXT(
        "ToolTip",
        "Create a Metric Gravity Provider Blueprint already parented to Metric Gravity Provider. Override Sample Gravity for Metric to connect any gravity solution.");
}

FString UMetricGravityProviderBlueprintFactory::GetDefaultNewAssetName() const
{
    return TEXT("BP_MetricGravityProvider");
}

FName UMetricGravityProviderBlueprintFactory::GetNewAssetIconOverride() const
{
    return TEXT("ClassIcon.MetricGravityProviderBlueprint");
}

#undef LOCTEXT_NAMESPACE
