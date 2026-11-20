#include "MetricActor.h"

#include "MetricProfile.h"
#include "MetricEditorSettings.h"
#include "MetricFrameUtils.h"
#include "MetricDiagnosticUtils.h"
#include "MetricSubsystem.h"
#include "MetricViewportComponent.h"
#include "MetricActorOverview.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"


namespace
{
    UTexture2D* LoadMetricActorBillboardTexture()
    {
        // Keep the editor billboard texture strongly owned for the lifetime of the Metric module.
        // FImageUtils imports into the transient package; a weak-only cache can allow that object
        // to disappear between component reconstruction/transactions and leaves the billboard open
        // to inheriting an unrelated editor sprite. Metric owns one stable texture identity instead.
        static TStrongObjectPtr<UTexture2D> CachedTexture;
        if (CachedTexture.IsValid())
        {
            return CachedTexture.Get();
        }

        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Metric"));
        if (!Plugin.IsValid())
        {
            return nullptr;
        }

        UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(FPaths::Combine(
            Plugin->GetBaseDir(), TEXT("Resources"), TEXT("MetricProfileAsset_64.png")));
        if (Texture)
        {
            CachedTexture.Reset(Texture);
        }
        return CachedTexture.Get();
    }
}

AMetricActor::AMetricActor()
{
    bIsEditorOnlyActor = true;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    SetActorEnableCollision(false);

    // Keep a neutral root so editor-only placement affordances do not inherit the
    // analysis visualization's visibility state.
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = SceneRoot;

    VisualizationComponent = CreateDefaultSubobject<UMetricViewportComponent>(TEXT("MetricVisualization"));
    VisualizationComponent->SetupAttachment(RootComponent);
    // Metric evaluation results are stored as world-space directions and world-relative offsets.
    // The Actor should contribute translation only when those results are rendered. Inheriting
    // Actor rotation/scale here would transform already-oriented analysis data a second time.
    VisualizationComponent->SetAbsolute(false, true, true);
    VisualizationComponent->SetIsVisualizationComponent(true);
    VisualizationComponent->SetAllowQueryInspection(true);
    VisualizationComponent->SetQueryInspectionOverride(true, false, EMetricInspectionTarget::Auto);

#if WITH_EDITOR
    // Keep Unreal's user-facing per-Actor Editor Billboard Scale at its native default.
    // Metric's authored icon size is controlled by the billboard component itself.
    SpriteScale = 1.0f;

    BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("MetricBillboard"));
    if (BillboardComponent)
    {
        BillboardComponent->SetFlags(RF_Transient);
        BillboardComponent->SetupAttachment(RootComponent);
        // Placement billboards are screen-space editor affordances. They follow Actor translation,
        // but authored Actor rotation/scale must never rotate, stretch, or resize the icon.
        BillboardComponent->SetAbsolute(false, true, true);
        BillboardComponent->SetIsVisualizationComponent(true);
        BillboardComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BillboardComponent->SetHiddenInGame(true);
        BillboardComponent->bIsScreenSizeScaled = true;
        BillboardComponent->bUseInEditorScaling = true;
        BillboardComponent->ScreenSize = GetDefault<UBillboardComponent>()->ScreenSize * 4.0f;
        BillboardComponent->OpacityMaskRefVal = 0.3f;
        BillboardComponent->SetRelativeScale3D(FVector::OneVector);
        if (UTexture2D* Texture = LoadMetricActorBillboardTexture())
        {
            BillboardComponent->SetSprite(Texture);
        }
    }
#endif
}

void AMetricActor::PostLoad()
{
    Super::PostLoad();
#if WITH_EDITOR
    RefreshBillboardSprite();
#endif
}

void AMetricActor::PostRegisterAllComponents()
{
    Super::PostRegisterAllComponents();
#if WITH_EDITOR
    RefreshBillboardSprite();
#endif
}

