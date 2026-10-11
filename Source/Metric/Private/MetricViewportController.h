#pragma once

#include "CoreMinimal.h"
#include "TickableEditorObject.h"

class FLevelEditorViewportClient;
class UMetricSubsystem;

class FMetricViewportController : public FTickableEditorObject
{
public:
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }

    static FLevelEditorViewportClient* GetActivePerspectiveLevelViewport();
    static bool IsLevelViewportFocused();

private:
    static bool BuildCursorRay(FLevelEditorViewportClient& ViewportClient, FVector& OutOrigin, FVector& OutDirection);
};
