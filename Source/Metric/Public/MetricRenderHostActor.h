#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetricTypes.h"
#include "MetricRenderHostActor.generated.h"

class UMetricViewportComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UMaterialInterface;

UCLASS(Transient, NotPlaceable)
class METRIC_API AMetricRenderHostActor : public AActor
{
    GENERATED_BODY()

public:
    AMetricRenderHostActor();

    void ApplyMetricState(const FMetricLiveState& State);
    void ApplyPinnedMetricState(const FMetricLiveState* State);
    void SetMetricVisible(bool bVisible);

    virtual bool IsEditorOnly() const override { return true; }
    virtual bool IsSelectable() const override { return false; }
    virtual bool IsListedInSceneOutliner() const override { return false; }
    virtual bool IsLevelBoundsRelevant() const override { return false; }

private:
    UPROPERTY(Transient)
    TObjectPtr<UMetricViewportComponent> MetricComponent;

    UPROPERTY(Transient)
    TObjectPtr<UMetricViewportComponent> PinnedMetricComponent;

    UPROPERTY(Transient)
    TObjectPtr<UStaticMeshComponent> PreviewMeshComponent;

    UPROPERTY(Transient)
    TObjectPtr<UTextRenderComponent> LabelComponent;

    UPROPERTY(Transient)
    TObjectPtr<UTextRenderComponent> LegendComponent;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> UnlitTextMaterial;

    void EnsureUnlitTextMaterial();
};
