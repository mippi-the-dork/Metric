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

    UPROPERTY(EditAnywhere, Category = "Metric", meta = (DisplayPriority = "1"))
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, Category = "Metric", meta = (DisplayPriority = "2"))
    TObjectPtr<UMetricPreset> Profile;

    UPROPERTY(EditAnywhere, Category = "Metric", meta = (DisplayPriority = "3"))
    EMetricActorUpdateMode UpdateMode = EMetricActorUpdateMode::OnRelevantChange;

    UPROPERTY(EditAnywhere, Category = "Metric - Analysis", meta = (DisplayName = "Environment Source"))
    EMetricEnvironmentSource EnvironmentSource = EMetricEnvironmentSource::Collision;

    UPROPERTY(EditAnywhere, Category = "Metric - Analysis", meta = (DisplayName = "Analysis Frame"))
    EMetricAnalysisFrameMode AnalysisFrame = EMetricAnalysisFrameMode::World;

    UPROPERTY(EditAnywhere, Category = "Metric - Analysis", meta = (DisplayName = "Custom Up Direction", EditCondition = "AnalysisFrame == EMetricAnalysisFrameMode::CustomUp"))
    FVector CustomAnalysisUp = FVector::UpVector;

    UPROPERTY(EditAnywhere, Category = "Metric - Display", meta = (DisplayName = "Show Visualization"))
    bool bShowVisualization = true;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric - Results")
    EMetricActorOverallResult OverallResult = EMetricActorOverallResult::MissingProfile;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric - Results")
    int32 IssueCount = 0;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric - Results")
    int32 WarningCount = 0;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric - Results")
    int32 FailureCount = 0;

    UPROPERTY(VisibleAnywhere, Transient, Category = "Metric - Results")
    FString ResultSummary;

    UFUNCTION(CallInEditor, Category = "Metric")
    void RecalculateMetric();

    /** Mark this actor for one editor evaluation when its environment may have changed. */
    void RequestRelevantUpdate();

    UFUNCTION(CallInEditor, Category = "Metric")
    void OpenMetricProfile();

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
