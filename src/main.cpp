// Rig firmware: level, flow, floats and valves, with signed telemetry, command polling and
// ack. Each device of the rig (the tank sensor and the two actuators) signs with its own
// identity. The loop is cooperative: nothing waits with delay().
#if !__has_include("secrets.h")
#error "Missing include/secrets.h. Copy include/secrets.example.h to include/secrets.h and fill it in."
#endif

#include <Arduino.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <sys/time.h>
#include <time.h>

#include "api_client.h"
#include "caudal/bodies.h"
#include "caudal/commands.h"
#include "caudal/encoding.h"
#include "caudal/valve_fsm.h"
#include "config.h"
#include "float_switches.h"
#include "flow_sensor.h"
#include "level_sensor.h"
#include "secrets.h"
#include "signer.h"
#include "valve_driver.h"
#include "wifi_link.h"

namespace {

constexpr size_t kMaxCommands = 16;
constexpr size_t kValves = 2;
constexpr time_t kMinValidTime = 1704067200;  // 2024-01-01T00:00:00Z

WifiLink wifi;
LevelSensor level;
FlowSensor flow;
FloatSwitches floats;
ValveDriver valves;
ApiClient api;
caudal::ValveFsm fsm;

DeviceSigner sensor_signer;             // LEVEL_SENSOR of the tank
DeviceSigner valve_signers[kValves];    // VALVE_ACTUATOR of each outlet

int64_t last_poll_ms[kValves] = {0, 0};
int64_t last_telemetry_ms = 0;
int64_t last_flow_log_ms = 0;

// Monotonic milliseconds since boot. 64-bit, so it never wraps in practice.
int64_t monotonic_ms() { return esp_timer_get_time() / 1000; }

int64_t epoch_ms() {
  struct timeval tv;
  gettimeofday(&tv, nullptr);
  return static_cast<int64_t>(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

caudal::FailSafe fail_safe_from_config(int value) {
  if (value == 1) return caudal::FailSafe::kOpen;
  if (value == 2) return caudal::FailSafe::kClosed;
  return caudal::FailSafe::kKeep;
}

void note_contact(int status, int64_t now_ms) {
  if (status >= 200 && status < 300) {
    fsm.on_contact(now_ms);
  }
}

void fill_uuid(char* out) {
  uint8_t random[16];
  esp_fill_random(random, sizeof(random));
  caudal::format_uuid_v4(random, out);
}

// Telemetry of the tank sensor: LEVEL and RSSI, signed by the sensor identity.
void send_sensor_telemetry(int64_t now_ms) {
  if (!level.has_level() || level.sampled_at_s() < static_cast<int64_t>(kMinValidTime)) {
    // The median was taken before the clock synced; wait for a fresh one.
    return;
  }
  caudal::TelemetryPoint points[2];

  caudal::TelemetryPoint& lvl = points[0];
  fill_uuid(lvl.id);
  lvl.metric = "LEVEL";
  lvl.value_centi = level.level_cm();  // 1 cm is 0.01 m, so cm is already centi-meters
  lvl.observed_at_s = level.sampled_at_s();

  caudal::TelemetryPoint& rssi = points[1];
  fill_uuid(rssi.id);
  rssi.metric = "RSSI";
  rssi.value_centi = wifi.rssi() * 100;
  rssi.observed_at_s = lvl.observed_at_s;

  char body[512];
  const size_t len = caudal::build_telemetry_body(points, 2, body, sizeof(body));
  if (len == 0) return;
  const int status = api.post_telemetry(sensor_signer, body, len, now_ms);
  note_contact(status, now_ms);
  Serial.printf("telemetry: level=%d cm, http=%d\n", level.level_cm(), status);
}

// Telemetry of one actuator: VALVE_POSITION read back from its relay (1 = open, 0 = closed).
// Each actuator reports its own position, so the metric is not ambiguous.
void send_valve_telemetry(size_t index, int64_t now_ms) {
  caudal::TelemetryPoint point;
  fill_uuid(point.id);
  point.metric = "VALVE_POSITION";
  point.value_centi = valves.actual_open(index) ? 100 : 0;
  point.observed_at_s = static_cast<int64_t>(time(nullptr));
  char body[256];
  const size_t len = caudal::build_telemetry_body(&point, 1, body, sizeof(body));
  if (len == 0) return;
  const int status = api.post_telemetry(valve_signers[index], body, len, now_ms);
  note_contact(status, now_ms);
}

void execute_command(size_t index, const caudal::ValveCommand& cmd, int64_t now_ms) {
  const caudal::Outcome outcome = fsm.evaluate(cmd, epoch_ms());
  switch (outcome) {
    case caudal::Outcome::kExecute: {
      valves.drive(index, cmd.type == caudal::CommandType::kOpen ? caudal::ValveState::kOpen
                                                                  : caudal::ValveState::kClosed);
      // Report the real relay position, not the order that was sent.
      const bool open = valves.actual_open(index);
      const int status =
          api.ack(valve_signers[index], cmd.id, true, open ? "pos=OPEN" : "pos=CLOSED", now_ms);
      note_contact(status, now_ms);
      Serial.printf("command %s valve %u -> %s, ack http=%d\n", cmd.id,
                    static_cast<unsigned>(index), open ? "OPEN" : "CLOSED", status);
      break;
    }
    case caudal::Outcome::kDuplicate: {
      // Already run. Ack again in case the earlier ack was lost. The backend treats it as idempotent.
      const int status = api.ack(valve_signers[index], cmd.id, true, "duplicate", now_ms);
      note_contact(status, now_ms);
      break;
    }
    case caudal::Outcome::kWait:
    case caudal::Outcome::kExpired:
    case caudal::Outcome::kSuperseded:
    case caudal::Outcome::kUnknownValve:
      // Not run and not acked: the backend expires it on its own clock.
      Serial.printf("command %s discarded (%d)\n", cmd.id, static_cast<int>(outcome));
      break;
  }
}

// Polls the commands of one actuator with its own identity and runs them.
void poll_valve_commands(size_t index, int64_t now_ms) {
  caudal::ValveCommand cmds[kMaxCommands];
  size_t count = 0;
  size_t skipped = 0;
  bool parsed = false;
  const int status =
      api.get_commands(valve_signers[index], cmds, kMaxCommands, &count, &skipped, &parsed, now_ms);
  note_contact(status, now_ms);
  if (!parsed) return;

  bool run[kMaxCommands];
  fsm.select_latest(cmds, count, epoch_ms(), run);
  for (size_t i = 0; i < count; ++i) {
    if (!run[i]) continue;
    if (fsm.index_of(cmds[i].valve_id) != static_cast<int>(index)) {
      // The backend only returns this actuator's commands. Anything else is discarded.
      Serial.printf("command %s is not for valve %u: discarded\n", cmds[i].id,
                    static_cast<unsigned>(index));
      continue;
    }
    execute_command(index, cmds[i], now_ms);
  }
  if (skipped > 0) Serial.printf("malformed commands: %u\n", static_cast<unsigned>(skipped));
}

void check_fail_safe(int64_t now_ms) {
  if (!fsm.link_lost(now_ms, FAILSAFE_TIMEOUT_MS)) return;
  caudal::FailSafeTarget plan[caudal::ValveFsm::kMaxValves];
  const size_t n = fsm.plan_fail_safe(plan, caudal::ValveFsm::kMaxValves);
  for (size_t i = 0; i < n; ++i) {
    valves.drive(plan[i].index, plan[i].state);
  }
  Serial.printf("no contact: fail_safe_position applied to %u valve(s)\n",
                static_cast<unsigned>(n));
}

}  // namespace

void setup() {
  // First thing: relays off (valves closed) before anything else.
  ValveDriver::force_safe_outputs();

  Serial.begin(115200);
  Serial.println("CAUDAL rig: boot");

  valves.begin();
  level.begin();
  flow.begin();
  floats.begin();

  fsm.add_valve(VALVE_1_ID, fail_safe_from_config(FAILSAFE_VALVE_1));
  fsm.add_valve(VALVE_2_ID, fail_safe_from_config(FAILSAFE_VALVE_2));

  // Derive each identity from its seed, then wipe the local copy of the seed.
  struct Identity {
    DeviceSigner* signer;
    const char* device_id;
    const char* seed_hex;
  };
  const Identity identities[] = {
      {&sensor_signer, SENSOR_DEVICE_ID, SENSOR_SEED_HEX},
      {&valve_signers[0], VALVE_1_DEVICE_ID, VALVE_1_SEED_HEX},
      {&valve_signers[1], VALVE_2_DEVICE_ID, VALVE_2_SEED_HEX},
  };
  for (const Identity& identity : identities) {
    uint8_t seed[32];
    if (!caudal::from_hex(identity.seed_hex, 64, seed, 32)) {
      Serial.println("a device seed is invalid: expected 64 hex digits");
      while (true) delay(1000);
    }
    identity.signer->begin(identity.device_id, seed);
    for (uint8_t& b : seed) b = 0;
  }

  api.begin();
  wifi.begin();
}

void loop() {
  const int64_t now = monotonic_ms();
  // Wi-Fi, level and flow use unsigned millisecond differences, which are wrap-safe.
  const uint32_t now32 = static_cast<uint32_t>(now);

  wifi.update(now32);
  if (level.update(now32)) {
    Serial.printf("level: %d cm (blind-zone rejects: %d)\n", level.level_cm(),
                  level.blind_zone_rejects());
  }
  flow.update(now32);

  if (now - last_flow_log_ms >= 10000) {
    last_flow_log_ms = now;
    Serial.printf("flow: %.2f and %.2f L/min, float high=%d low=%d\n", flow.flow_lpm(0),
                  flow.flow_lpm(1), floats.high_active() ? 1 : 0, floats.low_active() ? 1 : 0);
  }

  // Each actuator polls with its own identity, so each one has its own rate budget.
  for (size_t i = 0; i < kValves; ++i) {
    if (now - last_poll_ms[i] < COMMAND_POLL_MS) continue;
    if (!wifi.ready() || !api.can_send(now)) continue;
    last_poll_ms[i] = now;
    poll_valve_commands(i, now);
  }

  if (now - last_telemetry_ms >= TELEMETRY_INTERVAL_MS && wifi.ready() && api.can_send(now)) {
    last_telemetry_ms = now;
    send_sensor_telemetry(now);
    for (size_t i = 0; i < kValves; ++i) send_valve_telemetry(i, now);
  }

  check_fail_safe(now);
}
