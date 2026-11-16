// CAUDAL maqueta, Arduino UNO edition.
//
// The UNO has no network, so it is a plain USB serial device: it measures the raw distance of
// the HC-SR04 and moves two servos to raw angles. The PC bridge (`caudal-sim maqueta-usb` in
// caudal-simulador) owns the calibration, the signatures and the talk with caudal-backend.
//
// Wiring (see docs/images/maqueta-uno-conexiones.png):
//   HC-SR04  VCC -> 5V, GND -> GND, TRIG -> D8, ECHO -> D9
//   Servo 1  signal (orange) -> D5, red -> 5V rail, brown -> GND rail
//   Servo 2  signal (orange) -> D6, red -> 5V rail, brown -> GND rail
//
// Serial protocol, 115200 baud, one command per line ('\n'), answers are one line each:
//   PING            -> PONG
//   INFO            -> INFO caudal-uno <version> s1=<deg|-1> s2=<deg|-1>
//   D               -> D <mm>          median of the last pings; D -1 when none echoed back
//   S<n> <deg>      -> OK S<n> <deg>   starts a smooth move of servo n (1 or 2) to deg (0..180)
//                      ... later      -> DONE S<n> <deg> when the servo reaches the angle
//   R<n>            -> OK R<n>         releases servo n (no more holding torque)
//   anything else   -> ERR <reason>
// On boot it prints `READY caudal-uno <version>`. Opening the port resets the UNO, so the
// bridge waits for that line before it sends anything.

#include <Servo.h>

#include "config.h"

static const char FIRMWARE_VERSION[] = "1.0.0";
static const uint8_t SERVO_PINS[] = {SERVO_1_PIN, SERVO_2_PIN};
static const uint8_t SERVO_COUNT = 2;
static const int NO_ANGLE = -1;

Servo servos[SERVO_COUNT];
int servoAngle[SERVO_COUNT] = {NO_ANGLE, NO_ANGLE};
int servoTarget[SERVO_COUNT] = {NO_ANGLE, NO_ANGLE};
unsigned long lastServoStep = 0;

char line[LINE_CAPACITY];
uint8_t lineLength = 0;
bool lineOverflow = false;

long pingOnceMm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long echoUs = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (echoUs == 0) {
    return -1;
  }
  return (long)(echoUs * MM_PER_US_ROUND_TRIP + 0.5f);
}

// Latest pings, one every PING_GAP_MS. A ping that got no echo is stored as -1.
long pings[PING_SAMPLES];
uint8_t pingCursor = 0;
unsigned long lastPing = 0;

void pingIfDue() {
  unsigned long now = millis();
  if (now - lastPing < PING_GAP_MS) {
    return;
  }
  lastPing = now;
  pings[pingCursor] = pingOnceMm();
  pingCursor = (pingCursor + 1) % PING_SAMPLES;
}

// Median of the valid recent pings. A median ignores the odd echo from the bottle wall.
long distanceMm() {
  long samples[PING_SAMPLES];
  uint8_t valid = 0;
  for (uint8_t i = 0; i < PING_SAMPLES; i++) {
    if (pings[i] > 0) {
      samples[valid++] = pings[i];
    }
  }
  if (valid == 0) {
    return -1;
  }
  for (uint8_t i = 1; i < valid; i++) {
    long key = samples[i];
    int8_t j = i - 1;
    while (j >= 0 && samples[j] > key) {
      samples[j + 1] = samples[j];
      j--;
    }
    samples[j + 1] = key;
  }
  return samples[valid / 2];
}

// Index of the servo that is moving now, or -1. Servo 1 has priority.
int8_t movingServo() {
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    if (servoTarget[i] != NO_ANGLE && servoTarget[i] != servoAngle[i]) {
      return i;
    }
  }
  return -1;
}

