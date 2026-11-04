#include "MetricRenderHostActor.h"

#include "MetricViewportComponent.h"
#include "MetricEditorSettings.h"
#include "MetricPerformance.h"
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
    MetricPerformance::FScopedPipelineStage ApplyScope(TEXT("Host.ApplyMetricState.Total"));
    { MetricPerformance::FScopedPipelineStage Scope(TEXT("Host.SetActorLocation")); SetActorLocation(State.Location); }

    if (MetricComponent)
    {
        FMetricLiveState LocalState = State;
        LocalState.Location = FVector::ZeroVector;
        for (FMetricPinnedBenchmark& Pin : LocalState.PinnedBenchmarks)
        {
            Pin.WorldLocation -= State.Location;
        }
        { MetricPerformance::FScopedPipelineStage Scope(TEXT("Host.MetricComponent.SetState")); MetricComponent->SetMetricState(LocalState); }
        { MetricPerformance::FScopedPipelineStage Scope(TEXT("Host.MetricComponent.Visibility")); MetricComponent->SetVisibility(State.bHasHit || !State.PinnedBenchmarks.IsEmpty(), true); }
    }

    if (PreviewMeshComponent)
    {
        MetricPerformance::FScopedPipelineStage Scope(TEXT("Host.PreviewMesh"));
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

    // World-space label and legend rendering were retired in 0.16.0 when
    // Metric moved this information into the screen-space viewport HUD. Keep
    // the legacy components hidden for asset/layout compatibility, but do not
    // rebuild their text, materials, transforms, or render state every publish.
    if (LabelComponent && LabelComponent->IsVisible())
    {
        LabelComponent->SetVisibility(false, true);
    }
    if (LegendComponent && LegendComponent->IsVisible())
    {
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
    if (PreviewMeshComponent && !bVisible)
    {
        // ApplyMetricState owns the positive preview-mesh visibility decision.
        // Do not resurrect a live preview mesh merely because pinned benchmarks
        // keep the render host itself visible.
        PreviewMeshComponent->SetVisibility(false, true);
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
        LegendComponent->SetVisibility(false, true);
    }
}
