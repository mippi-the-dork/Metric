#pragma once

// Deprecated development-era header retained only so local includes from an
// in-progress workspace fail gracefully. Metric gravity integrations now derive
// from UMetricGravityProvider in MetricGravityProvider.h. No modular-feature
// registration or Metric-specific C++ interface is required.
#include "MetricGravityProvider.h"
