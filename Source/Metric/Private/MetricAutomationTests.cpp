#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "MetricEditorSettings.h"
#include "MetricActor.h"
#include "MetricEnvironmentQuery.h"
#include "MetricFrameUtils.h"
#include "MetricPerformance.h"
#include "MetricPreset.h"
#include "MetricProjectSettings.h"
#include "MetricSubsystem.h"
#include "MetricViewportComponent.h"

#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PreviewScene.h"

#define METRIC_AUTOMATION_FLAGS (EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace MetricAutomation
{

    struct FEnvironmentSourceScope
    {
        UMetricEditorSettings* Settings = nullptr;
        EMetricEnvironmentSource Previous = EMetricEnvironmentSource::Collision;

        explicit FEnvironmentSourceScope(EMetricEnvironmentSource Source)
        {
            Settings = GetMutableDefault<UMetricEditorSettings>();
            if (Settings)
            {
                Previous = Settings->EnvironmentSource;
                Settings->EnvironmentSource = Source;
            }
        }

        ~FEnvironmentSourceScope()
        {
            if (Settings)
            {
                Settings->EnvironmentSource = Previous;
            }
        }
    };

    struct FProjectExclusionScope
    {
        UMetricProjectSettings* Settings = nullptr;
        TArray<TSoftClassPtr<AActor>> ActorClasses;
        TArray<FName> ActorTags;
        TArray<TSoftClassPtr<UActorComponent>> ComponentClasses;
        TArray<FName> ComponentTags;

        FProjectExclusionScope()
        {
            Settings = GetMutableDefault<UMetricProjectSettings>();
            if (Settings)
            {
                ActorClasses = Settings->ExcludedActorClasses;
                ActorTags = Settings->ExcludedActorTags;
                ComponentClasses = Settings->ExcludedComponentClasses;
                ComponentTags = Settings->ExcludedComponentTags;
                Settings->ExcludedActorClasses.Reset();
                Settings->ExcludedActorTags.Reset();
                Settings->ExcludedComponentClasses.Reset();
                Settings->ExcludedComponentTags.Reset();
            }
        }

        ~FProjectExclusionScope()
        {
            if (Settings)
            {
                Settings->ExcludedActorClasses = MoveTemp(ActorClasses);
                Settings->ExcludedActorTags = MoveTemp(ActorTags);
                Settings->ExcludedComponentClasses = MoveTemp(ComponentClasses);
                Settings->ExcludedComponentTags = MoveTemp(ComponentTags);
            }
        }
    };

    struct FTestWorld
    {
        FPreviewScene PreviewScene;

        FTestWorld()
            : PreviewScene(
                FPreviewScene::ConstructionValues()
                    .SetCreatePhysicsScene(true)
                    .SetCreateDefaultLighting(false)
                    .SetEditor(true)
                    .SetTransactional(false)
                    .AllowAudioPlayback(false)
                    .ShouldSimulatePhysics(false))
        {
        }

        UWorld* GetWorld()
        {
            return PreviewScene.GetWorld();
        }
    };

    UBoxComponent* SpawnBox(UWorld* World, const FVector& Location, const FVector& Extent, bool bCollision, FName ActorTag = NAME_None, FName ComponentTag = NAME_None)
    {
        if (!World)
        {
            return nullptr;
        }

        AActor* Actor = World->SpawnActor<AActor>(Location, FRotator::ZeroRotator);
        if (!Actor)
        {
            return nullptr;
        }
        if (!ActorTag.IsNone())
        {
            Actor->Tags.AddUnique(ActorTag);
        }

        UBoxComponent* Box = NewObject<UBoxComponent>(Actor, NAME_None, RF_Transient);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetVisibility(true);
        Box->SetHiddenInGame(false);
        Box->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionResponseToAllChannels(ECR_Block);
        if (!ComponentTag.IsNone())
        {
            Box->ComponentTags.AddUnique(ComponentTag);
        }
        Box->RegisterComponent();
        Box->SetWorldLocation(Location);
        Box->UpdateBounds();
        return Box;
    }

    bool Trace(UWorld* World, FHitResult& Hit)
    {
        const FCollisionQueryParams Params = MetricEnvironmentQuery::MakeParams(World, TEXT("MetricAutomationTrace"), true);
        return MetricEnvironmentQuery::LineTraceSingleByChannel(
            World, Hit, FVector(0.0f, 0.0f, 50.0f), FVector(1000.0f, 0.0f, 50.0f), ECC_Visibility, Params);
    }

    FMetricLiveState MakeAnalysisState()
    {
        FMetricLiveState State;
        State.bHasHit = true;
        State.Location = FVector::ZeroVector;
        State.SurfaceNormal = FVector::UpVector;
        State.AnalysisUp = FVector::UpVector;
        State.AnalysisForward = FVector::ForwardVector;
        State.AnalysisRight = FVector::RightVector;
        State.Forward = FVector::ForwardVector;
        State.EntryType = EMetricCycleEntryType::BuiltInMetric;
        State.Hull.HullType = EMetricHullType::Capsule;
        State.Hull.Width = 60.0f;
        State.Hull.Length = 60.0f;
        State.Hull.Height = 180.0f;
        return State;
    }

    UBoxComponent* SpawnFloor(UWorld* World, float CenterX = 150.0f, float HalfX = 500.0f, float SurfaceZ = 0.0f)
    {
        return SpawnBox(World, FVector(CenterX, 0.0f, SurfaceZ - 10.0f), FVector(HalfX, 500.0f, 10.0f), true);
    }

    FMetricLiveState MakeTrajectoryState()
    {
        FMetricLiveState State;
        State.bHasHit = true;
        State.Location = FVector::ZeroVector;
        State.SurfaceNormal = FVector::UpVector;
        State.AnalysisUp = FVector::UpVector;
        State.AnalysisForward = FVector::ForwardVector;
        State.AnalysisRight = FVector::RightVector;
        State.Forward = FVector::ForwardVector;
        State.EntryType = EMetricCycleEntryType::BuiltInMetric;
        State.Hull.HullType = EMetricHullType::Sphere;
        State.Hull.Width = 20.0f;
        State.Hull.Length = 20.0f;
        State.Hull.Height = 20.0f;
        State.TrajectorySettings.bShowTrajectory = true;
        State.TrajectorySettings.HorizontalSpeed = 600.0f;
        State.TrajectorySettings.LaunchVerticalSpeed = 550.0f;
        State.TrajectorySettings.GravityScale = 1.0f;
        State.TrajectorySettings.MaxSimulationTime = 3.0f;
        State.TrajectorySettings.SampleInterval = 0.025f;
        State.TrajectorySettings.MaxLandingSlope = 45.0f;
        State.TrajectorySettings.MaximumDropDistance = 1000.0f;
        State.TrajectorySettings.ApexProbeDistance = 0.0f;
        State.TrajectorySettings.TraceChannel = ECC_Visibility;
        return State;
    }


    UMetricPreset* MakePerformanceProfile()
    {
        UMetricPreset* Profile = NewObject<UMetricPreset>(GetTransientPackage());
        Profile->DisplayName = FText::FromString(TEXT("Metric Performance Profile"));
        Profile->Clearance.bShowClearanceProbe = true;
        Profile->OverheadClearance.bShowOverheadClearance = true;
        Profile->SensorField.bShowSensorField = true;
        Profile->Trajectory.bShowTrajectory = true;
        Profile->CameraProbe.bShowCameraProbe = true;
        Profile->CameraSightline.bShowCameraSightline = true;
        Profile->GroundValidation.bShowGroundValidation = true;
        Profile->FitAnalysis.bShowFitAnalysis = true;
        Profile->ThresholdAnalysis.bShowThresholdAnalysis = true;
        Profile->TurnClearance.bShowTurnClearance = true;
        Profile->LedgeAnalysis.bShowLedgeAnalysis = true;
        Profile->StoppingDistance.bShowStoppingDistance = true;
        Profile->CoverExposure.bShowCoverExposure = true;
        Profile->JumpRunway.bShowJumpRunway = true;
        Profile->InteractionReach.bShowInteractionReach = true;
        Profile->ShoulderClearance.bShowShoulderClearance = true;
        Profile->CombatDensity.bShowCombatDensity = true;
        Profile->AlternateStance.bShowAlternateStance = true;
        Profile->MantleVault.bShowMantleVault = true;
        Profile->RecoverySpace.bShowRecoverySpace = true;
        Profile->Crossfire.bShowCrossfire = true;
        return Profile;
    }

    void BuildPerformanceFixture(UWorld* World)
    {
        SpawnFloor(World, 250.0f, 1000.0f, 0.0f);
        SpawnBox(World, FVector(250.0f, 140.0f, 90.0f), FVector(25.0f, 100.0f, 100.0f), true);
        SpawnBox(World, FVector(500.0f, -180.0f, 75.0f), FVector(60.0f, 60.0f, 75.0f), true);
        SpawnBox(World, FVector(-260.0f, 0.0f, 180.0f), FVector(30.0f, 220.0f, 180.0f), true);
    }
    UMetricSubsystem* GetSubsystem()
    {
        return GEditor ? GEditor->GetEditorSubsystem<UMetricSubsystem>() : nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentCollisionBaseline, "MetricPlugin.Environment.CollisionBaseline", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentCollisionBaseline::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    TestNotNull(TEXT("World"), World);
    TestNotNull(TEXT("Collision box"), MetricAutomation::SpawnBox(World, FVector(500, 0, 50), FVector(50), true));
    FHitResult Hit;
    TestTrue(TEXT("Collision source hits authored collision"), MetricAutomation::Trace(World, Hit));
    TestTrue(TEXT("Hit distance is near the box face"), FMath::IsNearlyEqual(Hit.Distance, 450.0f, 5.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentHybridFallback, "MetricPlugin.Environment.HybridFallback", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentHybridFallback::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Hybrid);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    UBoxComponent* Box = MetricAutomation::SpawnBox(World, FVector(500, 0, 50), FVector(50), false);
    TestNotNull(TEXT("No-collision visual box"), Box);
    FHitResult Hit;
    TestTrue(TEXT("Hybrid falls back to visible bounds when collision is absent"), MetricAutomation::Trace(World, Hit));
    TestEqual(TEXT("Fallback identifies the visual component"), Hit.GetComponent(), static_cast<UPrimitiveComponent*>(Box));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentVisualGeometryBounds, "MetricPlugin.Environment.VisualGeometryBounds", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentVisualGeometryBounds::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::VisualGeometry);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    UBoxComponent* Box = MetricAutomation::SpawnBox(World, FVector(500, 0, 50), FVector(50), false);
    FHitResult Hit;
    TestTrue(TEXT("Visual Geometry hits a visible component with no collision"), MetricAutomation::Trace(World, Hit));
    TestEqual(TEXT("Visual Geometry reports the component whose bounds were hit"), Hit.GetComponent(), static_cast<UPrimitiveComponent*>(Box));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentActorExclusion, "MetricPlugin.Environment.ActorExclusion", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentActorExclusion::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    UBoxComponent* Box = MetricAutomation::SpawnBox(World, FVector(500, 0, 50), FVector(50), true);
    TestNotNull(TEXT("Box"), Box);
    MetricEnvironmentQuery::AddActorExclusionForAutomation(World, Box ? Box->GetOwner() : nullptr);
    FHitResult Hit;
    TestFalse(TEXT("Explicit actor exclusion suppresses collision queries"), MetricAutomation::Trace(World, Hit));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentComponentExclusion, "MetricPlugin.Environment.ComponentExclusion", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentComponentExclusion::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::VisualGeometry);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    UBoxComponent* Box = MetricAutomation::SpawnBox(World, FVector(500, 0, 50), FVector(50), false);
    MetricEnvironmentQuery::AddComponentExclusionForAutomation(World, Box);
    FHitResult Hit;
    TestFalse(TEXT("Explicit component exclusion suppresses visual-geometry queries"), MetricAutomation::Trace(World, Hit));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentComponentClassExclusion, "MetricPlugin.Environment.ComponentClassExclusion", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentComponentClassExclusion::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::VisualGeometry);
    MetricAutomation::FProjectExclusionScope Exclusions;
    UMetricProjectSettings* Settings = GetMutableDefault<UMetricProjectSettings>();
    if (Settings)
    {
        Settings->ExcludedComponentClasses.Add(UBoxComponent::StaticClass());
    }
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(500, 0, 50), FVector(50), false);
    FHitResult Hit;
    TestFalse(TEXT("Component Class exclusion suppresses matching visual geometry"), MetricAutomation::Trace(World, Hit));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentActorTagExclusion, "MetricPlugin.Environment.ActorTagExclusion", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentActorTagExclusion::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::VisualGeometry);
    MetricAutomation::FProjectExclusionScope Exclusions;
    UMetricProjectSettings* Settings = GetMutableDefault<UMetricProjectSettings>();
    if (Settings)
    {
        Settings->ExcludedActorTags.Add(TEXT("MetricAutomationIgnore"));
    }
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(500, 0, 50), FVector(50), false, TEXT("MetricAutomationIgnore"));
    FHitResult Hit;
    TestFalse(TEXT("Actor Tag exclusion suppresses matching visual geometry"), MetricAutomation::Trace(World, Hit));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricEnvironmentWorldScopedExclusions, "MetricPlugin.Environment.WorldScopedInstanceExclusions", METRIC_AUTOMATION_FLAGS)
