#include "MetricProfile.h"

#include "MetricCustomVersion.h"

void UMetricProfile::Serialize(FArchive& Ar)
{
    Ar.UsingCustomVersion(FMetricCustomVersion::GUID);
    Super::Serialize(Ar);
}
