#include "caudal/flow_math.h"

namespace caudal {

float liters_from_pulses(uint32_t pulses, float pulses_per_liter) {
  if (pulses_per_liter <= 0.0f) return 0.0f;
  return static_cast<float>(pulses) / pulses_per_liter;
}

float flow_lpm_from_pulses(uint32_t pulses, uint32_t elapsed_ms, float pulses_per_liter) {
  if (elapsed_ms == 0 || pulses_per_liter <= 0.0f) return 0.0f;
  const float minutes = static_cast<float>(elapsed_ms) / 60000.0f;
  return liters_from_pulses(pulses, pulses_per_liter) / minutes;
}

}  // namespace caudal
