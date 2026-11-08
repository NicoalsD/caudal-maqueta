// Ultrasonic level math for the JSN-SR04T (distances in millimeters, levels in centimeters).
#ifndef CAUDAL_LEVEL_MATH_H
#define CAUDAL_LEVEL_MATH_H

#include <stddef.h>
#include <stdint.h>

namespace caudal {

enum class SampleStatus : uint8_t {
  kValid,
  kNoEcho,      // no echo within the timeout
  kBlindZone,   // closer than the sensor can measure (about 20 cm on the JSN-SR04T)
  kOutOfRange   // farther than the configured maximum
};

SampleStatus classify_distance_mm(int32_t distance_mm, int32_t blind_zone_mm,
                                  int32_t max_range_mm);

// Median of the samples. Returns false when there are fewer than `min_valid` samples.
// For an even count it returns the integer mean of the two middle values. Supports up to 16 samples.
bool median_of(const int32_t* samples, size_t n, size_t min_valid, int32_t* out);

// Level above the bottom, in cm, from the distance to the surface.
// `empty_distance_mm` is the sensor-to-bottom distance when the tank is empty.
// The result is clamped to 0 or more.
int32_t level_cm_from_distance_mm(int32_t distance_mm, int32_t empty_distance_mm);

// Writes meters with two decimals, for example 213 -> "2.13" and -5 -> "-0.05".
// `out` needs 12 bytes. Returns the length.
size_t format_meters_2dec(int32_t cm, char* out);

}  // namespace caudal

#endif  // CAUDAL_LEVEL_MATH_H
