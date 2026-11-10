#include "MetricFrameUtils.h"

#include "MetricGravityProvider.h"
#include "MetricProjectSettings.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace MetricFrameUtils
{
    bool TryResolveProviderGravity(
        UWorld* World,
        const FVector& WorldLocation,
        AActor* ContextActor,
        FVector& OutUpDirection,
        float& OutGravityMagnitude)
    {
        OutUpDirection = FVector::UpVector;
        OutGravityMagnitude = 0.0f;
        if (!World)
        {
            return false;
        }

        const UMetricProjectSettings* ProjectSettings = GetDefault<UMetricProjectSettings>();
        if (!ProjectSettings || ProjectSettings->GravityProviderClass.IsNull())
        {
            return false;
        }

        UClass* ProviderClass = ProjectSettings->GravityProviderClass.Get();
        if (!ProviderClass)
        {
            ProviderClass = ProjectSettings->GravityProviderClass.LoadSynchronous();
        }
        if (!ProviderClass || !ProviderClass->IsChildOf(UMetricGravityProvider::StaticClass()))
        {
            return false;
        }

        const UMetricGravityProvider* Provider = ProviderClass->GetDefaultObject<UMetricGravityProvider>();
        if (!Provider)
        {
            return false;
        }

        FVector CandidateUp = FVector::ZeroVector;
        float CandidateMagnitude = 0.0f;
        const bool bResolved = Provider->SampleGravityForMetric(
            World,
            WorldLocation,
            ContextActor,
            CandidateUp,
            CandidateMagnitude);

        if (!bResolved || CandidateUp.IsNearlyZero())
        {
            return false;
        }

        OutUpDirection = CandidateUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
        OutGravityMagnitude = FMath::Max(0.0f, CandidateMagnitude);
        return true;
    }

    bool TryResolveProviderUp(UWorld* World, const FVector& WorldLocation, FVector& OutUpDirection)
    {
        float IgnoredMagnitude = 0.0f;
        return TryResolveProviderGravity(World, WorldLocation, nullptr, OutUpDirection, IgnoredMagnitude);
    }

    FVector ResolveSurfaceUp(bool bFollowSurface, const FVector& FixedUp, const FVector& SampleSurfaceNormal)
    {
        if (bFollowSurface && !SampleSurfaceNormal.IsNearlyZero())
        {
            return SampleSurfaceNormal.GetSafeNormal(SMALL_NUMBER, FixedUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector));
        }
        return FixedUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
    }

    FVector TransportForward(const FVector& PreviousForward, const FVector& NewUp)
    {
        const FVector SafeUp = NewUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
        FVector Forward = FVector::VectorPlaneProject(PreviousForward, SafeUp).GetSafeNormal();
        if (Forward.IsNearlyZero())
        {
            Forward = FVector::VectorPlaneProject(FVector::ForwardVector, SafeUp).GetSafeNormal();
        }
        if (Forward.IsNearlyZero())
        {
            Forward = FVector::VectorPlaneProject(FVector::RightVector, SafeUp).GetSafeNormal();
        }
        return Forward;
    }

    FMetricAnalysisBasis BuildAnalysisBasis(
        EMetricAnalysisFrameMode Mode,
        const FVector& SurfaceNormal,
        const FVector& CustomUp,
        const FVector& PreferredForward)
    {
        FMetricAnalysisBasis Basis;

        switch (Mode)
        {
        case EMetricAnalysisFrameMode::SurfaceNormal:
            Basis.Up = SurfaceNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
            break;
        case EMetricAnalysisFrameMode::CustomUp:
        case EMetricAnalysisFrameMode::Provider:
            Basis.Up = CustomUp.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
            break;
        case EMetricAnalysisFrameMode::World:
        default:
            Basis.Up = FVector::UpVector;
            break;
        }

        Basis.Forward = TransportForward(PreferredForward, Basis.Up);
        Basis.Right = FVector::CrossProduct(Basis.Up, Basis.Forward).GetSafeNormal();
        if (Basis.Right.IsNearlyZero())
        {
            Basis.Right = FVector::RightVector;
        }
        Basis.Forward = FVector::CrossProduct(Basis.Right, Basis.Up).GetSafeNormal();
        return Basis;
    }
}
