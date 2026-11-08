#include "flow_sensor.h"

#include <Arduino.h>

#include "caudal/flow_math.h"
#include "config.h"

namespace {

// Pulse counters written from the interrupt handlers.
volatile uint32_t g_pulses[2] = {0, 0};

void IRAM_ATTR on_pulse_1() { ++g_pulses[0]; }
void IRAM_ATTR on_pulse_2() { ++g_pulses[1]; }

uint32_t read_pulses(int output) {
  noInterrupts();
  const uint32_t value = g_pulses[output];
  interrupts();
  return value;
}

}  // namespace

void FlowSensor::begin() {
  pinMode(PIN_FLOW_1, INPUT_PULLUP);
  pinMode(PIN_FLOW_2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_1), on_pulse_1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_FLOW_2), on_pulse_2, RISING);
  window_start_ms_ = millis();
}

void FlowSensor::update(uint32_t now_ms) {
  const uint32_t elapsed = now_ms - window_start_ms_;
  if (elapsed < FLOW_WINDOW_MS) return;
  for (int i = 0; i < 2; ++i) {
    const uint32_t pulses = read_pulses(i);
    const uint32_t delta = pulses - last_pulses_[i];
    last_pulses_[i] = pulses;
    flow_lpm_[i] = caudal::flow_lpm_from_pulses(delta, elapsed, FLOW_PULSES_PER_LITER);
    total_liters_[i] += caudal::liters_from_pulses(delta, FLOW_PULSES_PER_LITER);
  }
  window_start_ms_ = now_ms;
}

float FlowSensor::flow_lpm(int output) const { return flow_lpm_[output]; }

float FlowSensor::total_liters(int output) const { return total_liters_[output]; }
