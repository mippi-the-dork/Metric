#include "MetricEnvironmentQuery.h"

#include "MetricProjectSettings.h"
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

}
