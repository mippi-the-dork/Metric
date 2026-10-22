#include "MetricSubsystem.h"

#include "MetricEditorSettings.h"
#include "MetricPreset.h"
#include "MetricProjectSettings.h"
#include "MetricRenderHostActor.h"
#include "CollisionShape.h"
#include "Containers/Ticker.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/World.h"
#include "Settings/LevelEditorViewportSettings.h"

#define LOCTEXT_NAMESPACE "MetricSubsystem"

namespace MetricBenchmark
{
    static void ApplyVisualVisibility(FMetricLiveState& State, const FMetricLayerVisibility& Visibility)
    {
        State.bShowHull = State.bShowHull && Visibility.bHullVisual;
        State.bShowForward = State.bShowForward && Visibility.bHullVisual;
        State.ClearanceSettings.bShowClearanceProbe = State.ClearanceSettings.bShowClearanceProbe && Visibility.bClearanceVisual;
        State.GroundValidationSettings.bShowGroundValidation = State.GroundValidationSettings.bShowGroundValidation && Visibility.bGroundVisual;
        State.FitAnalysisSettings.bShowFitAnalysis = State.FitAnalysisSettings.bShowFitAnalysis && Visibility.bFitVisual;
        State.ThresholdAnalysisSettings.bShowThresholdAnalysis = State.ThresholdAnalysisSettings.bShowThresholdAnalysis && Visibility.bThresholdVisual;
        State.Sensors.bShowSensorHeights = State.Sensors.bShowSensorHeights && Visibility.bSensorsVisual;
        State.SensorFieldSettings.bShowSensorField = State.SensorFieldSettings.bShowSensorField && Visibility.bSensorFieldVisual;
        State.Ranges.bShowEngagementRanges = State.Ranges.bShowEngagementRanges && Visibility.bRangesVisual;
        State.Locomotion.bShowLocomotionBands = State.Locomotion.bShowLocomotionBands && Visibility.bLocomotionVisual;
        State.TrajectorySettings.bShowTrajectory = State.TrajectorySettings.bShowTrajectory && Visibility.bTrajectoryVisual;
        State.CameraProbeSettings.bShowCameraProbe = State.CameraProbeSettings.bShowCameraProbe && Visibility.bCameraVisual;
        State.CameraSightlineSettings.bShowCameraSightline = State.CameraSightlineSettings.bShowCameraSightline && Visibility.bSightlineVisual;
    }
    static FString BuildCapturedResults(const FMetricLiveState& State)
    {
        TArray<FString> Lines;

        if (State.bShowHull)
        {
            Lines.Add(FString::Printf(TEXT("HULL        %.0f W x %.0f H x %.0f L"), State.Hull.Width, State.Hull.Height, State.Hull.Length));
        }

        if (State.Clearance.bVisible)
        {
            if (State.Clearance.bBlocked)
            {
                Lines.Add(FString::Printf(TEXT("CLEARANCE   FAIL   %.0fu / %.0fu"), State.Clearance.ClearDistance, State.Clearance.ProbeDistance));
            }
            else if (State.Clearance.bNearThreshold)
            {
                Lines.Add(FString::Printf(TEXT("CLEARANCE   TIGHT  %.0fu | obstacle %.0fu"), State.Clearance.ProbeDistance, State.Clearance.ObstacleDistance));
            }
            else
            {
                Lines.Add(FString::Printf(TEXT("CLEARANCE   PASS   %.0fu+"), State.Clearance.ProbeDistance));
            }
        }

        if (State.GroundValidation.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("GROUND      %s   %d/%d | slope %.0f deg | up %.0fu down %.0fu"),
                State.GroundValidation.bPass ? TEXT("PASS") : TEXT("FAIL"),
                State.GroundValidation.TotalSamples - State.GroundValidation.FailedSamples,
                State.GroundValidation.TotalSamples,
                State.GroundValidation.MaxObservedSlope,
                State.GroundValidation.MaxObservedStepUp,
                State.GroundValidation.MaxObservedStepDown));
        }

        if (State.FitAnalysis.bVisible)
        {
            const TCHAR* FitStatus = State.FitAnalysis.bPass ? (State.FitAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL");
            Lines.Add(FString::Printf(TEXT("FIT         %s   W %.0fu H %.0fu | deficit W %.0fu H %.0fu"),
                FitStatus,
                State.FitAnalysis.MinimumAvailableWidth,
                State.FitAnalysis.MinimumAvailableHeight,
                State.FitAnalysis.MaxWidthDeficit,
                State.FitAnalysis.MaxHeightDeficit));
        }

        if (State.ThresholdAnalysis.bVisible)
        {
            const TCHAR* ThresholdStatus = State.ThresholdAnalysis.bPass ? (State.ThresholdAnalysis.bNearThreshold ? TEXT("TIGHT") : TEXT("PASS")) : TEXT("FAIL");
            Lines.Add(FString::Printf(TEXT("THRESHOLD   %s   %d/%d | W %.0fu H %.0fu | step +%.0f/-%.0fu"),
                ThresholdStatus,
                State.ThresholdAnalysis.TotalSamples - State.ThresholdAnalysis.FailedSamples,
                State.ThresholdAnalysis.TotalSamples,
                State.ThresholdAnalysis.MinimumAvailableWidth,
                State.ThresholdAnalysis.MinimumAvailableHeight,
                State.ThresholdAnalysis.MaxObservedStepUp,
                State.ThresholdAnalysis.MaxObservedStepDown));
        }

        if (State.CameraProbe.bVisible)
        {
            const TCHAR* CameraStatus = State.CameraProbe.bBelowMinimum ? TEXT("TOO CLOSE") : (State.CameraProbe.bCompressed ? TEXT("COMPRESSED") : TEXT("CLEAR"));
            Lines.Add(FString::Printf(TEXT("CAMERA      %s   %.0fu / %.0fu | %.0f%%"),
                CameraStatus,
                State.CameraProbe.ResolvedDistance,
                State.CameraProbe.TargetDistance,
                State.CameraProbe.CompressionPercent));
        }

        if (State.CameraSightline.bVisible)
        {
            const TCHAR* ViewStatus = State.CameraSightline.bOccluded ? TEXT("OCCLUDED") : (State.CameraSightline.bFramingFail ? TEXT("FRAMING FAIL") : TEXT("PASS"));
            Lines.Add(FString::Printf(TEXT("VIEW        %s   %.1f deg"), ViewStatus, State.CameraSightline.SubjectAngle));
        }

        if (State.Trajectory.bVisible)
        {
            const TCHAR* ArcStatus = State.Trajectory.bBlocked ? TEXT("BLOCKED") : (State.Trajectory.bLandingValid ? TEXT("LANDING OK") : TEXT("NO LANDING"));
            Lines.Add(FString::Printf(TEXT("TRAJECTORY  %s   %.0fu | %.2fs | apex %.0fu | headroom %.0fu"),
                ArcStatus,
                State.Trajectory.HorizontalDistance,
                State.Trajectory.FlightTime,
                State.Trajectory.ApexHeight,
                State.Trajectory.ApexHeadroom));
        }

        if (State.Sensors.bShowSensorHeights)
        {
            Lines.Add(FString::Printf(TEXT("SENSORS     primary %.0fu   low %.0fu"), State.Sensors.PrimarySensorZOffset, State.Sensors.LowProfileSensorZOffset));
        }

        if (State.SensorField.bVisible)
        {
            Lines.Add(FString::Printf(TEXT("SENSOR      %.0f%% CLEAR   %d/%d blocked | nearest %.0fu"),
                State.SensorField.ClearPercent,
                State.SensorField.BlockedRays,
                State.SensorField.TotalRays,
                State.SensorField.NearestBlockDistance));
        }

        if (State.Locomotion.bShowLocomotionBands)
        {
            Lines.Add(FString::Printf(TEXT("MOVE        %.0fu/s   %.1f / %.1f / %.1fs"),
                State.Locomotion.Speed,
                State.Locomotion.NearTime,
                State.Locomotion.MidTime,
                State.Locomotion.FarTime));
        }

        if (State.Ranges.bShowEngagementRanges)
        {
            Lines.Add(FString::Printf(TEXT("RANGE       %.0f / %.0f / %.0fu"),
                State.Ranges.ShortRange,
                State.Ranges.MediumRange,
                State.Ranges.LongRange));
        }

        return Lines.IsEmpty() ? TEXT("No analysis layers were active when this benchmark was captured.") : FString::Join(Lines, TEXT("\n"));
    }
}

void UMetricSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RefreshCycleEntries();

    // The subsystem outlives individual editor worlds. Release world-owned
    // visualization before LoadMap and again when any world begins teardown.
    // The weak RenderHost pointer is the primary ownership safeguard; these
    // delegates make cleanup deterministic and immediate.
    PreLoadMapHandle = FCoreUObjectDelegates::PreLoadMap.AddUObject(
        this,
        &UMetricSubsystem::HandlePreLoadMap);
    WorldBeginTearDownHandle = FWorldDelegates::OnWorldBeginTearDown.AddUObject(
        this,
        &UMetricSubsystem::HandleWorldBeginTearDown);

    // Creating/saving starter assets during subsystem initialization can run
    // before UE has finished registering all Data Validation validators. Defer
    // the one-time bootstrap until the editor has been alive for a moment.
    TWeakObjectPtr<UMetricSubsystem> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(
        TEXT("MetricStarterPresetBootstrap"),
        1.0f,
        [WeakThis](float)
        {
            if (UMetricSubsystem* StrongThis = WeakThis.Get())
            {
                StrongThis->EnsureStarterPresetAssets();
                StrongThis->RefreshCycleEntries();
            }
            return false;
        });
}

void UMetricSubsystem::EnsureStarterPresetAssets()
{
    if (IsRunningCommandlet())
    {
        return;
    }

    UMetricProjectSettings* ProjectSettings = GetMutableDefault<UMetricProjectSettings>();
    if (!ProjectSettings
        || !ProjectSettings->bCreateStarterPresetAssetsIfMissing
        || ProjectSettings->bStarterPresetsInitialized)
    {
        return;
    }

    FMetricHullSettings HumanHull;
    HumanHull.HullType = EMetricHullType::Capsule;
    HumanHull.Height = 180.0f;
    HumanHull.Width = 88.0f;
    HumanHull.Length = 88.0f;

    FMetricHullSettings HeavyHull;
    HeavyHull.HullType = EMetricHullType::OrientedBox;
    HeavyHull.Height = 300.0f;
    HeavyHull.Width = 180.0f;
    HeavyHull.Length = 140.0f;

    UMetricPreset* HumanPreset = CreateStarterPresetAsset(
        TEXT("/Game/Metric/Presets/DA_Metric_Human"),
        LOCTEXT("StarterHumanDisplayName", "Human Reference"),
        HumanHull);

    UMetricPreset* HeavyPreset = CreateStarterPresetAsset(
        TEXT("/Game/Metric/Presets/DA_Metric_Heavy"),
        LOCTEXT("StarterHeavyDisplayName", "Heavy Reference"),
        HeavyHull);

    bool bSettingsChanged = false;
    if (HumanPreset)
    {
        const TSoftObjectPtr<UMetricPreset> HumanSoft(HumanPreset);
        if (!ProjectSettings->RegisteredPresets.Contains(HumanSoft))
        {
            ProjectSettings->RegisteredPresets.Add(HumanSoft);
            bSettingsChanged = true;
        }
    }

    if (HeavyPreset)
    {
        const TSoftObjectPtr<UMetricPreset> HeavySoft(HeavyPreset);
        if (!ProjectSettings->RegisteredPresets.Contains(HeavySoft))
        {
            ProjectSettings->RegisteredPresets.Add(HeavySoft);
            bSettingsChanged = true;
        }
    }

    if (HumanPreset && HeavyPreset)
    {
        ProjectSettings->bStarterPresetsInitialized = true;
        bSettingsChanged = true;
    }

    if (bSettingsChanged)
    {
        ProjectSettings->SaveConfig();
    }
}

