#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MetricEditorSettings.h"
#include "MetricTypes.h"
#include "MetricActor.generated.h"

class UBillboardComponent;
class UMetricPreset;
class UMetricViewportComponent;

UENUM()
enum class EMetricActorUpdateMode : uint8
{
    OnRelevantChange UMETA(DisplayName = "On Relevant Change / Transform"),
    Continuous UMETA(DisplayName = "Continuous"),
    Manual UMETA(DisplayName = "Manual")
};

UENUM()
enum class EMetricActorOverallResult : uint8
{
    Pass UMETA(DisplayName = "Pass"),
    Tight UMETA(DisplayName = "Tight / Warning"),
    Fail UMETA(DisplayName = "Fail"),
    Disabled UMETA(DisplayName = "Disabled"),
    MissingProfile UMETA(DisplayName = "Missing Profile")
};

UCLASS(BlueprintType, AutoExpandCategories = ("Metric"))
class METRIC_API AMetricActor : public AActor
{
    GENERATED_BODY()

public:
    AMetricActor();

    UPROPERTY(EditAnywhere, Category = "Metric", meta = (DisplayPriority = "1", ToolTip = "Enable or disable this Metric Actor. Disabled actors keep their placement marker but do not evaluate their assigned Metric Profile."))
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, Category = "Metric", meta = (DisplayPriority = "2", ToolTip = "Metric Profile evaluated at this Actor location. The profile defines the gameplay dimensions, thresholds, and analysis layers used by this Actor."))
    TObjectPtr<UMetricPreset> Profile;

    UPROPERTY(EditAnywhere, Category = "Metric", meta = (DisplayPriority = "3", ToolTip = "Controls when this Actor re-evaluates. On Relevant Change updates after meaningful editor changes, Continuous evaluates every editor tick, and Manual evaluates only when Recalculate Metric is pressed."))
    EMetricActorUpdateMode UpdateMode = EMetricActorUpdateMode::OnRelevantChange;

    UPROPERTY(EditAnywhere, Category = "Metric|Analysis", meta = (DisplayName = "Environment Source", ToolTip = "Choose what world geometry Metric evaluates. Collision uses authored collision, Hybrid supplements collision with approximate visible bounds, and Visual Geometry uses approximate visible bounds without requiring collision."))
    EMetricEnvironmentSource EnvironmentSource = EMetricEnvironmentSource::Collision;

    UPROPERTY(EditAnywhere, Category = "Metric|Analysis", meta = (DisplayName = "Analysis Frame", ToolTip = "Controls the Up direction used by this Actor's spatial analyses. World Up uses +Z, Surface Normal follows the Actor's local Up direction, and Custom Up uses the direction below."))
    EMetricAnalysisFrameMode AnalysisFrame = EMetricAnalysisFrameMode::World;

    UPROPERTY(EditAnywhere, Category = "Metric|Analysis", meta = (DisplayName = "Custom Up Direction", EditCondition = "AnalysisFrame == EMetricAnalysisFrameMode::CustomUp", ToolTip = "World-space Up direction used when Analysis Frame is Custom Up. Zero-length values fall back to World Up."))
    FVector CustomAnalysisUp = FVector::UpVector;

    UPROPERTY(EditAnywhere, Category = "Metric|Display", meta = (DisplayName = "Show Visualization", ToolTip = "Show this Actor's Metric analysis visualization in the editor viewport. Disabling this does not disable evaluation or hide the Actor placement billboard."))
    bool bShowVisualization = true;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric|Results", meta = (ToolTip = "Overall result from the most recent evaluation. Fail takes priority over Tight/Warning, which takes priority over Pass."))
    EMetricActorOverallResult OverallResult = EMetricActorOverallResult::MissingProfile;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric|Results", meta = (ToolTip = "Total number of unique analysis families currently reporting either a warning or failure."))
    int32 IssueCount = 0;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric|Results", meta = (ToolTip = "Number of unique analysis families currently reporting a warning or tight result."))
    int32 WarningCount = 0;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric|Results", meta = (ToolTip = "Number of unique analysis families currently reporting a failure."))
    int32 FailureCount = 0;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric|Results", meta = (ToolTip = "Human-readable summary of the most recent Metric evaluation."))
    FString ResultSummary;

    UFUNCTION(CallInEditor, Category = "Metric", meta = (ToolTip = "Evaluate this Metric Actor immediately using its assigned profile, environment source, and analysis frame."))
    void RecalculateMetric();

    /** Mark this actor for one editor evaluation when its environment may have changed. */
    void RequestRelevantUpdate();

    UFUNCTION(CallInEditor, Category = "Metric", meta = (ToolTip = "Open this Actor's assigned Metric Profile in the Metric Profile Editor."))
    void OpenMetricProfile();

    UFUNCTION(CallInEditor, Category = "Metric", meta = (ToolTip = "Open the Metric Dashboard and focus this Metric Actor for level-wide review."))
    void OpenMetricDashboard();

    const FMetricLiveState& GetEvaluatedState() const { return EvaluatedState; }
    FString GetOverallResultText() const;

    virtual bool IsEditorOnly() const override { return true; }
    virtual bool IsLevelBoundsRelevant() const override { return false; }
    virtual void Tick(float DeltaSeconds) override;
    virtual void PostLoad() override;
    virtual void PostRegisterAllComponents() override;

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual void PostEditMove(bool bFinished) override;
    virtual void PostEditUndo() override;
#endif

private:
    UPROPERTY(VisibleAnywhere, Category = "Metric", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMetricViewportComponent> VisualizationComponent;

    UPROPERTY(VisibleAnywhere, Category = "Metric", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UBillboardComponent> BillboardComponent;

    FMetricLiveState EvaluatedState;
    bool bPendingRelevantUpdate = true;

    void UpdateSummary();
    void UpdateVisualization();
    void RefreshBillboardSprite();
};
