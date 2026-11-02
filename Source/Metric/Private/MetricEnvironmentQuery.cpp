#include "MetricEnvironmentQuery.h"

#include "MetricProjectSettings.h"
#include "MetricEditorSettings.h"
#include "MetricPerformance.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "ScopedTransaction.h"

namespace MetricEnvironmentQuery
{
    namespace
    {
        struct FSessionWorldExclusions
        {
            TArray<TWeakObjectPtr<AActor>> Actors;
            TArray<TWeakObjectPtr<UPrimitiveComponent>> Components;
        };

        TMap<TWeakObjectPtr<UWorld>, FSessionWorldExclusions> SessionExclusions;

        UWorld* GetCurrentEditorWorld()
        {
            return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        }

        FSessionWorldExclusions* FindSessionExclusions(UWorld* World)
        {
            if (!World)
            {
                return nullptr;
            }

            for (auto It = SessionExclusions.CreateIterator(); It; ++It)
            {
                if (!It.Key().IsValid())
                {
                    It.RemoveCurrent();
                }
            }

            return SessionExclusions.Find(TWeakObjectPtr<UWorld>(World));
        }

        FSessionWorldExclusions& FindOrAddSessionExclusions(UWorld* World)
        {
            check(World);
            return SessionExclusions.FindOrAdd(TWeakObjectPtr<UWorld>(World));
        }

        void CompactSessionExclusions(FSessionWorldExclusions& Exclusions)
        {
            Exclusions.Actors.RemoveAllSwap([](const TWeakObjectPtr<AActor>& Actor){ return !Actor.IsValid(); });
            Exclusions.Components.RemoveAllSwap([](const TWeakObjectPtr<UPrimitiveComponent>& Component){ return !Component.IsValid(); });
        }

        bool HasAnyTag(const AActor* Actor, const TArray<FName>& Tags)
        {
            if (!Actor)
            {
                return false;
            }
            for (const FName Tag : Tags)
            {
                if (!Tag.IsNone() && Actor->ActorHasTag(Tag))
                {
                    return true;
                }
            }
            return false;
        }

        bool HasAnyTag(const UActorComponent* Component, const TArray<FName>& Tags)
        {
            if (!Component)
            {
                return false;
            }
            for (const FName Tag : Tags)
            {
                if (!Tag.IsNone() && Component->ComponentHasTag(Tag))
                {
                    return true;
                }
            }
            return false;
        }

