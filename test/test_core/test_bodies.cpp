#include <string.h>
#include <unity.h>

#include "caudal/bodies.h"
#include "caudal/iso8601.h"

// Must match the literal body that the signature test hashes, byte for byte.
void test_telemetry_body_matches_signed_vector(void) {
  int64_t ms = 0;
  TEST_ASSERT_TRUE(caudal::parse_iso8601_ms("2026-10-08T23:00:00Z", 20, &ms));
  caudal::TelemetryPoint point;
  memset(&point, 0, sizeof(point));
  strcpy(point.id, "0190f3a2-6666-4000-8000-000000000001");
  point.metric = "LEVEL";
  point.value_centi = 210;
  point.observed_at_s = ms / 1000;

  char out[256];
  const size_t n = caudal::build_telemetry_body(&point, 1, out, sizeof(out));
  const char expected[] =
      "{\"points\":[{\"id\":\"0190f3a2-6666-4000-8000-000000000001\",\"metric\":\"LEVEL\","
      "\"value\":2.10,\"observed_at\":\"2026-10-08T23:00:00Z\"}]}";
  TEST_ASSERT_EQUAL_size_t(strlen(expected), n);
  TEST_ASSERT_EQUAL_STRING(expected, out);
}

void test_telemetry_body_has_two_decimals_for_negative_and_integer_values(void) {
  caudal::TelemetryPoint points[2];
  memset(points, 0, sizeof(points));
  strcpy(points[0].id, "0190f3a2-6666-4000-8000-000000000002");
  points[0].metric = "RSSI";
  points[0].value_centi = -6000;
  points[0].observed_at_s = 0;
  strcpy(points[1].id, "0190f3a2-6666-4000-8000-000000000003");
  points[1].metric = "LEVEL";
  points[1].value_centi = 5;
  points[1].observed_at_s = 0;
  char out[400];
  caudal::build_telemetry_body(points, 2, out, sizeof(out));
  TEST_ASSERT_NOT_NULL(strstr(out, "\"value\":-60.00"));
  TEST_ASSERT_NOT_NULL(strstr(out, "\"value\":0.05"));
}

void test_telemetry_body_rejects_small_buffer(void) {
  caudal::TelemetryPoint point;
  memset(&point, 0, sizeof(point));
  point.metric = "LEVEL";
  char out[20];
  TEST_ASSERT_EQUAL_size_t(0, caudal::build_telemetry_body(&point, 1, out, sizeof(out)));
}

void test_ack_body_acked_and_failed(void) {
  char out[160];
  caudal::build_ack_body(true, "pos=OPEN", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("{\"result\":\"ACKED\",\"detail\":\"pos=OPEN\"}", out);
  caudal::build_ack_body(false, "sin respuesta", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("{\"result\":\"FAILED\",\"detail\":\"sin respuesta\"}", out);
}

void test_ack_body_escapes_quotes_and_backslashes(void) {
  char out[160];
  caudal::build_ack_body(true, "a\"b\\c", out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING("{\"result\":\"ACKED\",\"detail\":\"a\\\"b\\\\c\"}", out);
}
