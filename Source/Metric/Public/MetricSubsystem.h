#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "MetricTypes.h"
#include "MetricSubsystem.generated.h"

class AMetricRenderHostActor;
class UWorld;

UCLASS()
class METRIC_API UMetricSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    bool IsMetricActive() const { return bMetricActive; }
    void ToggleMetric();
    void SetMetricActive(bool bActive);
    void CycleNext();

    void RefreshCycleEntries();
    const FMetricCycleEntry* GetActiveEntry() const;
    int32 GetActiveIndex() const { return ActiveIndex; }
    int32 GetEntryCount() const { return CycleEntries.Num(); }

    void UpdateLiveState(UWorld* World, const FMetricLiveState& NewState);
    const FMetricLiveState& GetLiveState() const { return LiveState; }

    void ClearRenderHost();

private:
    void HandlePreLoadMap(const FString& MapName);
    void HandleWorldBeginTearDown(UWorld* World);
    void EnsureStarterPresetAssets();
    class UMetricPreset* CreateStarterPresetAsset(const FString& PackagePath, const FText& DisplayName, const FMetricHullSettings& HullSettings);
    void EnsureRenderHost(UWorld* World);
    void ApplyEntryToState(FMetricLiveState& InOutState) const;
    void EvaluateClearance(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateTrajectory(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateCameraProbe(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateCameraSightline(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateGroundValidation(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateFitAnalysis(UWorld* World, FMetricLiveState& InOutState) const;
    void NotifyStateChanged();

    bool bMetricActive = false;
    int32 ActiveIndex = 0;
    TArray<FMetricCycleEntry> CycleEntries;
    FMetricLiveState LiveState;

    // The render host is owned by its UWorld/ULevel. Metric must not strongly
    // reference a world-owned Actor from this long-lived editor subsystem,
    // otherwise an old editor World can be kept alive across map changes.
    TWeakObjectPtr<AMetricRenderHostActor> RenderHost;

    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle WorldBeginTearDownHandle;
};
