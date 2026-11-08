// Solenoid valve driver (pattern P07 Bridge: ValveFsm decides, this class
// drives the GPIO). Valves are NC direct-acting: energizing the relay opens the valve.
#ifndef CAUDAL_VALVE_DRIVER_H
#define CAUDAL_VALVE_DRIVER_H

#include <stddef.h>

#include "caudal/valve_fsm.h"

class ValveDriver {
 public:
  // First call in setup(), before Serial and Wi-Fi: leaves both relays off
  // (valves closed) without waiting for anything.
  static void force_safe_outputs();

  void begin();
  void drive(size_t index, caudal::ValveState state);
  // Reads back the output pin: true when the relay is energized (valve open).
  bool actual_open(size_t index) const;
};

#endif  // CAUDAL_VALVE_DRIVER_H
