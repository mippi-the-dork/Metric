#include "MetricPerformance.h"
#include "HAL/PlatformTime.h"

namespace MetricPerformance
{
    namespace
    {
        FCaptureSummary Summary;
        bool bCapturing = false;
        FName CurrentAnalysis = NAME_None;
        double GenerationStartSeconds = 0.0;
    }

    static double Percentile(TArray<double> Values, double P)
    {
        if (Values.IsEmpty()) return 0.0;
        Values.Sort();
        const int32 Index = FMath::Clamp(FMath::RoundToInt((Values.Num() - 1) * P), 0, Values.Num() - 1);
        return Values[Index];
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
