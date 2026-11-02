#include "MetricPerformance.h"
#include "HAL/PlatformTime.h"
#include "HAL/CriticalSection.h"
#include "Misc/ScopeLock.h"

namespace MetricPerformance
{
    namespace
    {
        FCaptureSummary Summary;
        bool bCapturing = false;
        FName CurrentAnalysis = NAME_None;
        double GenerationStartSeconds = 0.0;

        FCriticalSection PipelineMutex;
        FPipelineCaptureSummary PipelineSummary;
        bool bPipelineCapturing = false;
    }

    static double Percentile(TArray<double> Values, double P)
    {
        if (Values.IsEmpty()) return 0.0;
        Values.Sort();
        const int32 Index = FMath::Clamp(FMath::RoundToInt((Values.Num() - 1) * P), 0, Values.Num() - 1);
        return Values[Index];
    }


    FString FPipelineCaptureSummary::ToString() const
    {
        FString Out = FString::Printf(TEXT("Metric Live Pipeline | %d publishes\n"), Publishes);
        Out += TEXT("Stage                              Avg ms  Median     P95   Max ms   Calls  Calls/Publish  Total ms\n");
        TArray<FName> Names;
        Stages.GetKeys(Names);
        Names.Sort([this](const FName& A, const FName& B)
        {
            return Stages[A].TotalMs > Stages[B].TotalMs;
        });
        for (const FName Name : Names)
        {
            const FPipelineStageStats& S = Stages[Name];
            const double CallsPerPublish = Publishes > 0 ? double(S.Calls) / double(Publishes) : 0.0;
            Out += FString::Printf(TEXT("%-34s %7.3f %7.3f %7.3f %8.3f %7lld %13.2f %9.3f\n"),
                *Name.ToString(), S.Calls ? S.TotalMs / double(S.Calls) : 0.0,
                Percentile(S.SamplesMs, 0.50), Percentile(S.SamplesMs, 0.95), S.MaxMs,
                S.Calls, CallsPerPublish, S.TotalMs);
        }
        return Out;
    }
    FString FCaptureSummary::ToString() const
    {
        const double AvgGeneration = Generations > 0 ? TotalGenerationMs / Generations : 0.0;
        FString Out = FString::Printf(TEXT("Metric Performance | %s\nGenerations: %d | Avg %.3f ms | Median %.3f ms | P95 %.3f ms | Max %.3f ms\nBudget frames: %d | Overruns: %d | Worst frame %.3f ms\n"),
            *Label, Generations, AvgGeneration, Percentile(GenerationSamplesMs, 0.50), Percentile(GenerationSamplesMs, 0.95), MaxGenerationMs,
            BudgetFrames, BudgetOverruns, WorstBudgetFrameMs);
        Out += TEXT("Analysis                         Avg ms  Median     P95   Max ms   Calls  Lines  Sweeps  Queries\n");
        TArray<FName> Names;
        Analyses.GetKeys(Names);
        for (const FName Name : Names)
        {
            const FAnalysisStats& S = Analyses[Name];
            Out += FString::Printf(TEXT("%-30s %7.3f %7.3f %7.3f %8.3f %6lld %6lld %7lld %8lld\n"),
                *Name.ToString(), S.Calls ? S.TotalMs / double(S.Calls) : 0.0, Percentile(S.SamplesMs, 0.50), Percentile(S.SamplesMs, 0.95), S.MaxMs,
                S.Calls, S.LineTraces, S.Sweeps, S.LineTraces + S.Sweeps);
        }
        return Out;
    }

    void BeginCapture(const FString& Label)
    {
        Summary = FCaptureSummary();
        Summary.Label = Label;
        bCapturing = true;
        CurrentAnalysis = NAME_None;
        GenerationStartSeconds = 0.0;
    }

    FCaptureSummary EndCapture()
    {
        bCapturing = false;
        CurrentAnalysis = NAME_None;
        return Summary;
    }

    bool IsCapturing() { return bCapturing; }

    void BeginGeneration()
    {
        if (bCapturing) GenerationStartSeconds = FPlatformTime::Seconds();
    }

