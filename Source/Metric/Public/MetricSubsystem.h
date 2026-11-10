#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "MetricTypes.h"
#include "MetricSubsystem.generated.h"

class AMetricRenderHostActor;
class UWorld;
class AActor;

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
    void PinCurrentBenchmark();
    void ClearPinnedBenchmarks();
    bool ArePinnedBenchmarksPersistent() const;
    int32 GetPinnedBenchmarkCount() const { return PinnedBenchmarks.Num(); }
    int32 GetSelectedPinnedBenchmarkIndex() const { return SelectedPinnedBenchmarkIndex; }
    const FMetricPinnedBenchmark* GetPinnedBenchmark(int32 Index) const;
    const FMetricLiveState* GetSelectedPinnedEvaluatedState() const { return bHasSelectedPinnedEvaluatedState ? &SelectedPinnedEvaluatedState : nullptr; }
    void SelectPinnedBenchmark(int32 Index);
    void CyclePinnedBenchmarkSelection();
    void DeletePinnedBenchmark(int32 Index);
    void RenamePinnedBenchmark(int32 Index, const FText& NewLabel);
    void ReplacePinnedBenchmarkWithLive(int32 Index);
    void FocusPinnedBenchmark(int32 Index);
    void SetPinnedBenchmarkVisibility(int32 Index, const FMetricLayerVisibility& Visibility);

    void RefreshCycleEntries();
    const FMetricCycleEntry* GetActiveEntry() const;
    int32 GetActiveIndex() const { return ActiveIndex; }
    int32 GetEntryCount() const { return CycleEntries.Num(); }

    void UpdateLiveState(UWorld* World, const FMetricLiveState& NewState);
    void BeginLiveAnalysis(UWorld* World, const FMetricLiveState& NewState);
    bool ProcessLiveAnalysisBudget(UWorld* World, double TimeBudgetMs);
    bool HasPendingLiveAnalysis() const { return bLiveAnalysisPending; }
    void CancelPendingLiveAnalysis();
    void UpdateWithLiveAnalysisDisabled(UWorld* World, const FVector& ViewerLocation);
    const FMetricLiveState& GetLiveState() const { return LiveState; }
    FString RunPerformanceProfileCurrentLevel(int32 Iterations = 120);

    // Shared authored-validation entry point used by Metric Actor. It evaluates
    // the same analysis backend as Live Metric without requiring Metric to be active.
    void EvaluateProfileAtLocation(
        UWorld* World,
        const class UMetricProfile* Preset,
        const FVector& Location,
        const FVector& Forward,
        const FVector& SurfaceNormal,
        const FVector& AnalysisUp,
        FMetricLiveState& OutState,
        EMetricAnalysisFrameMode AnalysisFrameMode = EMetricAnalysisFrameMode::World,
        EMetricFrameSamplingPolicy FrameSampling = EMetricFrameSamplingPolicy::FixedAtOrigin,
        float AnalysisGravityMagnitude = 0.0f,
        AActor* ContextActor = nullptr) const;

#if WITH_DEV_AUTOMATION_TESTS
    void EvaluateTrajectoryForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateTrajectory(World, InOutState); }
    void EvaluateClearanceForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateClearance(World, InOutState); }
    void EvaluateCameraProbeForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateCameraProbe(World, InOutState); }
    void EvaluateCameraSightlineForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateCameraSightline(World, InOutState); }
    void EvaluateGroundValidationForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateGroundValidation(World, InOutState); }
    void EvaluateFitAnalysisForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateFitAnalysis(World, InOutState); }
    void EvaluateThresholdAnalysisForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateThresholdAnalysis(World, InOutState); }
    void EvaluateLedgeAnalysisForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateLedgeAnalysis(World, InOutState); }
    void EvaluateStoppingDistanceForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateStoppingDistance(World, InOutState); }
    void EvaluateJumpRunwayForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateJumpRunway(World, InOutState); }
    void EvaluateAlternateStanceForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateAlternateStance(World, InOutState); }
    void EvaluateMantleVaultForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateMantleVault(World, InOutState); }
    void EvaluateRecoverySpaceForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateRecoverySpace(World, InOutState); }
    void EvaluateInteractionReachForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateInteractionReach(World, InOutState); }
    void EvaluateShoulderClearanceForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateShoulderClearance(World, InOutState); }
    void EvaluateCoverExposureForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateCoverExposure(World, InOutState); }
    void EvaluateCrossfireForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateCrossfire(World, InOutState); }
    void EvaluateSensorFieldForAutomation(UWorld* World, FMetricLiveState& InOutState) const { EvaluateSensorField(World, InOutState); }
    void BuildQueryEvidenceForAutomation(FMetricLiveState& InOutState) const { BuildQueryEvidence(InOutState); }
#endif

    void ClearRenderHost();

private:
    void HandlePreLoadMap(const FString& MapName);
    void HandleWorldBeginTearDown(UWorld* World);
    FString GetPersistableMapPackageName(UWorld* World) const;
    void SavePinnedBenchmarksForWorld(UWorld* World);
    void LoadPinnedBenchmarksForWorld(UWorld* World);
    void EnsurePinnedBenchmarkMap(UWorld* World);
    void EnsureStarterProfileAssets();
    class UMetricProfile* CreateStarterProfileAsset(const FString& PackagePath, const FText& DisplayName, const FMetricHullSettings& HullSettings);
    void EnsureRenderHost(UWorld* World);
    void ApplyEntryToState(FMetricLiveState& InOutState) const;
    void EvaluateClearance(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateOverheadClearance(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateSensorField(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateTrajectory(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateCameraProbe(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateCameraSightline(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateGroundValidation(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateFitAnalysis(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateThresholdAnalysis(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateTurnClearance(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateLedgeAnalysis(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateStoppingDistance(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateCoverExposure(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateJumpRunway(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateInteractionReach(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateShoulderClearance(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateCombatDensity(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateAlternateStance(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateMantleVault(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateRecoverySpace(UWorld* World, FMetricLiveState& InOutState) const;
    void EvaluateCrossfire(UWorld* World, FMetricLiveState& InOutState) const;
    void BuildQueryEvidence(FMetricLiveState& InOutState) const;
    void PublishLiveState(UWorld* World);
    bool BuildSelectedPinnedVisualState(UWorld* World, FMetricLiveState& OutState) const;
    void RefreshSelectedPinnedVisualization(UWorld* World);
    void NotifyStateChanged();

    bool bMetricActive = false;
    int32 ActiveIndex = 0;
    TArray<FMetricCycleEntry> CycleEntries;
    FMetricLiveState LiveState;
    TArray<FMetricPinnedBenchmark> PinnedBenchmarks;
    FString ActivePinnedBenchmarkMap;
    int32 SelectedPinnedBenchmarkIndex = INDEX_NONE;
    FMetricLiveState SelectedPinnedEvaluatedState;
    bool bHasSelectedPinnedEvaluatedState = false;

    FMetricLiveState PendingLiveState;
    TWeakObjectPtr<UWorld> PendingLiveWorld;
    int32 PendingLiveJobIndex = 0;
    bool bLiveAnalysisPending = false;

    // The render host is owned by its UWorld/ULevel. Metric must not strongly
    // reference a world-owned Actor from this long-lived editor subsystem,
    // otherwise an old editor World can be kept alive across map changes.
    TWeakObjectPtr<AMetricRenderHostActor> RenderHost;

    FDelegateHandle PreLoadMapHandle;
    FDelegateHandle WorldBeginTearDownHandle;
};
