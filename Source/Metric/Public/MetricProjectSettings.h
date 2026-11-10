#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/EngineTypes.h"
#include "MetricProjectSettings.generated.h"

class UMetricProfile;
class UMetricGravityProvider;
class AActor;
class UActorComponent;

UCLASS(Config = Editor, DefaultConfig, meta = (DisplayName = "Metric"))
class METRIC_API UMetricProjectSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMetricProjectSettings();

    UPROPERTY(Config, EditAnywhere, Category = "Profiles", meta = (ToolTip = "Create Metric's editable starter Data Assets in /Game/Metric/Profiles when they are missing."))
    bool bCreateStarterProfileAssetsIfMissing = true;

    UPROPERTY(Config, EditAnywhere, Category = "Profiles", meta = (ToolTip = "Keep the code-only Human Reference available when no registered Metric Profiles exist."))
    bool bIncludeBuiltInHumanReference = true;

    // Internal one-time bootstrap marker. Once the starter assets have been
    // created and registered, Metric will not recreate or re-register them if
    // the user later removes them intentionally.
    UPROPERTY(Config)
    bool bStarterProfilesInitialized = false;

    UPROPERTY(Config, EditAnywhere, Category = "Profiles", meta = (ToolTip = "Metric Profiles available to the Live Metric profile cycle. Add project-authored profiles here to make them available from the viewport workflow."))
    TArray<TSoftObjectPtr<UMetricProfile>> RegisteredProfiles;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (ToolTip = "Collision channel used by the cursor placement trace that finds the surface where Live Metric is evaluated."))
    TEnumAsByte<ECollisionChannel> PlacementTraceChannel = ECC_Visibility;

    UPROPERTY(Config, EditAnywhere, Category = "Queries", meta = (ClampMin = "1000.0", Units = "cm", ToolTip = "Maximum distance of the cursor placement trace used to find a Metric evaluation point."))
    float PlacementTraceDistance = 1000000.0f;

    UPROPERTY(Config, EditAnywhere, Category = "Analysis Frame", meta = (DisplayName = "Gravity Provider", ToolTip = "Optional Blueprint or C++ gravity adapter used by Gravity / Provider analysis frames. Derive from Metric Gravity Provider and implement the single Sample Gravity for Metric function. Metric and the gravity system remain independent of one another."))
    TSoftClassPtr<UMetricGravityProvider> GravityProviderClass;

    // Shared persistent project exclusion policy. Explicit Actor/Component
    // instance exclusions are intentionally session + world scoped and are not
    // serialized into either Project Settings or Metric User settings.
    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions", meta = (ToolTip = "Actor classes ignored by Metric environment queries across the project. All primitive components owned by matching Actors are excluded."))
    TArray<TSoftClassPtr<AActor>> ExcludedActorClasses;

    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions", meta = (ToolTip = "Actors with any of these Actor Tags are ignored by Metric environment queries across the project."))
    TArray<FName> ExcludedActorTags;

    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions", meta = (ToolTip = "Primitive component classes ignored by Metric environment queries across the project. Other components on the same Actor remain eligible."))
    TArray<TSoftClassPtr<UActorComponent>> ExcludedComponentClasses;

    UPROPERTY(Config, EditAnywhere, Category = "Geometry Exclusions", meta = (ToolTip = "Primitive components with any of these Component Tags are ignored by Metric environment queries across the project."))
    TArray<FName> ExcludedComponentTags;

    virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
    virtual FText GetSectionText() const override;
};