    void EndGeneration()
    {
        if (!bCapturing || GenerationStartSeconds <= 0.0) return;
        const double Ms = (FPlatformTime::Seconds() - GenerationStartSeconds) * 1000.0;
        ++Summary.Generations;
        Summary.TotalGenerationMs += Ms;
        Summary.MaxGenerationMs = FMath::Max(Summary.MaxGenerationMs, Ms);
        Summary.GenerationSamplesMs.Add(Ms);
        GenerationStartSeconds = 0.0;
    }

    void RecordBudgetFrame(double ElapsedMs, double BudgetMs)
    {
        if (!bCapturing) return;
        ++Summary.BudgetFrames;
        Summary.WorstBudgetFrameMs = FMath::Max(Summary.WorstBudgetFrameMs, ElapsedMs);
        if (BudgetMs > 0.0 && ElapsedMs > BudgetMs) ++Summary.BudgetOverruns;
    }

    void RecordLineTrace()
    {
        if (bCapturing && !CurrentAnalysis.IsNone()) ++Summary.Analyses.FindOrAdd(CurrentAnalysis).LineTraces;
    }
    void RecordSweep()
    {
        if (bCapturing && !CurrentAnalysis.IsNone()) ++Summary.Analyses.FindOrAdd(CurrentAnalysis).Sweeps;
    }

    void BeginPipelineCapture(int32 TargetPublishes)
    {
        FScopeLock Lock(&PipelineMutex);
        PipelineSummary = FPipelineCaptureSummary();
        PipelineSummary.TargetPublishes = FMath::Max(1, TargetPublishes);
        bPipelineCapturing = true;
    }

    bool IsPipelineCapturing()
    {
        FScopeLock Lock(&PipelineMutex);
        return bPipelineCapturing;
    }

    void RecordPipelineStage(FName Name, double ElapsedMs)
    {
        FScopeLock Lock(&PipelineMutex);
        if (!bPipelineCapturing) return;
        FPipelineStageStats& S = PipelineSummary.Stages.FindOrAdd(Name);
        ++S.Calls;
        S.TotalMs += ElapsedMs;
        S.MaxMs = FMath::Max(S.MaxMs, ElapsedMs);
        S.SamplesMs.Add(ElapsedMs);
    }

    void RecordPipelinePublish()
    {
        FPipelineCaptureSummary Completed;
        bool bCompleted = false;
        {
            FScopeLock Lock(&PipelineMutex);
            if (!bPipelineCapturing) return;
            ++PipelineSummary.Publishes;
            if (PipelineSummary.Publishes >= PipelineSummary.TargetPublishes)
            {
                Completed = PipelineSummary;
                bPipelineCapturing = false;
                bCompleted = true;
            }
        }
        if (bCompleted)
        {
            UE_LOG(LogTemp, Display, TEXT("%s"), *Completed.ToString());
        }
    }

    FScopedPipelineStage::FScopedPipelineStage(FName InName)
        : Name(InName)
    {
        if (!IsPipelineCapturing()) return;
        bActive = true;
        StartSeconds = FPlatformTime::Seconds();
    }

    FScopedPipelineStage::~FScopedPipelineStage()
    {
        if (!bActive) return;
        RecordPipelineStage(Name, (FPlatformTime::Seconds() - StartSeconds) * 1000.0);
    }

    FScopedAnalysis::FScopedAnalysis(FName InName)
        : Name(InName)
    {
        if (!bCapturing) return;
        bActive = true;
        PreviousName = CurrentAnalysis;
        CurrentAnalysis = Name;
        StartSeconds = FPlatformTime::Seconds();
    }

    FScopedAnalysis::~FScopedAnalysis()
    {
        if (!bActive) return;
        const double Ms = (FPlatformTime::Seconds() - StartSeconds) * 1000.0;
        FAnalysisStats& S = Summary.Analyses.FindOrAdd(Name);
        ++S.Calls;
        S.TotalMs += Ms;
        S.MaxMs = FMath::Max(S.MaxMs, Ms);
        S.SamplesMs.Add(Ms);
        CurrentAnalysis = PreviousName;
    }
}
