#include "MetricViewportController.h"

#include "SMetricViewportOverlay.h"
#include "ILevelEditor.h"
#include "SLevelViewport.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "Framework/Application/SlateApplication.h"

#include "MetricProjectSettings.h"
#include "MetricEditorSettings.h"
#include "MetricSubsystem.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "UnrealClient.h"
#include "Engine/World.h"


namespace
{
    FMetricViewportController* GMetricViewportControllerInstance = nullptr;
}

FMetricViewportController::FMetricViewportController()
{
    GMetricViewportControllerInstance = this;
}


FMetricViewportController::~FMetricViewportController()
{
    RemoveAllOverlays();
    if (GMetricViewportControllerInstance == this)
    {
        GMetricViewportControllerInstance = nullptr;
    }
}

TArray<TSharedPtr<SLevelViewport>> FMetricViewportController::GetViewportsInVisualOrder(const TArray<TSharedPtr<SLevelViewport>>& Viewports)
{
    TArray<TSharedPtr<SLevelViewport>> Ordered;
    for (const TSharedPtr<SLevelViewport>& Viewport : Viewports)
    {
        if (Viewport.IsValid())
        {
            const FVector2D Size = Viewport->GetCachedGeometry().GetLocalSize();
            if (Size.X > 8.0f && Size.Y > 8.0f)
            {
                Ordered.Add(Viewport);
            }
        }
    }

    Ordered.Sort([](const TSharedPtr<SLevelViewport>& A, const TSharedPtr<SLevelViewport>& B)
    {
        const FVector2D PosA = A->GetCachedGeometry().GetAbsolutePosition();
        const FVector2D PosB = B->GetCachedGeometry().GetAbsolutePosition();
        constexpr float RowTolerance = 24.0f;
        if (!FMath::IsNearlyEqual(PosA.Y, PosB.Y, RowTolerance))
        {
            return PosA.Y < PosB.Y;
        }
        return PosA.X < PosB.X;
    });
    return Ordered;
}

void FMetricViewportController::NotifyViewportInteractionAtScreenPosition(const FVector2D& ScreenPosition)
{
    FMetricViewportController* Controller = GMetricViewportControllerInstance;
    if (!Controller)
    {
        return;
    }

    FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
    if (!LevelEditorModule)
    {
        return;
    }

    TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule->GetFirstLevelEditor();
    if (!LevelEditor.IsValid())
    {
        return;
    }

    const TArray<TSharedPtr<SLevelViewport>>& Viewports = LevelEditor->GetViewports();
    for (const TSharedPtr<SLevelViewport>& Viewport : Viewports)
    {
        if (!Viewport.IsValid())
        {
            continue;
        }

        const FGeometry& Geometry = Viewport->GetCachedGeometry();
        const FVector2D LocalPosition = Geometry.AbsoluteToLocal(ScreenPosition);
        const FVector2D LocalSize = Geometry.GetLocalSize();
        if (LocalPosition.X >= 0.0f && LocalPosition.Y >= 0.0f
            && LocalPosition.X < LocalSize.X && LocalPosition.Y < LocalSize.Y)
        {
            Controller->LastInteractedViewport = Viewport;
            return;
        }
    }
}


bool FMetricViewportController::IsViewportSelectedBySettings(int32 ViewportIndex) const
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    if (!Settings)
    {
        return ViewportIndex == 0;
    }

    switch (ViewportIndex)
    {
    case 0: return Settings->bShowHUDInViewport1;
    case 1: return Settings->bShowHUDInViewport2;
    case 2: return Settings->bShowHUDInViewport3;
    case 3: return Settings->bShowHUDInViewport4;
    default: return false;
    }
}

void FMetricViewportController::AddOverlayToViewport(const TSharedPtr<SLevelViewport>& Viewport)
{
    if (!Viewport.IsValid())
    {
        return;
    }

    for (const FOverlayEntry& Entry : OverlayEntries)
    {
        if (Entry.Viewport.Pin() == Viewport)
        {
            return;
        }
    }

    FOverlayEntry& Entry = OverlayEntries.AddDefaulted_GetRef();
    Entry.Viewport = Viewport;
    Entry.Widget = SNew(SMetricViewportOverlay);
    Viewport->AddOverlayWidget(Entry.Widget.ToSharedRef(), 50);
}

void FMetricViewportController::RemoveAllOverlays()
{
    for (FOverlayEntry& Entry : OverlayEntries)
    {
        TSharedPtr<SLevelViewport> Viewport = Entry.Viewport.Pin();
        if (Viewport.IsValid() && Entry.Widget.IsValid())
        {
            Viewport->RemoveOverlayWidget(Entry.Widget.ToSharedRef());
        }
    }
    OverlayEntries.Reset();
}