UMetricPreset* UMetricSubsystem::CreateStarterPresetAsset(
    const FString& PackagePath,
    const FText& DisplayName,
    const FMetricHullSettings& HullSettings)
{
    const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
    const FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);

    if (UMetricPreset* ExistingPreset = LoadObject<UMetricPreset>(nullptr, *ObjectPath))
    {
        return ExistingPreset;
    }

    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return nullptr;
    }

    UMetricPreset* NewPreset = NewObject<UMetricPreset>(
        Package,
        *AssetName,
        RF_Public | RF_Standalone | RF_Transactional);
    if (!NewPreset)
    {
        return nullptr;
    }

    NewPreset->DisplayName = DisplayName;
    NewPreset->Hull = HullSettings;

    // Starter assets are authored from explicit Metric defaults. Existing
    // project assets are never overwritten by this path.
    const bool bHeavyStarter = AssetName.Contains(TEXT("Heavy"));
    NewPreset->Clearance.bShowClearanceProbe = true;
    NewPreset->Clearance.ProbeDistance = bHeavyStarter ? 400.0f : 300.0f;
    NewPreset->Clearance.WarningMargin = bHeavyStarter ? 50.0f : 25.0f;
    NewPreset->Clearance.TraceChannel = ECC_Visibility;

    NewPreset->Sensors.bShowSensorHeights = true;
    NewPreset->Sensors.PrimarySensorZOffset = bHeavyStarter ? 260.0f : 160.0f;
    NewPreset->Sensors.LowProfileSensorZOffset = bHeavyStarter ? 160.0f : 100.0f;
    NewPreset->Sensors.MarkerRadius = bHeavyStarter ? 32.0f : 24.0f;

    NewPreset->SensorField.bShowSensorField = false;
    NewPreset->SensorField.Range = bHeavyStarter ? 2200.0f : 1800.0f;
    NewPreset->SensorField.HorizontalHalfAngle = bHeavyStarter ? 40.0f : 45.0f;
    NewPreset->SensorField.VerticalHalfAngle = bHeavyStarter ? 25.0f : 30.0f;
    NewPreset->SensorField.HorizontalRayCount = bHeavyStarter ? 11 : 9;
    NewPreset->SensorField.TraceChannel = ECC_Visibility;

    NewPreset->EngagementRanges.bShowEngagementRanges = false;
    NewPreset->EngagementRanges.ShortRange = bHeavyStarter ? 700.0f : 500.0f;
    NewPreset->EngagementRanges.MediumRange = bHeavyStarter ? 2000.0f : 1500.0f;
    NewPreset->EngagementRanges.LongRange = bHeavyStarter ? 4000.0f : 3000.0f;

    NewPreset->LocomotionBands.bShowLocomotionBands = false;
    NewPreset->LocomotionBands.Speed = bHeavyStarter ? 350.0f : 600.0f;
    NewPreset->LocomotionBands.NearTime = 1.0f;
    NewPreset->LocomotionBands.MidTime = 3.0f;
    NewPreset->LocomotionBands.FarTime = 5.0f;

    NewPreset->Trajectory.bShowTrajectory = false;
    NewPreset->Trajectory.HorizontalSpeed = bHeavyStarter ? 400.0f : 600.0f;
    NewPreset->Trajectory.LaunchVerticalSpeed = bHeavyStarter ? 450.0f : 550.0f;
    NewPreset->Trajectory.GravityScale = 1.0f;
    NewPreset->Trajectory.MaxSimulationTime = bHeavyStarter ? 2.25f : 2.0f;
    NewPreset->Trajectory.SampleInterval = 0.05f;
    NewPreset->Trajectory.MaxLandingSlope = bHeavyStarter ? 40.0f : 45.0f;
    NewPreset->Trajectory.bShowTrajectoryEnvelope = true;
    NewPreset->Trajectory.EnvelopeSampleStride = bHeavyStarter ? 4 : 3;
    NewPreset->Trajectory.ApexProbeDistance = bHeavyStarter ? 250.0f : 200.0f;
    NewPreset->Trajectory.ApexWarningMargin = bHeavyStarter ? 75.0f : 50.0f;
    NewPreset->Trajectory.TraceChannel = ECC_Visibility;

    NewPreset->CameraProbe.bShowCameraProbe = true;
    NewPreset->CameraProbe.PivotZOffset = bHeavyStarter ? 260.0f : 160.0f;
    NewPreset->CameraProbe.TargetDistance = bHeavyStarter ? 500.0f : 350.0f;
    NewPreset->CameraProbe.VerticalOffset = bHeavyStarter ? 100.0f : 60.0f;
    NewPreset->CameraProbe.ShoulderOffset = 0.0f;
    NewPreset->CameraProbe.ProbeRadius = bHeavyStarter ? 20.0f : 12.0f;
    NewPreset->CameraProbe.MinDistance = bHeavyStarter ? 100.0f : 60.0f;
    NewPreset->CameraProbe.TraceChannel = ECC_Visibility;

    NewPreset->CameraSightline.bShowCameraSightline = false;
    NewPreset->CameraSightline.SubjectZOffset = bHeavyStarter ? 240.0f : 140.0f;
    NewPreset->CameraSightline.FramingHalfAngle = bHeavyStarter ? 22.0f : 25.0f;
    NewPreset->CameraSightline.TraceRadius = bHeavyStarter ? 6.0f : 4.0f;
    NewPreset->CameraSightline.TraceChannel = ECC_Visibility;

    NewPreset->GroundValidation.bShowGroundValidation = true;
    NewPreset->GroundValidation.ForwardDistance = bHeavyStarter ? 400.0f : 300.0f;
    NewPreset->GroundValidation.SampleSpacing = bHeavyStarter ? 75.0f : 50.0f;
    NewPreset->GroundValidation.LateralCoverage = bHeavyStarter ? 180.0f : 88.0f;
    NewPreset->GroundValidation.MaxWalkableSlope = bHeavyStarter ? 35.0f : 45.0f;
    NewPreset->GroundValidation.MaxStepUp = bHeavyStarter ? 30.0f : 45.0f;
    NewPreset->GroundValidation.MaxStepDown = bHeavyStarter ? 40.0f : 60.0f;
    NewPreset->GroundValidation.TraceHeight = bHeavyStarter ? 160.0f : 120.0f;
    NewPreset->GroundValidation.TraceDepth = bHeavyStarter ? 250.0f : 200.0f;
    NewPreset->GroundValidation.TraceChannel = ECC_Visibility;

    NewPreset->FitAnalysis.bShowFitAnalysis = true;
    NewPreset->FitAnalysis.ForwardDistance = bHeavyStarter ? 400.0f : 300.0f;
    NewPreset->FitAnalysis.SampleSpacing = bHeavyStarter ? 75.0f : 50.0f;
    NewPreset->FitAnalysis.MaxSideProbeDistance = bHeavyStarter ? 400.0f : 250.0f;
    NewPreset->FitAnalysis.MaxCeilingProbeHeight = bHeavyStarter ? 450.0f : 300.0f;
    NewPreset->FitAnalysis.WarningMargin = bHeavyStarter ? 20.0f : 15.0f;
    NewPreset->FitAnalysis.FloorTraceHeight = bHeavyStarter ? 160.0f : 120.0f;
    NewPreset->FitAnalysis.FloorTraceDepth = bHeavyStarter ? 250.0f : 200.0f;
    NewPreset->FitAnalysis.TraceChannel = ECC_Visibility;

    NewPreset->ThresholdAnalysis.bShowThresholdAnalysis = true;
    NewPreset->ThresholdAnalysis.ProbeDepth = bHeavyStarter ? 180.0f : 120.0f;
    NewPreset->ThresholdAnalysis.MaxSideProbeDistance = bHeavyStarter ? 400.0f : 250.0f;
    NewPreset->ThresholdAnalysis.MaxCeilingProbeHeight = bHeavyStarter ? 450.0f : 300.0f;
    NewPreset->ThresholdAnalysis.WarningMargin = bHeavyStarter ? 20.0f : 15.0f;
    NewPreset->ThresholdAnalysis.MaxStepUp = bHeavyStarter ? 30.0f : 45.0f;
    NewPreset->ThresholdAnalysis.MaxStepDown = bHeavyStarter ? 40.0f : 60.0f;
    NewPreset->ThresholdAnalysis.FloorTraceHeight = bHeavyStarter ? 160.0f : 120.0f;
    NewPreset->ThresholdAnalysis.FloorTraceDepth = bHeavyStarter ? 250.0f : 200.0f;
    NewPreset->ThresholdAnalysis.TraceChannel = ECC_Visibility;

    NewPreset->Representation.bShowStaticMesh = false;
    NewPreset->Representation.StaticMesh.Reset();
    NewPreset->Representation.RelativeTransform = FTransform::Identity;

    FAssetRegistryModule::AssetCreated(NewPreset);
    Package->MarkPackageDirty();

    const FString Filename = FPackageName::LongPackageNameToFilename(
        PackagePath,
        FPackageName::GetAssetPackageExtension());
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;

    if (!UPackage::SavePackage(Package, NewPreset, *Filename, SaveArgs))
    {
        return nullptr;
    }

    return NewPreset;
}

void UMetricSubsystem::Deinitialize()
{
    if (PreLoadMapHandle.IsValid())
    {
        FCoreUObjectDelegates::PreLoadMap.Remove(PreLoadMapHandle);
        PreLoadMapHandle.Reset();
    }

    if (WorldBeginTearDownHandle.IsValid())
    {
        FWorldDelegates::OnWorldBeginTearDown.Remove(WorldBeginTearDownHandle);
        WorldBeginTearDownHandle.Reset();
    }

    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }

    SetMetricActive(false);
    ClearRenderHost();
    CycleEntries.Reset();
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    ActivePinnedBenchmarkMap.Reset();
    Super::Deinitialize();
}

void UMetricSubsystem::HandlePreLoadMap(const FString& /*MapName*/)
{
    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }

    // LoadMap will synchronously tear down the current editor world. Drop every
    // world-owned reference before that process begins. Keep Metric's enabled
    // state so the Lens can resume lazily in the newly loaded editor world.
    ClearRenderHost();
    LiveState.bHasHit = false;
    LiveState.Clearance = FMetricClearanceResult();
    LiveState.Trajectory = FMetricTrajectoryResult();
    LiveState.CameraProbe = FMetricCameraProbeResult();
    LiveState.CameraSightline = FMetricCameraSightlineResult();
    LiveState.GroundValidation = FMetricGroundValidationResult();
    LiveState.FitAnalysis = FMetricFitAnalysisResult();
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    ActivePinnedBenchmarkMap.Reset();
}

void UMetricSubsystem::HandleWorldBeginTearDown(UWorld* World)
{
    // PreLoadMap normally saves first and clears ActivePinnedBenchmarkMap. If a
    // world is torn down through another editor path, persist it here only while
    // it is still the active pin owner. This avoids an empty post-PreLoad save
    // erasing the layout we just stored.
    if (World && !ActivePinnedBenchmarkMap.IsEmpty()
        && GetPersistableMapPackageName(World) == ActivePinnedBenchmarkMap)
    {
        SavePinnedBenchmarksForWorld(World);
    }

    AMetricRenderHostActor* Host = RenderHost.Get();
    if (!World || !Host || Host->GetWorld() != World)
    {
        return;
    }

    // Do not call DestroyActor from the world's teardown callback. The world is
    // already dismantling its Actors; simply release Metric's weak handle and
    // live visualization state so nothing attempts to use the dying world.
    RenderHost.Reset();
    LiveState.bHasHit = false;
    LiveState.Clearance = FMetricClearanceResult();
    LiveState.Trajectory = FMetricTrajectoryResult();
    LiveState.CameraProbe = FMetricCameraProbeResult();
    LiveState.CameraSightline = FMetricCameraSightlineResult();
    LiveState.GroundValidation = FMetricGroundValidationResult();
    LiveState.FitAnalysis = FMetricFitAnalysisResult();
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    ActivePinnedBenchmarkMap.Reset();
}

bool UMetricSubsystem::ArePinnedBenchmarksPersistent() const
{
    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    return Settings && Settings->bPersistPinnedBenchmarksPerMap;
}