        bool MatchesExcludedActorClass(const AActor* Actor, const TArray<TSoftClassPtr<AActor>>& Classes)
        {
            if (!Actor)
            {
                return false;
            }
            for (const TSoftClassPtr<AActor>& SoftClass : Classes)
            {
                if (UClass* Class = SoftClass.LoadSynchronous())
                {
                    if (Actor->IsA(Class))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        bool MatchesExcludedComponentClass(const UActorComponent* Component, const TArray<TSoftClassPtr<UActorComponent>>& Classes)
        {
            if (!Component)
            {
                return false;
            }
            for (const TSoftClassPtr<UActorComponent>& SoftClass : Classes)
            {
                if (UClass* Class = SoftClass.LoadSynchronous())
                {
                    if (Component->IsA(Class))
                    {
                        return true;
                    }
                }
            }
            return false;
        }

        bool IsVisuallyExcluded(UWorld* World, const AActor* Actor, const UPrimitiveComponent* Component)
        {
            if (!World || !Actor || !Component)
            {
                return true;
            }

            if (FSessionWorldExclusions* Exclusions = FindSessionExclusions(World))
            {
                CompactSessionExclusions(*Exclusions);
                if (Exclusions->Actors.ContainsByPredicate([Actor](const TWeakObjectPtr<AActor>& Item){ return Item.Get() == Actor; })
                    || Exclusions->Components.ContainsByPredicate([Component](const TWeakObjectPtr<UPrimitiveComponent>& Item){ return Item.Get() == Component; }))
                {
                    return true;
                }
            }

            const UMetricProjectSettings* ProjectSettings = GetDefault<UMetricProjectSettings>();
            if (!ProjectSettings)
            {
                return false;
            }

            return MatchesExcludedActorClass(Actor, ProjectSettings->ExcludedActorClasses)
                || HasAnyTag(Actor, ProjectSettings->ExcludedActorTags)
                || MatchesExcludedComponentClass(Component, ProjectSettings->ExcludedComponentClasses)
                || HasAnyTag(Component, ProjectSettings->ExcludedComponentTags);
        }

        bool IntersectSegmentBox(const FVector& Start, const FVector& End, const FBox& Box, float& OutTime, FVector& OutNormal)
        {
            const FVector Delta = End - Start;
            float TMin = 0.0f;
            float TMax = 1.0f;
            FVector EnterNormal = FVector::ZeroVector;

            for (int32 Axis = 0; Axis < 3; ++Axis)
            {
                const float S = Start[Axis];
                const float D = Delta[Axis];
                const float Min = Box.Min[Axis];
                const float Max = Box.Max[Axis];

                if (FMath::IsNearlyZero(D))
                {
                    if (S < Min || S > Max)
                    {
                        return false;
                    }
                    continue;
                }

                float T1 = (Min - S) / D;
                float T2 = (Max - S) / D;
                FVector N1 = FVector::ZeroVector;
                FVector N2 = FVector::ZeroVector;
                N1[Axis] = -1.0f;
                N2[Axis] = 1.0f;
                if (T1 > T2)
                {
                    Swap(T1, T2);
                    Swap(N1, N2);
                }

                if (T1 > TMin)
                {
                    TMin = T1;
                    EnterNormal = N1;
                }
                TMax = FMath::Min(TMax, T2);
                if (TMin > TMax)
                {
                    return false;
                }
            }

            if (TMin < 0.0f || TMin > 1.0f)
            {
                return false;
            }

            OutTime = TMin;
            OutNormal = EnterNormal.IsNearlyZero() ? FVector::UpVector : EnterNormal;
            return true;
        }

        bool TraceApproximateVisibleBounds(UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, const FVector& Inflate, bool bOnlyCollisionDisabled)
        {
            if (!World)
            {
                return false;
            }

            bool bFound = false;
            float BestTime = TNumericLimits<float>::Max();
            UPrimitiveComponent* BestComponent = nullptr;
            FVector BestNormal = FVector::UpVector;

            for (TActorIterator<AActor> It(World); It; ++It)
            {
                AActor* Actor = *It;
                if (!IsValid(Actor) || Actor->IsHiddenEd())
                {
                    continue;
                }

                TInlineComponentArray<UPrimitiveComponent*> Components;
                Actor->GetComponents(Components);
                for (UPrimitiveComponent* Component : Components)
                {
                    if (!IsValid(Component) || !Component->IsRegistered() || !Component->IsVisible() || IsVisuallyExcluded(World, Actor, Component))
                    {
                        continue;
                    }
                    if (bOnlyCollisionDisabled && Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
                    {
                        continue;
                    }

                    FBox Box = Component->Bounds.GetBox();
                    Box.Min -= Inflate;
                    Box.Max += Inflate;
                    float Time = 0.0f;
                    FVector Normal;
                    if (IntersectSegmentBox(Start, End, Box, Time, Normal) && Time < BestTime)
                    {
                        BestTime = Time;
                        BestComponent = Component;
                        BestNormal = Normal;
                        bFound = true;
                    }
                }
            }

            if (!bFound || !BestComponent)
            {
                return false;
            }

            const FVector Point = FMath::Lerp(Start, End, BestTime);
            OutHit = FHitResult();
            OutHit.bBlockingHit = true;
            OutHit.Time = BestTime;
            OutHit.Distance = FVector::Distance(Start, Point);
            OutHit.Location = Point;
            OutHit.ImpactPoint = Point;
            OutHit.Normal = BestNormal;
            OutHit.ImpactNormal = BestNormal;
            OutHit.Component = BestComponent;
            return true;
        }


    }

    void ApplyExclusions(UWorld* World, FCollisionQueryParams& Params)
    {
        if (!World)
        {
            return;
        }

        const UMetricProjectSettings* ProjectSettings = GetDefault<UMetricProjectSettings>();

        const TArray<TSoftClassPtr<AActor>> EmptyActorClasses;
        const TArray<FName> EmptyActorTags;
        const TArray<TSoftClassPtr<UActorComponent>> EmptyComponentClasses;
        const TArray<FName> EmptyComponentTags;
        const TArray<TSoftClassPtr<AActor>>& ActorClasses = ProjectSettings ? ProjectSettings->ExcludedActorClasses : EmptyActorClasses;
        const TArray<FName>& ActorTags = ProjectSettings ? ProjectSettings->ExcludedActorTags : EmptyActorTags;
        const TArray<TSoftClassPtr<UActorComponent>>& ComponentClasses = ProjectSettings ? ProjectSettings->ExcludedComponentClasses : EmptyComponentClasses;
        const TArray<FName>& ComponentTags = ProjectSettings ? ProjectSettings->ExcludedComponentTags : EmptyComponentTags;
        // Explicit Actor/Component exclusions are intentionally session + world scoped.
        // They are never serialized into Editor Settings, so changing maps cannot
        // accumulate stale level-object references.
        if (FSessionWorldExclusions* Exclusions = FindSessionExclusions(World))
        {
            CompactSessionExclusions(*Exclusions);
            for (const TWeakObjectPtr<AActor>& Actor : Exclusions->Actors)
            {
                if (Actor.IsValid() && Actor->GetWorld() == World)
                {
                    Params.AddIgnoredActor(Actor.Get());
                }
            }
            for (const TWeakObjectPtr<UPrimitiveComponent>& Component : Exclusions->Components)
            {
                if (Component.IsValid() && Component->GetWorld() == World)
                {
                    Params.AddIgnoredComponent(Component.Get());
                }
            }
        }

        const bool bHasActorRuleScan = ActorClasses.Num() > 0 || ActorTags.Num() > 0;
        const bool bHasComponentRuleScan = ComponentClasses.Num() > 0 || ComponentTags.Num() > 0;
        if (!bHasActorRuleScan && !bHasComponentRuleScan)
        {
            return;
        }

        for (TActorIterator<AActor> It(World); It; ++It)
        {
            AActor* Actor = *It;
            if (!IsValid(Actor))
            {
                continue;
            }

            if ((bHasActorRuleScan && MatchesExcludedActorClass(Actor, ActorClasses))
                || (bHasActorRuleScan && HasAnyTag(Actor, ActorTags)))
            {
                Params.AddIgnoredActor(Actor);
                continue;
            }

            if (bHasComponentRuleScan)
            {
                TInlineComponentArray<UActorComponent*> Components;
                Actor->GetComponents(Components);
                for (UActorComponent* Component : Components)
                {
                    if (!IsValid(Component))
                    {
                        continue;
                    }
                    if (MatchesExcludedComponentClass(Component, ComponentClasses)
                        || HasAnyTag(Component, ComponentTags))
                    {
                        if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component))
                        {
                            Params.AddIgnoredComponent(Primitive);
                        }
                    }
                }
            }
        }
    }

    FCollisionQueryParams MakeParams(UWorld* World, FName TraceTag, bool bTraceComplex)
    {
        FCollisionQueryParams Params(TraceTag, bTraceComplex);
        ApplyExclusions(World, Params);
        return Params;
    }

    FCollisionQueryParams MakeParams(UWorld* World, FName TraceTag, const TStatId& StatId, bool bTraceComplex)
    {
        FCollisionQueryParams Params(TraceTag, StatId, bTraceComplex);
        ApplyExclusions(World, Params);
        return Params;
    }


    bool LineTraceSingleByChannel(UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, ECollisionChannel Channel, const FCollisionQueryParams& Params)
    {
        MetricPerformance::RecordLineTrace();
        if (!World)
        {
            return false;
        }

        const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
        const EMetricEnvironmentSource Source = Settings ? Settings->EnvironmentSource : EMetricEnvironmentSource::Collision;
        if (Source == EMetricEnvironmentSource::Collision)
        {
            return World->LineTraceSingleByChannel(OutHit, Start, End, Channel, Params);
        }

        if (Source == EMetricEnvironmentSource::VisualGeometry)
        {
            return TraceApproximateVisibleBounds(World, OutHit, Start, End, FVector::ZeroVector, false);
        }

        FHitResult CollisionHit;
        const bool bCollisionHit = World->LineTraceSingleByChannel(CollisionHit, Start, End, Channel, Params);
        FHitResult ApproxHit;
        const bool bApproxHit = TraceApproximateVisibleBounds(World, ApproxHit, Start, End, FVector::ZeroVector, true);
        if (bCollisionHit && (!bApproxHit || CollisionHit.Distance <= ApproxHit.Distance))
        {
            OutHit = CollisionHit;
            return true;
        }
        if (bApproxHit)
        {
            OutHit = ApproxHit;
            return true;
        }
        return false;
    }

    bool SweepSingleByChannel(UWorld* World, FHitResult& OutHit, const FVector& Start, const FVector& End, const FQuat& Rotation, ECollisionChannel Channel, const FCollisionShape& Shape, const FCollisionQueryParams& Params)
    {
        MetricPerformance::RecordSweep();
        if (!World)
        {
            return false;
        }

        const UMetricEditorSettings* Settings = GetDefault<UMetricEditorSettings>();
        const EMetricEnvironmentSource Source = Settings ? Settings->EnvironmentSource : EMetricEnvironmentSource::Collision;
        if (Source == EMetricEnvironmentSource::Collision)
        {
            return World->SweepSingleByChannel(OutHit, Start, End, Rotation, Channel, Shape, Params);
        }

        const FVector Inflate = Shape.GetExtent();
        if (Source == EMetricEnvironmentSource::VisualGeometry)
        {
            return TraceApproximateVisibleBounds(World, OutHit, Start, End, Inflate, false);
        }

        FHitResult CollisionHit;
        const bool bCollisionHit = World->SweepSingleByChannel(CollisionHit, Start, End, Rotation, Channel, Shape, Params);
        FHitResult ApproxHit;
        const bool bApproxHit = TraceApproximateVisibleBounds(World, ApproxHit, Start, End, Inflate, true);
        if (bCollisionHit && (!bApproxHit || CollisionHit.Distance <= ApproxHit.Distance))
        {
            OutHit = CollisionHit;
            return true;
        }
        if (bApproxHit)
        {
            OutHit = ApproxHit;
            return true;
        }
        return false;
    }

    int32 GetSelectedActorCount()
    {
        return (GEditor && GEditor->GetSelectedActors()) ? GEditor->GetSelectedActors()->Num() : 0;
    }

    int32 GetSelectedComponentCount()
    {
        if (!GEditor || !GEditor->GetSelectedComponents())
        {
            return 0;
        }

        int32 Count = 0;
        for (FSelectionIterator It(*GEditor->GetSelectedComponents()); It; ++It)
        {
            if (IsValid(Cast<UPrimitiveComponent>(*It)))
            {
                ++Count;
            }
        }
        return Count;
    }

    int32 GetInstanceExclusionCount()
    {
        UWorld* World = GetCurrentEditorWorld();
        if (FSessionWorldExclusions* Exclusions = FindSessionExclusions(World))
        {
            CompactSessionExclusions(*Exclusions);
            return Exclusions->Actors.Num() + Exclusions->Components.Num();
        }
        return 0;
    }

    int32 ExcludeSelectedActors()
    {
        if (!GEditor || !GEditor->GetSelectedActors())
        {
            return 0;
        }

        int32 Added = 0;
        for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
        {
            AActor* Actor = Cast<AActor>(*It);
            if (!IsValid(Actor) || !Actor->GetWorld())
            {
                continue;
            }

            FSessionWorldExclusions& Exclusions = FindOrAddSessionExclusions(Actor->GetWorld());
            CompactSessionExclusions(Exclusions);
            if (!Exclusions.Actors.Contains(TWeakObjectPtr<AActor>(Actor)))
            {
                Exclusions.Actors.Add(Actor);
                ++Added;
            }
        }
        return Added;
    }

    int32 ExcludeSelectedComponents()
    {
        if (!GEditor || !GEditor->GetSelectedComponents())
        {
            return 0;
        }

        int32 Added = 0;
        for (FSelectionIterator It(*GEditor->GetSelectedComponents()); It; ++It)
        {
            UPrimitiveComponent* Component = Cast<UPrimitiveComponent>(*It);
            if (!IsValid(Component) || !Component->GetWorld())
            {
                continue;
            }

            FSessionWorldExclusions& Exclusions = FindOrAddSessionExclusions(Component->GetWorld());
            CompactSessionExclusions(Exclusions);
            if (!Exclusions.Components.Contains(TWeakObjectPtr<UPrimitiveComponent>(Component)))
            {
                Exclusions.Components.Add(Component);
                ++Added;
            }
        }
        return Added;
    }

    void ClearInstanceExclusions()
    {
        if (UWorld* World = GetCurrentEditorWorld())
        {
            SessionExclusions.Remove(TWeakObjectPtr<UWorld>(World));
        }
    }

#if WITH_DEV_AUTOMATION_TESTS
    void AddActorExclusionForAutomation(UWorld* World, AActor* Actor)
    {
        if (!World || !IsValid(Actor) || Actor->GetWorld() != World)
        {
            return;
        }
        FSessionWorldExclusions& Exclusions = FindOrAddSessionExclusions(World);
        CompactSessionExclusions(Exclusions);
        Exclusions.Actors.AddUnique(TWeakObjectPtr<AActor>(Actor));
    }

    void AddComponentExclusionForAutomation(UWorld* World, UPrimitiveComponent* Component)
    {
        if (!World || !IsValid(Component) || Component->GetWorld() != World)
        {
            return;
        }
        FSessionWorldExclusions& Exclusions = FindOrAddSessionExclusions(World);
        CompactSessionExclusions(Exclusions);
        Exclusions.Components.AddUnique(TWeakObjectPtr<UPrimitiveComponent>(Component));
    }

    int32 GetInstanceExclusionCountForAutomation(UWorld* World)
    {
        if (FSessionWorldExclusions* Exclusions = FindSessionExclusions(World))
        {
            CompactSessionExclusions(*Exclusions);
            return Exclusions->Actors.Num() + Exclusions->Components.Num();
        }
        return 0;
    }

    void ClearInstanceExclusionsForAutomation(UWorld* World)
    {
        if (World)
        {
            SessionExclusions.Remove(TWeakObjectPtr<UWorld>(World));
        }
    }
#endif

}