void AMetricActor::RefreshBillboardSprite()
{
#if WITH_EDITOR
    if (!BillboardComponent || IsTemplate() || !GetWorld() ||
        (GetWorld()->WorldType != EWorldType::Editor && GetWorld()->WorldType != EWorldType::EditorPreview) ||
        IsRunningCommandlet())
    {
        return;
    }

    BillboardComponent->SetAbsolute(false, true, true);
    BillboardComponent->bIsScreenSizeScaled = true;
    BillboardComponent->bUseInEditorScaling = true;
    BillboardComponent->ScreenSize = GetDefault<UBillboardComponent>()->ScreenSize * 4.0f;
    BillboardComponent->OpacityMaskRefVal = 0.3f;
    BillboardComponent->SetRelativeScale3D(FVector::OneVector);

    if (UTexture2D* Texture = LoadMetricActorBillboardTexture())
    {
        if (BillboardComponent->Sprite != Texture)
        {
            BillboardComponent->SetSprite(Texture);
        }
        BillboardComponent->SetVisibility(true);
        BillboardComponent->MarkRenderStateDirty();
    }
#endif
}

void AMetricActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

#if WITH_EDITOR
    // Editor transactions/component reconstruction can touch billboard component state independently
    // of Metric analysis. Cheaply enforce Metric's owned sprite identity whenever this Actor ticks.
    if (BillboardComponent)
    {
        if (UTexture2D* ExpectedTexture = LoadMetricActorBillboardTexture())
        {
            if (BillboardComponent->Sprite != ExpectedTexture)
            {
                RefreshBillboardSprite();
            }
        }
    }

    if (!bEnabled)
    {
        UpdateVisualization();
        SetActorTickEnabled(false);
        return;
    }

    if (UpdateMode == EMetricActorUpdateMode::Continuous || bPendingRelevantUpdate)
    {
        RecalculateMetric();
    }
#endif
}

void AMetricActor::RequestRelevantUpdate()
{
#if WITH_EDITOR
    if (!bEnabled || UpdateMode != EMetricActorUpdateMode::OnRelevantChange)
    {
        return;
    }

    bPendingRelevantUpdate = true;
    SetActorTickEnabled(true);
#endif
}

void AMetricActor::RecalculateMetric()
{
#if WITH_EDITOR
    bPendingRelevantUpdate = false;

    if (!bEnabled)
    {
        OverallResult = EMetricActorOverallResult::Disabled;
        IssueCount = 0;
        WarningCount = 0;
        FailureCount = 0;
        ResultSummary = TEXT("Disabled");
        ActiveIssues.Reset();
        ResolvedInspection.Reset();
        InspectionReason.Reset();
        EvaluatedState = FMetricLiveState();
        UpdateVisualization();
        SetActorTickEnabled(false);
        return;
    }

    if (!Profile)
    {
        OverallResult = EMetricActorOverallResult::MissingProfile;
        IssueCount = 0;
        WarningCount = 0;
        FailureCount = 0;
        ResultSummary = TEXT("Missing Metric Profile");
        ActiveIssues.Reset();
        ResolvedInspection.Reset();
        InspectionReason.Reset();
        EvaluatedState = FMetricLiveState();
        UpdateVisualization();
        SetActorTickEnabled(UpdateMode == EMetricActorUpdateMode::Continuous);
        return;
    }

    if (!GEditor || !GetWorld())
    {
        return;
    }

    if (UMetricSubsystem* MetricSubsystem = GEditor->GetEditorSubsystem<UMetricSubsystem>())
    {
        const FVector Location = GetActorLocation();
        const FVector Forward = GetActorForwardVector();
        FVector SurfaceNormal = FVector::UpVector;
        FVector Up = FVector::UpVector;
        float GravityMagnitude = 0.0f;

        switch (AnalysisFrame)
        {
        case EMetricAnalysisFrameMode::SurfaceNormal:
            SurfaceNormal = GetActorUpVector().GetSafeNormal();
            Up = SurfaceNormal;
            break;
        case EMetricAnalysisFrameMode::CustomUp:
            Up = CustomAnalysisUp.GetSafeNormal();
            if (Up.IsNearlyZero()) Up = FVector::UpVector;
            SurfaceNormal = Up;
            break;
        case EMetricAnalysisFrameMode::Provider:
            if (!MetricFrameUtils::TryResolveProviderGravity(GetWorld(), Location, this, Up, GravityMagnitude))
            {
                Up = FVector::UpVector;
                GravityMagnitude = FMath::Abs(GetWorld()->GetGravityZ());
            }
            SurfaceNormal = Up;
            break;
        case EMetricAnalysisFrameMode::World:
        default:
            Up = FVector::UpVector;
            SurfaceNormal = GetActorUpVector().GetSafeNormal();
            break;
        }

        UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>();
        const EMetricEnvironmentSource PreviousSource = Settings ? Settings->EnvironmentSource : EMetricEnvironmentSource::Collision;
        if (Settings) Settings->EnvironmentSource = EnvironmentSource;
        MetricSubsystem->EvaluateProfileAtLocation(
            GetWorld(), Profile, Location, Forward, SurfaceNormal, Up, EvaluatedState,
            AnalysisFrame, FrameSampling, GravityMagnitude, this);
        if (Settings) Settings->EnvironmentSource = PreviousSource;
        UpdateSummary();
        UpdateVisualization();
        SetActorTickEnabled(UpdateMode == EMetricActorUpdateMode::Continuous);
    }
#endif
}

