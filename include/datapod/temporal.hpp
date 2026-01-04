#pragma once

/**
 * @file temporal.hpp
 * @brief All temporal containers - Time-series and timestamped data
 *
 * This header includes all temporal containers for time-series
 * data, events, and time-based operations.
 */

#include "pods/temporal/circular_buffer.hpp"
#include "pods/temporal/event.hpp"
#include "pods/temporal/financial.hpp"
#include "pods/temporal/multi_series.hpp"
#include "pods/temporal/stamp.hpp"
#include "pods/temporal/time_series.hpp"
#include "pods/temporal/window.hpp"

// Optional short namespace alias (enabled with -DSHORT_NAMESPACE)
#if defined(SHORT_NAMESPACE)
namespace dp = datapod;
#endif