void FMetricViewportController::SyncOverlays()
{
    FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
    if (!LevelEditorModule)
    {
        RemoveAllOverlays();
        return;
    }

    TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule->GetFirstLevelEditor();
    if (!LevelEditor.IsValid())
    {
        RemoveAllOverlays();
        return;
    }

    const TArray<TSharedPtr<SLevelViewport>>& Viewports = LevelEditor->GetViewports();
    const TArray<TSharedPtr<SLevelViewport>> OrderedViewports = GetViewportsInVisualOrder(Viewports);

    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    const EMetricHUDAnchor Anchor = Settings ? Settings->HUDAnchor : EMetricHUDAnchor::TopLeft;
    const EMetricHUDViewportTargeting Targeting = Settings ? Settings->HUDViewportTargeting : EMetricHUDViewportTargeting::FollowLastInteracted;
    const EMetricHUDAnchor LegendAnchor = Settings ? Settings->LegendAnchor : EMetricHUDAnchor::BottomLeft;

    const uint8 AnchorValue = static_cast<uint8>(Anchor);
    const uint8 TargetingValue = static_cast<uint8>(Targeting);
    const uint8 LegendAnchorValue = static_cast<uint8>(LegendAnchor);
    if (AnchorValue != LastHUDAnchorValue || TargetingValue != LastHUDTargetingValue || LegendAnchorValue != LastLegendAnchorValue)
    {
        RemoveAllOverlays();
        LastHUDAnchorValue = AnchorValue;
        LastHUDTargetingValue = TargetingValue;
        LastLegendAnchorValue = LegendAnchorValue;
    }

    // Keep one overlay attached to every live Level Editor viewport. Targeting
    // only changes visibility. This is substantially more reliable than adding
    // and removing Slate overlay widgets while layouts/focus are changing.
    for (const TSharedPtr<SLevelViewport>& Viewport : OrderedViewports)
    {
        AddOverlayToViewport(Viewport);
    }

    for (int32 EntryIndex = OverlayEntries.Num() - 1; EntryIndex >= 0; --EntryIndex)
    {
        TSharedPtr<SLevelViewport> ExistingViewport = OverlayEntries[EntryIndex].Viewport.Pin();
        if (!ExistingViewport.IsValid() || !OrderedViewports.Contains(ExistingViewport))
        {
            if (ExistingViewport.IsValid() && OverlayEntries[EntryIndex].Widget.IsValid())
            {
                ExistingViewport->RemoveOverlayWidget(OverlayEntries[EntryIndex].Widget.ToSharedRef());
            }
            OverlayEntries.RemoveAt(EntryIndex);
            continue;
        }

        const int32 VisualIndex = OrderedViewports.IndexOfByKey(ExistingViewport);
        bool bShouldShow = true;
        if (Targeting == EMetricHUDViewportTargeting::FollowLastInteracted)
        {
            bShouldShow = LastInteractedViewport.Pin() == ExistingViewport;
        }

        if (OverlayEntries[EntryIndex].Widget.IsValid())
        {
            OverlayEntries[EntryIndex].Widget->SetViewportNumber(VisualIndex != INDEX_NONE ? VisualIndex + 1 : INDEX_NONE);
            OverlayEntries[EntryIndex].Widget->SetTargetEnabled(bShouldShow);
        }
    }
}

TStatId FMetricViewportController::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(FMetricViewportController, STATGROUP_Tickables);
}

FLevelEditorViewportClient* FMetricViewportController::GetActivePerspectiveLevelViewport()
{
    if (!GEditor)
    {
        return nullptr;
    }

    FViewport* ActiveViewport = GEditor->GetActiveViewport();
    if (!ActiveViewport)
    {
        return nullptr;
    }

    FViewportClient* ActiveClient = ActiveViewport->GetClient();
    if (!ActiveClient)
    {
        return nullptr;
    }

    for (FLevelEditorViewportClient* LevelClient : GEditor->GetLevelViewportClients())
    {
        if (LevelClient && LevelClient == ActiveClient && LevelClient->IsPerspective())
        {
            return LevelClient;
        }
    }

    return nullptr;
}

bool FMetricViewportController::IsLevelViewportFocused()
{
    FLevelEditorViewportClient* Client = GetActivePerspectiveLevelViewport();
    return Client && Client->Viewport && Client->Viewport->HasFocus();
}

