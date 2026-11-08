#include "MetricActor.h"

#include "MetricProfile.h"
#include "MetricEditorSettings.h"
#include "MetricSubsystem.h"
#include "MetricViewportComponent.h"
#include "MetricActorOverview.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"


namespace
{
    UTexture2D* LoadMetricActorBillboardTexture()
    {
        static TWeakObjectPtr<UTexture2D> CachedTexture;
        if (UTexture2D* Existing = CachedTexture.Get())
        {
            return Existing;
        }

        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("Metric"));
        if (!Plugin.IsValid())
        {
            return nullptr;
        }

        UTexture2D* Texture = FImageUtils::ImportFileAsTexture2D(FPaths::Combine(
            Plugin->GetBaseDir(), TEXT("Resources"), TEXT("MetricProfileAsset_64.png")));
        CachedTexture = Texture;
        return Texture;
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
    VisualizationComponent->SetIsVisualizationComponent(true);
    VisualizationComponent->SetAllowQueryInspection(false);

#if WITH_EDITOR
    // Keep Unreal's user-facing per-Actor Editor Billboard Scale at its native default.
    // Metric's authored icon size is controlled by the billboard component itself.
    SpriteScale = 1.0f;

    BillboardComponent = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("MetricBillboard"));
    if (BillboardComponent)
    {
        BillboardComponent->SetFlags(RF_Transient);
        BillboardComponent->SetupAttachment(RootComponent);
        BillboardComponent->SetIsVisualizationComponent(true);
        BillboardComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        BillboardComponent->SetHiddenInGame(true);
        BillboardComponent->bIsScreenSizeScaled = true;
        BillboardComponent->bUseInEditorScaling = true;
        BillboardComponent->ScreenSize = GetDefault<UBillboardComponent>()->ScreenSize * 4.0f;
        BillboardComponent->OpacityMaskRefVal = 0.3f;
        BillboardComponent->SetRelativeScale3D(FVector::OneVector);
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
    if (!BillboardComponent || IsTemplate() || !GetWorld() || GetWorld()->WorldType != EWorldType::Editor || IsRunningCommandlet())
    {
        return;
    }

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
        case EMetricAnalysisFrameMode::World:
        default:
            Up = FVector::UpVector;
            SurfaceNormal = GetActorUpVector().GetSafeNormal();
            break;
        }

        UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>();
        const EMetricEnvironmentSource PreviousSource = Settings ? Settings->EnvironmentSource : EMetricEnvironmentSource::Collision;
        if (Settings) Settings->EnvironmentSource = EnvironmentSource;
        MetricSubsystem->EvaluateProfileAtLocation(GetWorld(), Profile, Location, Forward, SurfaceNormal, Up, EvaluatedState);
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

    auto AddFail = [&](const TCHAR* Name)
    {
        FailAnalyses.Add(Name);
        WarningAnalyses.Remove(Name);
    };
    auto AddWarning = [&](const TCHAR* Name)
    {
        if (!FailAnalyses.Contains(Name)) WarningAnalyses.Add(Name);
    };

    // Query Evidence is the canonical classification source for analyses that
    // currently produce inspectable evidence. Count each analysis once even if
    // it has many failed samples/segments.
    for (const FMetricQueryEvidence& Evidence : EvaluatedState.QueryEvidence)
    {
        const FString Key = FString::Printf(TEXT("Inspection.%d"), static_cast<int32>(Evidence.Target));
        if (Evidence.Status == EMetricQueryEvidenceStatus::Fail) AddFail(*Key);
        else if (Evidence.Status == EMetricQueryEvidenceStatus::Warning) AddWarning(*Key);
    }

    // Remaining legacy analysis families do not all emit Query Evidence yet,
    // so include their explicit result state in the Actor-wide rollup.
    if (EvaluatedState.OverheadClearance.bVisible)
    {
        if (EvaluatedState.OverheadClearance.bBlocked) AddFail(TEXT("Overhead"));
        else if (EvaluatedState.OverheadClearance.bNearThreshold) AddWarning(TEXT("Overhead"));
    }
    if (EvaluatedState.TurnClearance.bVisible)
    {
        if ((EvaluatedState.TurnClearance.Left.bTested && EvaluatedState.TurnClearance.Left.bBlocked) ||
            (EvaluatedState.TurnClearance.Right.bTested && EvaluatedState.TurnClearance.Right.bBlocked))
        {
            AddFail(TEXT("TurnClearance"));
        }
    }
    if (EvaluatedState.JumpRunway.bVisible && (!EvaluatedState.JumpRunway.bTakeoffPass || !EvaluatedState.JumpRunway.bLandingPass))
    {
        AddFail(TEXT("JumpRunway"));
    }
    if (EvaluatedState.ShoulderClearance.bVisible && (EvaluatedState.ShoulderClearance.bLeftBlocked || EvaluatedState.ShoulderClearance.bRightBlocked))
    {
        AddWarning(TEXT("ShoulderClearance"));
    }
    if (EvaluatedState.AlternateStance.bVisible)
    {
        if (!EvaluatedState.AlternateStance.bStandingFits && !EvaluatedState.AlternateStance.bAlternateFits) AddFail(TEXT("AlternateStance"));
        else if (!EvaluatedState.AlternateStance.bStandingFits && EvaluatedState.AlternateStance.bAlternateFits) AddWarning(TEXT("AlternateStance"));
    }

    FailureCount = FailAnalyses.Num();
    WarningCount = WarningAnalyses.Num();
    IssueCount = FailureCount + WarningCount;
    OverallResult = FailureCount > 0
        ? EMetricActorOverallResult::Fail
        : (WarningCount > 0 ? EMetricActorOverallResult::Tight : EMetricActorOverallResult::Pass);

    ResultSummary = FString::Printf(TEXT("%s | %d issue%s (%d fail, %d warning)"),
        *GetOverallResultText(), IssueCount, IssueCount == 1 ? TEXT("") : TEXT("s"), FailureCount, WarningCount);
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
        LocalState.Location = FVector::ZeroVector;
        LocalState.PinnedBenchmarks.Reset();
        LocalState.SelectedPinnedBenchmarkIndex = INDEX_NONE;
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
    if (bFinished)
    {
        bPendingRelevantUpdate = false;
        if (UpdateMode == EMetricActorUpdateMode::OnRelevantChange)
        {
            RecalculateMetric();
        }
    }
}

void AMetricActor::PostEditUndo()
{
    Super::PostEditUndo();
    RefreshBillboardSprite();
    RequestRelevantUpdate();
}
#endif
