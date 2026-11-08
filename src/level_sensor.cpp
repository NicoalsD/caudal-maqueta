#include "level_sensor.h"

#include <Arduino.h>
#include <time.h>

#include "config.h"

using caudal::SampleStatus;

void LevelSensor::begin() {
  pinMode(PIN_US_TRIG, OUTPUT);
  digitalWrite(PIN_US_TRIG, LOW);
  pinMode(PIN_US_ECHO, INPUT);
}

int32_t LevelSensor::measure_distance_mm() const {
  digitalWrite(PIN_US_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_US_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_US_TRIG, LOW);
  // 30 ms covers about 5 m round trip. Zero means no echo.
  const unsigned long echo_us = pulseIn(PIN_US_ECHO, HIGH, 30000UL);
  if (echo_us == 0) return 0;
  // Sound travels 0.343 mm per microsecond and the echo covers the distance twice:
  // mm = us * 0.343 / 2 = us * 343 / 2000.
  return static_cast<int32_t>(echo_us * 343UL / 2000UL);
}

bool LevelSensor::update(uint32_t now_ms) {
  if (!started_) {
    started_ = true;
    last_sample_ms_ = now_ms - LEVEL_SAMPLE_INTERVAL_MS;
  }
  if (now_ms - last_sample_ms_ < LEVEL_SAMPLE_INTERVAL_MS) return false;
  last_sample_ms_ = now_ms;

  samples_[count_++] = measure_distance_mm();
  if (count_ < LEVEL_SAMPLES) return false;

  // Round finished: keep the valid samples and publish their median.
  int32_t valid[16];
  int valid_count = 0;
  blind_zone_rejects_ = 0;
  for (int i = 0; i < count_; ++i) {
    const SampleStatus status =
        caudal::classify_distance_mm(samples_[i], BLIND_ZONE_MM, MAX_RANGE_MM);
    if (status == SampleStatus::kValid) {
      valid[valid_count++] = samples_[i];
    } else if (status == SampleStatus::kBlindZone) {
      ++blind_zone_rejects_;
    }
  }
  count_ = 0;

  int32_t median_mm = 0;
  if (!caudal::median_of(valid, valid_count, LEVEL_MIN_VALID, &median_mm)) {
    // Too few valid samples: keep the previous level and report nothing new.
    return false;
  }
  level_cm_ = caudal::level_cm_from_distance_mm(median_mm, TANK_EMPTY_DISTANCE_MM);
  sampled_at_s_ = static_cast<int64_t>(time(nullptr));
  has_level_ = true;
  return true;
}
