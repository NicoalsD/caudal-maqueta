// Parser for the body of GET /api/v1/devices/commands (API.md, section 16.2).
#ifndef CAUDAL_COMMANDS_H
#define CAUDAL_COMMANDS_H

#include <stddef.h>
#include <stdint.h>

namespace caudal {

enum class CommandType : uint8_t { kOpen, kClose };

struct ValveCommand {
  char id[37];        // UUID text and NUL
  char valve_id[37];  // UUID text and NUL
  CommandType type;
  int64_t not_before_ms;
  int64_t expires_at_ms;
};

// Parses {"items":[{"id","valve_id","command","not_before","expires_at"}]}.
// Items with missing or invalid fields are skipped and counted in `skipped`.
// Items past `cap` are ignored. Returns false if the body is not JSON or has no
// "items" array; `count` is 0 in that case.
bool parse_commands(const char* json, size_t len, ValveCommand* out, size_t cap, size_t* count,
                    size_t* skipped);

}  // namespace caudal

#endif  // CAUDAL_COMMANDS_H
