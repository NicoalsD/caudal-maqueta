#include "valve_driver.h"

#include <Arduino.h>

#include "config.h"

namespace {

const uint8_t kPins[2] = {PIN_VALVE_1, PIN_VALVE_2};
constexpr uint8_t kEnergized = RELAY_ACTIVE_LOW ? LOW : HIGH;
constexpr uint8_t kReleased = RELAY_ACTIVE_LOW ? HIGH : LOW;

}  // namespace

void ValveDriver::force_safe_outputs() {
  for (uint8_t pin : kPins) {
    digitalWrite(pin, kReleased);
    pinMode(pin, OUTPUT);
    digitalWrite(pin, kReleased);
  }
}

void ValveDriver::begin() {
  for (uint8_t pin : kPins) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, kReleased);
  }
}

void ValveDriver::drive(size_t index, caudal::ValveState state) {
  if (index >= 2) return;
  digitalWrite(kPins[index], state == caudal::ValveState::kOpen ? kEnergized : kReleased);
}

bool ValveDriver::actual_open(size_t index) const {
  if (index >= 2) return false;
  return digitalRead(kPins[index]) == kEnergized;
}