FString UMetricSubsystem::GetPersistableMapPackageName(UWorld* World) const
{
    if (!World || World->WorldType != EWorldType::Editor)
    {
        return FString();
    }

    UPackage* Package = World->GetPackage();
    if (!Package)
    {
        return FString();
    }

    const FString PackageName = Package->GetName();
    if (PackageName.IsEmpty() || PackageName.StartsWith(TEXT("/Temp/")) || PackageName.StartsWith(TEXT("/Engine/Transient")))
    {
        return FString();
    }

    return PackageName;
}

void UMetricSubsystem::SavePinnedBenchmarksForWorld(UWorld* World)
{
    if (!ArePinnedBenchmarksPersistent())
    {
        return;
    }

    const FString MapPackageName = GetPersistableMapPackageName(World);
    if (MapPackageName.IsEmpty())
    {
        return;
    }

    UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>();
    if (!Settings)
    {
        return;
    }

    int32 ExistingIndex = INDEX_NONE;
    for (int32 Index = 0; Index < Settings->PersistentBenchmarkMaps.Num(); ++Index)
    {
        if (Settings->PersistentBenchmarkMaps[Index].MapPackageName == MapPackageName)
        {
            ExistingIndex = Index;
            break;
        }
    }

    if (PinnedBenchmarks.IsEmpty())
    {
        if (ExistingIndex != INDEX_NONE)
        {
            Settings->PersistentBenchmarkMaps.RemoveAt(ExistingIndex);
            Settings->SaveConfig();
        }
        return;
    }

    FMetricPersistentBenchmarkMap* StoredMap = ExistingIndex != INDEX_NONE
        ? &Settings->PersistentBenchmarkMaps[ExistingIndex]
        : &Settings->PersistentBenchmarkMaps.AddDefaulted_GetRef();

    StoredMap->MapPackageName = MapPackageName;
    StoredMap->Benchmarks.Reset();
    StoredMap->Benchmarks.Reserve(PinnedBenchmarks.Num());

    for (const FMetricPinnedBenchmark& Pin : PinnedBenchmarks)
    {
        FMetricPersistentBenchmark& StoredPin = StoredMap->Benchmarks.AddDefaulted_GetRef();
        StoredPin.WorldLocation = Pin.WorldLocation;
        StoredPin.Forward = Pin.Forward;
        StoredPin.Hull = Pin.Hull;
        StoredPin.Label = Pin.Label.ToString();
        StoredPin.SourceProfileLabel = Pin.SourceProfileLabel.ToString();
        StoredPin.CapturedResults = Pin.CapturedResults;
        StoredPin.bHasVisualSnapshot = Pin.bHasVisualSnapshot;
        StoredPin.Visibility = Pin.Visibility;
        StoredPin.SurfaceNormal = Pin.SurfaceNormal;
        StoredPin.ClearanceSettings = Pin.ClearanceSettings;
        StoredPin.Sensors = Pin.Sensors;
        StoredPin.SensorFieldSettings = Pin.SensorFieldSettings;
        StoredPin.Ranges = Pin.Ranges;
        StoredPin.Locomotion = Pin.Locomotion;
        StoredPin.TrajectorySettings = Pin.TrajectorySettings;
        StoredPin.CameraProbeSettings = Pin.CameraProbeSettings;
        StoredPin.CameraSightlineSettings = Pin.CameraSightlineSettings;
        StoredPin.GroundValidationSettings = Pin.GroundValidationSettings;
        StoredPin.FitAnalysisSettings = Pin.FitAnalysisSettings;
        StoredPin.ThresholdAnalysisSettings = Pin.ThresholdAnalysisSettings;
    }

    Settings->SaveConfig();
}

void UMetricSubsystem::LoadPinnedBenchmarksForWorld(UWorld* World)
{
    PinnedBenchmarks.Reset();

    const FString MapPackageName = GetPersistableMapPackageName(World);
    ActivePinnedBenchmarkMap = MapPackageName;
    if (MapPackageName.IsEmpty() || !ArePinnedBenchmarksPersistent())
    {
        LiveState.PinnedBenchmarks.Reset();
        return;
    }

    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    if (!Settings)
    {
        return;
    }

    const int32 MaxPins = FMath::Clamp(Settings->MaxPinnedBenchmarks, 1, 32);
    for (const FMetricPersistentBenchmarkMap& StoredMap : Settings->PersistentBenchmarkMaps)
    {
        if (StoredMap.MapPackageName != MapPackageName)
        {
            continue;
        }

        const int32 StartIndex = FMath::Max(0, StoredMap.Benchmarks.Num() - MaxPins);
        for (int32 Index = StartIndex; Index < StoredMap.Benchmarks.Num(); ++Index)
        {
            const FMetricPersistentBenchmark& StoredPin = StoredMap.Benchmarks[Index];
            FMetricPinnedBenchmark& Pin = PinnedBenchmarks.AddDefaulted_GetRef();
            Pin.WorldLocation = StoredPin.WorldLocation;
            Pin.Forward = StoredPin.Forward.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
            Pin.Hull = StoredPin.Hull;
            Pin.Label = FText::FromString(StoredPin.Label);
            Pin.SourceProfileLabel = StoredPin.SourceProfileLabel.IsEmpty() ? Pin.Label : FText::FromString(StoredPin.SourceProfileLabel);
            Pin.CapturedResults = StoredPin.CapturedResults;
            Pin.bHasVisualSnapshot = StoredPin.bHasVisualSnapshot;
            Pin.Visibility = StoredPin.Visibility;
            Pin.SurfaceNormal = StoredPin.SurfaceNormal;
            Pin.ClearanceSettings = StoredPin.ClearanceSettings;
            Pin.Sensors = StoredPin.Sensors;
            Pin.SensorFieldSettings = StoredPin.SensorFieldSettings;
            Pin.Ranges = StoredPin.Ranges;
            Pin.Locomotion = StoredPin.Locomotion;
            Pin.TrajectorySettings = StoredPin.TrajectorySettings;
            Pin.CameraProbeSettings = StoredPin.CameraProbeSettings;
            Pin.CameraSightlineSettings = StoredPin.CameraSightlineSettings;
            Pin.GroundValidationSettings = StoredPin.GroundValidationSettings;
            Pin.FitAnalysisSettings = StoredPin.FitAnalysisSettings;
            Pin.ThresholdAnalysisSettings = StoredPin.ThresholdAnalysisSettings;
        }
        break;
    }

    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    LiveState.SelectedPinnedBenchmarkIndex = INDEX_NONE;
}

void UMetricSubsystem::EnsurePinnedBenchmarkMap(UWorld* World)
{
    const FString MapPackageName = GetPersistableMapPackageName(World);
    if (MapPackageName == ActivePinnedBenchmarkMap)
    {
        return;
    }

    LoadPinnedBenchmarksForWorld(World);
}

void UMetricSubsystem::ToggleMetric()
{
    SetMetricActive(!bMetricActive);
}

void UMetricSubsystem::SetMetricActive(bool bActive)
{
    if (bMetricActive == bActive)
    {
        return;
    }

    bMetricActive = bActive;

    if (bMetricActive)
    {
        RefreshCycleEntries();
    }
    else
    {
        LiveState.bHasHit = false;
        if (AMetricRenderHostActor* Host = RenderHost.Get())
        {
            Host->SetMetricVisible(false);
        }
    }

    NotifyStateChanged();
}

void UMetricSubsystem::CycleNext()
{
    RefreshCycleEntries();
    if (CycleEntries.IsEmpty())
    {
        ActiveIndex = 0;
        return;
    }

    ActiveIndex = (ActiveIndex + 1) % CycleEntries.Num();
    bMetricActive = true;
    ApplyEntryToState(LiveState);
    NotifyStateChanged();
}

