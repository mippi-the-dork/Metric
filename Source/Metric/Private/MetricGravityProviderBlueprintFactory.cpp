#include "MetricGravityProviderBlueprintFactory.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
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
    // normal Blueprint editor and seed the one override users are expected to implement.
    Blueprint->bForceFullEditor = true;

    const FName SampleFunctionName = GET_FUNCTION_NAME_CHECKED(UMetricGravityProvider, SampleGravityForMetric);
    if (UFunction* SampleFunction = UMetricGravityProvider::StaticClass()->FindFunctionByName(SampleFunctionName))
    {
        UEdGraph* OverrideGraph = FBlueprintEditorUtils::CreateNewGraph(
            Blueprint,
            SampleFunctionName,
            UEdGraph::StaticClass(),
            UEdGraphSchema_K2::StaticClass());

        if (OverrideGraph)
        {
            FBlueprintEditorUtils::AddFunctionGraph(Blueprint, OverrideGraph, false, SampleFunction);
        }
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint);
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
