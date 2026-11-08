// Rig parameters. Every value can be overridden with build_flags (-D...).
// Pins follow docs/Simulador-y-hardware.md (section 8.2). Verify against the board before wiring.
#ifndef CAUDAL_CONFIG_H
#define CAUDAL_CONFIG_H

// Pins (ESP32 DevKit). GPIO 34 is input-only; GPIO 32 and 33 are not ADC2 pins.
#ifndef PIN_VALVE_1
#define PIN_VALVE_1 25
#endif
#ifndef PIN_VALVE_2
#define PIN_VALVE_2 26
#endif
#ifndef PIN_US_TRIG
#define PIN_US_TRIG 27
#endif
#ifndef PIN_US_ECHO
#define PIN_US_ECHO 34
#endif
#ifndef PIN_FLOW_1
#define PIN_FLOW_1 32
#endif
#ifndef PIN_FLOW_2
#define PIN_FLOW_2 33
#endif
#ifndef PIN_FLOAT_HIGH
#define PIN_FLOAT_HIGH 18
#endif
#ifndef PIN_FLOAT_LOW
#define PIN_FLOAT_LOW 19
#endif

// Relays active low (common on 2- and 4-channel modules). 1 = active low.
#ifndef RELAY_ACTIVE_LOW
#define RELAY_ACTIVE_LOW 1
#endif
// Floats are read closed to GND with a pull-up. 1 = closed reads LOW.
#ifndef FLOAT_ACTIVE_LOW
#define FLOAT_ACTIVE_LOW 1
#endif

// Level (JSN-SR04T). TO MEASURE: sensor-to-bottom distance with the tank empty.
#ifndef TANK_EMPTY_DISTANCE_MM
#define TANK_EMPTY_DISTANCE_MM 1000
#endif
// Blind zone (about 20 cm in the BOM) and maximum sensor range.
#ifndef BLIND_ZONE_MM
#define BLIND_ZONE_MM 200
#endif
#ifndef MAX_RANGE_MM
#define MAX_RANGE_MM 4500
#endif
#ifndef LEVEL_SAMPLES
#define LEVEL_SAMPLES 5
#endif
#ifndef LEVEL_MIN_VALID
#define LEVEL_MIN_VALID 3
#endif
#ifndef LEVEL_SAMPLE_INTERVAL_MS
#define LEVEL_SAMPLE_INTERVAL_MS 60
#endif

// Flow (YF-S201). TO CALIBRATE: nominal 450 pulses per liter.
#ifndef FLOW_PULSES_PER_LITER
#define FLOW_PULSES_PER_LITER 450.0f
#endif
#ifndef FLOW_WINDOW_MS
#define FLOW_WINDOW_MS 1000
#endif

// Backend timing. The limit is 60 signed requests per minute per device:
// 1200 ms polling (50 per minute) plus telemetry every 20 s (3 per minute) leaves headroom.
#ifndef COMMAND_POLL_MS
#define COMMAND_POLL_MS 1200
#endif
#ifndef TELEMETRY_INTERVAL_MS
#define TELEMETRY_INTERVAL_MS 20000
#endif
#ifndef RATE_LIMIT_BACKOFF_MS
#define RATE_LIMIT_BACKOFF_MS 15000
#endif
#ifndef HTTP_TIMEOUT_MS
#define HTTP_TIMEOUT_MS 4000
#endif
#ifndef WIFI_RETRY_MS
#define WIFI_RETRY_MS 5000
#endif
#ifndef NTP_SERVER
#define NTP_SERVER "pool.ntp.org"
#endif

// Loss of contact (to be defined in the protocol). With no 2xx response for this long
// fail_safe_position is applied once per outage.
#ifndef FAILSAFE_TIMEOUT_MS
#define FAILSAFE_TIMEOUT_MS 60000
#endif

// Local fail_safe_position per valve: 0 = KEEP, 1 = OPEN, 2 = CLOSED.
#ifndef FAILSAFE_VALVE_1
#define FAILSAFE_VALVE_1 2
#endif
#ifndef FAILSAFE_VALVE_2
#define FAILSAFE_VALVE_2 2
#endif

#endif  // CAUDAL_CONFIG_H
