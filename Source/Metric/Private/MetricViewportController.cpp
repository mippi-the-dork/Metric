#include "MetricViewportController.h"

#include "MetricProjectSettings.h"
#include "MetricSubsystem.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "UnrealClient.h"
#include "Engine/World.h"

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