void UMetricSubsystem::PinCurrentBenchmark()
{
    if (!bMetricActive || !LiveState.bHasHit || LiveState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
    const int32 MaxPins = FMath::Clamp(Settings ? Settings->MaxPinnedBenchmarks : 8, 1, 32);
    if (PinnedBenchmarks.Num() >= MaxPins)
    {
        PinnedBenchmarks.RemoveAt(0, PinnedBenchmarks.Num() - MaxPins + 1, EAllowShrinking::No);
    }

    FMetricPinnedBenchmark Pin;
    Pin.WorldLocation = LiveState.Location;
    Pin.SurfaceNormal = LiveState.SurfaceNormal;
    Pin.Forward = LiveState.Forward;
    Pin.Hull = LiveState.Hull;
    Pin.Label = LiveState.Label;
    Pin.SourceProfileLabel = LiveState.Label;
    Pin.CapturedResults = MetricBenchmark::BuildCapturedResults(LiveState);
    Pin.bHasVisualSnapshot = true;
    if (Settings) Pin.Visibility = Settings->LiveVisibility;
    Pin.ClearanceSettings = LiveState.ClearanceSettings;
    Pin.Sensors = LiveState.Sensors;
    Pin.SensorFieldSettings = LiveState.SensorFieldSettings;
    Pin.Ranges = LiveState.Ranges;
    Pin.Locomotion = LiveState.Locomotion;
    Pin.TrajectorySettings = LiveState.TrajectorySettings;
    Pin.CameraProbeSettings = LiveState.CameraProbeSettings;
    Pin.CameraSightlineSettings = LiveState.CameraSightlineSettings;
    Pin.GroundValidationSettings = LiveState.GroundValidationSettings;
    Pin.FitAnalysisSettings = LiveState.FitAnalysisSettings;
    Pin.ThresholdAnalysisSettings = LiveState.ThresholdAnalysisSettings;
    PinnedBenchmarks.Add(MoveTemp(Pin));
    SelectedPinnedBenchmarkIndex = PinnedBenchmarks.Num() - 1;

    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }

    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    LiveState.SelectedPinnedBenchmarkIndex = SelectedPinnedBenchmarkIndex;
    if (AMetricRenderHostActor* Host = RenderHost.Get())
    {
        Host->ApplyMetricState(LiveState);
    }
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::ClearPinnedBenchmarks()
{
    // Do not early-out when the runtime array is empty. A saved map may still
    // have persistent pins that the user is explicitly asking to clear.
    PinnedBenchmarks.Reset();
    SelectedPinnedBenchmarkIndex = INDEX_NONE;
    LiveState.PinnedBenchmarks.Reset();
    LiveState.SelectedPinnedBenchmarkIndex = INDEX_NONE;
    if (GEditor)
    {
        SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    }
    if (AMetricRenderHostActor* Host = RenderHost.Get())
    {
        Host->ApplyMetricState(LiveState);
        Host->ApplyPinnedMetricState(nullptr);
    }
    NotifyStateChanged();
}

const FMetricPinnedBenchmark* UMetricSubsystem::GetPinnedBenchmark(int32 Index) const
{
    return PinnedBenchmarks.IsValidIndex(Index) ? &PinnedBenchmarks[Index] : nullptr;
}

void UMetricSubsystem::SelectPinnedBenchmark(int32 Index)
{
    SelectedPinnedBenchmarkIndex = PinnedBenchmarks.IsValidIndex(Index) ? Index : INDEX_NONE;
    LiveState.SelectedPinnedBenchmarkIndex = SelectedPinnedBenchmarkIndex;
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::DeletePinnedBenchmark(int32 Index)
{
    if (!PinnedBenchmarks.IsValidIndex(Index)) return;
    PinnedBenchmarks.RemoveAt(Index);
    if (SelectedPinnedBenchmarkIndex == Index) SelectedPinnedBenchmarkIndex = INDEX_NONE;
    else if (SelectedPinnedBenchmarkIndex > Index) --SelectedPinnedBenchmarkIndex;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    LiveState.SelectedPinnedBenchmarkIndex = SelectedPinnedBenchmarkIndex;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::RenamePinnedBenchmark(int32 Index, const FText& NewLabel)
{
    if (!PinnedBenchmarks.IsValidIndex(Index)) return;
    const FString Trimmed = NewLabel.ToString().TrimStartAndEnd();
    PinnedBenchmarks[Index].Label = Trimmed.IsEmpty() ? FText::FromString(FString::Printf(TEXT("Benchmark %d"), Index + 1)) : FText::FromString(Trimmed);
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    NotifyStateChanged();
}

void UMetricSubsystem::ReplacePinnedBenchmarkWithLive(int32 Index)
{
    if (!PinnedBenchmarks.IsValidIndex(Index) || !bMetricActive || !LiveState.bHasHit || LiveState.EntryType == EMetricCycleEntryType::NativePreviewMesh) return;
    FText ExistingLabel = PinnedBenchmarks[Index].Label;
    PinnedBenchmarks[Index].WorldLocation = LiveState.Location;
    PinnedBenchmarks[Index].SurfaceNormal = LiveState.SurfaceNormal;
    PinnedBenchmarks[Index].Forward = LiveState.Forward;
    PinnedBenchmarks[Index].Hull = LiveState.Hull;
    PinnedBenchmarks[Index].Label = ExistingLabel;
    PinnedBenchmarks[Index].SourceProfileLabel = LiveState.Label;
    PinnedBenchmarks[Index].CapturedResults = MetricBenchmark::BuildCapturedResults(LiveState);
    PinnedBenchmarks[Index].bHasVisualSnapshot = true;
    PinnedBenchmarks[Index].ClearanceSettings = LiveState.ClearanceSettings;
    PinnedBenchmarks[Index].Sensors = LiveState.Sensors;
    PinnedBenchmarks[Index].SensorFieldSettings = LiveState.SensorFieldSettings;
    PinnedBenchmarks[Index].Ranges = LiveState.Ranges;
    PinnedBenchmarks[Index].Locomotion = LiveState.Locomotion;
    PinnedBenchmarks[Index].TrajectorySettings = LiveState.TrajectorySettings;
    PinnedBenchmarks[Index].CameraProbeSettings = LiveState.CameraProbeSettings;
    PinnedBenchmarks[Index].CameraSightlineSettings = LiveState.CameraSightlineSettings;
    PinnedBenchmarks[Index].GroundValidationSettings = LiveState.GroundValidationSettings;
    PinnedBenchmarks[Index].FitAnalysisSettings = LiveState.FitAnalysisSettings;
    PinnedBenchmarks[Index].ThresholdAnalysisSettings = LiveState.ThresholdAnalysisSettings;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::SetPinnedBenchmarkVisibility(int32 Index, const FMetricLayerVisibility& Visibility)
{
    if (!PinnedBenchmarks.IsValidIndex(Index)) return;
    PinnedBenchmarks[Index].Visibility = Visibility;
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    if (GEditor) SavePinnedBenchmarksForWorld(GEditor->GetEditorWorldContext().World());
    if (AMetricRenderHostActor* Host = RenderHost.Get()) Host->ApplyMetricState(LiveState);
    if (GEditor) RefreshSelectedPinnedVisualization(GEditor->GetEditorWorldContext().World());
    NotifyStateChanged();
}

void UMetricSubsystem::FocusPinnedBenchmark(int32 Index)
{
    if (!PinnedBenchmarks.IsValidIndex(Index) || !GEditor) return;
    SelectPinnedBenchmark(Index);
    FViewport* ActiveViewport = GEditor->GetActiveViewport();
    if (!ActiveViewport) return;
    FViewportClient* ActiveClient = ActiveViewport->GetClient();
    for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
    {
        if (!Client || Client != ActiveClient || !Client->IsPerspective()) continue;
        const FMetricPinnedBenchmark& Pin = PinnedBenchmarks[Index];
        FVector Forward = Pin.Forward.GetSafeNormal();
        if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
        const FVector Target = Pin.WorldLocation + FVector::UpVector * FMath::Max(50.0f, Pin.Hull.Height * 0.5f);
        const FVector Camera = Target - Forward * FMath::Max(450.0f, Pin.Hull.Height * 2.0f) + FVector::UpVector * FMath::Max(120.0f, Pin.Hull.Height * 0.4f);
        Client->SetViewLocation(Camera);
        Client->SetViewRotation((Target - Camera).Rotation());
        Client->Invalidate();
        break;
    }
}

void UMetricSubsystem::RefreshCycleEntries()
{
    FSoftObjectPath PreviousIdentity;
    EMetricCycleEntryType PreviousType = EMetricCycleEntryType::BuiltInMetric;
    if (CycleEntries.IsValidIndex(ActiveIndex))
    {
        PreviousType = CycleEntries[ActiveIndex].Type;
        PreviousIdentity = PreviousType == EMetricCycleEntryType::MetricPreset
            ? CycleEntries[ActiveIndex].Preset.ToSoftObjectPath()
            : CycleEntries[ActiveIndex].NativePreviewMesh;
    }

    CycleEntries.Reset();

    const UMetricProjectSettings* ProjectSettings = GetDefault<UMetricProjectSettings>();
    if (!ProjectSettings || (ProjectSettings->bIncludeBuiltInHumanReference && ProjectSettings->RegisteredPresets.IsEmpty()))
    {
        FMetricCycleEntry& BuiltIn = CycleEntries.AddDefaulted_GetRef();
        BuiltIn.Type = EMetricCycleEntryType::BuiltInMetric;
        BuiltIn.DisplayName = LOCTEXT("BuiltInHuman", "Human Reference");
    }

    if (ProjectSettings)
    {
        for (const TSoftObjectPtr<UMetricPreset>& PresetPtr : ProjectSettings->RegisteredPresets)
        {
            if (PresetPtr.IsNull())
            {
                continue;
            }

            FMetricCycleEntry& Entry = CycleEntries.AddDefaulted_GetRef();
            Entry.Type = EMetricCycleEntryType::MetricPreset;
            Entry.Preset = PresetPtr;
            if (const UMetricPreset* LoadedPreset = PresetPtr.LoadSynchronous())
            {
                Entry.DisplayName = LoadedPreset->DisplayName.IsEmpty()
                    ? FText::FromString(LoadedPreset->GetName())
                    : LoadedPreset->DisplayName;
            }
            else
            {
                Entry.DisplayName = FText::FromString(PresetPtr.ToSoftObjectPath().GetAssetName());
            }
        }
    }

    const UMetricEditorSettings* EditorSettings = GetDefault<UMetricEditorSettings>();
    if (!EditorSettings || EditorSettings->bIncludeUnrealPreviewMeshes)
    {
        const ULevelEditorViewportSettings* ViewportSettings = GetDefault<ULevelEditorViewportSettings>();
        if (ViewportSettings)
        {
            for (const FSoftObjectPath& MeshPath : ViewportSettings->PreviewMeshes)
            {
                if (!MeshPath.IsValid())
                {
                    continue;
                }

                FMetricCycleEntry& Entry = CycleEntries.AddDefaulted_GetRef();
                Entry.Type = EMetricCycleEntryType::NativePreviewMesh;
                Entry.NativePreviewMesh = MeshPath;
                Entry.DisplayName = FText::FromString(MeshPath.GetAssetName());
            }
        }
    }

    ActiveIndex = FMath::Clamp(ActiveIndex, 0, FMath::Max(0, CycleEntries.Num() - 1));

    if (PreviousIdentity.IsValid())
    {
        for (int32 Index = 0; Index < CycleEntries.Num(); ++Index)
        {
            const FMetricCycleEntry& Candidate = CycleEntries[Index];
            const FSoftObjectPath CandidateIdentity = Candidate.Type == EMetricCycleEntryType::MetricPreset
                ? Candidate.Preset.ToSoftObjectPath()
                : Candidate.NativePreviewMesh;
            if (Candidate.Type == PreviousType && CandidateIdentity == PreviousIdentity)
            {
                ActiveIndex = Index;
                break;
            }
        }
    }

    ApplyEntryToState(LiveState);
}

const FMetricCycleEntry* UMetricSubsystem::GetActiveEntry() const
{
    return CycleEntries.IsValidIndex(ActiveIndex) ? &CycleEntries[ActiveIndex] : nullptr;
}

void UMetricSubsystem::ApplyEntryToState(FMetricLiveState& InOutState) const
{
    InOutState.Hull = FMetricHullSettings();
    InOutState.ClearanceSettings = FMetricClearanceSettings();
    InOutState.Clearance = FMetricClearanceResult();
    InOutState.Sensors = FMetricSensorSettings();
    InOutState.SensorFieldSettings = FMetricSensorFieldSettings();
    InOutState.SensorField = FMetricSensorFieldResult();
    InOutState.Ranges = FMetricRangeSettings();
    InOutState.Locomotion = FMetricLocomotionSettings();
    InOutState.TrajectorySettings = FMetricTrajectorySettings();
    InOutState.Trajectory = FMetricTrajectoryResult();
    InOutState.CameraProbeSettings = FMetricCameraProbeSettings();
    InOutState.CameraProbe = FMetricCameraProbeResult();
    InOutState.CameraSightlineSettings = FMetricCameraSightlineSettings();
    InOutState.CameraSightline = FMetricCameraSightlineResult();
    InOutState.GroundValidationSettings = FMetricGroundValidationSettings();
    InOutState.GroundValidation = FMetricGroundValidationResult();
    InOutState.FitAnalysisSettings = FMetricFitAnalysisSettings();
    InOutState.FitAnalysis = FMetricFitAnalysisResult();
    InOutState.ThresholdAnalysisSettings = FMetricThresholdAnalysisSettings();
    InOutState.ThresholdAnalysis = FMetricThresholdAnalysisResult();
    InOutState.bShowHull = true;
    InOutState.bShowForward = true;
    InOutState.PreviewMesh = FSoftObjectPath();
    InOutState.PreviewRelativeTransform = FTransform::Identity;
    InOutState.EntryType = EMetricCycleEntryType::BuiltInMetric;
    InOutState.Label = LOCTEXT("MetricNoEntry", "METRIC");

    const FMetricCycleEntry* Entry = GetActiveEntry();
    if (!Entry)
    {
        return;
    }

    InOutState.EntryType = Entry->Type;

    if (Entry->Type == EMetricCycleEntryType::NativePreviewMesh)
    {
        InOutState.Label = FText::Format(LOCTEXT("NativePreviewLabel", "PREVIEW MESH - {0}"), Entry->DisplayName);
        InOutState.bShowHull = false;
        InOutState.bShowForward = false;
        InOutState.ClearanceSettings.bShowClearanceProbe = false;
        InOutState.Sensors.bShowSensorHeights = false;
        InOutState.SensorFieldSettings.bShowSensorField = false;
        InOutState.Ranges.bShowEngagementRanges = false;
        InOutState.Locomotion.bShowLocomotionBands = false;
        InOutState.TrajectorySettings.bShowTrajectory = false;
        InOutState.CameraProbeSettings.bShowCameraProbe = false;
        InOutState.CameraSightlineSettings.bShowCameraSightline = false;
        InOutState.GroundValidationSettings.bShowGroundValidation = false;
        InOutState.FitAnalysisSettings.bShowFitAnalysis = false;
        InOutState.ThresholdAnalysisSettings.bShowThresholdAnalysis = false;
        InOutState.PreviewMesh = Entry->NativePreviewMesh;
        return;
    }

    if (Entry->Type == EMetricCycleEntryType::MetricPreset)
    {
        if (const UMetricPreset* Preset = Entry->Preset.LoadSynchronous())
        {
            InOutState.Hull = Preset->Hull;
            InOutState.ClearanceSettings = Preset->Clearance;
            InOutState.Sensors = Preset->Sensors;
            InOutState.SensorFieldSettings = Preset->SensorField;
            InOutState.Ranges = Preset->EngagementRanges;
            InOutState.Locomotion = Preset->LocomotionBands;
            InOutState.TrajectorySettings = Preset->Trajectory;
            InOutState.CameraProbeSettings = Preset->CameraProbe;
            InOutState.CameraSightlineSettings = Preset->CameraSightline;
            InOutState.GroundValidationSettings = Preset->GroundValidation;
            InOutState.FitAnalysisSettings = Preset->FitAnalysis;
            InOutState.ThresholdAnalysisSettings = Preset->ThresholdAnalysis;
            InOutState.Label = FText::Format(LOCTEXT("PresetLabel", "METRIC - {0}"), Entry->DisplayName);
            if (Preset->Representation.bShowStaticMesh)
            {
                InOutState.PreviewMesh = Preset->Representation.StaticMesh.ToSoftObjectPath();
                InOutState.PreviewRelativeTransform = Preset->Representation.RelativeTransform;
            }
            return;
        }
    }

    InOutState.Label = FText::Format(LOCTEXT("BuiltInLabel", "METRIC - {0}"), Entry->DisplayName);
}

void UMetricSubsystem::UpdateLiveState(UWorld* World, const FMetricLiveState& NewState)
{
    EnsurePinnedBenchmarkMap(World);
    if (!bMetricActive || !World)
    {
        return;
    }

    LiveState = NewState;
    ApplyEntryToState(LiveState);
    LiveState.PinnedBenchmarks = PinnedBenchmarks;
    EvaluateClearance(World, LiveState);
    EvaluateSensorField(World, LiveState);
    EvaluateTrajectory(World, LiveState);
    EvaluateCameraProbe(World, LiveState);
    EvaluateCameraSightline(World, LiveState);
    EvaluateGroundValidation(World, LiveState);
    EvaluateFitAnalysis(World, LiveState);
    EvaluateThresholdAnalysis(World, LiveState);
    EnsureRenderHost(World);

    if (AMetricRenderHostActor* Host = RenderHost.Get())
    {
        FMetricLiveState RenderState = LiveState;
        if (const UMetricEditorSettings* EditorSettings = GetDefault<UMetricEditorSettings>())
        {
            MetricBenchmark::ApplyVisualVisibility(RenderState, EditorSettings->LiveVisibility);
        }
        Host->ApplyMetricState(RenderState);
        Host->SetMetricVisible(LiveState.bHasHit || !PinnedBenchmarks.IsEmpty());
    }
    RefreshSelectedPinnedVisualization(World);
}

bool UMetricSubsystem::BuildSelectedPinnedVisualState(UWorld* World, FMetricLiveState& OutState) const
{
    if (!World || !PinnedBenchmarks.IsValidIndex(SelectedPinnedBenchmarkIndex))
    {
        return false;
    }

    const FMetricPinnedBenchmark& Pin = PinnedBenchmarks[SelectedPinnedBenchmarkIndex];
    if (!Pin.bHasVisualSnapshot)
    {
        return false;
    }

    OutState = FMetricLiveState();
    OutState.bHasHit = true;
    OutState.bShowHull = false;
    OutState.bShowForward = false;
    OutState.Location = Pin.WorldLocation;
    OutState.SurfaceNormal = Pin.SurfaceNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    OutState.Forward = Pin.Forward.GetSafeNormal(SMALL_NUMBER, FVector::ForwardVector);
    OutState.ViewerLocation = LiveState.ViewerLocation;
    OutState.Hull = Pin.Hull;
    OutState.ClearanceSettings = Pin.ClearanceSettings;
    OutState.Sensors = Pin.Sensors;
    OutState.SensorFieldSettings = Pin.SensorFieldSettings;
    OutState.Ranges = Pin.Ranges;
    OutState.Locomotion = Pin.Locomotion;
    OutState.TrajectorySettings = Pin.TrajectorySettings;
    OutState.CameraProbeSettings = Pin.CameraProbeSettings;
    OutState.CameraSightlineSettings = Pin.CameraSightlineSettings;
    OutState.GroundValidationSettings = Pin.GroundValidationSettings;
    OutState.FitAnalysisSettings = Pin.FitAnalysisSettings;
    OutState.ThresholdAnalysisSettings = Pin.ThresholdAnalysisSettings;
    OutState.Label = Pin.Label;
    MetricBenchmark::ApplyVisualVisibility(OutState, Pin.Visibility);

    EvaluateClearance(World, OutState);
    EvaluateSensorField(World, OutState);
    EvaluateTrajectory(World, OutState);
    EvaluateCameraProbe(World, OutState);
    EvaluateCameraSightline(World, OutState);
    EvaluateGroundValidation(World, OutState);
    EvaluateFitAnalysis(World, OutState);
    EvaluateThresholdAnalysis(World, OutState);
    return true;
}

void UMetricSubsystem::RefreshSelectedPinnedVisualization(UWorld* World)
{
    AMetricRenderHostActor* Host = RenderHost.Get();
    if (!Host)
    {
        return;
    }

    FMetricLiveState PinnedState;
    if (BuildSelectedPinnedVisualState(World, PinnedState))
    {
        Host->ApplyPinnedMetricState(&PinnedState);
    }
    else
    {
        Host->ApplyPinnedMetricState(nullptr);
    }
}

void UMetricSubsystem::EvaluateClearance(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.Clearance = FMetricClearanceResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.ClearanceSettings.bShowClearanceProbe
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    const float Height = FMath::Max(1.0f, InOutState.Hull.Height);
    const float Width = FMath::Max(1.0f, InOutState.Hull.Width);
    const float Length = FMath::Max(1.0f, InOutState.Hull.Length);
    const float ProbeDistance = FMath::Max(10.0f, InOutState.ClearanceSettings.ProbeDistance);

    FCollisionShape Shape;
    FQuat Rotation = FQuat::Identity;
    float CenterHeight = Height * 0.5f;
    bool bApproximateShape = false;

    switch (InOutState.Hull.HullType)
    {
    case EMetricHullType::Capsule:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        CenterHeight = HalfHeight;
        break;
    }
    case EMetricHullType::OrientedBox:
    {
        const FVector HalfExtent(Length * 0.5f, Width * 0.5f, Height * 0.5f);
        Shape = FCollisionShape::MakeBox(HalfExtent);
        Rotation = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();
        CenterHeight = HalfExtent.Z;
        break;
    }
    case EMetricHullType::Sphere:
    {
        const float Radius = FMath::Max3(Width, Length, Height) * 0.5f;
        Shape = FCollisionShape::MakeSphere(FMath::Max(1.0f, Radius));
        CenterHeight = Radius;
        break;
    }
    case EMetricHullType::Cylinder:
    default:
    {
        // UE's standard FCollisionShape does not expose an exact cylinder. Use
        // a capsule with matching width/height and mark the result approximate.
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        CenterHeight = HalfHeight;
        bApproximateShape = true;
        break;
    }
    }

    const float WarningMargin = FMath::Max(0.0f, InOutState.ClearanceSettings.WarningMargin);
    const FVector StartOffset = InOutState.Hull.LocalOffset + FVector::UpVector * (CenterHeight + 1.0f);
    const FVector EndOffset = StartOffset + Forward * ProbeDistance;
    const FVector ExtendedEndOffset = StartOffset + Forward * (ProbeDistance + WarningMargin);
    const FVector Start = InOutState.Location + StartOffset;
    const FVector End = InOutState.Location + ExtendedEndOffset;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricClearanceSweep), true);
    QueryParams.bTraceComplex = true;

    FHitResult Hit;
    const bool bBlocked = World->SweepSingleByChannel(
        Hit,
        Start,
        End,
        Rotation,
        InOutState.ClearanceSettings.TraceChannel.GetValue(),
        Shape,
        QueryParams);

    FMetricClearanceResult& Result = InOutState.Clearance;
    Result.bVisible = true;
    Result.bHasObstacle = bBlocked;
    Result.bApproximateShape = bApproximateShape;
    Result.ProbeDistance = ProbeDistance;
    Result.WarningMargin = WarningMargin;
    Result.SweepStartOffset = StartOffset;
    Result.SweepEndOffset = EndOffset;

    if (bBlocked)
    {
        Result.ObstacleDistance = FMath::Max(0.0f, Hit.Distance);
        Result.bBlocked = Result.ObstacleDistance <= ProbeDistance + KINDA_SMALL_NUMBER;
        Result.bNearThreshold = !Result.bBlocked && Result.ObstacleDistance <= ProbeDistance + WarningMargin + KINDA_SMALL_NUMBER;
        Result.ClearDistance = FMath::Min(Result.ObstacleDistance, ProbeDistance);
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
        Result.ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
    }
    else
    {
        Result.bBlocked = false;
        Result.bNearThreshold = false;
        Result.ObstacleDistance = ProbeDistance + WarningMargin;
        Result.ClearDistance = ProbeDistance;
        Result.ImpactPointOffset = EndOffset;
        Result.ImpactNormal = FVector::ZeroVector;
    }
}


void UMetricSubsystem::EvaluateSensorField(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.SensorField = FMetricSensorFieldResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.SensorFieldSettings.bShowSensorField
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    const FVector Up = FVector::UpVector;
    const float Range = FMath::Max(10.0f, InOutState.SensorFieldSettings.Range);
    const float HalfAngle = FMath::Clamp(InOutState.SensorFieldSettings.HorizontalHalfAngle, 1.0f, 89.0f);
    int32 RayCount = FMath::Clamp(InOutState.SensorFieldSettings.HorizontalRayCount, 3, 31);
    if ((RayCount % 2) == 0)
    {
        ++RayCount;
    }

    const FVector OriginOffset = InOutState.Hull.LocalOffset + Up * FMath::Max(0.0f, InOutState.Sensors.PrimarySensorZOffset);
    const FVector Origin = InOutState.Location + OriginOffset;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricSensorFieldTrace), true);
    QueryParams.bTraceComplex = true;

    FMetricSensorFieldResult& Result = InOutState.SensorField;
    Result.bVisible = true;
    Result.TotalRays = RayCount;
    Result.OriginOffset = OriginOffset;
    Result.NearestBlockDistance = Range;
    Result.Rays.Reserve(RayCount);

    for (int32 Index = 0; Index < RayCount; ++Index)
    {
        const float Alpha = RayCount > 1
            ? static_cast<float>(Index) / static_cast<float>(RayCount - 1)
            : 0.5f;
        const float YawDegrees = FMath::Lerp(-HalfAngle, HalfAngle, Alpha);
        const FVector Direction = FQuat(Up, FMath::DegreesToRadians(YawDegrees)).RotateVector(Forward).GetSafeNormal();
        const FVector End = Origin + Direction * Range;

        FHitResult Hit;
        const bool bBlocked = World->LineTraceSingleByChannel(
            Hit,
            Origin,
            End,
            InOutState.SensorFieldSettings.TraceChannel.GetValue(),
            QueryParams);

        FMetricSensorFieldRay Ray;
        Ray.bBlocked = bBlocked;
        Ray.Distance = bBlocked ? FMath::Max(0.0f, Hit.Distance) : Range;
        const FVector ResolvedEnd = bBlocked ? Hit.ImpactPoint : End;
        Ray.EndOffset = ResolvedEnd - InOutState.Location;
        Result.Rays.Add(Ray);

        if (bBlocked)
        {
            ++Result.BlockedRays;
            Result.NearestBlockDistance = FMath::Min(Result.NearestBlockDistance, Ray.Distance);
        }
    }

    Result.ClearPercent = Result.TotalRays > 0
        ? (static_cast<float>(Result.TotalRays - Result.BlockedRays) / static_cast<float>(Result.TotalRays)) * 100.0f
        : 100.0f;
}


