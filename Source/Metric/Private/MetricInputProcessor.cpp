#include "MetricInputProcessor.h"

#include "MetricCommands.h"
#include "MetricSubsystem.h"
#include "MetricEditorSettings.h"
#include "MetricViewportController.h"
#include "Editor.h"

namespace MetricInput
{
    static bool MatchesChord(const FKeyEvent& KeyEvent, const FInputChord& Chord)
    {
        return Chord.IsValidChord()
            && Chord.Key == KeyEvent.GetKey()
            && Chord.bShift == KeyEvent.IsShiftDown()
            && Chord.bCtrl == KeyEvent.IsControlDown()
            && Chord.bAlt == KeyEvent.IsAltDown()
            && Chord.bCmd == KeyEvent.IsCommandDown();
    }

    static bool MatchesCommand(const FKeyEvent& KeyEvent, const TSharedPtr<FUICommandInfo>& Command)
    {
        if (!Command.IsValid())
        {
            return false;
        }

        return MatchesChord(KeyEvent, *Command->GetActiveChord(EMultipleKeyBindingIndex::Primary))
            || MatchesChord(KeyEvent, *Command->GetActiveChord(EMultipleKeyBindingIndex::Secondary));
    }
}


void FMetricInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
    (void)DeltaTime;
    (void)SlateApp;
    (void)Cursor;

    // Metric's continuous viewport work is handled by FMetricViewportController.
    // The Slate input processor only intercepts the Metric key chords.
}

bool FMetricInputProcessor::HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
    (void)SlateApp;
    FMetricViewportController::NotifyViewportInteractionAtScreenPosition(MouseEvent.GetScreenSpacePosition());
    return false;
}

bool FMetricInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
    // Record the viewport under the pointer before any Metric shortcut changes
    // HUD state. This makes the first interaction after editor startup authoritative
    // instead of inheriting Unreal's often-stale ActiveViewport value.
    FMetricViewportController::NotifyViewportInteractionAtScreenPosition(SlateApp.GetCursorPos());
    if (InKeyEvent.IsRepeat() || !FMetricViewportController::IsLevelViewportFocused() || !GEditor)
    {
        return false;
    }

    const FMetricCommands& Commands = FMetricCommands::Get();
    const bool bBackslash = InKeyEvent.GetKey() == EKeys::Backslash;
    const bool bNoAltOrCmd = !InKeyEvent.IsAltDown() && !InKeyEvent.IsCommandDown();
    const bool bPhysicalNextBenchmark = bBackslash
        && InKeyEvent.IsAltDown() && !InKeyEvent.IsControlDown() && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsCommandDown();
    const bool bPhysicalToggleLiveAnalysis = bBackslash
        && InKeyEvent.IsAltDown() && InKeyEvent.IsControlDown() && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsCommandDown();

    // Always recognize Metric's documented Backslash chords directly. This is
    // intentionally independent of FUICommandInfo so Ctrl-modified Backslash
    // remains reliable even when Slate detects a binding conflict or a user
    // keyboard-shortcut profile has not persisted the command chord.
    const bool bPhysicalClearPins = bBackslash && bNoAltOrCmd
        && InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown();
    const bool bPhysicalPin = bBackslash && bNoAltOrCmd
        && InKeyEvent.IsControlDown() && !InKeyEvent.IsShiftDown();
    const bool bLegacyShiftBackslash = bBackslash && bNoAltOrCmd
        && !InKeyEvent.IsControlDown() && InKeyEvent.IsShiftDown();
    const bool bLegacyPlainBackslash = bBackslash && bNoAltOrCmd
        && !InKeyEvent.IsControlDown() && !InKeyEvent.IsShiftDown();

    // Resolve the most-specific modifier chords first so Ctrl+Shift+Backslash
    // can never fall through to cycle or toggle handling. Rebound command
    // chords are still supported in addition to the documented defaults.
    const bool bClearPins = bPhysicalClearPins
        || MetricInput::MatchesCommand(InKeyEvent, Commands.ClearMetricBenchmarks);
    const bool bToggleLiveAnalysis = !bClearPins && (bPhysicalToggleLiveAnalysis
        || MetricInput::MatchesCommand(InKeyEvent, Commands.ToggleLiveAnalysis));
    const bool bPin = !bClearPins && !bToggleLiveAnalysis && (bPhysicalPin
        || MetricInput::MatchesCommand(InKeyEvent, Commands.PinMetricBenchmark));
    const bool bNextBenchmark = !bClearPins && !bToggleLiveAnalysis && !bPin && (bPhysicalNextBenchmark
        || MetricInput::MatchesCommand(InKeyEvent, Commands.NextMetricBenchmark));
    const bool bCycle = !bClearPins && !bToggleLiveAnalysis && !bPin && !bNextBenchmark && (bLegacyShiftBackslash
        || MetricInput::MatchesCommand(InKeyEvent, Commands.CycleMetric));
    const bool bToggle = !bClearPins && !bToggleLiveAnalysis && !bPin && !bNextBenchmark && !bCycle && (bLegacyPlainBackslash
        || MetricInput::MatchesCommand(InKeyEvent, Commands.ToggleMetric));

    if (!bToggle && !bToggleLiveAnalysis && !bCycle && !bPin && !bClearPins && !bNextBenchmark)
    {
        return false;
    }

    UMetricSubsystem* MetricSubsystem = GEditor->GetEditorSubsystem<UMetricSubsystem>();
    if (!MetricSubsystem)
    {
        return false;
    }

    if (bClearPins)
    {
        MetricSubsystem->ClearPinnedBenchmarks();
    }
    else if (bToggleLiveAnalysis)
    {
        if (UMetricEditorSettings* Settings = GetMutableDefault<UMetricEditorSettings>())
        {
            Settings->bShowLiveVisualizer = !Settings->bShowLiveVisualizer;
            Settings->SaveConfig();
        }
    }
    else if (bPin)
    {
        MetricSubsystem->PinCurrentBenchmark();
    }
    else if (bNextBenchmark)
    {
        MetricSubsystem->CyclePinnedBenchmarkSelection();
    }
    else if (bCycle)
    {
        MetricSubsystem->CycleNext();
    }
    else
    {
        MetricSubsystem->ToggleMetric();
    }

    // Consume the event so Unreal's native preview mesh commands do not also fire.
    return true;
}
