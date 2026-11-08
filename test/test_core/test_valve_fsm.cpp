#include <string.h>
#include <unity.h>

#include "caudal/valve_fsm.h"

using caudal::CommandType;
using caudal::FailSafe;
using caudal::Outcome;
using caudal::ValveCommand;
using caudal::ValveFsm;
using caudal::ValveState;

namespace {

const char kValve1[] = "0190f3a2-0000-7000-8000-000000000001";
const char kValve2[] = "0190f3a2-0000-7000-8000-000000000002";

ValveCommand make(const char* id, const char* valve, CommandType type, int64_t not_before,
                  int64_t expires_at) {
  ValveCommand c;
  memset(&c, 0, sizeof(c));
  strncpy(c.id, id, 36);
  strncpy(c.valve_id, valve, 36);
  c.type = type;
  c.not_before_ms = not_before;
  c.expires_at_ms = expires_at;
  return c;
}

ValveFsm make_fsm(FailSafe fs1, FailSafe fs2) {
  ValveFsm fsm;
  TEST_ASSERT_TRUE(fsm.add_valve(kValve1, fs1));
  TEST_ASSERT_TRUE(fsm.add_valve(kValve2, fs2));
  return fsm;
}

const char kIdA[] = "00000000-0000-7000-8000-00000000000a";
const char kIdB[] = "00000000-0000-7000-8000-00000000000b";
const char kIdC[] = "00000000-0000-7000-8000-00000000000c";

}  // namespace

void test_command_runs_inside_its_window(void) {
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  const ValveCommand open = make(kIdA, kValve1, CommandType::kOpen, 1000, 5000);
  TEST_ASSERT_TRUE(fsm.evaluate(open, 999) == Outcome::kWait);
  TEST_ASSERT_TRUE(fsm.state(0) == ValveState::kClosed);
  TEST_ASSERT_TRUE(fsm.evaluate(open, 1000) == Outcome::kExecute);
  TEST_ASSERT_TRUE(fsm.state(0) == ValveState::kOpen);
}

// expires_at is exclusive: at that instant the command is already expired.
void test_expired_command_never_runs(void) {
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  const ValveCommand open = make(kIdA, kValve1, CommandType::kOpen, 1000, 5000);
  TEST_ASSERT_TRUE(fsm.evaluate(open, 5000) == Outcome::kExpired);
  TEST_ASSERT_TRUE(fsm.state(0) == ValveState::kClosed);
}

// A command that was already run is not run again, but it is acked again (kDuplicate).
void test_redelivered_command_is_duplicate_not_rerun(void) {
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  const ValveCommand open = make(kIdA, kValve1, CommandType::kOpen, 1000, 5000);
  TEST_ASSERT_TRUE(fsm.evaluate(open, 1500) == Outcome::kExecute);
  TEST_ASSERT_TRUE(fsm.evaluate(open, 2000) == Outcome::kDuplicate);
}

// Opening an open valve is a no-op for the hardware but still a valid execution.
void test_open_then_close_in_order(void) {
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  const ValveCommand open = make(kIdA, kValve1, CommandType::kOpen, 1000, 5000);
  const ValveCommand close = make(kIdB, kValve1, CommandType::kClose, 3000, 9000);
  TEST_ASSERT_TRUE(fsm.evaluate(open, 1000) == Outcome::kExecute);
  TEST_ASSERT_TRUE(fsm.evaluate(close, 3000) == Outcome::kExecute);
  TEST_ASSERT_TRUE(fsm.state(0) == ValveState::kClosed);
}

// An older command arriving after a newer one is discarded.
void test_older_command_after_newer_is_superseded(void) {
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  const ValveCommand close_new = make(kIdB, kValve1, CommandType::kClose, 3000, 9000);
  const ValveCommand open_old = make(kIdA, kValve1, CommandType::kOpen, 1000, 9000);
  TEST_ASSERT_TRUE(fsm.evaluate(close_new, 3500) == Outcome::kExecute);
  TEST_ASSERT_TRUE(fsm.evaluate(open_old, 3600) == Outcome::kSuperseded);
  TEST_ASSERT_TRUE(fsm.state(0) == ValveState::kClosed);
}