void AMetricActor::UpdateSummary()
{
    TSet<FString> FailAnalyses;
    TSet<FString> WarningAnalyses;
    TMap<FString, FString> IssueLabels;

    auto AddFail = [&](const FString& Key, const FString& Label)
    {
        FailAnalyses.Add(Key);
        WarningAnalyses.Remove(Key);
        if (!Label.IsEmpty()) IssueLabels.FindOrAdd(Key) = Label;
    };
    auto AddWarning = [&](const FString& Key, const FString& Label)
    {
        if (!FailAnalyses.Contains(Key))
        {
            WarningAnalyses.Add(Key);
            if (!Label.IsEmpty() && !IssueLabels.Contains(Key)) IssueLabels.Add(Key, Label);
        }
    };

    // Query Evidence is the canonical issue source. Keep one best reason per
    // analysis target so the Actor reports actionable problems rather than raw
    // sample counts.
    for (const FMetricQueryEvidence& Evidence : EvaluatedState.QueryEvidence)
    {
        const FString Key = FString::Printf(TEXT("Inspection.%d"), static_cast<int32>(Evidence.Target));
        if (Evidence.Status == EMetricQueryEvidenceStatus::Fail) AddFail(Key, Evidence.Label);
        else if (Evidence.Status == EMetricQueryEvidenceStatus::Warning) AddWarning(Key, Evidence.Label);
    }

    // Query Evidence now covers all validation analyses that can produce Actor issues.

    FailureCount = FailAnalyses.Num();
    WarningCount = WarningAnalyses.Num();
    IssueCount = FailureCount + WarningCount;
    OverallResult = FailureCount > 0
        ? EMetricActorOverallResult::Fail
        : (WarningCount > 0 ? EMetricActorOverallResult::Tight : EMetricActorOverallResult::Pass);

    ActiveIssues.Reset();
    TArray<FString> FailKeys = FailAnalyses.Array();
    FailKeys.Sort();
    for (const FString& Key : FailKeys)
    {
        ActiveIssues.Add(FString::Printf(TEXT("FAIL: %s"), *IssueLabels.FindRef(Key)));
    }
    TArray<FString> WarningKeys = WarningAnalyses.Array();
    WarningKeys.Sort();
    for (const FString& Key : WarningKeys)
    {
        ActiveIssues.Add(FString::Printf(TEXT("WARN: %s"), *IssueLabels.FindRef(Key)));
    }

    ResultSummary = FString::Printf(TEXT("%s | %d issue%s (%d fail, %d warning)"),
        *GetOverallResultText(), IssueCount, IssueCount == 1 ? TEXT("") : TEXT("s"), FailureCount, WarningCount);

    if (bInspectQueries)
    {
        FString ResolvedReason;
        const EMetricInspectionTarget ResolvedTarget = MetricDiagnosticUtils::ResolveInspectionTarget(
            EvaluatedState, InspectionTarget, &ResolvedReason);
        ResolvedInspection = MetricDiagnosticUtils::GetInspectionTargetDisplayName(ResolvedTarget).ToString();
        InspectionReason = ResolvedReason;
    }
    else
    {
        ResolvedInspection.Reset();
        InspectionReason.Reset();
    }
}

