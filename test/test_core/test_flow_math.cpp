#include <unity.h>

#include "caudal/flow_math.h"

void test_liters_from_pulses_uses_calibration(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, caudal::liters_from_pulses(450, 450.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, caudal::liters_from_pulses(225, 450.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, caudal::liters_from_pulses(450, 0.0f));
}

// 450 pulses per liter: 450 pulses in one minute is 1 L/min.
void test_flow_lpm_from_pulses(void) {
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, caudal::flow_lpm_from_pulses(450, 60000, 450.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 2.0f, caudal::flow_lpm_from_pulses(450, 30000, 450.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, caudal::flow_lpm_from_pulses(450, 0, 450.0f));
}