bool FMetricViewportController::BuildCursorRay(
    FLevelEditorViewportClient& ViewportClient,
    FVector& OutOrigin,
    FVector& OutDirection)
{
    FViewport* Viewport = ViewportClient.Viewport;
    if (!Viewport)
    {
        return false;
    }

    const FIntPoint Size = Viewport->GetSizeXY();
    if (Size.X <= 1 || Size.Y <= 1)
    {
        return false;
    }

    const int32 MouseX = Viewport->GetMouseX();
    const int32 MouseY = Viewport->GetMouseY();
    if (MouseX < 0 || MouseY < 0 || MouseX >= Size.X || MouseY >= Size.Y)
    {
        return false;
    }

    const float NdcX = ((static_cast<float>(MouseX) + 0.5f) / static_cast<float>(Size.X)) * 2.0f - 1.0f;
    const float NdcY = 1.0f - ((static_cast<float>(MouseY) + 0.5f) / static_cast<float>(Size.Y)) * 2.0f;

    const float Aspect = static_cast<float>(Size.X) / static_cast<float>(Size.Y);
    const float TanHalfHorizontalFov = FMath::Tan(FMath::DegreesToRadians(ViewportClient.ViewFOV * 0.5f));
    const float TanHalfVerticalFov = TanHalfHorizontalFov / FMath::Max(Aspect, KINDA_SMALL_NUMBER);

    const FRotationMatrix CameraBasis(ViewportClient.GetViewRotation());
    const FVector Forward = CameraBasis.GetScaledAxis(EAxis::X);
    const FVector Right = CameraBasis.GetScaledAxis(EAxis::Y);
    const FVector Up = CameraBasis.GetScaledAxis(EAxis::Z);

    OutOrigin = ViewportClient.GetViewLocation();
    OutDirection = (Forward + Right * (NdcX * TanHalfHorizontalFov) + Up * (NdcY * TanHalfVerticalFov)).GetSafeNormal();
    return !OutDirection.IsNearlyZero();
}

void FMetricViewportController::Tick(float DeltaTime)
{
    SyncOverlays();
    if (!GEditor)
    {
        return;
    }

    UMetricSubsystem* MetricSubsystem = GEditor->GetEditorSubsystem<UMetricSubsystem>();
    if (!MetricSubsystem || !MetricSubsystem->IsMetricActive())
    {
        return;
    }

    FLevelEditorViewportClient* ViewportClient = GetActivePerspectiveLevelViewport();
    if (!ViewportClient || !ViewportClient->Viewport)
    {
        return;
    }

    UWorld* World = ViewportClient->GetWorld();
    if (!World || World->WorldType != EWorldType::Editor)
    {
        return;
    }

    const UMetricEditorSettings* EditorSettings = GetDefault<UMetricEditorSettings>();
    if (EditorSettings && !EditorSettings->bShowLiveVisualizer)
    {
        // Truly pause the cursor-driven analyzer. No cursor ray, placement trace,
        // or live Metric sweeps/sampling are performed while Live Metric is Off.
        // The selected benchmark, if any, is still re-evaluated live.
        MetricSubsystem->UpdateWithLiveAnalysisDisabled(World, ViewportClient->GetViewLocation());
        ViewportClient->Invalidate();
        return;
    }

    FVector RayOrigin;
    FVector RayDirection;
    if (!BuildCursorRay(*ViewportClient, RayOrigin, RayDirection))
    {
        return;
    }

    const UMetricProjectSettings* ProjectSettings = GetDefault<UMetricProjectSettings>();
    const float TraceDistance = ProjectSettings ? ProjectSettings->PlacementTraceDistance : 1000000.0f;
    const ECollisionChannel TraceChannel = ProjectSettings ? ProjectSettings->PlacementTraceChannel.GetValue() : ECC_Visibility;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricPlacementTrace), true);
    QueryParams.bTraceComplex = true;

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(
        Hit,
        RayOrigin,
        RayOrigin + RayDirection * TraceDistance,
        TraceChannel,
        QueryParams);

    FMetricLiveState State = MetricSubsystem->GetLiveState();
    State.bHasHit = bHit;
    State.ViewerLocation = ViewportClient->GetViewLocation();

    if (bHit)
    {
        State.Location = Hit.ImpactPoint;
        State.SurfaceNormal = Hit.ImpactNormal.GetSafeNormal();

        FVector GroundForward = ViewportClient->GetViewRotation().Vector();
        GroundForward.Z = 0.0f;
        GroundForward.Normalize();
        if (GroundForward.IsNearlyZero())
        {
            GroundForward = FVector::ForwardVector;
        }
        State.Forward = GroundForward;
    }

    MetricSubsystem->UpdateLiveState(World, State);
    ViewportClient->Invalidate();
}
