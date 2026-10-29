#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "MetricProjectSettings.generated.h"

class UMetricPreset;
class AActor;
class UActorComponent;

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Metric"))
class METRIC_API UMetricProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMetricProjectSettings();

    UPROPERTY(Config, EditAnywhere, Category = "Presets", meta = (ToolTip = "Create Metric's editable starter Data Assets in /Game/Metric/Presets when they are missing."))
    bool bCreateStarterPresetAssetsIfMissing = true;

    UPROPERTY(Config, EditAnywhere, Category = "Presets", meta = (ToolTip = "Keep the code-only Human Reference available when no registered Metric presets exist."))
    bool bIncludeBuiltInHumanReference = true;

    // Internal one-time bootstrap marker. Once the starter assets have been
    // created and registered, Metric will not recreate or re-register them if
    // the user later removes them intentionally.
    UPROPERTY(Config)
    bool bStarterPresetsInitialized = false;

    UPROPERTY(Config, EditAnywhere, Category = "Presets")
    TArray<TSoftObjectPtr<UMetricPreset>> RegisteredPresets;

    UPROPERTY(Config, EditAnywhere, Category = "Queries")
    TEnumAsByte<ECollisionChannel> PlacementTraceChannel = ECC_Visibility;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (ClampMin = "1000.0", Units = "cm"))
    float PlacementTraceDistance = 1000000.0f;

    // Shared project exclusion policy. Instance-specific exclusions live in
    // Metric User settings because level-object references are user/editor
    // workflow state rather than portable project defaults.
    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions")
    TArray<TSoftClassPtr<AActor>> ExcludedActorClasses;

    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions")
    TArray<FName> ExcludedActorTags;

    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions")
    TArray<TSoftClassPtr<UActorComponent>> ExcludedComponentClasses;

    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions")
    TArray<FName> ExcludedComponentTags;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FText GetSectionText() const override;
};