void UMetricSubsystem::EvaluateTrajectory(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.Trajectory = FMetricTrajectoryResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.TrajectorySettings.bShowTrajectory
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    const float Height = FMath::Max(1.0f, InOutState.Hull.Height);
    const float Width = FMath::Max(1.0f, InOutState.Hull.Width);
    const float Length = FMath::Max(1.0f, InOutState.Hull.Length);

    FCollisionShape Shape;
    FQuat Rotation = FQuat::Identity;
    float CenterHeight = Height * 0.5f;
    bool bApproximateShape = false;

    switch (InOutState.Hull.HullType)
    {
    case EMetricHullType::Capsule:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        CenterHeight = HalfHeight;
        break;
    }
    case EMetricHullType::OrientedBox:
    {
        const FVector HalfExtent(Length * 0.5f, Width * 0.5f, Height * 0.5f);
        Shape = FCollisionShape::MakeBox(HalfExtent);
        Rotation = FRotationMatrix::MakeFromXZ(Forward, FVector::UpVector).ToQuat();
        CenterHeight = HalfExtent.Z;
        break;
    }
    case EMetricHullType::Sphere:
    {
        const float Radius = FMath::Max3(Width, Length, Height) * 0.5f;
        Shape = FCollisionShape::MakeSphere(FMath::Max(1.0f, Radius));
        CenterHeight = Radius;
        break;
    }
    case EMetricHullType::Cylinder:
    default:
    {
        const float Radius = FMath::Max(1.0f, Width * 0.5f);
        const float HalfHeight = FMath::Max(Radius, Height * 0.5f);
        Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
        CenterHeight = HalfHeight;
        bApproximateShape = true;
        break;
    }
    }

    const float HorizontalSpeed = FMath::Max(1.0f, InOutState.TrajectorySettings.HorizontalSpeed);
    const float LaunchVerticalSpeed = FMath::Max(1.0f, InOutState.TrajectorySettings.LaunchVerticalSpeed);
    const float GravityScale = FMath::Max(0.05f, InOutState.TrajectorySettings.GravityScale);
    const float MaxTime = FMath::Clamp(InOutState.TrajectorySettings.MaxSimulationTime, 0.25f, 10.0f);
    const float Step = FMath::Clamp(InOutState.TrajectorySettings.SampleInterval, 0.01f, 0.2f);
    const float GravityZ = World->GetGravityZ() * GravityScale;
    const float MinLandingNormalZ = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(InOutState.TrajectorySettings.MaxLandingSlope, 0.0f, 89.0f)));

    const FVector StartOffset = InOutState.Hull.LocalOffset + FVector::UpVector * (CenterHeight + 1.0f);
    const FVector StartWorld = InOutState.Location + StartOffset;

    FMetricTrajectoryResult& Result = InOutState.Trajectory;
    Result.bVisible = true;
    Result.bApproximateShape = bApproximateShape;
    Result.SampleOffsets.Add(StartOffset);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricTrajectorySweep), true);
    QueryParams.bTraceComplex = true;

    FVector PreviousWorld = StartWorld;
    float PreviousTime = 0.0f;

    for (float Time = Step; Time <= MaxTime + KINDA_SMALL_NUMBER; Time += Step)
    {
        const float T = FMath::Min(Time, MaxTime);
        const FVector CurrentOffset = StartOffset
            + Forward * (HorizontalSpeed * T)
            + FVector::UpVector * (LaunchVerticalSpeed * T + 0.5f * GravityZ * T * T);
        const FVector CurrentWorld = InOutState.Location + CurrentOffset;

        FHitResult Hit;
        const bool bHit = World->SweepSingleByChannel(
            Hit,
            PreviousWorld,
            CurrentWorld,
            Rotation,
            InOutState.TrajectorySettings.TraceChannel.GetValue(),
            Shape,
            QueryParams);

        if (bHit)
        {
            const float SegmentTime = T - PreviousTime;
            const float HitTime = PreviousTime + SegmentTime * FMath::Clamp(Hit.Time, 0.0f, 1.0f);
            const float VerticalVelocityAtHit = LaunchVerticalSpeed + GravityZ * HitTime;
            const bool bDescending = VerticalVelocityAtHit <= 0.0f;
            const bool bWalkableLanding = Hit.ImpactNormal.GetSafeNormal().Z >= MinLandingNormalZ;

            Result.FlightTime = HitTime;
            Result.HorizontalDistance = HorizontalSpeed * HitTime;
            Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
            Result.ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
            Result.bHasLanding = bDescending && bWalkableLanding;
            Result.bLandingValid = Result.bHasLanding;
            Result.bBlocked = !Result.bLandingValid;

            const FVector ResolvedOffset = Hit.Location - InOutState.Location;
            Result.SampleOffsets.Add(ResolvedOffset);

            // Find the highest sampled hull-center point and measure remaining overhead
            // clearance from the top of the active hull. This is independent of the
            // sweep result so near-ceiling jumps can be identified before collision.
            float HighestZ = -FLT_MAX;
            for (int32 SampleIndex = 0; SampleIndex < Result.SampleOffsets.Num(); ++SampleIndex)
            {
                if (Result.SampleOffsets[SampleIndex].Z > HighestZ)
                {
                    HighestZ = Result.SampleOffsets[SampleIndex].Z;
                    Result.ApexSampleIndex = SampleIndex;
                    Result.ApexOffset = Result.SampleOffsets[SampleIndex];
                }
            }
            Result.ApexHeight = FMath::Max(0.0f, Result.ApexOffset.Z - StartOffset.Z);
            const float ApexProbeDistance = FMath::Max(0.0f, InOutState.TrajectorySettings.ApexProbeDistance);
            if (ApexProbeDistance > KINDA_SMALL_NUMBER)
            {
                const FVector ApexCenterWorld = InOutState.Location + Result.ApexOffset;
                const float HullTopOffset = CenterHeight;
                const FVector ProbeStart = ApexCenterWorld + FVector::UpVector * HullTopOffset;
                const FVector ProbeEnd = ProbeStart + FVector::UpVector * ApexProbeDistance;
                FHitResult ApexHit;
                Result.bApexHasObstacle = World->LineTraceSingleByChannel(
                    ApexHit, ProbeStart, ProbeEnd,
                    InOutState.TrajectorySettings.TraceChannel.GetValue(), QueryParams);
                Result.ApexHeadroom = Result.bApexHasObstacle ? ApexHit.Distance : ApexProbeDistance;
                Result.bApexTight = Result.bApexHasObstacle
                    && Result.ApexHeadroom <= FMath::Max(0.0f, InOutState.TrajectorySettings.ApexWarningMargin);
                if (Result.bApexHasObstacle)
                {
                    Result.ApexClearanceImpactOffset = ApexHit.ImpactPoint - InOutState.Location;
                }
            }
            return;
        }

        Result.SampleOffsets.Add(CurrentOffset);
        PreviousWorld = CurrentWorld;
        PreviousTime = T;

        if (T >= MaxTime - KINDA_SMALL_NUMBER)
        {
            break;
        }
    }

    Result.FlightTime = MaxTime;
    Result.HorizontalDistance = HorizontalSpeed * MaxTime;
    Result.bBlocked = false;
    Result.bHasLanding = false;
    Result.bLandingValid = false;

    float HighestZ = -FLT_MAX;
    for (int32 SampleIndex = 0; SampleIndex < Result.SampleOffsets.Num(); ++SampleIndex)
    {
        if (Result.SampleOffsets[SampleIndex].Z > HighestZ)
        {
            HighestZ = Result.SampleOffsets[SampleIndex].Z;
            Result.ApexSampleIndex = SampleIndex;
            Result.ApexOffset = Result.SampleOffsets[SampleIndex];
        }
    }
    Result.ApexHeight = FMath::Max(0.0f, Result.ApexOffset.Z - StartOffset.Z);
    const float ApexProbeDistance = FMath::Max(0.0f, InOutState.TrajectorySettings.ApexProbeDistance);
    if (ApexProbeDistance > KINDA_SMALL_NUMBER)
    {
        const FVector ApexCenterWorld = InOutState.Location + Result.ApexOffset;
        const float HullTopOffset = CenterHeight;
        const FVector ProbeStart = ApexCenterWorld + FVector::UpVector * HullTopOffset;
        const FVector ProbeEnd = ProbeStart + FVector::UpVector * ApexProbeDistance;
        FHitResult ApexHit;
        Result.bApexHasObstacle = World->LineTraceSingleByChannel(
            ApexHit, ProbeStart, ProbeEnd,
            InOutState.TrajectorySettings.TraceChannel.GetValue(), QueryParams);
        Result.ApexHeadroom = Result.bApexHasObstacle ? ApexHit.Distance : ApexProbeDistance;
        Result.bApexTight = Result.bApexHasObstacle
            && Result.ApexHeadroom <= FMath::Max(0.0f, InOutState.TrajectorySettings.ApexWarningMargin);
        if (Result.bApexHasObstacle)
        {
            Result.ApexClearanceImpactOffset = ApexHit.ImpactPoint - InOutState.Location;
        }
    }
}

