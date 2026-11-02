#pragma once

#include "CoreMinimal.h"

namespace MetricPerformance
{
    struct FAnalysisStats
    {
        double TotalMs = 0.0;
        double MaxMs = 0.0;
        int64 Calls = 0;
        int64 LineTraces = 0;
        int64 Sweeps = 0;
        TArray<double> SamplesMs;
    };

    struct FCaptureSummary
    {
        FString Label;
        int32 Generations = 0;
        double TotalGenerationMs = 0.0;
        double MaxGenerationMs = 0.0;
        TArray<double> GenerationSamplesMs;
        int32 BudgetFrames = 0;
        int32 BudgetOverruns = 0;
        double WorstBudgetFrameMs = 0.0;
        TMap<FName, FAnalysisStats> Analyses;

        FString ToString() const;
    };

    void BeginCapture(const FString& Label);
    FCaptureSummary EndCapture();
    bool IsCapturing();

    void BeginGeneration();
    void EndGeneration();
    void RecordBudgetFrame(double ElapsedMs, double BudgetMs);
    void RecordLineTrace();
    void RecordSweep();

    class FScopedAnalysis
    {
    public:
        explicit FScopedAnalysis(FName InName);
        ~FScopedAnalysis();
    private:
        FName Name;
        FName PreviousName;
        double StartSeconds = 0.0;
        bool bActive = false;
    };
}
