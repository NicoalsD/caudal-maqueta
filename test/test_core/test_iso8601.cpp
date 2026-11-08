#include <string.h>
#include <unity.h>

#include "caudal/iso8601.h"

namespace {

int64_t parse(const char* text) {
  int64_t ms = -1;
  TEST_ASSERT_TRUE(caudal::parse_iso8601_ms(text, strlen(text), &ms));
  return ms;
}

}  // namespace

void test_parse_utc_epoch(void) {
  TEST_ASSERT_EQUAL_INT64(0, parse("1970-01-01T00:00:00Z"));
  TEST_ASSERT_EQUAL_INT64(1794031400000LL, parse("2026-11-07T06:03:20Z"));
}

void test_parse_fraction_and_offsets(void) {
  TEST_ASSERT_EQUAL_INT64(1794031400500LL, parse("2026-11-07T06:03:20.5Z"));
  TEST_ASSERT_EQUAL_INT64(1794031400123LL, parse("2026-11-07T06:03:20.123456Z"));  // micro dropped
  // 01:03:20 at -05:00 is 06:03:20 UTC.
  TEST_ASSERT_EQUAL_INT64(1794031400000LL, parse("2026-11-07T01:03:20-05:00"));
  TEST_ASSERT_EQUAL_INT64(1794031400000LL, parse("2026-11-07T01:03:20-0500"));
  TEST_ASSERT_EQUAL_INT64(1794031400000LL, parse("2026-11-07T11:33:20+05:30"));
}

void test_parse_leap_day(void) {
  TEST_ASSERT_EQUAL_INT64(951782400000LL, parse("2000-02-29T00:00:00Z"));
}

void test_parse_rejects_bad_input(void) {
  int64_t ms;
  const char* bad[] = {
      "2026-11-07T06:03:20",        // no zone
      "2026-11-07 06:03:20Z",       // space instead of T
      "2026-02-30T00:00:00Z",       // no such day
      "2026-13-01T00:00:00Z",       // no such month
      "2026-11-07T24:00:00Z",       // hour out of range
      "2026-11-07T06:03:20.Z",      // empty fraction
      "2026-11-07T06:03:20Zx",      // trailing text
      "2026-11-07T06:03:20+0530x",  // trailing text after offset
  };
  for (const char* text : bad) {
    TEST_ASSERT_FALSE_MESSAGE(caudal::parse_iso8601_ms(text, strlen(text), &ms), text);
  }
}

void test_format_round_trips_with_parse(void) {
  char out[32];
  TEST_ASSERT_TRUE(caudal::format_iso8601_utc(1794031400, out));
  TEST_ASSERT_EQUAL_STRING("2026-11-07T06:03:20Z", out);
  TEST_ASSERT_TRUE(caudal::format_iso8601_utc(951782400, out));
  TEST_ASSERT_EQUAL_STRING("2000-02-29T00:00:00Z", out);
  TEST_ASSERT_TRUE(caudal::format_iso8601_utc(0, out));
  TEST_ASSERT_EQUAL_STRING("1970-01-01T00:00:00Z", out);
}
