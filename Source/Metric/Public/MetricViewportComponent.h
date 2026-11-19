#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "MetricTypes.h"
#include "MetricViewportComponent.generated.h"

UCLASS(ClassGroup = Rendering, meta = (BlueprintSpawnableComponent = false))
class METRIC_API UMetricViewportComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UMetricViewportComponent();

    void SetMetricState(const FMetricLiveState& NewState);
    void SetAllowQueryInspection(bool bAllow);
    void SetQueryInspectionOverride(bool bUseOverride, bool bEnabled, EMetricInspectionTarget Target = EMetricInspectionTarget::Auto);

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual void SendRenderDynamicData_Concurrent() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
    FMetricLiveState State;
    bool bAllowQueryInspection = true;
    bool bUseQueryInspectionOverride = false;
    bool bQueryInspectionOverrideEnabled = false;
    EMetricInspectionTarget QueryInspectionOverrideTarget = EMetricInspectionTarget::Auto;
};
