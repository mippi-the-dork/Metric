#include "MetricInputProcessor.h"

#include "MetricCommands.h"
#include "MetricSubsystem.h"
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

bool FMetricInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.IsRepeat() || !FMetricViewportController::IsLevelViewportFocused() || !GEditor)
    {
        return false;
    }

    const FMetricCommands& Commands = FMetricCommands::Get();
    const bool bToggle = MetricInput::MatchesCommand(InKeyEvent, Commands.ToggleMetric);
    const bool bCycle = MetricInput::MatchesCommand(InKeyEvent, Commands.CycleMetric);

    // Metric replaces Unreal's legacy Backslash / Shift+Backslash preview-mesh
    // interaction. If the user rebinds Metric elsewhere, keep these legacy
    // chords consumed rather than allowing the native preview to double-trigger.
    const bool bLegacyPlainBackslash = InKeyEvent.GetKey() == EKeys::Backslash
        && !InKeyEvent.IsShiftDown() && !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown() && !InKeyEvent.IsCommandDown();
    const bool bLegacyShiftBackslash = InKeyEvent.GetKey() == EKeys::Backslash
        && InKeyEvent.IsShiftDown() && !InKeyEvent.IsControlDown() && !InKeyEvent.IsAltDown() && !InKeyEvent.IsCommandDown();

    if (!bToggle && !bCycle)
    {
        return bLegacyPlainBackslash || bLegacyShiftBackslash;
    }

    UMetricSubsystem* MetricSubsystem = GEditor->GetEditorSubsystem<UMetricSubsystem>();
    if (!MetricSubsystem)
    {
        return false;
    }

    if (bCycle)
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
