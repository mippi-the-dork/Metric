#pragma once

#include "CoreMinimal.h"
#include "TickableEditorObject.h"

class FLevelEditorViewportClient;
class SLevelViewport;
class SMetricViewportOverlay;
class UMetricSubsystem;

class FMetricViewportController : public FTickableEditorObject
{
public:
    FMetricViewportController();
    virtual ~FMetricViewportController() override;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }

    static FLevelEditorViewportClient* GetActivePerspectiveLevelViewport();
    static bool IsLevelViewportFocused();
    static void NotifyViewportInteractionAtScreenPosition(const FVector2D& ScreenPosition);

private:
    struct FOverlayEntry
    {
        TWeakPtr<SLevelViewport> Viewport;
        TSharedPtr<SMetricViewportOverlay> Widget;
    };

    static bool BuildCursorRay(FLevelEditorViewportClient& ViewportClient, FVector& OutOrigin, FVector& OutDirection);
    void SyncOverlays();
    static TArray<TSharedPtr<SLevelViewport>> GetViewportsInVisualOrder(const TArray<TSharedPtr<SLevelViewport>>& Viewports);
    bool IsViewportSelectedBySettings(int32 ViewportIndex) const;
    void AddOverlayToViewport(const TSharedPtr<SLevelViewport>& Viewport);
    void RemoveAllOverlays();

    TArray<FOverlayEntry> OverlayEntries;
    TWeakPtr<SLevelViewport> LastInteractedViewport;
    uint8 LastHUDAnchorValue = 255;
    uint8 LastHUDTargetingValue = 255;
    uint8 LastLegendAnchorValue = 255;
};