void UMetricSubsystem::EvaluateCameraProbe(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.CameraProbe = FMetricCameraProbeResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.CameraProbeSettings.bShowCameraProbe
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    FVector Forward = InOutState.Forward;
    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = FVector::RightVector;
    }

    const FMetricCameraProbeSettings& Settings = InOutState.CameraProbeSettings;
    const float PivotZ = FMath::Max(0.0f, Settings.PivotZOffset);
    const float TargetDistance = FMath::Max(10.0f, Settings.TargetDistance);
    const float ProbeRadius = FMath::Max(1.0f, Settings.ProbeRadius);
    const float MinDistance = FMath::Max(0.0f, Settings.MinDistance);

    const FVector PivotOffset = InOutState.Hull.LocalOffset + FVector::UpVector * PivotZ;
    const FVector DesiredDelta = -Forward * TargetDistance
        + Right * Settings.ShoulderOffset
        + FVector::UpVector * Settings.VerticalOffset;
    const FVector DesiredCameraOffset = PivotOffset + DesiredDelta;
    const float DesiredArmLength = FMath::Max(1.0f, DesiredDelta.Size());

    const FVector PivotWorld = InOutState.Location + PivotOffset;
    const FVector DesiredWorld = InOutState.Location + DesiredCameraOffset;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricCameraProbeSweep), true);
    QueryParams.bTraceComplex = true;

    FHitResult Hit;
    const bool bHit = World->SweepSingleByChannel(
        Hit,
        PivotWorld,
        DesiredWorld,
        FQuat::Identity,
        Settings.TraceChannel.GetValue(),
        FCollisionShape::MakeSphere(ProbeRadius),
        QueryParams);

    FMetricCameraProbeResult& Result = InOutState.CameraProbe;
    Result.bVisible = true;
    Result.TargetDistance = DesiredArmLength;
    Result.PivotOffset = PivotOffset;
    Result.DesiredCameraOffset = DesiredCameraOffset;

    if (bHit)
    {
        Result.bCompressed = true;
        Result.ResolvedCameraOffset = Hit.Location - InOutState.Location;
        Result.ResolvedDistance = FMath::Max(0.0f, (Hit.Location - PivotWorld).Size());
        Result.CompressionPercent = 1.0f - FMath::Clamp(Result.ResolvedDistance / DesiredArmLength, 0.0f, 1.0f);
        Result.bBelowMinimum = Result.ResolvedDistance < MinDistance;
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
        Result.ImpactNormal = Hit.ImpactNormal.GetSafeNormal();
    }
    else
    {
        Result.bCompressed = false;
        Result.bBelowMinimum = false;
        Result.ResolvedCameraOffset = DesiredCameraOffset;
        Result.ResolvedDistance = DesiredArmLength;
        Result.CompressionPercent = 0.0f;
        Result.ImpactPointOffset = DesiredCameraOffset;
        Result.ImpactNormal = FVector::ZeroVector;
    }
}


