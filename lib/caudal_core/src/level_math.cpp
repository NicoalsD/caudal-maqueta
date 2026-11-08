#include "caudal/level_math.h"

namespace caudal {

namespace {

constexpr size_t kMaxSamples = 16;

}  // namespace

SampleStatus classify_distance_mm(int32_t distance_mm, int32_t blind_zone_mm,
                                  int32_t max_range_mm) {
  if (distance_mm <= 0) return SampleStatus::kNoEcho;
  if (distance_mm < blind_zone_mm) return SampleStatus::kBlindZone;
  if (distance_mm > max_range_mm) return SampleStatus::kOutOfRange;
  return SampleStatus::kValid;
}

bool median_of(const int32_t* samples, size_t n, size_t min_valid, int32_t* out) {
  if (samples == nullptr || out == nullptr || n == 0 || n > kMaxSamples || n < min_valid) {
    return false;
  }
  int32_t sorted[kMaxSamples];
  for (size_t i = 0; i < n; ++i) {
    // Insertion sort: n is at most 16.
    int32_t value = samples[i];
    size_t j = i;
    while (j > 0 && sorted[j - 1] > value) {
      sorted[j] = sorted[j - 1];
      --j;
    }
    sorted[j] = value;
  }
  if (n % 2 == 1) {
    *out = sorted[n / 2];
  } else {
    *out = (sorted[n / 2 - 1] + sorted[n / 2]) / 2;
  }
  return true;
}

int32_t level_cm_from_distance_mm(int32_t distance_mm, int32_t empty_distance_mm) {
  const int32_t level_mm = empty_distance_mm - distance_mm;
  if (level_mm <= 0) return 0;
  return (level_mm + 5) / 10;  // round half up to the nearest cm
}

size_t format_meters_2dec(int32_t cm, char* out) {
  size_t pos = 0;
  uint32_t magnitude;
  if (cm < 0) {
    out[pos++] = '-';
    magnitude = static_cast<uint32_t>(-(static_cast<int64_t>(cm)));
  } else {
    magnitude = static_cast<uint32_t>(cm);
  }
  const uint32_t meters = magnitude / 100;
  const uint32_t centimeters = magnitude % 100;
  char digits[11];
  size_t n = 0;
  uint32_t m = meters;
  do {
    digits[n++] = static_cast<char>('0' + m % 10);
    m /= 10;
  } while (m > 0);
  while (n > 0) out[pos++] = digits[--n];
  out[pos++] = '.';
  out[pos++] = static_cast<char>('0' + centimeters / 10);
  out[pos++] = static_cast<char>('0' + centimeters % 10);
  out[pos] = '\0';
  return pos;
}

}  // namespace caudal