void AMetricActor::UpdateVisualization()
{
    if (!VisualizationComponent)
    {
        return;
    }

    const bool bVisible = bEnabled && bShowVisualization && Profile != nullptr && EvaluatedState.bHasHit;
    if (bVisible)
    {
        FMetricLiveState LocalState = EvaluatedState;
        LocalState.ApplyVisualVisibilityMask(LocalState.ProfileVisibility);
        LocalState.Location = FVector::ZeroVector;
        LocalState.PinnedBenchmarks.Reset();
        LocalState.SelectedPinnedBenchmarkIndex = INDEX_NONE;
        VisualizationComponent->SetQueryInspectionOverride(true, bInspectQueries, InspectionTarget);
        VisualizationComponent->SetMetricState(LocalState);
    }
    // The Metric billboard is a sibling of the analysis visualization and must remain independently visible.
    // Do not propagate analysis-visualization visibility to unrelated editor components.
    VisualizationComponent->SetVisibility(bVisible, false);
}

FString AMetricActor::GetOverallResultText() const
{
    switch (OverallResult)
    {
    case EMetricActorOverallResult::Pass: return TEXT("Pass");
    case EMetricActorOverallResult::Tight: return TEXT("Tight / Warning");
    case EMetricActorOverallResult::Fail: return TEXT("Fail");
    case EMetricActorOverallResult::Disabled: return TEXT("Disabled");
    case EMetricActorOverallResult::MissingProfile: return TEXT("Missing Profile");
    default: return TEXT("Unknown");
    }
}

void AMetricActor::OpenMetricProfile()
{
#if WITH_EDITOR
    if (Profile && GEditor)
    {
        if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            AssetEditorSubsystem->OpenEditorForAsset(Profile);
        }
    }
#endif
}

void AMetricActor::OpenMetricDashboard()
{
#if WITH_EDITOR
    MetricActorOverview::OpenAndFocusActor(this);
#endif
}

#if WITH_EDITOR
void AMetricActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RefreshBillboardSprite();
    bPendingRelevantUpdate = false;
    SetActorTickEnabled(UpdateMode == EMetricActorUpdateMode::Continuous);
    if (UpdateMode == EMetricActorUpdateMode::OnRelevantChange)
    {
        RecalculateMetric();
    }
    else
    {
        UpdateVisualization();
    }
}

void AMetricActor::PostEditMove(bool bFinished)
{
    Super::PostEditMove(bFinished);

    RefreshBillboardSprite();

    if (UpdateMode != EMetricActorUpdateMode::OnRelevantChange)
    {
        return;
    }

    if (bFinished)
    {
        // Commit one final authoritative evaluation after the gizmo drag finishes.
        bPendingRelevantUpdate = false;
        RecalculateMetric();
        return;
    }

    if (!bLiveTransformPreview)
    {
        return;
    }

    // PostEditMove(false) is delivered during Level Editor transform-gizmo motion. Schedule the
    // evaluation on Tick instead of evaluating directly in the editor callback so multiple raw
    // mouse/transform events in one frame collapse into one Metric update. This keeps the visualizer
    // live while dragging without multiplying expensive profile evaluations within the same frame.
    bPendingRelevantUpdate = true;
    SetActorTickEnabled(true);
}

void AMetricActor::PostEditUndo()
{
    Super::PostEditUndo();
    RefreshBillboardSprite();
    RequestRelevantUpdate();
}
#endif