void UMetricSubsystem::EvaluateCameraSightline(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.CameraSightline = FMetricCameraSightlineResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.CameraSightlineSettings.bShowCameraSightline
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricCameraSightlineSettings& Settings = InOutState.CameraSightlineSettings;

    FVector CameraOffset;
    FVector AimOffset;
    if (InOutState.CameraProbe.bVisible)
    {
        CameraOffset = InOutState.CameraProbe.ResolvedCameraOffset;
        AimOffset = InOutState.CameraProbe.PivotOffset;
    }
    else
    {
        FVector Forward = InOutState.Forward.GetSafeNormal2D();
        if (Forward.IsNearlyZero()) Forward = FVector::ForwardVector;
        CameraOffset = InOutState.Hull.LocalOffset - Forward * 350.0f + FVector::UpVector * 220.0f;
        AimOffset = InOutState.Hull.LocalOffset + FVector::UpVector * (InOutState.Hull.Height * 0.5f);
    }

    const FVector SubjectOffset = InOutState.Hull.LocalOffset + FVector::UpVector * FMath::Max(0.0f, Settings.SubjectZOffset);
    const FVector CameraWorld = InOutState.Location + CameraOffset;
    const FVector AimWorld = InOutState.Location + AimOffset;
    const FVector SubjectWorld = InOutState.Location + SubjectOffset;

    const FVector AimDirection = (AimWorld - CameraWorld).GetSafeNormal();
    const FVector SubjectDirection = (SubjectWorld - CameraWorld).GetSafeNormal();
    if (AimDirection.IsNearlyZero() || SubjectDirection.IsNearlyZero())
    {
        return;
    }

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricCameraSightlineTrace), true);
    QueryParams.bTraceComplex = true;

    FHitResult Hit;
    const float TraceRadius = FMath::Max(0.0f, Settings.TraceRadius);
    bool bHit = false;
    if (TraceRadius > KINDA_SMALL_NUMBER)
    {
        bHit = World->SweepSingleByChannel(
            Hit, CameraWorld, SubjectWorld, FQuat::Identity, Settings.TraceChannel.GetValue(),
            FCollisionShape::MakeSphere(TraceRadius), QueryParams);
    }
    else
    {
        bHit = World->LineTraceSingleByChannel(
            Hit, CameraWorld, SubjectWorld, Settings.TraceChannel.GetValue(), QueryParams);
    }

    FMetricCameraSightlineResult& Result = InOutState.CameraSightline;
    Result.bVisible = true;
    Result.CameraOffset = CameraOffset;
    Result.AimOffset = AimOffset;
    Result.SubjectOffset = SubjectOffset;
    Result.SubjectAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(AimDirection, SubjectDirection), -1.0f, 1.0f)));
    Result.bFramingFail = Result.SubjectAngle > FMath::Clamp(Settings.FramingHalfAngle, 1.0f, 80.0f);
    Result.bOccluded = bHit && Hit.Time < 0.995f;
    if (Result.bOccluded)
    {
        Result.ImpactPointOffset = Hit.ImpactPoint - InOutState.Location;
    }
}

void UMetricSubsystem::EvaluateGroundValidation(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.GroundValidation = FMetricGroundValidationResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.GroundValidationSettings.bShowGroundValidation
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricGroundValidationSettings& Settings = InOutState.GroundValidationSettings;
    FMetricGroundValidationResult& Result = InOutState.GroundValidation;
    Result.bVisible = true;
    Result.bPass = true;

    FVector Forward = InOutState.Forward;
    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = FVector::RightVector;
    }

    const float ForwardDistance = FMath::Max(50.0f, Settings.ForwardDistance);
    const float SampleSpacing = FMath::Max(10.0f, Settings.SampleSpacing);
    const float HalfCoverage = FMath::Max(0.0f, Settings.LateralCoverage * 0.5f);
    const float TraceHeight = FMath::Max(10.0f, Settings.TraceHeight);
    const float TraceDepth = FMath::Max(10.0f, Settings.TraceDepth);
    const float MaxSlope = FMath::Clamp(Settings.MaxWalkableSlope, 0.0f, 89.0f);
    const float MaxStepUp = FMath::Max(0.0f, Settings.MaxStepUp);
    const float MaxStepDown = FMath::Max(0.0f, Settings.MaxStepDown);

    const int32 RowCount = FMath::Max(1, FMath::CeilToInt(ForwardDistance / SampleSpacing));
    const int32 LaneCount = HalfCoverage > KINDA_SMALL_NUMBER ? 3 : 1;
    TArray<float> PreviousZ;
    PreviousZ.Init(InOutState.Location.Z, LaneCount);

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricGroundValidationTrace), true);
    QueryParams.bTraceComplex = true;

    for (int32 Row = 0; Row <= RowCount; ++Row)
    {
        const float ForwardOffset = FMath::Min(ForwardDistance, Row * SampleSpacing);
        for (int32 Lane = 0; Lane < LaneCount; ++Lane)
        {
            const float LateralAlpha = LaneCount == 1 ? 0.0f : (static_cast<float>(Lane) - 1.0f);
            const float LateralOffset = LateralAlpha * HalfCoverage;
            const FVector PlanarOffset = Forward * ForwardOffset + Right * LateralOffset;
            const FVector Target = InOutState.Location + PlanarOffset;
            const FVector TraceStart = Target + FVector::UpVector * TraceHeight;
            const FVector TraceEnd = Target - FVector::UpVector * TraceDepth;

            FHitResult Hit;
            const bool bHit = World->LineTraceSingleByChannel(
                Hit,
                TraceStart,
                TraceEnd,
                Settings.TraceChannel.GetValue(),
                QueryParams);

            FMetricGroundSample& Sample = Result.Samples.AddDefaulted_GetRef();
            Sample.bHit = bHit;
            ++Result.TotalSamples;

            if (!bHit)
            {
                Sample.Offset = PlanarOffset - FVector::UpVector * TraceDepth;
                Sample.Normal = FVector::UpVector;
                Sample.bStepDownFail = true;
                Sample.StepDelta = -TraceDepth;
                ++Result.FailedSamples;
                Result.bPass = false;
                Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, TraceDepth);
                continue;
            }

            Sample.Offset = Hit.ImpactPoint - InOutState.Location;
            Sample.Normal = Hit.ImpactNormal.GetSafeNormal();
            Sample.SlopeDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Sample.Normal.Z, -1.0f, 1.0f)));
            Result.MaxObservedSlope = FMath::Max(Result.MaxObservedSlope, Sample.SlopeDegrees);
            Sample.bSlopeFail = Sample.SlopeDegrees > MaxSlope + KINDA_SMALL_NUMBER;

            // Row zero establishes each lane's own baseline. This avoids falsely
            // classifying a continuous cross-slope as a step just because the
            // left/right lane begins above or below the cursor's center hit.
            const float StepDelta = Row > 0 ? (Hit.ImpactPoint.Z - PreviousZ[Lane]) : 0.0f;
            Sample.StepDelta = StepDelta;
            Sample.bStepUpFail = Row > 0 && StepDelta > MaxStepUp + KINDA_SMALL_NUMBER;
            Sample.bStepDownFail = Row > 0 && StepDelta < -(MaxStepDown + KINDA_SMALL_NUMBER);
            if (Row > 0)
            {
                Result.MaxObservedStepUp = FMath::Max(Result.MaxObservedStepUp, FMath::Max(0.0f, StepDelta));
                Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, FMath::Max(0.0f, -StepDelta));
            }
            PreviousZ[Lane] = Hit.ImpactPoint.Z;

            if (Sample.bSlopeFail || Sample.bStepUpFail || Sample.bStepDownFail)
            {
                ++Result.FailedSamples;
                Result.bPass = false;
            }
        }
    }
}


void UMetricSubsystem::EvaluateFitAnalysis(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.FitAnalysis = FMetricFitAnalysisResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.FitAnalysisSettings.bShowFitAnalysis
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricFitAnalysisSettings& Settings = InOutState.FitAnalysisSettings;
    FMetricFitAnalysisResult& Result = InOutState.FitAnalysis;
    Result.bVisible = true;
    Result.bPass = true;

    FVector Forward = InOutState.Forward;
    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = FVector::RightVector;
    }

    const float RequiredWidth = FMath::Max(1.0f, InOutState.Hull.Width);
    const float RequiredHeight = FMath::Max(1.0f, InOutState.Hull.Height);
    const float RequiredHalfWidth = RequiredWidth * 0.5f;
    const float WarningMargin = FMath::Max(0.0f, Settings.WarningMargin);
    const float ForwardDistance = FMath::Max(0.0f, Settings.ForwardDistance);
    const float SampleSpacing = FMath::Max(10.0f, Settings.SampleSpacing);
    const float SideProbeDistance = FMath::Max(FMath::Max(10.0f, Settings.MaxSideProbeDistance), RequiredHalfWidth + WarningMargin);
    const float CeilingProbeHeight = FMath::Max(FMath::Max(10.0f, Settings.MaxCeilingProbeHeight), RequiredHeight + WarningMargin);
    const float FloorTraceHeight = FMath::Max(10.0f, Settings.FloorTraceHeight);
    const float FloorTraceDepth = FMath::Max(10.0f, Settings.FloorTraceDepth);
    const int32 SampleCount = ForwardDistance <= KINDA_SMALL_NUMBER ? 0 : FMath::Max(1, FMath::CeilToInt(ForwardDistance / SampleSpacing));

    Result.RequiredWidth = RequiredWidth;
    Result.RequiredHeight = RequiredHeight;
    Result.MinimumAvailableWidth = TNumericLimits<float>::Max();
    Result.MinimumAvailableHeight = TNumericLimits<float>::Max();

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricFitAnalysisTrace), true);
    QueryParams.bTraceComplex = true;

    for (int32 SampleIndex = 0; SampleIndex <= SampleCount; ++SampleIndex)
    {
        const float ForwardOffset = ForwardDistance <= KINDA_SMALL_NUMBER
            ? 0.0f
            : FMath::Min(ForwardDistance, SampleIndex * SampleSpacing);
        const FVector PlanarOffset = Forward * ForwardOffset;
        const FVector Station = InOutState.Location + PlanarOffset;

        FMetricFitSample& Sample = Result.Samples.AddDefaulted_GetRef();
        ++Result.TotalSamples;

        FHitResult FloorHit;
        const bool bHasFloor = World->LineTraceSingleByChannel(
            FloorHit,
            Station + FVector::UpVector * FloorTraceHeight,
            Station - FVector::UpVector * FloorTraceDepth,
            Settings.TraceChannel.GetValue(),
            QueryParams);

        Sample.bHasFloor = bHasFloor;
        if (!bHasFloor)
        {
            Sample.BaseOffset = PlanarOffset - FVector::UpVector * FloorTraceDepth;
            ++Result.InvalidSamples;
            ++Result.FailedSamples;
            Result.bPass = false;
            continue;
        }

        const FVector FloorPoint = FloorHit.ImpactPoint;
        Sample.BaseOffset = FloorPoint - InOutState.Location;

        // Measure lateral room at the hull's widest representative height.
        const float WidthMeasureHeight = FMath::Clamp(RequiredHeight * 0.5f, 1.0f, FMath::Max(1.0f, RequiredHeight - 1.0f));
        const FVector WidthCenter = FloorPoint + FVector::UpVector * WidthMeasureHeight;

        FHitResult LeftHit;
        FHitResult RightHit;
        const bool bLeftHit = World->LineTraceSingleByChannel(
            LeftHit,
            WidthCenter,
            WidthCenter - Right * SideProbeDistance,
            Settings.TraceChannel.GetValue(),
            QueryParams);
        const bool bRightHit = World->LineTraceSingleByChannel(
            RightHit,
            WidthCenter,
            WidthCenter + Right * SideProbeDistance,
            Settings.TraceChannel.GetValue(),
            QueryParams);

        const float LeftDistance = bLeftHit ? FMath::Max(0.0f, LeftHit.Distance) : SideProbeDistance;
        const float RightDistance = bRightHit ? FMath::Max(0.0f, RightHit.Distance) : SideProbeDistance;
        Sample.bLeftBounded = bLeftHit;
        Sample.bRightBounded = bRightHit;
        Sample.LeftOffset = (bLeftHit ? LeftHit.Location : WidthCenter - Right * SideProbeDistance) - InOutState.Location;
        Sample.RightOffset = (bRightHit ? RightHit.Location : WidthCenter + Right * SideProbeDistance) - InOutState.Location;
        Sample.AvailableWidth = LeftDistance + RightDistance;
        Sample.WidthDeficit = FMath::Max(0.0f, RequiredHalfWidth - LeftDistance)
            + FMath::Max(0.0f, RequiredHalfWidth - RightDistance);
        Sample.bWidthFail = Sample.WidthDeficit > KINDA_SMALL_NUMBER;

        // Measure floor-to-ceiling room independently of the width rays.
        FHitResult CeilingHit;
        const FVector CeilingStart = FloorPoint + FVector::UpVector * 1.0f;
        const bool bCeilingHit = World->LineTraceSingleByChannel(
            CeilingHit,
            CeilingStart,
            FloorPoint + FVector::UpVector * CeilingProbeHeight,
            Settings.TraceChannel.GetValue(),
            QueryParams);

        const float AvailableHeight = bCeilingHit
            ? FMath::Max(0.0f, CeilingHit.Location.Z - FloorPoint.Z)
            : CeilingProbeHeight;
        Sample.bCeilingBounded = bCeilingHit;
        Sample.CeilingOffset = (bCeilingHit ? CeilingHit.Location : FloorPoint + FVector::UpVector * CeilingProbeHeight) - InOutState.Location;
        Sample.AvailableHeight = AvailableHeight;
        Sample.HeightDeficit = bCeilingHit ? FMath::Max(0.0f, RequiredHeight - AvailableHeight) : 0.0f;
        Sample.bHeightFail = Sample.HeightDeficit > KINDA_SMALL_NUMBER;

        const bool bWidthNear = !Sample.bWidthFail
            && (LeftDistance < RequiredHalfWidth + WarningMargin || RightDistance < RequiredHalfWidth + WarningMargin);
        const bool bHeightNear = !Sample.bHeightFail
            && bCeilingHit
            && AvailableHeight < RequiredHeight + WarningMargin;
        Sample.bNearThreshold = bWidthNear || bHeightNear;

        Result.MinimumAvailableWidth = FMath::Min(Result.MinimumAvailableWidth, Sample.AvailableWidth);
        Result.MinimumAvailableHeight = FMath::Min(Result.MinimumAvailableHeight, Sample.AvailableHeight);
        Result.MaxWidthDeficit = FMath::Max(Result.MaxWidthDeficit, Sample.WidthDeficit);
        Result.MaxHeightDeficit = FMath::Max(Result.MaxHeightDeficit, Sample.HeightDeficit);
        Result.bNearThreshold = Result.bNearThreshold || Sample.bNearThreshold;

        if (Sample.bWidthFail || Sample.bHeightFail)
        {
            ++Result.FailedSamples;
            Result.bPass = false;
        }
    }

    if (Result.MinimumAvailableWidth == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableWidth = 0.0f;
    }
    if (Result.MinimumAvailableHeight == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableHeight = 0.0f;
    }
}