bool FMetricEnvironmentWorldScopedExclusions::RunTest(const FString& Parameters)
{
    MetricAutomation::FTestWorld TestWorldA;
    UWorld* WorldA = TestWorldA.GetWorld();
    UBoxComponent* BoxA = MetricAutomation::SpawnBox(WorldA, FVector(500, 0, 50), FVector(50), false);
    MetricEnvironmentQuery::AddActorExclusionForAutomation(WorldA, BoxA ? BoxA->GetOwner() : nullptr);
    TestEqual(TEXT("World A owns one explicit exclusion"), MetricEnvironmentQuery::GetInstanceExclusionCountForAutomation(WorldA), 1);

    MetricAutomation::FTestWorld TestWorldB;
    UWorld* WorldB = TestWorldB.GetWorld();
    TestNotNull(TEXT("Second world"), WorldB);
    TestEqual(TEXT("A newly opened world does not inherit instance exclusions"), MetricEnvironmentQuery::GetInstanceExclusionCountForAutomation(WorldB), 0);
    MetricEnvironmentQuery::ClearInstanceExclusionsForAutomation(WorldB);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricFrameWorldUp, "MetricPlugin.Frame.WorldUp", METRIC_AUTOMATION_FLAGS)
bool FMetricFrameWorldUp::RunTest(const FString& Parameters)
{
    const FMetricAnalysisBasis Basis = MetricFrameUtils::BuildAnalysisBasis(
        EMetricAnalysisFrameMode::World, FVector::RightVector, FVector::ForwardVector, FVector(1, 1, 1));
    TestTrue(TEXT("World mode uses +Z"), Basis.Up.Equals(FVector::UpVector, KINDA_SMALL_NUMBER));
    TestTrue(TEXT("Forward is perpendicular to Up"), FMath::IsNearlyZero(FVector::DotProduct(Basis.Forward, Basis.Up), KINDA_SMALL_NUMBER));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricFrameSurfaceNormal, "MetricPlugin.Frame.SurfaceNormal", METRIC_AUTOMATION_FLAGS)
