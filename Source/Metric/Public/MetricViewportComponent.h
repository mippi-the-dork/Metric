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

    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual void SendRenderDynamicData_Concurrent() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
    FMetricLiveState State;
    bool bAllowQueryInspection = true;
};
