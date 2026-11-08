#include "float_switches.h"

#include <Arduino.h>

#include "config.h"

namespace {

bool is_active(uint8_t pin) {
  const bool low = digitalRead(pin) == LOW;
  return FLOAT_ACTIVE_LOW ? low : !low;
}

}  // namespace

void FloatSwitches::begin() {
  pinMode(PIN_FLOAT_HIGH, INPUT_PULLUP);
  pinMode(PIN_FLOAT_LOW, INPUT_PULLUP);
}

bool FloatSwitches::high_active() const { return is_active(PIN_FLOAT_HIGH); }

bool FloatSwitches::low_active() const { return is_active(PIN_FLOAT_LOW); }
