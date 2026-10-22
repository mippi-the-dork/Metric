#include "MetricRenderHostActor.h"

#include "MetricViewportComponent.h"
#include "MetricEditorSettings.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "UObject/UObjectGlobals.h"

AMetricRenderHostActor::AMetricRenderHostActor()
{
    SetFlags(RF_Transient);
    bIsEditorOnlyActor = true;
    SetActorEnableCollision(false);
    SetCanBeDamaged(false);

    MetricComponent = CreateDefaultSubobject<UMetricViewportComponent>(TEXT("MetricViewport"));
    RootComponent = MetricComponent;

    PinnedMetricComponent = CreateDefaultSubobject<UMetricViewportComponent>(TEXT("PinnedMetricViewport"));
    PinnedMetricComponent->SetupAttachment(RootComponent);
    PinnedMetricComponent->SetIsVisualizationComponent(true);
    PinnedMetricComponent->SetVisibility(false, true);

    PreviewMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MetricPreviewMesh"));
    PreviewMeshComponent->SetupAttachment(RootComponent);
    PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PreviewMeshComponent->SetGenerateOverlapEvents(false);
    PreviewMeshComponent->SetCastShadow(false);
    PreviewMeshComponent->SetHiddenInGame(true);
    PreviewMeshComponent->SetIsVisualizationComponent(true);
    PreviewMeshComponent->SetVisibility(false, true);
    PreviewMeshComponent->Mobility = EComponentMobility::Movable;

    LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MetricLabel"));
    LabelComponent->SetupAttachment(RootComponent);
    LabelComponent->SetHorizontalAlignment(EHTA_Center);
    LabelComponent->SetVerticalAlignment(EVRTA_TextCenter);
    const UMetricEditorSettings* MetricSettings = GetDefault<UMetricEditorSettings>();
    LabelComponent->SetWorldSize(MetricSettings ? MetricSettings->LabelWorldSize : 18.0f);
    LabelComponent->SetTextRenderColor(FColor::White);
    LabelComponent->SetHiddenInGame(true);
    LabelComponent->SetIsVisualizationComponent(true);
    LabelComponent->SetVisibility(false, true);
    LabelComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));

    LegendComponent = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MetricLegend"));
    LegendComponent->SetupAttachment(RootComponent);
    LegendComponent->SetHorizontalAlignment(EHTA_Center);
    LegendComponent->SetVerticalAlignment(EVRTA_TextCenter);
    LegendComponent->SetWorldSize(11.0f);
    const FLinearColor LegendLinearColor = MetricSettings ? MetricSettings->LegendTextColor : FLinearColor(0.82f, 0.86f, 0.92f, 1.0f);
    LegendComponent->SetTextRenderColor(LegendLinearColor.ToFColor(true));
    LegendComponent->SetHiddenInGame(true);
    LegendComponent->SetIsVisualizationComponent(true);
    LegendComponent->SetVisibility(false, true);
    LegendComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 275.0f));
}

void AMetricRenderHostActor::ApplyPinnedMetricState(const FMetricLiveState* State)
{
    if (!PinnedMetricComponent)
    {
        return;
    }

    if (!State || !State->bHasHit)
    {
        PinnedMetricComponent->SetVisibility(false, true);
        return;
    }

    const FVector RelativeLocation = State->Location - GetActorLocation();
    PinnedMetricComponent->SetRelativeLocation(RelativeLocation);

    FMetricLiveState LocalState = *State;
    LocalState.Location = FVector::ZeroVector;
    LocalState.PinnedBenchmarks.Reset();
    LocalState.SelectedPinnedBenchmarkIndex = INDEX_NONE;
    PinnedMetricComponent->SetMetricState(LocalState);
    PinnedMetricComponent->SetVisibility(true, true);
}