void UMetricSubsystem::EvaluateThresholdAnalysis(UWorld* World, FMetricLiveState& InOutState) const
{
    InOutState.ThresholdAnalysis = FMetricThresholdAnalysisResult();

    if (!World
        || !InOutState.bHasHit
        || !InOutState.ThresholdAnalysisSettings.bShowThresholdAnalysis
        || InOutState.EntryType == EMetricCycleEntryType::NativePreviewMesh)
    {
        return;
    }

    const FMetricThresholdAnalysisSettings& Settings = InOutState.ThresholdAnalysisSettings;
    FMetricThresholdAnalysisResult& Result = InOutState.ThresholdAnalysis;
    Result.bVisible = true;
    Result.bPass = true;

    FVector Forward = InOutState.Forward;
    Forward.Z = 0.0f;
    Forward = Forward.GetSafeNormal();
    if (Forward.IsNearlyZero())
    {
        Forward = FVector::ForwardVector;
    }

    FVector Right = FVector::CrossProduct(FVector::UpVector, Forward).GetSafeNormal();
    if (Right.IsNearlyZero())
    {
        Right = FVector::RightVector;
    }

    const float RequiredWidth = FMath::Max(1.0f, InOutState.Hull.Width);
    const float RequiredHeight = FMath::Max(1.0f, InOutState.Hull.Height);
    const float RequiredHalfWidth = RequiredWidth * 0.5f;
    const float ProbeDepth = FMath::Max(10.0f, Settings.ProbeDepth);
    const float SideProbeDistance = FMath::Max(FMath::Max(10.0f, Settings.MaxSideProbeDistance), RequiredHalfWidth + Settings.WarningMargin);
    const float CeilingProbeHeight = FMath::Max(FMath::Max(10.0f, Settings.MaxCeilingProbeHeight), RequiredHeight + Settings.WarningMargin);
    const float WarningMargin = FMath::Max(0.0f, Settings.WarningMargin);
    const float MaxStepUp = FMath::Max(0.0f, Settings.MaxStepUp);
    const float MaxStepDown = FMath::Max(0.0f, Settings.MaxStepDown);
    const float FloorTraceHeight = FMath::Max(10.0f, Settings.FloorTraceHeight);
    const float FloorTraceDepth = FMath::Max(10.0f, Settings.FloorTraceDepth);

    Result.RequiredWidth = RequiredWidth;
    Result.RequiredHeight = RequiredHeight;
    Result.MinimumAvailableWidth = TNumericLimits<float>::Max();
    Result.MinimumAvailableHeight = TNumericLimits<float>::Max();

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MetricThresholdAnalysisTrace), true);
    QueryParams.bTraceComplex = true;

    bool bHavePreviousFloor = false;
    float PreviousFloorZ = 0.0f;

    // Entry, midpoint, and exit. Threshold analysis is intentionally a compact
    // transition test rather than another long corridor sampler.
    for (int32 SampleIndex = 0; SampleIndex < 3; ++SampleIndex)
    {
        const float Alpha = static_cast<float>(SampleIndex) * 0.5f;
        const float ForwardOffset = ProbeDepth * Alpha;
        const FVector Station = InOutState.Location + Forward * ForwardOffset;

        FMetricThresholdSample& Sample = Result.Samples.AddDefaulted_GetRef();
        ++Result.TotalSamples;

        FHitResult FloorHit;
        const bool bHasFloor = World->LineTraceSingleByChannel(
            FloorHit,
            Station + FVector::UpVector * FloorTraceHeight,
            Station - FVector::UpVector * FloorTraceDepth,
            Settings.TraceChannel.GetValue(),
            QueryParams);

        Sample.bHasFloor = bHasFloor;
        if (!bHasFloor)
        {
            Sample.BaseOffset = Forward * ForwardOffset - FVector::UpVector * FloorTraceDepth;
            Sample.bStepDownFail = true;
            Sample.StepDelta = -FloorTraceDepth;
            ++Result.InvalidSamples;
            ++Result.FailedSamples;
            Result.bPass = false;
            Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, FloorTraceDepth);
            bHavePreviousFloor = false;
            continue;
        }

        const FVector FloorPoint = FloorHit.ImpactPoint;
        Sample.BaseOffset = FloorPoint - InOutState.Location;

        if (bHavePreviousFloor)
        {
            Sample.StepDelta = FloorPoint.Z - PreviousFloorZ;
            Sample.bStepUpFail = Sample.StepDelta > MaxStepUp + KINDA_SMALL_NUMBER;
            Sample.bStepDownFail = Sample.StepDelta < -(MaxStepDown + KINDA_SMALL_NUMBER);
            Result.MaxObservedStepUp = FMath::Max(Result.MaxObservedStepUp, FMath::Max(0.0f, Sample.StepDelta));
            Result.MaxObservedStepDown = FMath::Max(Result.MaxObservedStepDown, FMath::Max(0.0f, -Sample.StepDelta));
        }
        PreviousFloorZ = FloorPoint.Z;
        bHavePreviousFloor = true;

        const float WidthMeasureHeight = FMath::Clamp(RequiredHeight * 0.5f, 1.0f, FMath::Max(1.0f, RequiredHeight - 1.0f));
        const FVector WidthCenter = FloorPoint + FVector::UpVector * WidthMeasureHeight;

        FHitResult LeftHit;
        FHitResult RightHit;
        const bool bLeftHit = World->LineTraceSingleByChannel(
            LeftHit, WidthCenter, WidthCenter - Right * SideProbeDistance, Settings.TraceChannel.GetValue(), QueryParams);
        const bool bRightHit = World->LineTraceSingleByChannel(
            RightHit, WidthCenter, WidthCenter + Right * SideProbeDistance, Settings.TraceChannel.GetValue(), QueryParams);

        const float LeftDistance = bLeftHit ? FMath::Max(0.0f, LeftHit.Distance) : SideProbeDistance;
        const float RightDistance = bRightHit ? FMath::Max(0.0f, RightHit.Distance) : SideProbeDistance;
        Sample.LeftOffset = (bLeftHit ? LeftHit.Location : WidthCenter - Right * SideProbeDistance) - InOutState.Location;
        Sample.RightOffset = (bRightHit ? RightHit.Location : WidthCenter + Right * SideProbeDistance) - InOutState.Location;
        Sample.AvailableWidth = LeftDistance + RightDistance;
        Sample.WidthDeficit = FMath::Max(0.0f, RequiredHalfWidth - LeftDistance)
            + FMath::Max(0.0f, RequiredHalfWidth - RightDistance);
        Sample.bWidthFail = Sample.WidthDeficit > KINDA_SMALL_NUMBER;

        FHitResult CeilingHit;
        const bool bCeilingHit = World->LineTraceSingleByChannel(
            CeilingHit,
            FloorPoint + FVector::UpVector * 1.0f,
            FloorPoint + FVector::UpVector * CeilingProbeHeight,
            Settings.TraceChannel.GetValue(),
            QueryParams);
        Sample.CeilingOffset = (bCeilingHit ? CeilingHit.Location : FloorPoint + FVector::UpVector * CeilingProbeHeight) - InOutState.Location;
        Sample.AvailableHeight = bCeilingHit ? FMath::Max(0.0f, CeilingHit.Location.Z - FloorPoint.Z) : CeilingProbeHeight;
        Sample.HeightDeficit = bCeilingHit ? FMath::Max(0.0f, RequiredHeight - Sample.AvailableHeight) : 0.0f;
        Sample.bHeightFail = Sample.HeightDeficit > KINDA_SMALL_NUMBER;

        const bool bWidthNear = !Sample.bWidthFail
            && (LeftDistance < RequiredHalfWidth + WarningMargin || RightDistance < RequiredHalfWidth + WarningMargin);
        const bool bHeightNear = !Sample.bHeightFail
            && bCeilingHit
            && Sample.AvailableHeight < RequiredHeight + WarningMargin;
        Sample.bNearThreshold = bWidthNear || bHeightNear;

        Result.MinimumAvailableWidth = FMath::Min(Result.MinimumAvailableWidth, Sample.AvailableWidth);
        Result.MinimumAvailableHeight = FMath::Min(Result.MinimumAvailableHeight, Sample.AvailableHeight);
        Result.MaxWidthDeficit = FMath::Max(Result.MaxWidthDeficit, Sample.WidthDeficit);
        Result.MaxHeightDeficit = FMath::Max(Result.MaxHeightDeficit, Sample.HeightDeficit);
        Result.bNearThreshold = Result.bNearThreshold || Sample.bNearThreshold;

        if (Sample.bWidthFail || Sample.bHeightFail || Sample.bStepUpFail || Sample.bStepDownFail)
        {
            ++Result.FailedSamples;
            Result.bPass = false;
        }
    }

    if (Result.MinimumAvailableWidth == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableWidth = 0.0f;
    }
    if (Result.MinimumAvailableHeight == TNumericLimits<float>::Max())
    {
        Result.MinimumAvailableHeight = 0.0f;
    }
}

void UMetricSubsystem::EnsureRenderHost(UWorld* World)
{
    if (!World)
    {
        return;
    }

    if (AMetricRenderHostActor* ExistingHost = RenderHost.Get())
    {
        if (IsValid(ExistingHost) && ExistingHost->GetWorld() == World)
        {
            return;
        }
    }

    ClearRenderHost();

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = MakeUniqueObjectName(World, AMetricRenderHostActor::StaticClass(), TEXT("MetricRenderHost"));
    SpawnParams.ObjectFlags |= RF_Transient;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.bHideFromSceneOutliner = true;
    SpawnParams.bTemporaryEditorActor = true;

    RenderHost = World->SpawnActor<AMetricRenderHostActor>(AMetricRenderHostActor::StaticClass(), FTransform::Identity, SpawnParams);
}

void UMetricSubsystem::ClearRenderHost()
{
    // Reset the subsystem handle first. Even if actor destruction triggers GC or
    // world callbacks, Metric can no longer be part of a strong reference chain.
    AMetricRenderHostActor* Host = RenderHost.Get();
    RenderHost.Reset();

    if (!IsValid(Host))
    {
        return;
    }

    UWorld* World = Host->GetWorld();
    if (!World || World->IsBeingCleanedUp())
    {
        return;
    }

    Host->SetMetricVisible(false);
    World->DestroyActor(Host, false, true);
}

void UMetricSubsystem::NotifyStateChanged()
{
    if (GEditor)
    {
        GEditor->RedrawAllViewports(false);
    }
}

#undef LOCTEXT_NAMESPACE
