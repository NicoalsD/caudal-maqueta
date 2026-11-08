#include "caudal/commands.h"

#include <ArduinoJson.h>
#include <string.h>

#include "caudal/iso8601.h"

namespace caudal {

namespace {

// UUID text: 36 characters.
bool copy_uuid(const char* text, char* out) {
  if (text == nullptr || strlen(text) != 36) return false;
  memcpy(out, text, 37);
  return true;
}

bool read_command(JsonObjectConst item, ValveCommand* out) {
  const char* command = item["command"];
  if (command == nullptr) return false;
  if (strcmp(command, "OPEN") == 0) {
    out->type = CommandType::kOpen;
  } else if (strcmp(command, "CLOSE") == 0) {
    out->type = CommandType::kClose;
  } else {
    return false;
  }
  if (!copy_uuid(item["id"], out->id) || !copy_uuid(item["valve_id"], out->valve_id)) {
    return false;
  }
  const char* not_before = item["not_before"];
  const char* expires_at = item["expires_at"];
  if (not_before == nullptr || expires_at == nullptr) return false;
  if (!parse_iso8601_ms(not_before, strlen(not_before), &out->not_before_ms)) return false;
  if (!parse_iso8601_ms(expires_at, strlen(expires_at), &out->expires_at_ms)) return false;
  return out->expires_at_ms > out->not_before_ms;
}

}  // namespace

bool parse_commands(const char* json, size_t len, ValveCommand* out, size_t cap, size_t* count,
                    size_t* skipped) {
  *count = 0;
  *skipped = 0;
  JsonDocument doc;
  if (deserializeJson(doc, json, len) != DeserializationError::Ok) return false;
  JsonArrayConst items = doc["items"];
  if (items.isNull()) return false;
  for (JsonObjectConst item : items) {
    if (*count >= cap) break;
    if (read_command(item, &out[*count])) {
      ++(*count);
    } else {
      ++(*skipped);
    }
  }
  return true;
}

}  // namespace caudal
