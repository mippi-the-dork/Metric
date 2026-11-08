#pragma once

#include "CoreMinimal.h"

struct METRIC_API FMetricCustomVersion
{
    static const FGuid GUID;

    enum Type : int32
    {
        BeforeCustomVersionWasAdded = 0,

        // First schema intended to be carried forward into public releases.
        InitialPublicSchema = 1,

        LatestVersion = InitialPublicSchema
    };

private:
    FMetricCustomVersion() = delete;
};