void stepServos() {
  unsigned long now = millis();
  if (now - lastServoStep < SERVO_STEP_MS) {
    return;
  }
  lastServoStep = now;
  int8_t i = movingServo();
  if (i < 0) {
    return;
  }
  if (!servos[i].attached()) {
    servos[i].attach(SERVO_PINS[i]);
  }
  if (servoAngle[i] == NO_ANGLE) {
    // First move after boot: the real position is unknown, so it jumps to the target.
    servoAngle[i] = servoTarget[i];
  } else {
    servoAngle[i] += (servoTarget[i] > servoAngle[i]) ? 1 : -1;
  }
  servos[i].write(servoAngle[i]);
  if (servoAngle[i] == servoTarget[i]) {
    Serial.print(F("DONE S"));
    Serial.print(i + 1);
    Serial.print(' ');
    Serial.println(servoAngle[i]);
  }
}

// Parses "<digit>" at text[0] as a servo number. Returns the index or -1.
int8_t servoIndex(const char *text) {
  if (text[0] < '1' || text[0] > '0' + SERVO_COUNT) {
    return -1;
  }
  return text[0] - '1';
}

void handleServoMove(const char *args) {
  int8_t i = servoIndex(args);
  if (i < 0 || args[1] != ' ') {
    Serial.println(F("ERR servo desconocido, usa S1 <grados> o S2 <grados>"));
    return;
  }
  char *end = nullptr;
  long deg = strtol(args + 2, &end, 10);
  if (end == args + 2 || *end != '\0' || deg < SERVO_MIN_DEG || deg > SERVO_MAX_DEG) {
    Serial.println(F("ERR angulo fuera de 0..180"));
    return;
  }
  servoTarget[i] = (int)deg;
  Serial.print(F("OK S"));
  Serial.print(i + 1);
  Serial.print(' ');
  Serial.println(deg);
  if (servoAngle[i] == servoTarget[i]) {
    if (!servos[i].attached()) {
      servos[i].attach(SERVO_PINS[i]);
      servos[i].write(servoAngle[i]);
    }
    Serial.print(F("DONE S"));
    Serial.print(i + 1);
    Serial.print(' ');
    Serial.println(deg);
  }
}

void handleServoRelease(const char *args) {
  int8_t i = servoIndex(args);
  if (i < 0 || args[1] != '\0') {
    Serial.println(F("ERR servo desconocido, usa R1 o R2"));
    return;
  }
  servos[i].detach();
  servoTarget[i] = servoAngle[i];
  Serial.print(F("OK R"));
  Serial.println(i + 1);
}

void printInfo() {
  Serial.print(F("INFO caudal-uno "));
  Serial.print(FIRMWARE_VERSION);
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    Serial.print(F(" s"));
    Serial.print(i + 1);
    Serial.print('=');
    Serial.print(servoAngle[i]);
  }
  Serial.println();
}

void handleLine(const char *text) {
  if (strcmp(text, "PING") == 0) {
    Serial.println(F("PONG"));
  } else if (strcmp(text, "INFO") == 0) {
    printInfo();
  } else if (strcmp(text, "D") == 0) {
    Serial.print(F("D "));
    Serial.println(distanceMm());
  } else if (text[0] == 'S') {
    handleServoMove(text + 1);
  } else if (text[0] == 'R') {
    handleServoRelease(text + 1);
  } else {
    Serial.println(F("ERR comando desconocido"));
  }
}

void readSerial() {
  while (Serial.available() > 0) {
    char c = (char)Serial.read();
    if (c == '\r') {
      continue;
    }
    if (c != '\n') {
      if (lineLength < LINE_CAPACITY - 1) {
        line[lineLength++] = c;
      } else {
        lineOverflow = true;
      }
      continue;
    }
    line[lineLength] = '\0';
    if (lineOverflow) {
      Serial.println(F("ERR linea demasiado larga"));
    } else if (lineLength > 0) {
      handleLine(line);
    }
    lineLength = 0;
    lineOverflow = false;
  }
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  for (uint8_t i = 0; i < PING_SAMPLES; i++) {
    pings[i] = -1;
  }
  // The servos stay detached until the first command, so they do not jump at power on.
  Serial.begin(BAUD_RATE);
  Serial.print(F("READY caudal-uno "));
  Serial.println(FIRMWARE_VERSION);
}

void loop() {
  readSerial();
  pingIfDue();
  stepServos();
}
