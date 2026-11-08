// Pulse-to-flow conversion for the YF-S201 (pulses per liter from calibration).
#ifndef CAUDAL_FLOW_MATH_H
#define CAUDAL_FLOW_MATH_H

#include <stdint.h>

namespace caudal {

// Liters for `pulses` at `pulses_per_liter`. Returns 0 if the calibration is not positive.
float liters_from_pulses(uint32_t pulses, float pulses_per_liter);

// Flow in liters per minute over `elapsed_ms`. Returns 0 if the window or the calibration is empty.
float flow_lpm_from_pulses(uint32_t pulses, uint32_t elapsed_ms, float pulses_per_liter);

}  // namespace caudal

#endif  // CAUDAL_FLOW_MATH_H
