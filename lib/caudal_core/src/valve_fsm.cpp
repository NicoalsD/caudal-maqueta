#include "caudal/valve_fsm.h"

#include <string.h>

namespace caudal {

namespace {

bool in_window(const ValveCommand& cmd, int64_t now_ms) {
  return now_ms >= cmd.not_before_ms && now_ms < cmd.expires_at_ms;
}

ValveState target_of(CommandType type) {
  return type == CommandType::kOpen ? ValveState::kOpen : ValveState::kClosed;
}

}  // namespace

ValveFsm::ValveFsm()
    : count_(0),
      recent_count_(0),
      recent_next_(0),
      outage_(false),
      has_contact_(false),
      last_contact_ms_(0) {
  memset(valves_, 0, sizeof(valves_));
  memset(recent_ids_, 0, sizeof(recent_ids_));
}

bool ValveFsm::add_valve(const char* valve_id, FailSafe fail_safe) {
  if (count_ >= kMaxValves || valve_id == nullptr || strlen(valve_id) != 36) return false;
  Valve& v = valves_[count_++];
  memcpy(v.id, valve_id, 37);
  v.fail_safe = fail_safe;
  v.state = ValveState::kClosed;  // the firmware drives every valve closed at boot
  v.has_last = false;
  v.last_not_before_ms = 0;
  return true;
}

int ValveFsm::index_of(const char* valve_id) const {
  if (valve_id == nullptr) return -1;
  for (size_t i = 0; i < count_; ++i) {
    if (strcmp(valves_[i].id, valve_id) == 0) return static_cast<int>(i);
  }
  return -1;
}

void ValveFsm::select_latest(const ValveCommand* cmds, size_t n, int64_t now_ms,
                             bool* run) const {
  for (size_t i = 0; i < n; ++i) {
    run[i] = false;
    if (!in_window(cmds[i], now_ms)) continue;
    bool latest = true;
    for (size_t j = 0; j < n && latest; ++j) {
      if (j == i || !in_window(cmds[j], now_ms)) continue;
      if (strcmp(cmds[j].valve_id, cmds[i].valve_id) != 0) continue;
      // A later not_before wins. On a tie the earlier entry in the batch wins.
      if (cmds[j].not_before_ms > cmds[i].not_before_ms ||
          (cmds[j].not_before_ms == cmds[i].not_before_ms && j < i)) {
        latest = false;
      }
    }
    run[i] = latest;
  }
}

Outcome ValveFsm::evaluate(const ValveCommand& cmd, int64_t now_ms) {
  const int index = index_of(cmd.valve_id);
  if (index < 0) return Outcome::kUnknownValve;
  if (now_ms >= cmd.expires_at_ms) return Outcome::kExpired;
  if (now_ms < cmd.not_before_ms) return Outcome::kWait;
  if (remembered(cmd.id)) return Outcome::kDuplicate;

  Valve& v = valves_[index];
  if (v.has_last && cmd.not_before_ms < v.last_not_before_ms) return Outcome::kSuperseded;

  v.state = target_of(cmd.type);
  v.has_last = true;
  v.last_not_before_ms = cmd.not_before_ms;
  remember(cmd.id);
  return Outcome::kExecute;
}

void ValveFsm::on_contact(int64_t now_ms) {
  has_contact_ = true;
  last_contact_ms_ = now_ms;
  outage_ = false;
}

bool ValveFsm::link_lost(int64_t now_ms, int64_t timeout_ms) {
  if (outage_) return false;
  // Before the first contact the clock starts at the first call, so a device that
  // boots with no network applies its fail-safe too.
  if (!has_contact_) {
    has_contact_ = true;
    last_contact_ms_ = now_ms;
  }
  if (now_ms - last_contact_ms_ < timeout_ms) return false;
  outage_ = true;
  return true;
}

size_t ValveFsm::plan_fail_safe(FailSafeTarget* out, size_t cap) {
  size_t written = 0;
  for (size_t i = 0; i < count_; ++i) {
    Valve& v = valves_[i];
    if (v.fail_safe == FailSafe::kKeep) continue;
    const ValveState target = v.fail_safe == FailSafe::kOpen ? ValveState::kOpen
                                                             : ValveState::kClosed;
    v.state = target;
    if (written < cap) out[written++] = FailSafeTarget{i, target};
  }
  return written;
}

bool ValveFsm::remembered(const char* command_id) const {
  for (size_t i = 0; i < recent_count_; ++i) {
    if (strcmp(recent_ids_[i], command_id) == 0) return true;
  }
  return false;
}

void ValveFsm::remember(const char* command_id) {
  memcpy(recent_ids_[recent_next_], command_id, 37);
  recent_next_ = (recent_next_ + 1) % kRememberedCommands;
  if (recent_count_ < kRememberedCommands) ++recent_count_;
}

}  // namespace caudal
