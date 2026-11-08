// Flow on the two outputs with YF-S201 sensors, counting pulses from interrupts.
#ifndef CAUDAL_FLOW_SENSOR_H
#define CAUDAL_FLOW_SENSOR_H

#include <stdint.h>

class FlowSensor {
 public:
  void begin();

  // Closes the FLOW_WINDOW_MS window and computes the flow of each output.
  void update(uint32_t now_ms);

  // Liters per minute of output 0 or 1, from the last closed window.
  float flow_lpm(int output) const;
  // Liters accumulated since boot.
  float total_liters(int output) const;

 private:
  uint32_t window_start_ms_ = 0;
  uint32_t last_pulses_[2] = {0, 0};
  float flow_lpm_[2] = {0.0f, 0.0f};
  float total_liters_[2] = {0.0f, 0.0f};
};

#endif  // CAUDAL_FLOW_SENSOR_H
