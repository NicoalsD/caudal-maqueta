#include <string.h>
#include <unity.h>

#include "caudal/level_math.h"

using caudal::SampleStatus;

void test_classify_blind_zone_and_range(void) {
  const int32_t blind = 200, max = 4500;
  TEST_ASSERT_TRUE(caudal::classify_distance_mm(0, blind, max) == SampleStatus::kNoEcho);
  TEST_ASSERT_TRUE(caudal::classify_distance_mm(199, blind, max) == SampleStatus::kBlindZone);
  TEST_ASSERT_TRUE(caudal::classify_distance_mm(200, blind, max) == SampleStatus::kValid);
  TEST_ASSERT_TRUE(caudal::classify_distance_mm(4500, blind, max) == SampleStatus::kValid);
  TEST_ASSERT_TRUE(caudal::classify_distance_mm(4501, blind, max) == SampleStatus::kOutOfRange);
}

void test_median_of_five_ignores_spikes(void) {
  // A single bad echo at 9000 mm must not move the median.
  const int32_t samples[] = {1210, 1205, 9000, 1208, 1207};
  int32_t out = 0;
  TEST_ASSERT_TRUE(caudal::median_of(samples, 5, 3, &out));
  TEST_ASSERT_EQUAL_INT32(1208, out);
}

void test_median_requires_min_valid_samples(void) {
  const int32_t samples[] = {1200, 1201};
  int32_t out = 0;
  TEST_ASSERT_FALSE(caudal::median_of(samples, 2, 3, &out));
  TEST_ASSERT_TRUE(caudal::median_of(samples, 2, 2, &out));
  TEST_ASSERT_EQUAL_INT32(1200, out);  // integer mean of 1200 and 1201
}

void test_level_from_distance_with_rounding_and_clamp(void) {
  // Sensor 2.60 m above an empty tank bottom; surface at 0.40 m from the sensor.
  TEST_ASSERT_EQUAL_INT32(220, caudal::level_cm_from_distance_mm(400, 2600));
  // 2600 mm empty distance minus 2000 mm measured distance is 600 mm, or 60 cm.
  TEST_ASSERT_EQUAL_INT32(60, caudal::level_cm_from_distance_mm(2000, 2600));
  // Half a centimeter rounds up.
  TEST_ASSERT_EQUAL_INT32(61, caudal::level_cm_from_distance_mm(1995, 2600));
  // Surface above the sensor reference clamps to zero.
  TEST_ASSERT_EQUAL_INT32(0, caudal::level_cm_from_distance_mm(3000, 2600));
}

void test_format_meters_two_decimals(void) {
  char out[16];
  caudal::format_meters_2dec(213, out);
  TEST_ASSERT_EQUAL_STRING("2.13", out);
  caudal::format_meters_2dec(5, out);
  TEST_ASSERT_EQUAL_STRING("0.05", out);
  caudal::format_meters_2dec(0, out);
  TEST_ASSERT_EQUAL_STRING("0.00", out);
  caudal::format_meters_2dec(-5, out);
  TEST_ASSERT_EQUAL_STRING("-0.05", out);
  caudal::format_meters_2dec(1200, out);
  TEST_ASSERT_EQUAL_STRING("12.00", out);
}
