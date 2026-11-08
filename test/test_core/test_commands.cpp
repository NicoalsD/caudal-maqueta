#include <string.h>
#include <unity.h>

#include "caudal/commands.h"

namespace {

const char kBody[] =
    "{\"items\":["
    "{\"id\":\"0190f3a2-0000-7000-8000-000000000001\",\"valve_id\":\"0190f3a2-0000-7000-8000-00000000000a\","
    "\"command\":\"OPEN\",\"not_before\":\"2026-11-07T06:03:20.000123Z\",\"expires_at\":\"2026-11-07T06:13:20Z\"},"
    "{\"id\":\"0190f3a2-0000-7000-8000-000000000002\",\"valve_id\":\"0190f3a2-0000-7000-8000-00000000000b\","
    "\"command\":\"CLOSE\",\"not_before\":\"2026-11-07T06:04:00Z\",\"expires_at\":\"2026-11-07T06:14:00Z\"},"
    "{\"id\":\"bad\",\"valve_id\":\"0190f3a2-0000-7000-8000-00000000000b\",\"command\":\"OPEN\","
    "\"not_before\":\"2026-11-07T06:04:00Z\",\"expires_at\":\"2026-11-07T06:14:00Z\"},"
    "{\"id\":\"0190f3a2-0000-7000-8000-000000000003\",\"valve_id\":\"0190f3a2-0000-7000-8000-00000000000b\","
    "\"command\":\"TOGGLE\",\"not_before\":\"2026-11-07T06:04:00Z\",\"expires_at\":\"2026-11-07T06:14:00Z\"}"
    "]}";

}  // namespace

void test_parse_commands_reads_items_and_skips_invalid_ones(void) {
  caudal::ValveCommand cmds[8];
  size_t count = 99, skipped = 99;
  TEST_ASSERT_TRUE(caudal::parse_commands(kBody, strlen(kBody), cmds, 8, &count, &skipped));
  TEST_ASSERT_EQUAL_size_t(2, count);
  TEST_ASSERT_EQUAL_size_t(2, skipped);

  TEST_ASSERT_EQUAL_STRING("0190f3a2-0000-7000-8000-000000000001", cmds[0].id);
  TEST_ASSERT_EQUAL_STRING("0190f3a2-0000-7000-8000-00000000000a", cmds[0].valve_id);
  TEST_ASSERT_TRUE(cmds[0].type == caudal::CommandType::kOpen);
  TEST_ASSERT_EQUAL_INT64(1794031400000LL, cmds[0].not_before_ms);  // 000123 us is under 1 ms
  TEST_ASSERT_EQUAL_INT64(1794032000000LL, cmds[0].expires_at_ms);

  TEST_ASSERT_TRUE(cmds[1].type == caudal::CommandType::kClose);
  TEST_ASSERT_EQUAL_INT64(1794031440000LL, cmds[1].not_before_ms);
}

void test_parse_commands_empty_items(void) {
  caudal::ValveCommand cmds[2];
  size_t count = 9, skipped = 9;
  const char* body = "{\"items\":[]}";
  TEST_ASSERT_TRUE(caudal::parse_commands(body, strlen(body), cmds, 2, &count, &skipped));
  TEST_ASSERT_EQUAL_size_t(0, count);
  TEST_ASSERT_EQUAL_size_t(0, skipped);
}

void test_parse_commands_rejects_non_json_and_missing_items(void) {
  caudal::ValveCommand cmds[2];
  size_t count = 9, skipped = 9;
  const char* not_json = "<html>502</html>";
  TEST_ASSERT_FALSE(caudal::parse_commands(not_json, strlen(not_json), cmds, 2, &count, &skipped));
  TEST_ASSERT_EQUAL_size_t(0, count);
  const char* no_items = "{\"other\":1}";
  TEST_ASSERT_FALSE(caudal::parse_commands(no_items, strlen(no_items), cmds, 2, &count, &skipped));
}

void test_parse_commands_respects_capacity(void) {
  caudal::ValveCommand cmds[1];
  size_t count = 0, skipped = 0;
  TEST_ASSERT_TRUE(caudal::parse_commands(kBody, strlen(kBody), cmds, 1, &count, &skipped));
  TEST_ASSERT_EQUAL_size_t(1, count);
}