void AMetricRenderHostActor::EnsureUnlitTextMaterial()
{
    if (UnlitTextMaterial)
    {
        if (LabelComponent)
        {
            LabelComponent->SetTextMaterial(UnlitTextMaterial);
        }
        if (LegendComponent)
        {
            LegendComponent->SetTextMaterial(UnlitTextMaterial);
        }
        return;
    }

    if (!LabelComponent)
    {
        return;
    }

    // Unreal's stock TextRender material is Default Lit. Metric duplicates it
    // transiently, preserves its font/opacity graph, moves the Base Color input
    // to Emissive, and switches the copy to Unlit so viewport lighting can never
    // dim or tint Metric's readout. Nothing is saved into the project.
    UMaterial* SourceMaterial = LoadObject<UMaterial>(
        nullptr,
        TEXT("/Engine/EngineMaterials/DefaultTextMaterialOpaque.DefaultTextMaterialOpaque"));
    if (!SourceMaterial)
    {
        return;
    }

    UMaterial* MetricMaterial = DuplicateObject<UMaterial>(
        SourceMaterial,
        GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UMaterial::StaticClass(), TEXT("MetricUnlitTextMaterial")));
    if (!MetricMaterial)
    {
        return;
    }

    MetricMaterial->SetFlags(RF_Transient);
    MetricMaterial->SetShadingModel(MSM_Unlit);

#if WITH_EDITORONLY_DATA
    if (UMaterialEditorOnlyData* EditorData = MetricMaterial->GetEditorOnlyData())
    {
        EditorData->EmissiveColor = EditorData->BaseColor;
        EditorData->BaseColor = FColorMaterialInput();
    }
#endif

    MetricMaterial->PostEditChange();
    UnlitTextMaterial = MetricMaterial;
    LabelComponent->SetTextMaterial(UnlitTextMaterial);
    if (LegendComponent)
    {
        LegendComponent->SetTextMaterial(UnlitTextMaterial);
    }
}

