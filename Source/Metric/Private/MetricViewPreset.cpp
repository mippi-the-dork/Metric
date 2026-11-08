#include "MetricViewPreset.h"

#include "MetricCustomVersion.h"

void UMetricViewPreset::Serialize(FArchive& Ar)
{
    Ar.UsingCustomVersion(FMetricCustomVersion::GUID);
    Super::Serialize(Ar);
}
