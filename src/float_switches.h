// High and low level floats: backup for the ultrasonic sensor, read through GPIO.
// The pump cut-off float is wired separately in hardware and does not depend on this code.
#ifndef CAUDAL_FLOAT_SWITCHES_H
#define CAUDAL_FLOAT_SWITCHES_H

class FloatSwitches {
 public:
  void begin();
  bool high_active() const;
  bool low_active() const;
};

#endif  // CAUDAL_FLOAT_SWITCHES_H