bool FMetricFrameSurfaceNormal::RunTest(const FString& Parameters)
{
    const FVector Normal = FVector(0.0f, 1.0f, 0.0f);
    const FMetricAnalysisBasis Basis = MetricFrameUtils::BuildAnalysisBasis(
        EMetricAnalysisFrameMode::SurfaceNormal, Normal, FVector::UpVector, FVector::ForwardVector);
    TestTrue(TEXT("Surface mode uses the placement normal"), Basis.Up.Equals(Normal, KINDA_SMALL_NUMBER));
    TestTrue(TEXT("Basis remains orthogonal"), FMath::IsNearlyZero(FVector::DotProduct(Basis.Right, Basis.Up), KINDA_SMALL_NUMBER));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricFrameCustomUp, "MetricPlugin.Frame.CustomUp", METRIC_AUTOMATION_FLAGS)
bool FMetricFrameCustomUp::RunTest(const FString& Parameters)
{
    const FVector Custom(1.0f, 1.0f, 1.0f);
    const FMetricAnalysisBasis Basis = MetricFrameUtils::BuildAnalysisBasis(
        EMetricAnalysisFrameMode::CustomUp, FVector::UpVector, Custom, FVector::ForwardVector);
    TestTrue(TEXT("Custom Up is normalized"), Basis.Up.Equals(Custom.GetSafeNormal(), KINDA_SMALL_NUMBER));
    TestTrue(TEXT("Forward/Right/Up are orthogonal"),
        FMath::IsNearlyZero(FVector::DotProduct(Basis.Forward, Basis.Up), KINDA_SMALL_NUMBER)
        && FMath::IsNearlyZero(FVector::DotProduct(Basis.Right, Basis.Up), KINDA_SMALL_NUMBER)
        && FMath::IsNearlyZero(FVector::DotProduct(Basis.Forward, Basis.Right), KINDA_SMALL_NUMBER));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricFrameParallelForwardFallback, "MetricPlugin.Frame.ParallelForwardFallback", METRIC_AUTOMATION_FLAGS)
bool FMetricFrameParallelForwardFallback::RunTest(const FString& Parameters)
{
    const FMetricAnalysisBasis Basis = MetricFrameUtils::BuildAnalysisBasis(
        EMetricAnalysisFrameMode::World, FVector::UpVector, FVector::UpVector, FVector::UpVector);
    TestFalse(TEXT("Forward fallback is non-zero when preferred forward is parallel to Up"), Basis.Forward.IsNearlyZero());
    TestTrue(TEXT("Fallback Forward is perpendicular to Up"), FMath::IsNearlyZero(FVector::DotProduct(Basis.Forward, Basis.Up), KINDA_SMALL_NUMBER));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricTrajectoryEnvelopeSampling, "MetricPlugin.Trajectory.EnvelopeSampling", METRIC_AUTOMATION_FLAGS)
bool FMetricTrajectoryEnvelopeSampling::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    TestNotNull(TEXT("Metric subsystem"), Subsystem);
    FMetricLiveState State = MetricAutomation::MakeTrajectoryState();
    State.TrajectorySettings.DisplayMode = EMetricTrajectoryDisplayMode::JumpEnvelope;
    State.TrajectorySettings.JumpEnvelopeArcCount = 7;
    State.TrajectorySettings.MinimumHorizontalSpeedFraction = 0.1f;
    State.TrajectorySettings.MaximumDropDistance = 0.0f;
    State.TrajectorySettings.MaxSimulationTime = 0.5f;
    if (Subsystem)
    {
        Subsystem->EvaluateTrajectoryForAutomation(World, State);
    }
    TestEqual(TEXT("Requested envelope arc count is sampled"), State.Trajectory.TestedEnvelopeArcs, 7);
    if (State.Trajectory.EnvelopeArcs.Num() == 7)
    {
        TestTrue(TEXT("First arc starts at minimum horizontal fraction"), FMath::IsNearlyEqual(State.Trajectory.EnvelopeArcs[0].HorizontalSpeedFraction, 0.1f, KINDA_SMALL_NUMBER));
        TestTrue(TEXT("Last arc reaches full horizontal speed"), FMath::IsNearlyEqual(State.Trajectory.EnvelopeArcs.Last().HorizontalSpeedFraction, 1.0f, KINDA_SMALL_NUMBER));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricTrajectoryLowerLanding, "MetricPlugin.Trajectory.LowerLanding", METRIC_AUTOMATION_FLAGS)
