// Tank level with the JSN-SR04T. Conversion and median live in lib/caudal_core.
// Takes LEVEL_SAMPLES readings spaced by LEVEL_SAMPLE_INTERVAL_MS, drops blind-zone readings
// and publishes the median of the valid ones. Each reading blocks at most about 30 ms.
#ifndef CAUDAL_LEVEL_SENSOR_H
#define CAUDAL_LEVEL_SENSOR_H

#include <stdint.h>

#include "caudal/level_math.h"

class LevelSensor {
 public:
  void begin();

  // Call on every loop pass. Returns true when a new level is published.
  bool update(uint32_t now_ms);

  bool has_level() const { return has_level_; }
  // Level in centimeters above the bottom, from the last published median.
  int32_t level_cm() const { return level_cm_; }
  // Epoch seconds of the sample that closed the median.
  int64_t sampled_at_s() const { return sampled_at_s_; }
  // Samples discarded in the blind zone during the last round (for diagnostics).
  int blind_zone_rejects() const { return blind_zone_rejects_; }

 private:
  int32_t measure_distance_mm() const;

  int32_t samples_[16];
  int count_ = 0;
  int blind_zone_rejects_ = 0;
  uint32_t last_sample_ms_ = 0;
  bool started_ = false;
  bool has_level_ = false;
  int32_t level_cm_ = 0;
  int64_t sampled_at_s_ = 0;
};

#endif  // CAUDAL_LEVEL_SENSOR_H
