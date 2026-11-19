#include "MetricDiagnosticUtils.h"

#define LOCTEXT_NAMESPACE "MetricDiagnosticUtils"

namespace MetricDiagnosticUtils
{
    FText GetInspectionTargetDisplayName(EMetricInspectionTarget Target)
    {
        switch (Target)
        {
        case EMetricInspectionTarget::Clearance: return LOCTEXT("Clearance", "Clearance");
        case EMetricInspectionTarget::OverheadClearance: return LOCTEXT("Overhead", "Overhead Clearance");
        case EMetricInspectionTarget::GroundValidation: return LOCTEXT("Ground", "Ground Validation");
        case EMetricInspectionTarget::FitAnalysis: return LOCTEXT("Fit", "Fit Analysis");
        case EMetricInspectionTarget::ThresholdAnalysis: return LOCTEXT("Threshold", "Threshold Analysis");
        case EMetricInspectionTarget::LedgeAnalysis: return LOCTEXT("Ledge", "Ledge Analysis");
        case EMetricInspectionTarget::StoppingDistance: return LOCTEXT("Stopping", "Stopping Distance");
        case EMetricInspectionTarget::TurnClearance: return LOCTEXT("Turn", "Turn Clearance");
        case EMetricInspectionTarget::JumpRunway: return LOCTEXT("JumpRunway", "Jump Runway");
        case EMetricInspectionTarget::InteractionReach: return LOCTEXT("Interaction", "Interaction Reach");
        case EMetricInspectionTarget::ShoulderClearance: return LOCTEXT("Shoulder", "Shoulder Camera Clearance");
        case EMetricInspectionTarget::AlternateStance: return LOCTEXT("Stance", "Alternate Stance Fit");
        case EMetricInspectionTarget::MantleVault: return LOCTEXT("Mantle", "Mantle / Vault");
        case EMetricInspectionTarget::RecoverySpace: return LOCTEXT("Recovery", "Recovery Space");
        case EMetricInspectionTarget::CameraProbe: return LOCTEXT("Camera", "Camera Probe");
        case EMetricInspectionTarget::CameraSightline: return LOCTEXT("Sightline", "Camera Sightline");
        case EMetricInspectionTarget::Trajectory: return LOCTEXT("Trajectory", "Trajectory");
        default: return LOCTEXT("Auto", "Auto (Most Relevant)");
        }
    }

    bool IsInspectionTargetVisible(const FMetricLiveState& State, EMetricInspectionTarget Target)
    {
        switch (Target)
        {
        case EMetricInspectionTarget::Clearance: return State.Clearance.bVisible;
        case EMetricInspectionTarget::OverheadClearance: return State.OverheadClearance.bVisible;
        case EMetricInspectionTarget::GroundValidation: return State.GroundValidation.bVisible;
        case EMetricInspectionTarget::FitAnalysis: return State.FitAnalysis.bVisible;
        case EMetricInspectionTarget::ThresholdAnalysis: return State.ThresholdAnalysis.bVisible;
        case EMetricInspectionTarget::LedgeAnalysis: return State.LedgeAnalysis.bVisible;
        case EMetricInspectionTarget::StoppingDistance: return State.StoppingDistance.bVisible;
        case EMetricInspectionTarget::TurnClearance: return State.TurnClearance.bVisible;
        case EMetricInspectionTarget::JumpRunway: return State.JumpRunway.bVisible;
        case EMetricInspectionTarget::InteractionReach: return State.InteractionReach.bVisible;
        case EMetricInspectionTarget::ShoulderClearance: return State.ShoulderClearance.bVisible;
        case EMetricInspectionTarget::AlternateStance: return State.AlternateStance.bVisible;
        case EMetricInspectionTarget::MantleVault: return State.MantleVault.bVisible;
        case EMetricInspectionTarget::RecoverySpace: return State.RecoverySpace.bVisible;
        case EMetricInspectionTarget::CameraProbe: return State.CameraProbe.bVisible;
        case EMetricInspectionTarget::CameraSightline: return State.CameraSightline.bVisible;
        case EMetricInspectionTarget::Trajectory: return State.Trajectory.bVisible;
        default: return false;
        }
    }


    const FMetricQueryEvidence* ResolvePrimaryProblemEvidence(const FMetricLiveState& State)
    {
        const EMetricInspectionTarget Target = ResolveInspectionTarget(State, EMetricInspectionTarget::Auto);
        const FMetricQueryEvidence* Best = nullptr;
        for (const FMetricQueryEvidence& Evidence : State.QueryEvidence)
        {
            if (Evidence.Target != Target
                || (Evidence.Status != EMetricQueryEvidenceStatus::Fail && Evidence.Status != EMetricQueryEvidenceStatus::Warning))
            {
                continue;
            }
            if (!Best
                || static_cast<uint8>(Evidence.Status) > static_cast<uint8>(Best->Status)
                || (Evidence.Status == Best->Status && Evidence.bHasImpact && !Best->bHasImpact))
            {
                Best = &Evidence;
            }
        }
        return Best;
    }

    EMetricInspectionTarget ResolveInspectionTarget(
        const FMetricLiveState& State,
        EMetricInspectionTarget RequestedTarget,
        FString* OutReason,
        EMetricQueryEvidenceStatus* OutStatus)
    {
        if (OutReason) OutReason->Reset();
        if (OutStatus) *OutStatus = EMetricQueryEvidenceStatus::Info;

        if (RequestedTarget != EMetricInspectionTarget::Auto)
        {
            const FMetricQueryEvidence* Best = nullptr;
            for (const FMetricQueryEvidence& Evidence : State.QueryEvidence)
            {
                if (Evidence.Target != RequestedTarget || Evidence.Label.IsEmpty()) continue;
                if (!Best
                    || static_cast<uint8>(Evidence.Status) > static_cast<uint8>(Best->Status))
                {
                    Best = &Evidence;
                }
            }
            if (Best)
            {
                if (OutReason) *OutReason = Best->Label;
                if (OutStatus) *OutStatus = Best->Status;
            }
            return RequestedTarget;
        }

        auto PickByStatus = [&](EMetricQueryEvidenceStatus Wanted, EMetricInspectionTarget& OutTarget)
        {
            for (const FMetricQueryEvidence& Evidence : State.QueryEvidence)
            {
                if (Evidence.Status == Wanted && IsInspectionTargetVisible(State, Evidence.Target))
                {
                    OutTarget = Evidence.Target;
                    if (OutReason) *OutReason = Evidence.Label;
                    if (OutStatus) *OutStatus = Evidence.Status;
                    return true;
                }
            }
            return false;
        };

        EMetricInspectionTarget Resolved = EMetricInspectionTarget::Auto;
        if (PickByStatus(EMetricQueryEvidenceStatus::Fail, Resolved)
            || PickByStatus(EMetricQueryEvidenceStatus::Warning, Resolved))
        {
            return Resolved;
        }

        for (const FMetricQueryEvidence& Evidence : State.QueryEvidence)
        {
            if (IsInspectionTargetVisible(State, Evidence.Target))
            {
                if (OutReason) *OutReason = Evidence.Label;
                if (OutStatus) *OutStatus = Evidence.Status;
                return Evidence.Target;
            }
        }

        return EMetricInspectionTarget::Auto;
    }
}

#undef LOCTEXT_NAMESPACE