bool FMetricTrajectoryLowerLanding::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(950, 0, -325), FVector(300, 300, 25), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeTrajectoryState();
    State.TrajectorySettings.DisplayMode = EMetricTrajectoryDisplayMode::SingleArc;
    State.TrajectorySettings.MaximumDropDistance = 500.0f;
    if (Subsystem)
    {
        Subsystem->EvaluateTrajectoryForAutomation(World, State);
    }
    TestTrue(TEXT("Descending trajectory finds a lower landing"), State.Trajectory.bHasLanding);
    TestTrue(TEXT("Lower landing is valid within Maximum Drop"), State.Trajectory.bLandingValid);
    TestTrue(TEXT("Landing reports a positive drop"), State.Trajectory.EnvelopeArcs.Num() > 0 && State.Trajectory.EnvelopeArcs[0].LandingDrop > 250.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricTrajectoryMaximumDrop, "MetricPlugin.Trajectory.MaximumDrop", METRIC_AUTOMATION_FLAGS)
bool FMetricTrajectoryMaximumDrop::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeTrajectoryState();
    State.TrajectorySettings.DisplayMode = EMetricTrajectoryDisplayMode::SingleArc;
    State.TrajectorySettings.MaximumDropDistance = 100.0f;
    if (Subsystem)
    {
        Subsystem->EvaluateTrajectoryForAutomation(World, State);
    }
    TestTrue(TEXT("Arc terminates when Maximum Drop is exceeded"),
        State.Trajectory.EnvelopeArcs.Num() == 1 && State.Trajectory.EnvelopeArcs[0].bExceededMaxDrop);
    TestFalse(TEXT("Exceeding Maximum Drop is not a valid landing"), State.Trajectory.bLandingValid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricTrajectoryBlockedArc, "MetricPlugin.Trajectory.BlockedArc", METRIC_AUTOMATION_FLAGS)
bool FMetricTrajectoryBlockedArc::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(250, 0, 150), FVector(20, 200, 200), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeTrajectoryState();
    State.TrajectorySettings.DisplayMode = EMetricTrajectoryDisplayMode::SingleArc;
    if (Subsystem)
    {
        Subsystem->EvaluateTrajectoryForAutomation(World, State);
    }
    TestTrue(TEXT("Ascending obstacle blocks the arc"), State.Trajectory.bBlocked);
    TestFalse(TEXT("Blocked ascending arc is not classified as a landing"), State.Trajectory.bHasLanding);
    TestTrue(TEXT("Blocked arc captures an impact point"), !State.Trajectory.ImpactPointOffset.IsNearlyZero());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricClearanceOpen, "MetricPlugin.Analysis.Clearance.Open", METRIC_AUTOMATION_FLAGS)
bool FMetricClearanceOpen::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.ClearanceSettings.bShowClearanceProbe = true;
    State.ClearanceSettings.ProbeDistance = 300.0f;
    State.ClearanceSettings.WarningMargin = 25.0f;
    if (Subsystem) Subsystem->EvaluateClearanceForAutomation(TestWorld.GetWorld(), State);
    TestTrue(TEXT("Clearance result is visible"), State.Clearance.bVisible);
    TestFalse(TEXT("Open space is not blocked"), State.Clearance.bBlocked);
    TestFalse(TEXT("Open space has no obstacle"), State.Clearance.bHasObstacle);
    TestTrue(TEXT("Clear distance reaches the requested probe"), FMath::IsNearlyEqual(State.Clearance.ClearDistance, 300.0f, 1.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricClearanceBlocked, "MetricPlugin.Analysis.Clearance.Blocked", METRIC_AUTOMATION_FLAGS)
bool FMetricClearanceBlocked::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(210.0f, 0.0f, 90.0f), FVector(20.0f, 100.0f, 100.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.ClearanceSettings.bShowClearanceProbe = true;
    State.ClearanceSettings.ProbeDistance = 300.0f;
    if (Subsystem) Subsystem->EvaluateClearanceForAutomation(World, State);
    TestTrue(TEXT("Obstacle is detected"), State.Clearance.bHasObstacle);
    TestTrue(TEXT("Obstacle inside requested distance blocks clearance"), State.Clearance.bBlocked);
    TestTrue(TEXT("Impact point is captured"), !State.Clearance.ImpactPointOffset.IsNearlyZero());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricGroundFlatPass, "MetricPlugin.Analysis.Ground.FlatPass", METRIC_AUTOMATION_FLAGS)
bool FMetricGroundFlatPass::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.GroundValidationSettings.bShowGroundValidation = true;
    State.GroundValidationSettings.ForwardDistance = 200.0f;
    State.GroundValidationSettings.SampleSpacing = 50.0f;
    State.GroundValidationSettings.LateralCoverage = 60.0f;
    if (Subsystem) Subsystem->EvaluateGroundValidationForAutomation(World, State);
    TestTrue(TEXT("Ground result is visible"), State.GroundValidation.bVisible);
    TestTrue(TEXT("Flat supported floor passes"), State.GroundValidation.bPass);
    TestEqual(TEXT("Flat floor has no failed samples"), State.GroundValidation.FailedSamples, 0);
    TestTrue(TEXT("Ground analysis sampled the path"), State.GroundValidation.TotalSamples > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricGroundMissingFloor, "MetricPlugin.Analysis.Ground.MissingFloorFail", METRIC_AUTOMATION_FLAGS)
bool FMetricGroundMissingFloor::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.GroundValidationSettings.bShowGroundValidation = true;
    State.GroundValidationSettings.ForwardDistance = 100.0f;
    State.GroundValidationSettings.SampleSpacing = 50.0f;
    State.GroundValidationSettings.LateralCoverage = 0.0f;
    if (Subsystem) Subsystem->EvaluateGroundValidationForAutomation(TestWorld.GetWorld(), State);
    TestFalse(TEXT("Missing floor fails ground validation"), State.GroundValidation.bPass);
    TestTrue(TEXT("Missing floor records failed samples"), State.GroundValidation.FailedSamples > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricFitOpenSpace, "MetricPlugin.Analysis.Fit.OpenSpace", METRIC_AUTOMATION_FLAGS)
bool FMetricFitOpenSpace::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.FitAnalysisSettings.bShowFitAnalysis = true;
    State.FitAnalysisSettings.ForwardDistance = 150.0f;
    State.FitAnalysisSettings.SampleSpacing = 50.0f;
    if (Subsystem) Subsystem->EvaluateFitAnalysisForAutomation(World, State);
    TestTrue(TEXT("Fit result is visible"), State.FitAnalysis.bVisible);
    TestTrue(TEXT("Open supported corridor passes fit"), State.FitAnalysis.bPass);
    TestEqual(TEXT("Open fit has no failed samples"), State.FitAnalysis.FailedSamples, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricThresholdOpenSpace, "MetricPlugin.Analysis.Threshold.OpenSpace", METRIC_AUTOMATION_FLAGS)
bool FMetricThresholdOpenSpace::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.ThresholdAnalysisSettings.bShowThresholdAnalysis = true;
    State.ThresholdAnalysisSettings.ProbeDepth = 120.0f;
    if (Subsystem) Subsystem->EvaluateThresholdAnalysisForAutomation(World, State);
    TestTrue(TEXT("Threshold result is visible"), State.ThresholdAnalysis.bVisible);
    TestTrue(TEXT("Open supported threshold passes"), State.ThresholdAnalysis.bPass);
    TestEqual(TEXT("Threshold evaluates entry/mid/exit"), State.ThresholdAnalysis.TotalSamples, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricLedgeEdgeDetected, "MetricPlugin.Analysis.Ledge.EdgeDetected", METRIC_AUTOMATION_FLAGS)
bool FMetricLedgeEdgeDetected::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World, -100.0f, 150.0f, 0.0f);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.LedgeAnalysisSettings.bShowLedgeAnalysis = true;
    State.LedgeAnalysisSettings.ForwardDistance = 150.0f;
    State.LedgeAnalysisSettings.SampleSpacing = 15.0f;
    State.LedgeAnalysisSettings.MaxProbeDepth = 250.0f;
    if (Subsystem) Subsystem->EvaluateLedgeAnalysisForAutomation(World, State);
    TestTrue(TEXT("Ledge analysis detects the end of the floor"), State.LedgeAnalysis.bHasEdge);
    TestFalse(TEXT("No lower landing exists in the fixture"), State.LedgeAnalysis.bHasLanding);
    TestFalse(TEXT("Unlandable drop is not safe"), State.LedgeAnalysis.bSafeDrop);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricStoppingDistanceBlocked, "MetricPlugin.Analysis.StoppingDistance.Blocked", METRIC_AUTOMATION_FLAGS)