void test_unknown_valve_is_discarded(void) {
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  const ValveCommand stranger = make(kIdA, kIdC, CommandType::kOpen, 1000, 5000);
  TEST_ASSERT_TRUE(fsm.evaluate(stranger, 1500) == Outcome::kUnknownValve);
}

// In one batch, only the command with the most recent not_before runs for each valve.
void test_select_latest_keeps_most_recent_per_valve(void) {
  const ValveCommand batch[] = {
      make(kIdA, kValve1, CommandType::kOpen, 1000, 9000),
      make(kIdB, kValve1, CommandType::kClose, 2000, 9000),  // newest for valve 1
      make(kIdC, kValve2, CommandType::kOpen, 1500, 9000),   // only one for valve 2
  };
  bool run[3];
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  fsm.select_latest(batch, 3, 2500, run);
  TEST_ASSERT_FALSE(run[0]);
  TEST_ASSERT_TRUE(run[1]);
  TEST_ASSERT_TRUE(run[2]);
}

void test_select_latest_ignores_commands_outside_window(void) {
  const ValveCommand batch[] = {
      make(kIdA, kValve1, CommandType::kOpen, 1000, 9000),
      make(kIdB, kValve1, CommandType::kClose, 5000, 6000),  // newer but expired at 7000
  };
  bool run[2];
  ValveFsm fsm = make_fsm(FailSafe::kKeep, FailSafe::kKeep);
  fsm.select_latest(batch, 2, 7000, run);
  TEST_ASSERT_TRUE(run[0]);
  TEST_ASSERT_FALSE(run[1]);
}

// Fail-safe: once per outage, only for valves whose position is not KEEP.
void test_fail_safe_fires_once_per_outage(void) {
  ValveFsm fsm = make_fsm(FailSafe::kClosed, FailSafe::kKeep);
  fsm.on_contact(0);
  TEST_ASSERT_FALSE(fsm.link_lost(59999, 60000));
  TEST_ASSERT_TRUE(fsm.link_lost(60000, 60000));
  TEST_ASSERT_FALSE(fsm.link_lost(70000, 60000));  // same outage, already reported

  caudal::FailSafeTarget plan[4];
  const size_t n = fsm.plan_fail_safe(plan, 4);
  TEST_ASSERT_EQUAL_size_t(1, n);
  TEST_ASSERT_EQUAL_size_t(0, plan[0].index);
  TEST_ASSERT_TRUE(plan[0].state == ValveState::kClosed);

  fsm.on_contact(71000);  // contact again: a new outage can fire later
  TEST_ASSERT_TRUE(fsm.link_lost(131000, 60000));
}

void test_fail_safe_open_drives_open_and_keep_is_untouched(void) {
  ValveFsm fsm = make_fsm(FailSafe::kOpen, FailSafe::kKeep);
  fsm.on_contact(0);
  TEST_ASSERT_TRUE(fsm.link_lost(60000, 60000));
  caudal::FailSafeTarget plan[4];
  TEST_ASSERT_EQUAL_size_t(1, fsm.plan_fail_safe(plan, 4));
  TEST_ASSERT_TRUE(plan[0].state == ValveState::kOpen);
  TEST_ASSERT_TRUE(fsm.state(0) == ValveState::kOpen);
  TEST_ASSERT_TRUE(fsm.state(1) == ValveState::kClosed);
}

void test_device_without_contact_eventually_fails_safe(void) {
  ValveFsm fsm = make_fsm(FailSafe::kClosed, FailSafe::kClosed);
  TEST_ASSERT_FALSE(fsm.link_lost(0, 60000));
  TEST_ASSERT_TRUE(fsm.link_lost(60000, 60000));
}
