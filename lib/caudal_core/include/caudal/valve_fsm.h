// Valve state machine (pattern P18 State, and P13 Command for the commands it executes).
// Pure logic: it decides what to do and keeps the intended state of each valve. The
// GPIO work is done by the firmware's valve driver (pattern P07 Bridge).
#ifndef CAUDAL_VALVE_FSM_H
#define CAUDAL_VALVE_FSM_H

#include <stddef.h>
#include <stdint.h>

#include "caudal/commands.h"

namespace caudal {

enum class ValveState : uint8_t { kClosed, kOpen };

// fail_safe_position of the valve (protocol section 6). kKeep does nothing.
enum class FailSafe : uint8_t { kKeep, kOpen, kClosed };

enum class Outcome : uint8_t {
  kExecute,      // run the command now: drive the valve and ack ACKED
  kDuplicate,    // already executed: do not drive again, but ack again (the ack may have been lost)
  kWait,         // before not_before: ignore for now
  kExpired,      // at or after expires_at: never run, never ack
  kSuperseded,   // an older command for the same valve: discard
  kUnknownValve  // valve_id not configured on this device: discard
};

struct FailSafeTarget {
  size_t index;
  ValveState state;
};

class ValveFsm {
 public:
  static constexpr size_t kMaxValves = 4;
  static constexpr size_t kRememberedCommands = 16;

  ValveFsm();

  // Configures one valve. Returns false when the table is full or the id is malformed.
  bool add_valve(const char* valve_id, FailSafe fail_safe);

  // Marks in `run[i]` which commands of a batch must run: for each valve only the
  // command with the most recent not_before is in the window. The rest stay false.
  void select_latest(const ValveCommand* cmds, size_t n, int64_t now_ms, bool* run) const;

  // Decides one command. On kExecute the intended state of the valve changes and the
  // command is remembered.
  Outcome evaluate(const ValveCommand& cmd, int64_t now_ms);

  // Index of the valve, or -1 when unknown.
  int index_of(const char* valve_id) const;

  ValveState state(size_t index) const { return valves_[index].state; }
  size_t valve_count() const { return count_; }

  // Contact with the backend (any 2xx to a signed request).
  void on_contact(int64_t now_ms);

  // True exactly once per outage: when no contact has been seen for `timeout_ms`.
  bool link_lost(int64_t now_ms, int64_t timeout_ms);

  // After link_lost() returned true: writes the valves whose fail_safe is not kKeep
  // and updates their intended state. Returns the number written.
  size_t plan_fail_safe(FailSafeTarget* out, size_t cap);

 private:
  struct Valve {
    char id[37];
    FailSafe fail_safe;
    ValveState state;
    bool has_last;
    int64_t last_not_before_ms;
  };

  bool remembered(const char* command_id) const;
  void remember(const char* command_id);

  Valve valves_[kMaxValves];
  size_t count_;
  char recent_ids_[kRememberedCommands][37];
  size_t recent_count_;
  size_t recent_next_;
  bool outage_;
  bool has_contact_;
  int64_t last_contact_ms_;
};

}  // namespace caudal

#endif  // CAUDAL_VALVE_FSM_H
