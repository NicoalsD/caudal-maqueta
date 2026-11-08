// ISO-8601 instants in UTC: parses the times the backend sends (not_before,
// expires_at) and formats the observed_at of telemetry points.
#ifndef CAUDAL_ISO8601_H
#define CAUDAL_ISO8601_H

#include <stddef.h>
#include <stdint.h>

namespace caudal {

// Parses "YYYY-MM-DDTHH:MM:SS", an optional fraction of 1 to 9 digits, and a
// zone (Z, +HH:MM, -HH:MM, +HHMM or -HHMM). Without a zone it returns false.
// The result is milliseconds since the Unix epoch; sub-millisecond digits are dropped.
bool parse_iso8601_ms(const char* text, size_t len, int64_t* epoch_ms);

// Formats seconds since the epoch as "YYYY-MM-DDTHH:MM:SSZ". `out` needs 21 bytes.
// Returns false if the year is outside 0000..9999.
bool format_iso8601_utc(int64_t epoch_seconds, char* out);

}  // namespace caudal

#endif  // CAUDAL_ISO8601_H
