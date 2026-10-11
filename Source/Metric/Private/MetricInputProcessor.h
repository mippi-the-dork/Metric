#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"

class FMetricInputProcessor final : public IInputProcessor
{
public:
    virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;
    virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
    virtual const TCHAR* GetDebugName() const override { return TEXT("MetricInputProcessor"); }
};
