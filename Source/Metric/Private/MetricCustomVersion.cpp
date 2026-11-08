#include "MetricCustomVersion.h"

#include "Serialization/CustomVersion.h"

const FGuid FMetricCustomVersion::GUID(0x3F08A1D2, 0xC7564B4A, 0xB39A6D15, 0x71E4F902);

static FCustomVersionRegistration GRegisterMetricCustomVersion(
    FMetricCustomVersion::GUID,
    FMetricCustomVersion::LatestVersion,
    TEXT("Metric"));
