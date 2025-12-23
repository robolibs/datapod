#pragma once

/**
 * @file temporal.hpp
 * @brief All temporal containers - Time-series and timestamped data
 *
 * This header includes all temporal containers for time-series
 * data, events, and time-based operations.
 */

#include "temporal/circular_buffer.hpp"
#include "temporal/event.hpp"
#include "temporal/financial.hpp"
#include "temporal/multi_series.hpp"
#include "temporal/stamp.hpp"
#include "temporal/time_series.hpp"
#include "temporal/window.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
