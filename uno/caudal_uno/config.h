// Parameters of the UNO firmware. Each one can be overridden with a -D build flag.
#pragma once

#ifndef BAUD_RATE
#define BAUD_RATE 115200L
#endif

// HC-SR04 pins. The UNO is 5 V logic, so ECHO needs no voltage divider.
#ifndef TRIG_PIN
#define TRIG_PIN 8
#endif
#ifndef ECHO_PIN
#define ECHO_PIN 9
#endif

// Servo signal pins. Any digital pin works; the Servo library only takes PWM from D9/D10,
// and D9 here is a plain input.
#ifndef SERVO_1_PIN
#define SERVO_1_PIN 5
#endif
#ifndef SERVO_2_PIN
#define SERVO_2_PIN 6
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
#ifndef SERVO_MIN_DEG
#define SERVO_MIN_DEG 0
#endif
#ifndef SERVO_MAX_DEG
#define SERVO_MAX_DEG 180
#endif

#ifndef LINE_CAPACITY
#define LINE_CAPACITY 32
#endif
