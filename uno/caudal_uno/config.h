// Parameters of the UNO firmware. Each one can be overridden with a -D build flag.
#pragma once

#ifndef BAUD_RATE
#define BAUD_RATE 115200L
#endif

// HC-SR04 pins. The UNO is 5 V logic, so ECHO needs no voltage divider.
#ifndef TRIG_PIN
#define TRIG_PIN 9
#endif
#ifndef ECHO_PIN
#define ECHO_PIN 8
#endif

// Servo signal pins. Any digital pin works: the Servo library makes its own pulses (Timer1),
// so analogWrite on D9/D10 is lost, which this firmware does not use. The pins are chosen so
// that no wire crosses another on the breadboard (docs/images/maqueta-uno-protoboard.png).
#ifndef SERVO_1_PIN
#define SERVO_1_PIN 10
#endif
#ifndef SERVO_2_PIN
#define SERVO_2_PIN 11
#endif

// A 30 ms echo timeout covers about 5 m. The pulseIn default of one second would freeze the
// serial line when nothing echoes back.
#ifndef ECHO_TIMEOUT_US
#define ECHO_TIMEOUT_US 30000UL
#endif
// Time between pings, so the echo of the previous one dies out (datasheet: 60 ms cycle).
#ifndef PING_GAP_MS
#define PING_GAP_MS 60UL
#endif
// Pings kept for the median that `D` answers.
#ifndef PING_SAMPLES
#define PING_SAMPLES 5
#endif
// Speed of sound at about 20 C is 0.343 mm/us; halved for the round trip.
#ifndef MM_PER_US_ROUND_TRIP
#define MM_PER_US_ROUND_TRIP 0.1715f
#endif

// Smooth servo motion: one degree per step keeps the current peaks low, and only one servo
// moves at a time, so the USB port can feed both MG90S in a bench test.
#ifndef SERVO_STEP_MS
#define SERVO_STEP_MS 8UL
#endif
// Time given to the full-speed jump of the first move after boot (MG90S: ~0.1 s per 60 deg).
#ifndef SERVO_FIRST_MOVE_SETTLE_MS
#define SERVO_FIRST_MOVE_SETTLE_MS 600UL
#endif
#ifndef SERVO_MIN_DEG
#define SERVO_MIN_DEG 0
#endif
#ifndef SERVO_MAX_DEG
#define SERVO_MAX_DEG 180
#endif

// Timed spin of a continuous-rotation (360 deg) servo: the pulse width sets speed and direction,
// so the limits keep it inside what the Servo library can make, and the longest spin stays short.
#ifndef SPIN_MIN_US
#define SPIN_MIN_US 500
#endif
#ifndef SPIN_MAX_US
#define SPIN_MAX_US 2500
#endif
#ifndef SPIN_MAX_MS
#define SPIN_MAX_MS 10000UL
#endif

#ifndef LINE_CAPACITY
#define LINE_CAPACITY 32
#endif