void AMetricRenderHostActor::ApplyMetricState(const FMetricLiveState& State)
{
    SetActorLocation(State.Location);

    if (MetricComponent)
    {
        FMetricLiveState LocalState = State;
        LocalState.Location = FVector::ZeroVector;
        for (FMetricPinnedBenchmark& Pin : LocalState.PinnedBenchmarks)
        {
            Pin.WorldLocation -= State.Location;
        }
        MetricComponent->SetMetricState(LocalState);
        MetricComponent->SetVisibility(State.bHasHit || !State.PinnedBenchmarks.IsEmpty(), true);
    }

    if (PreviewMeshComponent)
    {
        UStaticMesh* StaticMesh = Cast<UStaticMesh>(State.PreviewMesh.TryLoad());
        PreviewMeshComponent->SetStaticMesh(StaticMesh);

        FTransform PreviewTransform = State.PreviewRelativeTransform;
        if (StaticMesh && State.EntryType == EMetricCycleEntryType::NativePreviewMesh)
        {
            const FBox MeshBox = StaticMesh->GetBoundingBox();
            FVector Translation = PreviewTransform.GetTranslation();
            Translation.Z -= MeshBox.Min.Z * PreviewTransform.GetScale3D().Z;
            PreviewTransform.SetTranslation(Translation);
        }

        PreviewMeshComponent->SetRelativeTransform(PreviewTransform);
        PreviewMeshComponent->SetVisibility(State.bHasHit && StaticMesh != nullptr, true);
    }

    if (LabelComponent)
    {
        EnsureUnlitTextMaterial();

        const UMetricEditorSettings* MetricSettings = GetDefault<UMetricEditorSettings>();
        const float LabelPadding = MetricSettings ? MetricSettings->LabelHeightPadding : 35.0f;
        const float LabelWorldSize = MetricSettings ? MetricSettings->LabelWorldSize : 18.0f;
        const FVector RelativeLabelLocation(0.0f, 0.0f, FMath::Max(220.0f, State.Hull.Height + LabelPadding));
        LabelComponent->SetWorldSize(LabelWorldSize);

        FText DisplayText = State.Label;
        if (State.Clearance.bVisible)
        {
            const FString Prefix = State.Clearance.bApproximateShape ? TEXT("EST ") : TEXT("");
            FString ClearanceLine;
            if (State.Clearance.bBlocked)
            {
                ClearanceLine = FString::Printf(
                    TEXT("%sBLOCKED %.0fu / %.0fu"),
                    *Prefix,
                    State.Clearance.ClearDistance,
                    State.Clearance.ProbeDistance);
            }
            else if (State.Clearance.bNearThreshold)
            {
                const float ExtraDistance = FMath::Max(0.0f, State.Clearance.ObstacleDistance - State.Clearance.ProbeDistance);
                ClearanceLine = FString::Printf(
                    TEXT("%sCLEAR %.0fu  |  obstacle +%.0fu"),
                    *Prefix,
                    State.Clearance.ProbeDistance,
                    ExtraDistance);
            }
            else
            {
                ClearanceLine = FString::Printf(
                    TEXT("%sCLEAR %.0fu+"),
                    *Prefix,
                    State.Clearance.ProbeDistance);
            }

            DisplayText = FText::FromString(State.Label.ToString() + TEXT("\n") + ClearanceLine);
        }

        if (State.SensorField.bVisible)
        {
            FString SensorFieldLine;
            if (State.SensorField.BlockedRays == 0)
            {
                SensorFieldLine = FString::Printf(TEXT("SENSOR OPEN  %.0fu / %.0f deg"), State.SensorFieldSettings.Range, State.SensorFieldSettings.HorizontalHalfAngle * 2.0f);
            }
            else
            {
                SensorFieldLine = FString::Printf(
                    TEXT("SENSOR %.0f%% CLEAR  |  %d/%d blocked  |  nearest %.0fu"),
                    State.SensorField.ClearPercent,
                    State.SensorField.BlockedRays,
                    State.SensorField.TotalRays,
                    State.SensorField.NearestBlockDistance);
            }
            DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + SensorFieldLine);
        }

        if (State.Trajectory.bVisible)
        {
            const FString Prefix = State.Trajectory.bApproximateShape ? TEXT("EST ") : TEXT("");
            FString TrajectoryLine;
            if (State.Trajectory.bBlocked)
            {
                TrajectoryLine = FString::Printf(
                    TEXT("%sARC BLOCKED  %.1fs / %.0fu"),
                    *Prefix,
                    State.Trajectory.FlightTime,
                    State.Trajectory.HorizontalDistance);
            }
            else if (State.Trajectory.bLandingValid)
            {
                TrajectoryLine = FString::Printf(
                    TEXT("%sLANDING OK  %.1fs / %.0fu"),
                    *Prefix,
                    State.Trajectory.FlightTime,
                    State.Trajectory.HorizontalDistance);
            }
            else
            {
                TrajectoryLine = FString::Printf(
                    TEXT("%sNO LANDING  %.1fs / %.0fu"),
                    *Prefix,
                    State.Trajectory.FlightTime,
                    State.Trajectory.HorizontalDistance);
            }

            DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + TrajectoryLine);

            if (State.Trajectory.ApexSampleIndex != INDEX_NONE && State.TrajectorySettings.ApexProbeDistance > KINDA_SMALL_NUMBER)
            {
                FString ApexLine;
                if (State.Trajectory.bApexHasObstacle)
                {
                    ApexLine = FString::Printf(
                        TEXT("APEX +%.0fu  |  HEADROOM %.0fu%s"),
                        State.Trajectory.ApexHeight,
                        State.Trajectory.ApexHeadroom,
                        State.Trajectory.bApexTight ? TEXT(" TIGHT") : TEXT(""));
                }
                else
                {
                    ApexLine = FString::Printf(
                        TEXT("APEX +%.0fu  |  HEADROOM %.0fu+"),
                        State.Trajectory.ApexHeight,
                        State.TrajectorySettings.ApexProbeDistance);
                }
                DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + ApexLine);
            }
        }

        if (State.CameraProbe.bVisible)
        {
            FString CameraLine;
            if (!State.CameraProbe.bCompressed)
            {
                CameraLine = FString::Printf(
                    TEXT("CAM CLEAR  %.0fu"),
                    State.CameraProbe.TargetDistance);
            }
            else if (State.CameraProbe.bBelowMinimum)
            {
                CameraLine = FString::Printf(
                    TEXT("CAM TOO CLOSE  %.0fu / %.0fu"),
                    State.CameraProbe.ResolvedDistance,
                    State.CameraProbe.TargetDistance);
            }
            else
            {
                CameraLine = FString::Printf(
                    TEXT("CAM COMPRESSED  %.0fu / %.0fu  (%.0f%%)"),
                    State.CameraProbe.ResolvedDistance,
                    State.CameraProbe.TargetDistance,
                    State.CameraProbe.CompressionPercent * 100.0f);
            }
            DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + CameraLine);
        }


        if (State.CameraSightline.bVisible)
        {
            FString SightLine;
            if (State.CameraSightline.bOccluded && State.CameraSightline.bFramingFail)
            {
                SightLine = FString::Printf(TEXT("VIEW BLOCKED + FRAMING FAIL  %.1f deg"), State.CameraSightline.SubjectAngle);
            }
            else if (State.CameraSightline.bOccluded)
            {
                SightLine = TEXT("VIEW OCCLUDED");
            }
            else if (State.CameraSightline.bFramingFail)
            {
                SightLine = FString::Printf(TEXT("FRAMING FAIL  %.1f deg"), State.CameraSightline.SubjectAngle);
            }
            else
            {
                SightLine = FString::Printf(TEXT("VIEW OK  %.1f deg"), State.CameraSightline.SubjectAngle);
            }
            DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + SightLine);
        }

        if (State.GroundValidation.bVisible)
        {
            FString GroundLine;
            if (State.GroundValidation.bPass)
            {
                GroundLine = FString::Printf(
                    TEXT("GROUND OK  %d/%d"),
                    State.GroundValidation.TotalSamples,
                    State.GroundValidation.TotalSamples);
            }
            else
            {
                GroundLine = FString::Printf(
                    TEXT("GROUND FAIL  %d/%d  |  slope %.0f deg  |  up %.0fu  down %.0fu"),
                    State.GroundValidation.FailedSamples,
                    State.GroundValidation.TotalSamples,
                    State.GroundValidation.MaxObservedSlope,
                    State.GroundValidation.MaxObservedStepUp,
                    State.GroundValidation.MaxObservedStepDown);
            }
            DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + GroundLine);
        }


        if (State.FitAnalysis.bVisible)
        {
            FString FitLine;
            if (State.FitAnalysis.InvalidSamples > 0
                && State.FitAnalysis.MaxWidthDeficit <= KINDA_SMALL_NUMBER
                && State.FitAnalysis.MaxHeightDeficit <= KINDA_SMALL_NUMBER)
            {
                FitLine = FString::Printf(
                    TEXT("FIT NO FLOOR  %d/%d"),
                    State.FitAnalysis.InvalidSamples,
                    State.FitAnalysis.TotalSamples);
            }
            else if (!State.FitAnalysis.bPass)
            {
                FitLine = FString::Printf(
                    TEXT("FIT FAIL  %d/%d  |  width -%.0fu  height -%.0fu"),
                    State.FitAnalysis.FailedSamples,
                    State.FitAnalysis.TotalSamples,
                    State.FitAnalysis.MaxWidthDeficit,
                    State.FitAnalysis.MaxHeightDeficit);
            }
            else if (State.FitAnalysis.bNearThreshold)
            {
                FitLine = FString::Printf(
                    TEXT("FIT TIGHT  |  min %.0fu W / %.0fu H"),
                    State.FitAnalysis.MinimumAvailableWidth,
                    State.FitAnalysis.MinimumAvailableHeight);
            }
            else
            {
                FitLine = FString::Printf(
                    TEXT("FIT OK  |  min %.0fu W / %.0fu H"),
                    State.FitAnalysis.MinimumAvailableWidth,
                    State.FitAnalysis.MinimumAvailableHeight);
            }
            DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + FitLine);
        }

        if (State.ThresholdAnalysis.bVisible)
        {
            FString ThresholdLine;
            if (State.ThresholdAnalysis.InvalidSamples > 0
                && State.ThresholdAnalysis.MaxWidthDeficit <= KINDA_SMALL_NUMBER
                && State.ThresholdAnalysis.MaxHeightDeficit <= KINDA_SMALL_NUMBER)
            {
                ThresholdLine = FString::Printf(
                    TEXT("THRESHOLD NO FLOOR  %d/%d"),
                    State.ThresholdAnalysis.InvalidSamples,
                    State.ThresholdAnalysis.TotalSamples);
            }
            else if (!State.ThresholdAnalysis.bPass)
            {
                ThresholdLine = FString::Printf(
                    TEXT("THRESHOLD FAIL  %d/%d  |  W -%.0fu H -%.0fu  |  step +%.0f/-%.0fu"),
                    State.ThresholdAnalysis.FailedSamples,
                    State.ThresholdAnalysis.TotalSamples,
                    State.ThresholdAnalysis.MaxWidthDeficit,
                    State.ThresholdAnalysis.MaxHeightDeficit,
                    State.ThresholdAnalysis.MaxObservedStepUp,
                    State.ThresholdAnalysis.MaxObservedStepDown);
            }
            else if (State.ThresholdAnalysis.bNearThreshold)
            {
                ThresholdLine = FString::Printf(
                    TEXT("THRESHOLD TIGHT  |  min %.0fu W / %.0fu H"),
                    State.ThresholdAnalysis.MinimumAvailableWidth,
                    State.ThresholdAnalysis.MinimumAvailableHeight);
            }
            else
            {
                ThresholdLine = FString::Printf(
                    TEXT("THRESHOLD OK  |  min %.0fu W / %.0fu H"),
                    State.ThresholdAnalysis.MinimumAvailableWidth,
                    State.ThresholdAnalysis.MinimumAvailableHeight);
            }
            DisplayText = FText::FromString(DisplayText.ToString() + TEXT("\n") + ThresholdLine);
        }

        LabelComponent->SetText(DisplayText);
        LabelComponent->SetRelativeLocation(RelativeLabelLocation);

        const FVector LabelWorldLocation = State.Location + RelativeLabelLocation;
        const FVector ToViewer = (State.ViewerLocation - LabelWorldLocation).GetSafeNormal();
        if (!ToViewer.IsNearlyZero())
        {
            LabelComponent->SetWorldRotation(FRotationMatrix::MakeFromX(ToViewer).Rotator());
        }

        // Detailed Metric readout moved to the screen-space viewport HUD in 0.16.0.
        LabelComponent->SetVisibility(false, true);
    }

    if (LegendComponent)
    {
        EnsureUnlitTextMaterial();

        const UMetricEditorSettings* MetricSettings = GetDefault<UMetricEditorSettings>();
        const bool bShowLegend = MetricSettings ? MetricSettings->bShowLegend : true;
        const float LegendWorldSize = 11.0f;
        const float LegendHeightOffset = 55.0f;
        const FLinearColor LegendLinearColor = MetricSettings ? MetricSettings->LegendTextColor : FLinearColor(0.82f, 0.86f, 0.92f, 1.0f);

        FString LegendText;
        if (State.EntryType == EMetricCycleEntryType::NativePreviewMesh)
        {
            LegendText = TEXT("VISUAL SCALE REFERENCE ONLY");
        }
        else
        {
            TArray<FString> ActiveLayers;
            if (State.bShowHull)
            {
                ActiveLayers.Add(TEXT("HULL"));
            }
            if (State.Clearance.bVisible)
            {
                ActiveLayers.Add(TEXT("CLEARANCE"));
            }
            if (State.Sensors.bShowSensorHeights)
            {
                ActiveLayers.Add(TEXT("SENSORS"));
            }
            if (State.SensorField.bVisible)
            {
                ActiveLayers.Add(TEXT("FOV"));
            }
            if (State.Ranges.bShowEngagementRanges)
            {
                ActiveLayers.Add(TEXT("ENGAGEMENT"));
            }
            if (State.Locomotion.bShowLocomotionBands)
            {
                ActiveLayers.Add(TEXT("MOVE"));
            }
            if (State.Trajectory.bVisible)
            {
                ActiveLayers.Add(TEXT("ARC"));
            }
            if (State.CameraProbe.bVisible)
            {
                ActiveLayers.Add(TEXT("CAMERA"));
            }
            if (State.CameraSightline.bVisible)
            {
                ActiveLayers.Add(TEXT("VIEW"));
            }
            if (State.GroundValidation.bVisible)
            {
                ActiveLayers.Add(TEXT("GROUND"));
            }
            if (State.FitAnalysis.bVisible)
            {
                ActiveLayers.Add(TEXT("FIT"));
            }
            if (State.ThresholdAnalysis.bVisible)
            {
                ActiveLayers.Add(TEXT("THRESHOLD"));
            }
            if (!State.PinnedBenchmarks.IsEmpty())
            {
                ActiveLayers.Add(TEXT("PINS"));
            }

            LegendText = FString::Printf(TEXT("ACTIVE: %s"), *FString::Join(ActiveLayers, TEXT(" | ")));

            TArray<FString> Semantics;
            if (State.bShowHull)
            {
                Semantics.Add(TEXT("CYAN hull"));
            }
            if (State.Clearance.bVisible)
            {
                Semantics.Add(TEXT("GREEN/AMBER/RED clearance"));
            }
            if (State.Sensors.bShowSensorHeights || State.Ranges.bShowEngagementRanges)
            {
                Semantics.Add(TEXT("BLUE info"));
            }
            if (State.SensorField.bVisible)
            {
                Semantics.Add(TEXT("BLUE/RED sensor field"));
            }
            if (State.Locomotion.bShowLocomotionBands)
            {
                Semantics.Add(TEXT("PURPLE movement"));
            }
            if (State.Trajectory.bVisible)
            {
                Semantics.Add(TEXT("ORANGE/GREEN/RED arc + envelope"));
            }
            if (State.CameraProbe.bVisible)
            {
                Semantics.Add(TEXT("MAGENTA camera | RED compressed"));
            }
            if (State.CameraSightline.bVisible)
            {
                Semantics.Add(TEXT("LILAC view | RED blocked/framing"));
            }
            if (State.GroundValidation.bVisible)
            {
                Semantics.Add(TEXT("GREEN/AMBER/RED ground"));
            }
            if (State.FitAnalysis.bVisible)
            {
                Semantics.Add(TEXT("GREEN/AMBER/RED fit"));
            }
            if (State.ThresholdAnalysis.bVisible)
            {
                Semantics.Add(TEXT("GREEN/AMBER/RED threshold"));
            }
            if (!State.PinnedBenchmarks.IsEmpty())
            {
                Semantics.Add(TEXT("YELLOW pinned benchmark"));
            }

            if (!Semantics.IsEmpty())
            {
                LegendText += TEXT("\n") + FString::Join(Semantics, TEXT(" | "));
            }

            if (State.SensorField.bVisible)
            {
                LegendText += FString::Printf(
                    TEXT("\nFOV %.0f deg x %.0f deg | %.0fu | %.0f%% clear"),
                    State.SensorFieldSettings.HorizontalHalfAngle * 2.0f,
                    State.SensorFieldSettings.VerticalHalfAngle * 2.0f,
                    State.SensorFieldSettings.Range,
                    State.SensorField.ClearPercent);
            }

            if (State.Locomotion.bShowLocomotionBands)
            {
                LegendText += FString::Printf(
                    TEXT("\nMOVE %.1fs / %.1fs / %.1fs @ %.0fu/s"),
                    State.Locomotion.NearTime,
                    State.Locomotion.MidTime,
                    State.Locomotion.FarTime,
                    State.Locomotion.Speed);
            }
            if (State.Trajectory.bVisible)
            {
                LegendText += FString::Printf(
                    TEXT("\nARC %.0fu/s + %.0fu/s Z  |  g x%.2f"),
                    State.TrajectorySettings.HorizontalSpeed,
                    State.TrajectorySettings.LaunchVerticalSpeed,
                    State.TrajectorySettings.GravityScale);
            }
            if (State.CameraProbe.bVisible)
            {
                LegendText += FString::Printf(
                    TEXT("\nCAM %.0fu target | radius %.0fu"),
                    State.CameraProbeSettings.TargetDistance,
                    State.CameraProbeSettings.ProbeRadius);
            }
            if (State.CameraSightline.bVisible)
            {
                LegendText += FString::Printf(
                    TEXT("\nVIEW subject %.0fu | frame <= %.0f deg"),
                    State.CameraSightlineSettings.SubjectZOffset,
                    State.CameraSightlineSettings.FramingHalfAngle);
            }
            if (State.GroundValidation.bVisible)
            {
                LegendText += FString::Printf(
                    TEXT("\nGROUND %.0fu x %.0fu | slope <= %.0f deg | step +%.0f/-%.0fu"),
                    State.GroundValidationSettings.ForwardDistance,
                    State.GroundValidationSettings.LateralCoverage,
                    State.GroundValidationSettings.MaxWalkableSlope,
                    State.GroundValidationSettings.MaxStepUp,
                    State.GroundValidationSettings.MaxStepDown);
            }
            if (State.FitAnalysis.bVisible)
            {
                LegendText += FString::Printf(
                    TEXT("\nFIT %.0fu x %.0fu required | sample %.0fu | warn %.0fu"),
                    State.FitAnalysis.RequiredWidth,
                    State.FitAnalysis.RequiredHeight,
                    State.FitAnalysisSettings.SampleSpacing,
                    State.FitAnalysisSettings.WarningMargin);
            }
            if (State.ThresholdAnalysis.bVisible)
            {
                LegendText += FString::Printf(
                    TEXT("\nTHRESHOLD %.0fu deep | step +%.0f/-%.0fu | warn %.0fu"),
                    State.ThresholdAnalysisSettings.ProbeDepth,
                    State.ThresholdAnalysisSettings.MaxStepUp,
                    State.ThresholdAnalysisSettings.MaxStepDown,
                    State.ThresholdAnalysisSettings.WarningMargin);
            }
            if (!State.PinnedBenchmarks.IsEmpty())
            {
                LegendText += FString::Printf(TEXT("\nPINS %d %s | Ctrl+\\ pin | Ctrl+Shift+\\ clear"), State.PinnedBenchmarks.Num(), (MetricSettings && MetricSettings->bPersistPinnedBenchmarksPerMap) ? TEXT("SAVED") : TEXT("SESSION"));
            }
        }

        const float LabelPadding = MetricSettings ? MetricSettings->LabelHeightPadding : 35.0f;
        const float LabelBaseZ = FMath::Max(220.0f, State.Hull.Height + LabelPadding);
        const FVector RelativeLegendLocation(0.0f, 0.0f, LabelBaseZ + LegendHeightOffset);
        LegendComponent->SetText(FText::FromString(LegendText));
        LegendComponent->SetWorldSize(LegendWorldSize);
        LegendComponent->SetTextRenderColor(LegendLinearColor.ToFColor(true));
        LegendComponent->SetRelativeLocation(RelativeLegendLocation);

        const FVector LegendWorldLocation = State.Location + RelativeLegendLocation;
        const FVector ToViewer = (State.ViewerLocation - LegendWorldLocation).GetSafeNormal();
        if (!ToViewer.IsNearlyZero())
        {
            LegendComponent->SetWorldRotation(FRotationMatrix::MakeFromX(ToViewer).Rotator());
        }

        // Legend moved into the screen-space viewport HUD in 0.16.0.
        LegendComponent->SetVisibility(false, true);
    }
}

void AMetricRenderHostActor::SetMetricVisible(bool bVisible)
{
    SetActorHiddenInGame(!bVisible);
    if (MetricComponent)
    {
        MetricComponent->SetVisibility(bVisible, true);
    }
    if (PreviewMeshComponent)
    {
        PreviewMeshComponent->SetVisibility(bVisible && PreviewMeshComponent->GetStaticMesh() != nullptr, true);
    }
    if (PinnedMetricComponent && !bVisible)
    {
        PinnedMetricComponent->SetVisibility(false, true);
    }
    if (LabelComponent)
    {
        LabelComponent->SetVisibility(false, true);
    }
    if (LegendComponent)
    {
        const UMetricEditorSettings* MetricSettings = GetDefault<UMetricEditorSettings>();
        const bool bShowLegend = MetricSettings ? MetricSettings->bShowLegend : true;
        LegendComponent->SetVisibility(false, true);
    }
}