bool FMetricStoppingDistanceBlocked::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(120.0f, 0.0f, 90.0f), FVector(15.0f, 100.0f, 100.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.StoppingDistanceSettings.bShowStoppingDistance = true;
    State.StoppingDistanceSettings.Speed = 600.0f;
    State.StoppingDistanceSettings.Deceleration = 1800.0f;
    State.StoppingDistanceSettings.ReactionTime = 0.15f;
    if (Subsystem) Subsystem->EvaluateStoppingDistanceForAutomation(World, State);
    TestTrue(TEXT("Stopping result is visible"), State.StoppingDistance.bVisible);
    TestTrue(TEXT("Obstacle inside stopping requirement is blocked"), State.StoppingDistance.bBlocked);
    TestTrue(TEXT("Required distance includes reaction and braking"), State.StoppingDistance.RequiredDistance > 150.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricMantleValidObstacle, "MetricPlugin.Analysis.MantleVault.ValidObstacle", METRIC_AUTOMATION_FLAGS)
bool FMetricMantleValidObstacle::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(75.0f, 0.0f, 40.0f), FVector(25.0f, 100.0f, 40.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.MantleVaultSettings.bShowMantleVault = true;
    State.MantleVaultSettings.ForwardReach = 120.0f;
    State.MantleVaultSettings.MinObstacleHeight = 45.0f;
    State.MantleVaultSettings.MaxObstacleHeight = 120.0f;
    State.MantleVaultSettings.MaxObstacleDepth = 100.0f;
    if (Subsystem) Subsystem->EvaluateMantleVaultForAutomation(World, State);
    TestTrue(TEXT("Mantle fixture finds an obstacle"), State.MantleVault.bHasObstacle);
    TestTrue(TEXT("Obstacle height is valid"), State.MantleVault.bHeightValid);
    TestTrue(TEXT("Obstacle depth is valid"), State.MantleVault.bDepthValid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricRecoveryDirectionalBlock, "MetricPlugin.Analysis.RecoverySpace.DirectionalBlock", METRIC_AUTOMATION_FLAGS)
bool FMetricRecoveryDirectionalBlock::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(140.0f, 0.0f, 90.0f), FVector(20.0f, 100.0f, 100.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.RecoverySpaceSettings.bShowRecoverySpace = true;
    State.RecoverySpaceSettings.ForwardDistance = 250.0f;
    State.RecoverySpaceSettings.BackwardDistance = 180.0f;
    State.RecoverySpaceSettings.SideDistance = 220.0f;
    if (Subsystem) Subsystem->EvaluateRecoverySpaceForAutomation(World, State);
    TestFalse(TEXT("Forward recovery is blocked"), State.RecoverySpace.bForwardClear);
    TestTrue(TEXT("Backward recovery remains clear"), State.RecoverySpace.bBackwardClear);
    TestTrue(TEXT("Left recovery remains clear"), State.RecoverySpace.bLeftClear);
    TestTrue(TEXT("Right recovery remains clear"), State.RecoverySpace.bRightClear);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricCameraProbeCompression, "MetricPlugin.Analysis.Camera.ProbeCompression", METRIC_AUTOMATION_FLAGS)
bool FMetricCameraProbeCompression::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(-160.0f, 0.0f, 190.0f), FVector(20.0f, 120.0f, 120.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.CameraProbeSettings.bShowCameraProbe = true;
    State.CameraProbeSettings.PivotZOffset = 160.0f;
    State.CameraProbeSettings.TargetDistance = 350.0f;
    State.CameraProbeSettings.VerticalOffset = 60.0f;
    State.CameraProbeSettings.ProbeRadius = 12.0f;
    if (Subsystem) Subsystem->EvaluateCameraProbeForAutomation(World, State);
    TestTrue(TEXT("Camera probe is visible"), State.CameraProbe.bVisible);
    TestTrue(TEXT("Rear obstacle compresses the camera arm"), State.CameraProbe.bCompressed);
    TestTrue(TEXT("Compressed camera is shorter than target"), State.CameraProbe.ResolvedDistance < State.CameraProbe.TargetDistance);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricCameraSightlineOcclusion, "MetricPlugin.Analysis.Camera.SightlineOcclusion", METRIC_AUTOMATION_FLAGS)
bool FMetricCameraSightlineOcclusion::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FProjectExclusionScope Exclusions;
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.CameraProbe.bVisible = true;
    State.CameraProbe.PivotOffset = FVector(0.0f, 0.0f, 90.0f);
    State.CameraProbe.ResolvedCameraOffset = FVector(-300.0f, 0.0f, 180.0f);
    State.CameraSightlineSettings.bShowCameraSightline = true;
    State.CameraSightlineSettings.SubjectZOffset = 140.0f;
    State.CameraSightlineSettings.TraceRadius = 4.0f;
    MetricAutomation::SpawnBox(World, FVector(-150.0f, 0.0f, 160.0f), FVector(15.0f, 100.0f, 100.0f), true);
    if (Subsystem) Subsystem->EvaluateCameraSightlineForAutomation(World, State);
    TestTrue(TEXT("Sightline result is visible"), State.CameraSightline.bVisible);
    TestTrue(TEXT("Fixture obstacle occludes the subject"), State.CameraSightline.bOccluded);
    TestTrue(TEXT("Sightline records the obstruction"), !State.CameraSightline.ImpactPointOffset.IsNearlyZero());
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricInspectionClearanceEvidence, "MetricPlugin.Inspection.ClearanceFailureEvidence", METRIC_AUTOMATION_FLAGS)
bool FMetricInspectionClearanceEvidence::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(140.0f, 0.0f, 90.0f), FVector(20.0f, 100.0f, 100.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.ClearanceSettings.bShowClearanceProbe = true;
    if (Subsystem)
    {
        Subsystem->EvaluateClearanceForAutomation(World, State);
        Subsystem->BuildQueryEvidenceForAutomation(State);
    }
    const FMetricQueryEvidence* Found = State.QueryEvidence.FindByPredicate([](const FMetricQueryEvidence& E){ return E.Target == EMetricInspectionTarget::Clearance; });
    TestNotNull(TEXT("Clearance evidence is generated"), Found);
    if (Found)
    {
        TestEqual(TEXT("Blocked clearance is failure evidence"), Found->Status, EMetricQueryEvidenceStatus::Fail);
        TestTrue(TEXT("Failure evidence carries contact"), Found->bHasImpact);
        TestTrue(TEXT("Failure evidence carries a non-zero query span"), !Found->StartOffset.Equals(Found->EndOffset));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricInspectionGroundEvidence, "MetricPlugin.Inspection.GroundFailureEvidence", METRIC_AUTOMATION_FLAGS)
bool FMetricInspectionGroundEvidence::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.GroundValidationSettings.bShowGroundValidation = true;
    State.GroundValidationSettings.ForwardDistance = 100.0f;
    State.GroundValidationSettings.SampleSpacing = 50.0f;
    State.GroundValidationSettings.LateralCoverage = 0.0f;
    if (Subsystem)
    {
        Subsystem->EvaluateGroundValidationForAutomation(TestWorld.GetWorld(), State);
        Subsystem->BuildQueryEvidenceForAutomation(State);
    }
    int32 FailCount = 0;
    for (const FMetricQueryEvidence& E : State.QueryEvidence)
    {
        if (E.Target == EMetricInspectionTarget::GroundValidation && E.Status == EMetricQueryEvidenceStatus::Fail)
        {
            ++FailCount;
            TestTrue(TEXT("Missing-floor evidence names the reason"), E.Label.Contains(TEXT("Missing floor")));
        }
    }
    TestTrue(TEXT("Missing floor produces failure evidence"), FailCount > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricInspectionRecoveryEvidence, "MetricPlugin.Inspection.RecoveryDirectionalEvidence", METRIC_AUTOMATION_FLAGS)
bool FMetricInspectionRecoveryEvidence::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(140.0f, 0.0f, 90.0f), FVector(20.0f, 100.0f, 100.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.RecoverySpaceSettings.bShowRecoverySpace = true;
    if (Subsystem)
    {
        Subsystem->EvaluateRecoverySpaceForAutomation(World, State);
        Subsystem->BuildQueryEvidenceForAutomation(State);
    }
    int32 RecoveryCount = 0;
    int32 RecoveryFails = 0;
    for (const FMetricQueryEvidence& E : State.QueryEvidence)
    {
        if (E.Target == EMetricInspectionTarget::RecoverySpace)
        {
            ++RecoveryCount;
            if (E.Status == EMetricQueryEvidenceStatus::Fail) ++RecoveryFails;
        }
    }
    TestEqual(TEXT("Recovery inspection exposes four directional sweeps"), RecoveryCount, 4);
    TestEqual(TEXT("Fixture blocks exactly one recovery direction"), RecoveryFails, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricInspectionCameraEvidence, "MetricPlugin.Inspection.CameraProbeEvidence", METRIC_AUTOMATION_FLAGS)
bool FMetricInspectionCameraEvidence::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnBox(World, FVector(-160.0f, 0.0f, 190.0f), FVector(20.0f, 120.0f, 120.0f), true);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.CameraProbeSettings.bShowCameraProbe = true;
    State.CameraProbeSettings.PivotZOffset = 160.0f;
    State.CameraProbeSettings.TargetDistance = 350.0f;
    State.CameraProbeSettings.VerticalOffset = 60.0f;
    State.CameraProbeSettings.ProbeRadius = 12.0f;
    if (Subsystem)
    {
        Subsystem->EvaluateCameraProbeForAutomation(World, State);
        Subsystem->BuildQueryEvidenceForAutomation(State);
    }
    const FMetricQueryEvidence* Found = State.QueryEvidence.FindByPredicate([](const FMetricQueryEvidence& E){ return E.Target == EMetricInspectionTarget::CameraProbe; });
    TestNotNull(TEXT("Camera probe evidence is generated"), Found);
    if (Found)
    {
        TestTrue(TEXT("Compressed camera carries impact evidence"), Found->bHasImpact);
        TestTrue(TEXT("Camera evidence reports resolved/target distance"), Found->Label.Contains(TEXT("Camera")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricInspectionTrajectoryEvidence, "MetricPlugin.Inspection.TrajectoryEvidence", METRIC_AUTOMATION_FLAGS)
bool FMetricInspectionTrajectoryEvidence::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.TrajectorySettings.bShowTrajectory = true;
    State.TrajectorySettings.DisplayMode = EMetricTrajectoryDisplayMode::SingleArc;
    if (Subsystem)
    {
        Subsystem->EvaluateTrajectoryForAutomation(World, State);
        Subsystem->BuildQueryEvidenceForAutomation(State);
    }
    int32 SegmentCount = 0;
    for (const FMetricQueryEvidence& E : State.QueryEvidence)
    {
        if (E.Target == EMetricInspectionTarget::Trajectory && E.Kind == EMetricQueryEvidenceKind::Sweep)
        {
            ++SegmentCount;
        }
    }
    TestTrue(TEXT("Trajectory inspection exposes sampled sweep segments"), SegmentCount > 1);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricActorMissingProfile, "MetricPlugin.Actor.MissingProfile", METRIC_AUTOMATION_FLAGS)
bool FMetricActorMissingProfile::RunTest(const FString& Parameters)
{
    MetricAutomation::FTestWorld TestWorld;
    AMetricActor* Actor = TestWorld.GetWorld()->SpawnActor<AMetricActor>();
    TestNotNull(TEXT("Metric Actor spawned in isolated test world"), Actor);
    if (Actor)
    {
        Actor->Profile = nullptr;
        Actor->bEnabled = true;
        Actor->RecalculateMetric();
        TestEqual(TEXT("Missing profile is reported"), Actor->OverallResult, EMetricActorOverallResult::MissingProfile);
        TestEqual(TEXT("Missing profile has no analysis issues"), Actor->IssueCount, 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricActorBillboardVisibility, "MetricPlugin.Actor.BillboardVisibilityIndependentFromVisualization", METRIC_AUTOMATION_FLAGS)
bool FMetricActorBillboardVisibility::RunTest(const FString& Parameters)
{
    MetricAutomation::FTestWorld TestWorld;
    AMetricActor* Actor = TestWorld.GetWorld()->SpawnActor<AMetricActor>();
    TestNotNull(TEXT("Metric Actor spawned in isolated test world"), Actor);
    if (Actor)
    {
        UBillboardComponent* Billboard = Actor->FindComponentByClass<UBillboardComponent>();
        TestNotNull(TEXT("Metric Actor owns an editor billboard component"), Billboard);
        TestEqual(TEXT("Metric Actor keeps Editor Billboard Scale at Unreal default"), Actor->SpriteScale, 1.0f);
        if (Billboard)
        {
            TestTrue(TEXT("Metric billboard authors its size on the component rather than Actor SpriteScale"),
                Billboard->ScreenSize > GetDefault<UBillboardComponent>()->ScreenSize);
            UMetricViewportComponent* Visualization = Actor->FindComponentByClass<UMetricViewportComponent>();
            TestNotNull(TEXT("Metric Actor owns an analysis visualization component"), Visualization);
            if (Visualization)
            {
                TestTrue(TEXT("Billboard and analysis visualization are siblings, not parent/child"), Billboard->GetAttachParent() == Actor->GetRootComponent() && Visualization->GetAttachParent() == Actor->GetRootComponent());
            }

            Actor->Profile = nullptr;
            Actor->bEnabled = true;
            Actor->RecalculateMetric();
            const bool bBillboardVisibleBefore = Billboard->GetVisibleFlag();
            if (Visualization)
            {
                Visualization->SetVisibility(false, false);
            }
            TestEqual(TEXT("Hiding Metric analysis visualization preserves the billboard component visibility flag"), Billboard->GetVisibleFlag(), bBillboardVisibleBefore);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricActorDisabled, "MetricPlugin.Actor.Disabled", METRIC_AUTOMATION_FLAGS)
bool FMetricActorDisabled::RunTest(const FString& Parameters)
{
    MetricAutomation::FTestWorld TestWorld;
    AMetricActor* Actor = TestWorld.GetWorld()->SpawnActor<AMetricActor>();
    TestNotNull(TEXT("Metric Actor spawned in isolated test world"), Actor);
    if (Actor)
    {
        Actor->bEnabled = false;
        Actor->RecalculateMetric();
        TestEqual(TEXT("Disabled actor is reported"), Actor->OverallResult, EMetricActorOverallResult::Disabled);
        TestEqual(TEXT("Disabled actor has no analysis issues"), Actor->IssueCount, 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricActorProfilePass, "MetricPlugin.Actor.ProfilePass", METRIC_AUTOMATION_FLAGS)
bool FMetricActorProfilePass::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World, 100.0f, 500.0f, 0.0f);

    UMetricPreset* Profile = NewObject<UMetricPreset>(GetTransientPackage());
    Profile->DisplayName = FText::FromString(TEXT("Automation Pass Profile"));
    Profile->Clearance.bShowClearanceProbe = true;
    Profile->Clearance.ProbeDistance = 100.0f;
    Profile->GroundValidation.bShowGroundValidation = true;
    Profile->GroundValidation.ForwardDistance = 50.0f;
    Profile->GroundValidation.SampleSpacing = 50.0f;
    Profile->GroundValidation.LateralCoverage = 0.0f;

    AMetricActor* Actor = World->SpawnActor<AMetricActor>(FVector::ZeroVector, FRotator::ZeroRotator);
    TestNotNull(TEXT("Metric Actor spawned"), Actor);
    if (Actor)
    {
        Actor->Profile = Profile;
        Actor->RecalculateMetric();
        TestTrue(TEXT("Profile evaluation produces a live state"), Actor->GetEvaluatedState().bHasHit);
        TestEqual(TEXT("Open fixture passes"), Actor->OverallResult, EMetricActorOverallResult::Pass);
        TestEqual(TEXT("Passing fixture has zero failures"), Actor->FailureCount, 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricActorProfileFail, "MetricPlugin.Actor.ProfileFail", METRIC_AUTOMATION_FLAGS)
bool FMetricActorProfileFail::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World, 100.0f, 500.0f, 0.0f);
    MetricAutomation::SpawnBox(World, FVector(60.0f, 0.0f, 90.0f), FVector(10.0f, 100.0f, 100.0f), true);

    UMetricPreset* Profile = NewObject<UMetricPreset>(GetTransientPackage());
    Profile->DisplayName = FText::FromString(TEXT("Automation Fail Profile"));
    Profile->Clearance.bShowClearanceProbe = true;
    Profile->Clearance.ProbeDistance = 150.0f;

    AMetricActor* Actor = World->SpawnActor<AMetricActor>(FVector::ZeroVector, FRotator::ZeroRotator);
    TestNotNull(TEXT("Metric Actor spawned"), Actor);
    if (Actor)
    {
        Actor->Profile = Profile;
        Actor->RecalculateMetric();
        TestEqual(TEXT("Blocking fixture fails"), Actor->OverallResult, EMetricActorOverallResult::Fail);
        TestTrue(TEXT("Failure count is non-zero"), Actor->FailureCount > 0);
        TestTrue(TEXT("Actor exposes query evidence"), Actor->GetEvaluatedState().QueryEvidence.Num() > 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricActorRelevantChangeRequest, "MetricPlugin.Actor.RelevantChangeRequest", METRIC_AUTOMATION_FLAGS)
bool FMetricActorRelevantChangeRequest::RunTest(const FString& Parameters)
{
    MetricAutomation::FTestWorld TestWorld;
    AMetricActor* Actor = TestWorld.GetWorld()->SpawnActor<AMetricActor>();
    TestNotNull(TEXT("Metric Actor spawned in isolated test world"), Actor);
    if (Actor)
    {
        Actor->UpdateMode = EMetricActorUpdateMode::OnRelevantChange;
        Actor->SetActorTickEnabled(false);
        Actor->RequestRelevantUpdate();
        TestTrue(TEXT("Relevant world change schedules one editor evaluation"), Actor->IsActorTickEnabled());

        Actor->UpdateMode = EMetricActorUpdateMode::Manual;
        Actor->SetActorTickEnabled(false);
        Actor->RequestRelevantUpdate();
        TestFalse(TEXT("Manual actors ignore automatic relevant-change requests"), Actor->IsActorTickEnabled());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricActorIndependentFromLiveTool, "MetricPlugin.Actor.IndependentFromLiveTool", METRIC_AUTOMATION_FLAGS)
bool FMetricActorIndependentFromLiveTool::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope Source(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::SpawnFloor(World, 100.0f, 500.0f, 0.0f);

    UMetricPreset* Profile = NewObject<UMetricPreset>(GetTransientPackage());
    Profile->DisplayName = FText::FromString(TEXT("Automation Independent Profile"));
    Profile->GroundValidation.bShowGroundValidation = true;
    Profile->GroundValidation.ForwardDistance = 50.0f;
    Profile->GroundValidation.SampleSpacing = 50.0f;
    Profile->GroundValidation.LateralCoverage = 0.0f;

    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    const bool bWasActive = Subsystem ? Subsystem->IsMetricActive() : false;
    if (Subsystem) Subsystem->SetMetricActive(false);

    AMetricActor* Actor = World->SpawnActor<AMetricActor>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (Actor)
    {
        Actor->Profile = Profile;
        Actor->RecalculateMetric();
        TestTrue(TEXT("Metric Actor evaluates while Live Metric is off"), Actor->GetEvaluatedState().bHasHit);
    }

    if (Subsystem) Subsystem->SetMetricActive(bWasActive);
    return true;
}


static bool RunMetricPerformanceEnvironmentBenchmark(FAutomationTestBase& Test, EMetricEnvironmentSource Source, const TCHAR* Label)
{
    MetricAutomation::FEnvironmentSourceScope SourceScope(Source);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::BuildPerformanceFixture(World);
    UMetricPreset* Profile = MetricAutomation::MakePerformanceProfile();
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    Test.TestNotNull(TEXT("Metric subsystem"), Subsystem);
    if (!Subsystem) return false;

    MetricPerformance::BeginCapture(FString::Printf(TEXT("Synthetic %s | 60 iterations"), Label));
    for (int32 Index = 0; Index < 60; ++Index)
    {
        FMetricLiveState State;
        Subsystem->EvaluatePresetAtLocation(World, Profile, FVector::ZeroVector, FVector::ForwardVector, FVector::UpVector, FVector::UpVector, State);
    }
    const MetricPerformance::FCaptureSummary Summary = MetricPerformance::EndCapture();
    Test.AddInfo(Summary.ToString());
    Test.TestEqual(TEXT("All benchmark generations completed"), Summary.Generations, 60);
    Test.TestTrue(TEXT("Benchmark issued environment queries"), Summary.Analyses.Num() > 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricPerformanceCollision, "MetricPlugin.Performance.Environment.Collision", METRIC_AUTOMATION_FLAGS)
bool FMetricPerformanceCollision::RunTest(const FString& Parameters)
{
    return RunMetricPerformanceEnvironmentBenchmark(*this, EMetricEnvironmentSource::Collision, TEXT("Collision"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricPerformanceHybrid, "MetricPlugin.Performance.Environment.Hybrid", METRIC_AUTOMATION_FLAGS)
bool FMetricPerformanceHybrid::RunTest(const FString& Parameters)
{
    return RunMetricPerformanceEnvironmentBenchmark(*this, EMetricEnvironmentSource::Hybrid, TEXT("Hybrid"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricPerformanceVisual, "MetricPlugin.Performance.Environment.VisualGeometry", METRIC_AUTOMATION_FLAGS)
bool FMetricPerformanceVisual::RunTest(const FString& Parameters)
{
    return RunMetricPerformanceEnvironmentBenchmark(*this, EMetricEnvironmentSource::VisualGeometry, TEXT("Visual Geometry"));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetricPerformanceBudgeted, "MetricPlugin.Performance.LiveAnalysis.Budgeted", METRIC_AUTOMATION_FLAGS)
bool FMetricPerformanceBudgeted::RunTest(const FString& Parameters)
{
    MetricAutomation::FEnvironmentSourceScope SourceScope(EMetricEnvironmentSource::Collision);
    MetricAutomation::FTestWorld TestWorld;
    UWorld* World = TestWorld.GetWorld();
    MetricAutomation::BuildPerformanceFixture(World);
    UMetricSubsystem* Subsystem = MetricAutomation::GetSubsystem();
    TestNotNull(TEXT("Metric subsystem"), Subsystem);
    if (!Subsystem) return false;

    const bool bWasActive = Subsystem->IsMetricActive();
    Subsystem->SetMetricActive(true);
    FMetricLiveState State = MetricAutomation::MakeAnalysisState();
    State.SensorFieldSettings.bShowSensorField = true;
    State.TrajectorySettings.bShowTrajectory = true;
    State.GroundValidationSettings.bShowGroundValidation = true;
    State.FitAnalysisSettings.bShowFitAnalysis = true;
    State.ThresholdAnalysisSettings.bShowThresholdAnalysis = true;
    State.RecoverySpaceSettings.bShowRecoverySpace = true;

    MetricPerformance::BeginCapture(TEXT("Synthetic Budgeted Live Analysis | 0.10 ms soft budget"));
    Subsystem->BeginLiveAnalysis(World, State);
    int32 Frames = 0;
    while (Subsystem->HasPendingLiveAnalysis() && Frames < 128)
    {
        Subsystem->ProcessLiveAnalysisBudget(World, 0.10);
        ++Frames;
    }
    const MetricPerformance::FCaptureSummary Summary = MetricPerformance::EndCapture();
    AddInfo(Summary.ToString());
    TestTrue(TEXT("Budgeted generation completed"), !Subsystem->HasPendingLiveAnalysis());
    TestTrue(TEXT("Budget scheduler recorded frame slices"), Summary.BudgetFrames > 0);
    Subsystem->SetMetricActive(bWasActive);
    return true;
}


#undef METRIC_AUTOMATION_FLAGS

#endif // WITH_DEV_AUTOMATION_TESTS
